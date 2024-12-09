/*
 id_ecu2
 id_ecu2이 초기에는 backup controller(Can Master)
 페일오버가 이뤄지면, id_ecu2가 backup 상태로 전환
*/



#include <SPI.h>
#include <mcp2515.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h> // I2C 추가

SemaphoreHandle_t binarySemaphore;

#define id_ecu_SFA_Main 0x022
#define id_ecu_SFA_Backup 0x033
#define id_ecu_RWA 0x011

// I2C용 주소
#define I2C_ADDR_SFA_MAIN 0x22
#define I2C_ADDR_SFA_BACKUP 0x33

#define job1 0x00 
#define job2 0x01
#define job3 0x02
#define job4 0x03

unsigned long lastMoveTime = 0;  
float currentAngle = 90;  
int previousData = -1;  
int vehicle_speed = 8;  

struct can_frame canMsg;
struct can_frame canMsg1;
struct can_frame canMsg2;
volatile byte arbiter_flag = 0x01;
MCP2515 mcp2515(10);

// I2C 기반 역할 제어 변수
volatile bool isSlave = true;  
TaskHandle_t slaveI2CTaskHandle = NULL;

void setup() {
  Serial.begin(57600);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  arbiter_flag = 0x01; // Backup은 초기 slave

  binarySemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(binarySemaphore);
  Serial.println("initialized");

  // FreeRTOS 태스크 생성(기존)
  xTaskCreate(messageTask, "messageTask", 256, NULL, 1, NULL);
  xTaskCreate(processingTask, "processingTask", 256, NULL, 1, NULL);

  // I2C Slave 모드 시작
  Wire.begin(I2C_ADDR_SFA_BACKUP); 
  Wire.onRequest(onRequestFromMaster);
  Wire.onReceive(onReceiveFromMaster);
  Serial.println("Backup ECU: Started as Slave (flag=0x01 means Slave for Backup)");

  // I2C Slave 태스크
  xTaskCreate(i2cSlaveTask, "I2CSlaveTask", 256, NULL, 1, &slaveI2CTaskHandle);
}

void loop() {
  // FreeRTOS 사용 시 loop() 비움
}

// 기존 messageTask
void messageTask(void *pvParameters) {
  while (true) {
    if (uxSemaphoreGetCount(binarySemaphore) == 0){
        byte steering_angle = (byte)currentAngle;
        byte send_data[8] = {id_ecu_SFA_Backup, steering_angle, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        send_message(id_ecu_RWA, send_data);
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
    else{
      Serial.println("SFA-Main controls RWA");
    }

    read_response();
    vTaskDelay(50 / portTICK_PERIOD_MS); 
    checkAndProcessSemaphore();
  }
}

// 기존 processingTask
void processingTask(void *pvParameters) {
  while (true) {
    vTaskDelay(100 / portTICK_PERIOD_MS); 
    if (uxSemaphoreGetCount(binarySemaphore) == 0){
      get_angle_data();
    }
  }
}

// I2C Slave 태스크 (역할 전환 후 Master 모드 등 필요 시 추가 동작)
void i2cSlaveTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (isSlave) {
      // Slave 모드 동작
      vTaskDelay(pdMS_TO_TICKS(50));
    } else {
      // Master 모드로 전환 시(필요할 경우) 추가 동작
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// I2C Slave 요청 콜백
void onRequestFromMaster() {
  // 현재 arbiter_flag 전송
  Wire.write(arbiter_flag);
  Serial.print("[I2C Slave(Backup)] Responded with flag: 0x");
  Serial.println(arbiter_flag, HEX);
}

// I2C Slave 수신 콜백
void onReceiveFromMaster(int numBytes) {
  while (Wire.available()) {
    byte received_flag = Wire.read();
    Serial.print("[I2C Slave(Backup)] Received flag from Main: 0x");
    Serial.println(received_flag, HEX);

    if (received_flag == 0x00 && isSlave) { 
      Serial.println("[I2C Slave(Backup)] Switching to Master Mode...");
      isSlave = false;
      arbiter_flag = 0x00;

      // I2C Master로 전환
      Wire.end();
      Wire.begin();
    }
  }
}

// 기존 함수들
void send_message(unsigned int id_out, byte data[]) {
  struct can_frame canMsg1;
  canMsg1.can_id = id_out;
  canMsg1.can_dlc = 8;
  for (int i = 0; i < 8; i++) {
    canMsg1.data[i] = data[i];
  }
  
  Serial.println();
  mcp2515.sendMessage(&canMsg1);
  Serial.print("[send msg] id =");
  Serial.print(id_out, HEX);
  Serial.print(" - data =");
  for (int i = 0; i < canMsg1.can_dlc; i++) {
    Serial.print(canMsg1.data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void read_response() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id != id_ecu_SFA_Backup){
      return;
    } 
    Serial.print("[get msg] id = ");
    Serial.print(canMsg.can_id, HEX);
    Serial.print(" - 데이터 = ");
    for (int i = 0; i < canMsg.can_dlc; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    switch (canMsg.data[0]) {
      case id_ecu_RWA:
        if (uxSemaphoreGetCount(binarySemaphore) == 0) {
          get_road_wheel_angle_by_RWA(); 
        }
        break;
      
      case id_ecu_SFA_Main:
        if (canMsg.data[1] == 0x01) {
          arbiter_flag = 0x01;
          Serial.println("Main제어기가 제어중");
        }
        else if (canMsg.data[1] == 0x00){
          arbiter_flag = 0x00;
          Serial.println("Backup 제어기가 제어중");
        }
        if (canMsg.data[7] == 0x01) { 
          arbiter_flag = 0x00;
          Serial.println("Error injection!! ");
          byte response_data[8] = {id_ecu_SFA_Backup, arbiter_flag, 0x00, 0x00,0x00,0x00,0x00,0x00};
          send_message(id_ecu_SFA_Main, response_data);
        }
        if (uxSemaphoreGetCount(binarySemaphore) == 0){
          byte response_data[8] = {id_ecu_SFA_Backup, arbiter_flag, 0x00,0x00,0x00,0x00,0x00,0x00};
          send_message(id_ecu_SFA_Main, response_data);
        }
        
        break;

      default:
        Serial.println("알 수 없는 CAN ID 메시지 수신"+String(canMsg.data[0]));
        break;
    }
  }
}

void checkAndProcessSemaphore() {
  if (arbiter_flag==0x01) {
    xSemaphoreGive(binarySemaphore);
  }else if (arbiter_flag==0x00) {
    if (xSemaphoreTake(binarySemaphore, (TickType_t)10) == pdTRUE) {
      Serial.println("세마포어 획득");
    }
  }
}

void get_angle_data() {
  int data = analogRead(A2); 
  int degree = map(data, 28, 596, 0, 180); 
  currentAngle = degree;
  if (abs(data - previousData) > 2) {
    Serial.print("현재 각도 = ");
    Serial.print(degree);
    Serial.print(", 저항값 = ");
    Serial.println(data);
    lastMoveTime = millis();
    previousData = data;
  }
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

