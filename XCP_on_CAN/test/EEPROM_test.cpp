#include <EEPROM.h>

int steer_mode;  // steer_mode 변수 선언
float steering_weight;

void setup() {
  Serial.begin(9600);

  // EEPROM에서 steer_mode 읽어오기
  steer_mode = EEPROM.read(0);

  // 유효하지 않은 값이면 초기화 (0~4 범위 밖의 값일 때)
  if (steer_mode < 0 || steer_mode > 4) {
    steer_mode = 0; // 기본 값으로 설정
    EEPROM.write(0, steer_mode); // EEPROM에 기본 값 저장
  }

  steering_weight = 1 + 0.25 * steer_mode;

  Serial.print("현재 steer_mode: ");
  Serial.println(steer_mode);
  Serial.print("현재 steering_weight: ");
  Serial.println(steering_weight);
}

void loop() {
  if (Serial.available() > 0) {
    // 시리얼 입력을 문자열로 받기
    String inputStr = Serial.readString();  // 입력 문자열 읽기
    inputStr.trim();  // 문자열에서 공백 제거

    // 빈 입력값을 무시
    if (inputStr.length() == 0) {
      return;  // 입력이 없으면 리턴
    }

    int input = inputStr.toInt();  // 입력을 정수로 변환

    // 유효한 입력 값 확인 (0~4)
    if (input >= 0 && input <= 4) {
      steer_mode = input;
      steering_weight = 1 + 0.25 * steer_mode;

      // steer_mode를 EEPROM에 저장
      EEPROM.write(0, steer_mode);

      // 업데이트된 값 출력
      Serial.print("업데이트된 steer_mode: ");
      Serial.println(steer_mode);
      Serial.print("업데이트된 steering_weight: ");
      Serial.println(steering_weight);

    } else {
      // 유효하지 않은 입력
      Serial.println("잘못된 입력입니다. 0에서 4 사이의 값을 입력하세요.");
    }
  }

  for (int steering_angle = -5; steering_angle <= 5; steering_angle++) {
    Serial.println(steering_angle * steering_weight);
    delay(500);
  }

  // 현재 EEPROM 값을 확인
  Serial.print("현재 EEPROM : ");
  Serial.println(EEPROM.read(0));
}
