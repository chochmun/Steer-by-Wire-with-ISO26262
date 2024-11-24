/*
 id_ecu1
 id_ecu1이 초기에는 Main Controller(Can Slave)
 페일오버가 이뤄지면, id_ecu2가 backup 상태로 전환
*/

#include <SPI.h>
#include <mcp2515.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

SemaphoreHandle_t binarySemaphore;

#define id_ecu1 0x011
#define id_ecu2 0x022

struct can_frame canMsg;
struct can_frame sendMsg;
MCP2515 mcp2515(10);
volatile byte arbiter_flag = 0x00;
byte switchState = LOW;

void setup() {
  pinMode(7, INPUT);
  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  arbiter_flag = 0x01; // Take a role

  binarySemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(binarySemaphore);
  xSemaphoreTake(binarySemaphore, (TickType_t)10);
  Serial.println("initialized");

  // FreeRTOS 태스크 생성
  xTaskCreate(messageTask, "MessageTask", 256, NULL, 1, NULL);
  xTaskCreate(processingTask, "ProcessingTask", 256, NULL, 1, NULL);
}

void loop() {
  // FreeRTOS에서 loop 함수는 빈 상태로 남겨둡니다.
}

// 메시지 처리 태스크
void messageTask(void *pvParameters) {
  
  while (true) {
    switchState = digitalRead(7);
    
    byte response_data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, switchState, 0x00, 0x00};
    byte* main_msg=read_response(id_ecu1, id_ecu2, response_data);
    if(main_msg[4]==0x01){ // backup_ecu's arbiter_flag=true
      arbiter_flag=0x00;
    }
    else if(main_msg[4]==0x00){
      arbiter_flag=0x01;
    }
    vTaskDelay(25 / portTICK_PERIOD_MS); // 25ms 대기

    checkAndProcessSemaphore(); // Arbiter Flag를 대기
  }
}

// 데이터 처리 태스크
void processingTask(void *pvParameters) {
  while (true) {
    if (uxSemaphoreGetCount(binarySemaphore) == 0) {
      // get_angle_data();
      processing_angle_data();
      // transmit_control_message_to_RWA();
      // get_road_wheel_angle_by_RWA();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 필요에 따라 주기를 조절하세요
  }
}

// 응답을 읽고 전송하는 함수
byte* read_response(unsigned int id_in, unsigned int id_out, byte data[]) {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id != id_in) return;

    Serial.print("[get msg by Backup ecu] id = ");
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" - data =");

    for (int i = 0; i < canMsg.can_dlc; i++) { // print the data
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }

    struct can_frame canMsg1;
    canMsg1.can_id = id_out; // Backup controller에게 현재 main 상태를 제공
    canMsg1.can_dlc = 8;
    for (int i = 0; i < 8; i++) {
      canMsg1.data[i] = data[i];
    }
    Serial.println();
    // 메시지를 보냄
    mcp2515.sendMessage(&canMsg1);
    Serial.print("[send msg to Backup ecu] id = ");
    Serial.print(id_out, HEX);
    Serial.print(" - data =");
    for (int i = 0; i < canMsg1.can_dlc; i++) { // print the data
      Serial.print(canMsg1.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    Serial.println();
  }
  return canMsg.data;
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

// 플래그에 따라 세마포어를 획득, 해제, 유지하는 함수
void checkAndProcessSemaphore() {
  //Serial.print("arbiter_flag---");
  //Serial.println(arbiter_flag);
  if (arbiter_flag==0x00) {
    // 세마포어가 이미 give된 상태인지 확인
    xSemaphoreGive(binarySemaphore);
    
  } else {
    // 플래그가 true일 때 세마포어 획득 (take)
    if (xSemaphoreTake(binarySemaphore, (TickType_t)10) == pdTRUE) {
      Serial.println("세마포어 획득");
    } else {
      // Serial.println("세마포어 상태 유지"); // 세마포어를 획득하지 못한 경우 유지
    }
  }
}
