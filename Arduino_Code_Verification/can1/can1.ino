/*
 id_ecu2
 id_ecu2이 초기에는 backup controller(Can Master)
 페일오버가 이뤄지면, id_ecu2가 backup 상태로 전환
*/

#include <SPI.h>
#include <mcp2515.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

SemaphoreHandle_t binarySemaphore;

#define id_ecu1 0x011
#define id_ecu2 0x022

#define job1 0x00 // 생존확인용 job
#define job2 0x01
#define job3 0x02
#define job4 0x03

struct can_frame canMsg;
struct can_frame canMsg1;
struct can_frame canMsg2;
volatile byte arbiter_flag = 0x00;
MCP2515 mcp2515(10);

void setup() {
  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();

  binarySemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(binarySemaphore);
  Serial.println("initialized");

  // FreeRTOS 태스크 생성
  xTaskCreate(task1, "Task1", 256, NULL, 1, NULL);
  xTaskCreate(task2, "Task2", 256, NULL, 1, NULL);
}

void loop() {
  // FreeRTOS 환경에서 loop 함수는 빈 상태로 남겨둡니다.
}

// 첫 번째 태스크: CAN 메시지 송수신 및 세마포어 관리
void task1(void *pvParameters) {
  byte send_data[8] = {0x00, 0x00, 0x00, 0x00, arbiter_flag, 0x00, 0x00, 0x00};
  
  while (true) {
    byte send_data[8] = {0x00, 0x00, 0x00, 0x00, arbiter_flag, 0x00, 0x00, 0x00};
    
    send_message(id_ecu1, send_data);
    vTaskDelay(25 / portTICK_PERIOD_MS); // 25ms 대기

    byte* response_msg= receive_response_with_timeout(id_ecu2, 500);
    if(response_msg[5]==0x01){ // backup_ecu's arbiter_flag=true
      arbiter_flag=0x01;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS); // 500ms 대기

    // 세마포어 상태 확인 및 처리
    checkAndProcessSemaphore();
  }
}

// 두 번째 태스크: 세마포어 상태에 따른 처리 작업
void task2(void *pvParameters) {
  while (true) {
    if (uxSemaphoreGetCount(binarySemaphore) == 0){
      //get_angle_data();
      processing_angle_data();
      //transmit_control_message_to_RWA();
      //get_road_wheel_angle_by_RWA();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 작업 주기 설정 (필요에 따라 조절)
  }
}

// 메시지 전송 함수
void send_message(unsigned int id_out, byte data[]) {
  struct can_frame canMsg1;
  canMsg1.can_id = id_out;
  canMsg1.can_dlc = 8;
  for (int i = 0; i < 8; i++) {
    canMsg1.data[i] = data[i];
  }
  
  Serial.println();
  mcp2515.sendMessage(&canMsg1);
  Serial.print("[Backup제어기가 Main제어기로 보낸 메시지] id =");
  Serial.print(id_out, HEX);
  Serial.print(" - data =");
  for (int i = 0; i < canMsg1.can_dlc; i++) {
    Serial.print(canMsg1.data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// 메시지 수신 함수
byte* recieve_reponse(unsigned int id_in) {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print("[Main제어기의 응답 메세지] data =");
    for (int i = 0; i < canMsg.can_dlc; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
  }
  Serial.println();
  return canMsg.data;
}

// 세마포어 상태 확인 및 해제 함수
void checkAndProcessSemaphore() {
  if (arbiter_flag==0x00) {
    // 세마포어가 이미 give된 상태인지 확인
    if (uxSemaphoreGetCount(binarySemaphore) == 0) {
      xSemaphoreGive(binarySemaphore);
      Serial.println("세마포어 해제");
    } else {
      //Serial.println("세마포어 상태 유지"); // 이미 해제된 상태면 유지
    }
  } else {
    // 플래그가 true일 때 세마포어 획득 (take)
    if (xSemaphoreTake(binarySemaphore, (TickType_t)10) == pdTRUE) {
      Serial.println("세마포어 획득");
    } else {
      //Serial.println("세마포어 상태 유지"); // 세마포어를 획득하지 못한 경우 유지
    }
  }
}

// 각 기능 함수 정의
void get_angle_data() {
  Serial.println("angle data acquire");
}
void processing_angle_data() {
  Serial.println("process angle data");
}
void transmit_control_message_to_RWA() {
  Serial.println("transmit control message to RWA");
}
void get_road_wheel_angle_by_RWA() {
  Serial.println("get road wheel angle by RWA");
}

byte* receive_response_with_timeout(unsigned int id_in, unsigned long timeout_ms) {
  unsigned long start_time = millis();
  while (millis() - start_time < timeout_ms) {
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      if (canMsg.can_id == id_in) {
        Serial.print("[Main제어기의 응답 메세지]: ");
        for (int i = 0; i < canMsg.can_dlc; i++) {
          Serial.print(canMsg.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        
        return canMsg.data; // 메시지 수신 성공
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // 짧은 지연 후 재시도
  }
  // 타임아웃 발생
  Serial.println("[Timeout] No response received within 1 second.");
  Serial.println("백업 ecu 제어권한 획득");
  arbiter_flag=0x01;
  //return false;
}
