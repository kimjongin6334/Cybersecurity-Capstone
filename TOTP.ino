#include "sha1.h" //SHA1 헤더
#include "TOTP.h" //TOTP 헤더
#include <Keypad.h> //키패드 헤더
#include <SoftwareSerial.h> //블루투스 통신용 헤더
#include <DS3231.h> //RTC 헤더

//===키패드 관련==============================================================
const byte rows = 4;
const byte cols = 4;
// 키패드의 행, 열의 갯수

char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// 키패드 버튼 위치 설정

byte rowPins[rows] = {9, 8, 7, 6};
byte colPins[cols] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

char position = 0; 
char wrong = 0;

//블루투스
SoftwareSerial BTSerial(12, 13);

//RTC
DS3231 rtc(SDA, SCL); //

//===OTP 관련================================================================
//Hmac 키
uint8_t hmacKey [ ]  = {0x4d, 0x79, 0x4c, 0x65, 0x67, 0x6f, 0x44, 0x6f, 0x6f, 0x72};
uint8_t carHmacKey [ ]  =  {0x77, 0x73, 0x41, 0x6e, 0x63, 0x61, 0x70, 0x73, 0x74, 0x6f, 0x6e, 0x65}; 

char hmacKeyLen = sizeof(hmacKey)/sizeof(hmacKey[0]);
char carHmacKeyLen = sizeof(carHmacKey)/sizeof(carHmacKey[0]);

TOTP totp = TOTP(hmacKey, hmacKeyLen);
TOTP totpCar = TOTP(carHmacKey,  carHmacKeyLen);

char code[7];
char carCode[7];

long GMT; //타임스탬프

//===그 외 사용 전역변수=======================================================
int interval = 20000; //키 잠금 해제 지속시간
long prevMil = 0; //지속시간 확인용 변수

bool keyLock = true; //키 잠금해제 여부
String cmd = "";
//============================================================================

void setup() {
  //serial init
  Serial.begin(9600);
  
  //bluetooth init
  BTSerial.begin(9600);

  //rtc init
  rtc.begin();
}

void loop() {
  long curMil = millis();
  DateTime now = rtc.getDateTime();
   
  //타임스탬프 테스트 코드
  long CGMT = rtc.getUnixTime(now) - 32388; //모듈에서 받아온 타임스탬프와 실제 타임스탬프가 일치하도록 -32388 계산
  if(CGMT != GMT){
    GMT = CGMT;
    Serial.println(GMT);
  }

  char* newCode = totp.getCode(GMT);
  if(strcmp(code, newCode) != 0) {
    strcpy(code, newCode);
    Serial.println(code);
  }


  if(!keyLock && (curMil - prevMil > interval)){ //키 잠금 해제 후 일정 시간 지나면 키 잠김
    Serial.println("키 잠금 시작");
    keyLock = true;
  }
  else if(!keyLock && (curMil - prevMil <= interval)){ //키 잠금 해제된 동안 차 키와 같은 OTP, 이벤트 값 생성
    char* newCarCode = totp.getCodeFromSteps((GMT/3));
    if(strcmp(carCode, newCarCode) != 0) {
      strcpy(carCode, newCarCode);
      Serial.println(carCode);
    }
  }
  char key = keypad.getKey(); // 키패드에서 입력된 값을 가져옵니다.
  Serial.print(key); ///키패드 입력 확인용

  if((key >= '0' && key <= '9') || (key >= 'A' && key <='D') || (key == '*' || key == '#')) // 키패드에서 입력된 값을 조사하여 맞게 입력된 값일 경우(키패드에 있는 버튼이 맞을경우) 비교
  {
    if(key == 'A' && !keyLock){ //A 누르면 문 열림 신호 전송
      Serial.println("차 문 열림");
      BTSerial.write(strcat("Open",carCode));
    } else if(key == 'A' && keyLock){
      Serial.println("기능 잠김");
    }

    if(key == 'B' && !keyLock){ //B 누르면 문 잠금 신호 전송
      Serial.println("차 문 잠김");
      BTSerial.write(strcat("Close",carCode));
    } else if(key == 'B' && keyLock){
      Serial.println("기능 잠김");
    }

    if(key == 'C' && !keyLock){ //C 누르면 트렁크 신호 전송
      Serial.println("트렁크 열림");
      BTSerial.write(strcat("Trunk",carCode));
    } else if(key == 'C' && keyLock){
      Serial.println("기능 잠김");
    }

    if(key == '*' || key == '#') // *, # 버튼을 눌렀을 경우
    { 
      position = 0; 
      wrong = 0; // 입력 초기화
    }
    else if(key == code[position] && key < 'A')// 해당 자리에 맞는 비밀번호가 입력됬을 경우
    { 
      position++; // 다음 자리로 넘어 감
    }
    else if(key != code[position] && key < 'A')// 해당 자리에 맞지 않는 비밀번호가 입력됬을 경우
    { 
      position++;
      wrong++; // 비밀번호 오류 값을 늘려준다
    } 

    if(position >= 6) //6자리 전부 입력된 경우
    { 
      if(wrong == 0) // 비밀번호가 모두 맞았을 경우
      {
        Serial.println("true");
        Serial.println("키 잠금 해제");
        position = 0;
        prevMil = curMil;
        keyLock = false;
      }
          
      else //비밀번호 틀린 경우
      {
        Serial.println("wrong");
        position = 0; 
        wrong = 0; // 비밀번호 오류 값을 0으로 만들어 준다.
      }
    }
  }
}
