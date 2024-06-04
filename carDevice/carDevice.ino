#include "sha1.h"
#include "TOTP.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"
#include <IRremote.h>

//LED
const char ledPin = 8;

//블루투스
SoftwareSerial BTSerial_With_CarKey(2, 3); // HC - 06 통신을 위한 TX, RX의 PIN번호를 입력 합니다

//IC
#define STO     0xE31CFF00   //The remote control II key
#define ADVAN    0xF708FF00  //The remote control << key
#define BAC   0xAD52FF00  //The remote control >> key

IRrecv irrecv(11);//Initialize the
decode_results results;//Define the type of structure

//RTC
RTC_DS3231 rtc;

//모터의 전후좌우 방향을 컨트롤하는 PIN번,호를 상수로 선언하였습니다.
#define IN1 A0
#define IN2 A2
#define IN3 A1
#define IN4 A3

const char MotorRPWM=6;  
const char MotorLPWM=5; 

//===OTP 관련================================================================
//Hmac 키
uint8_t carHmacKey [ ]  =  {0x77, 0x73, 0x41, 0x6e, 0x63, 0x61, 0x70, 0x73, 0x74, 0x6f, 0x6e, 0x65}; 

char carHmacKeyLen = sizeof(carHmacKey)/sizeof(carHmacKey[0]);

TOTP totp = TOTP(carHmacKey,  carHmacKeyLen);

char Code[7];

long GMT; //타임스탬프

bool CarFuncEnable = false;

void setup()
{
  irrecv.enableIRIn();// Begin to receive
  BTSerial_With_CarKey.begin(9600);
  Serial.begin(9600);

  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));

  pinMode(ledPin, OUTPUT);

  //모터를 컨트롤하는 출력핀을 설정해줍니다.
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
}


void loop() 
{
  DateTime now = rtc.now(); 

  GMT = now.unixtime() - 32391; //시간 맞추기 위함


  char* newCode = totp.getCodeFromSteps((GMT/10));
  if(strcmp(Code, newCode) != 0) {
    strcpy(Code, newCode);
    Serial.println(Code);
  }
 
  String op = "Open" + String(Code);
  String cl = "Close" + String(Code);
//블루투스 통신
  if (BTSerial_With_CarKey.available()) {
    // 수신된 문자열 읽기
    String msg = BTSerial_With_CarKey.readStringUntil('\n');
    msg.trim();
    Serial.println(msg);
    if(msg == op){
      Serial.println("OPEN");
      CarFuncEnable = true;
      digitalWrite(ledPin, HIGH);
    }
    else if(msg == cl){
      Serial.println("CLOSE");
      CarFuncEnable = false;
      digitalWrite(ledPin, LOW);
    }
    else{
      Serial.println("Failed...");
    }

  }

  if (irrecv.decode()) //차 키에서 받은 OTP 인증 성공 시 스마트폰에서 받은 차량 조작 신호에 맞게 작동
  {
    Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
    if(irrecv.decodedIRData.decodedRawData == ADVAN && CarFuncEnable){
      Serial.println("F");         
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(MotorRPWM, 140);  //값을 변경하여 속도 조절 가능 0~255
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      analogWrite(MotorLPWM, 140);   //값을 변경하여 속도 조절 가능 0~255
    }
    else if(irrecv.decodedIRData.decodedRawData == BAC && CarFuncEnable){
      Serial.println("B");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(MotorRPWM, 140);  //값을 변경하여 속도 조절 가능 0~255 
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      analogWrite(MotorLPWM, 140);  //값을 변경하여 속도 조절 가능 0~255

    }
    else if(irrecv.decodedIRData.decodedRawData == STO){
        Serial.println("S");
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(MotorRPWM, 0);  //값을 변경하여 속도 조절 가능 0~255
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        analogWrite(MotorRPWM, 0);  //값을 변경하여 속도 조절 가능 0~255
    }
    else
      delay(100);
    irrecv.resume(); // To receive the next value
  }
}
