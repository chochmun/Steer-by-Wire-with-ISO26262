#include <SPI.h>
#include <mcp2515.h>

#define slave1 0x0F6
#define slave2 0x036

struct can_frame canMsg;
struct can_frame sendMsg;
MCP2515 mcp2515(10);

unsigned long previousReceiveTime = 0;   // 이전 메시지 수신 시간을 기록

void setup() {
  Serial.begin(9600);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS);
  mcp2515.setNormalMode();
  
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA   Interval(ms)");
}

void loop() {
  // 메시지 수신 대기
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id != slave1) return;  // 내 id에 해당하는 데이터가 아니면 무시

    unsigned long currentReceiveTime = millis();  // 현재 수신 시간을 기록
 
    // 이전 수신된 시점과 다음 수신된 시점의 차이를 계산
    unsigned long interval = currentReceiveTime - previousReceiveTime;

    // 수신된 시점과 메시지 내용을 출력
    Serial.print("[ID] = ");
    Serial.print(canMsg.can_id, HEX); // ID 출력
    Serial.print(", [ length ] = ");
    Serial.print(canMsg.can_dlc, HEX); // DLC 출력
    Serial.print(", [received Data] = ");
    
    for (int i = 0; i < canMsg.can_dlc; i++) {  // 데이터 출력
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }

    // 이전 수신 시점과의 간격 출력
    Serial.print(", [Interval] = ");
    Serial.print(interval);
    Serial.println(" ms");

    // 이전 수신 시간을 업데이트
    previousReceiveTime = currentReceiveTime;
  }
}
