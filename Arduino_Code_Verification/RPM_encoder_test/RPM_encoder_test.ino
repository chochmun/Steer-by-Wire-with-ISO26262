// 아날로그 핀 A0에서 들어오는 값을 읽고 이를 RPM으로 변환하여 Serial 출력하는 코드
const int sensorPin = A0;  // 아날로그 입력 핀
int sensorValue = 0;       // 아날로그 입력 값
float rpm = 0;             // 계산된 RPM 값

void setup() {
  // Serial 통신 시작
  Serial.begin(9600);
}

void loop() {
  // A0 핀의 아날로그 값을 읽기 (0 ~ 1023 범위)
  sensorValue = analogRead(sensorPin);

  // 아날로그 값을 RPM으로 변환 (예: 임의의 변환 공식)
  // 이 부분은 인코더에 따라 달라질 수 있음. 아래는 예시 변환:
  rpm = (sensorValue / 1023.0) * 5000.0;  // 0~5000 RPM 범위로 변환

  // 결과를 Serial로 출력
  Serial.print("Analog Value: ");
  Serial.print(sensorValue);
  Serial.print(" -> RPM: ");
  Serial.println(rpm);

  // 1초 간격으로 업데이트
  delay(1000);
}
