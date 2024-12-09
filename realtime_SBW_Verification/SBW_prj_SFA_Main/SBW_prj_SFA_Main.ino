/*
 id_ecu1
 id_ecu1이 초기에는 Main Controller(Can Slave)
 페일오버가 이뤄지면, id_ecu2가 backup 상태로 전환
*/

#include <Servo.h>
#include <SPI.h>
#include <mcp2515.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h> // I2C 추가

// CAN용 ID
#define id_ecu_SFA_Main 0x022
#define id_ecu_SFA_Backup 0x033
#define id_ecu_RWA 0x011

// I2C용 주소(7비트)
#define I2C_ADDR_SFA_MAIN 0x22
#define I2C_ADDR_SFA_BACKUP 0x33

// 기존 코드 변수들
Servo myservo;
MCP2515 mcp2515(10);

SemaphoreHandle_t binarySemaphore;
struct can_frame canMsg;
struct can_frame sendMsg;

unsigned long lastMoveTime = 0;  // 마지막으로 모터가 움직인 시간
float currentAngle = 90;  
int previousData = -1;  
int vehicle_speed = 8;  

volatile byte arbiter_flag = 0x01; // 초기 main이 master역할
byte switchState = LOW;

// I2C 기반 역할 제어 관련 변수
volatile bool isMaster = true;  
TaskHandle_t masterI2CTaskHandle = NULL;
TaskHandle_t serialI2CTaskHandle = NULL;

void setup() {
  pinMode(3, OUTPUT);
  pinMode(7, INPUT);
  
  Serial.begin(57600);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  arbiter_flag = 0x01; // 초기 main은 Master

  binarySemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(binarySemaphore);
  xSemaphoreTake(binarySemaphore, (TickType_t)10);
  Serial.println("initialized");

  // FreeRTOS 태스크 생성(기존)
  xTaskCreate(messageTask, "MessageTask", 256, NULL, 1, NULL);
  xTaskCreate(processingTask, "ProcessingTask", 256, NULL, 1, NULL);

  // I2C 초기화: Main은 초기에 Master 역할이므로 그냥 Wire.begin()
  Wire.begin(); // Master 모드 시작
  Serial.println("Main ECU: Started as Master (flag=0x01 means Master for Main)");

  // I2C 관련 태스크 생성
  xTaskCreate(i2cMasterTask, "I2CMasterTask", 256, NULL, 1, &masterI2CTaskHandle);
  xTaskCreate(i2cSerialTask, "I2CSerialTask", 128, NULL, 1, &serialI2CTaskHandle);

  // FreeRTOS 스케줄러는 이미 시작됨(기존코드에서)
}

void loop() {
  // FreeRTOS에서 loop 함수는 빈 상태
}

// 메시지 처리 태스크(기존)
void messageTask(void *pvParameters) {
  while (true) {
    
    checkAndProcessSemaphore(); // Arbiter Flag 대기/처리
    if (arbiter_flag==0x00){
      Serial.print("SFA Backup controls Road-Wheel-Actuator");
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    read_response();
    if (uxSemaphoreGetCount(binarySemaphore) == 0){
        byte steering_angle = (byte)currentAngle;
        byte send_data2[8] = {id_ecu_SFA_Main, steering_angle, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        send_message(id_ecu_RWA, send_data2);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    Serial.println();
  }
}

// 데이터 처리 태스크(기존)
void processingTask(void *pvParameters) {
  while (true) {
    if (uxSemaphoreGetCount(binarySemaphore) == 0){
      get_angle_data();
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// I2C Master 태스크 (I2C를 통한 arbiter_flag 교환)
void i2cMasterTask(void *pvParameters) {
  (void) pvParameters;
  
  for (;;) {
    if (isMaster) {
      // Backup에게 arbiter_flag 전송
      Wire.beginTransmission(I2C_ADDR_SFA_BACKUP);
      Wire.write(arbiter_flag);
      Wire.endTransmission();
      Serial.print("[I2C Master] Sent flag to Backup: 0x");
      Serial.println(arbiter_flag, HEX);

      // Backup으로부터 arbiter_flag 수신
      Wire.requestFrom(I2C_ADDR_SFA_BACKUP, (uint8_t)1);
      if (Wire.available()) {
        byte received_flag = Wire.read();
        Serial.print("[I2C Master] Received flag from Backup: 0x");
        Serial.println(received_flag, HEX);

        // Backup이 0x00을 응답하고, 현재 arbiter_flag도 0x00이면 역전환
        // 여기서는 예제 로직에 맞추어 처리
        if (received_flag == 0x00 && arbiter_flag == 0x00) { 
          Serial.println("[I2C Master] Switching to Slave Mode...");
          isMaster = false;
          arbiter_flag = 0x00; 
          // I2C 슬레이브 전환
          Wire.end();
          Wire.begin(I2C_ADDR_SFA_MAIN);
          Wire.onRequest(onRequestFromMaster); 
          Wire.onReceive(onReceiveFromMaster);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    } else {
      // Slave모드일 때는 여기서 별도 동작 없음
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// I2C를 통한 시리얼 명령 태스크 (예: 'x'입력 시 역할 전환)
void i2cSerialTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    if (Serial.available()) {
      char input = Serial.read();
      if (input == 'x' || input == 'X') {
        if (isMaster) {
          arbiter_flag = 0x00; // 역할 전환 플래그 설정
          Serial.println("[Serial(I2C)] 'x' received: Initiating role swap.");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// I2C Slave용 콜백 함수 (Main이 Slave로 전환될 경우 사용)
void onRequestFromMaster() {
  // arbiter_flag 전송
  Wire.write(arbiter_flag);
  Serial.print("[I2C Slave(Main)] Responded with flag: 0x");
  Serial.println(arbiter_flag, HEX);
}

void onReceiveFromMaster(int numBytes) {
  while (Wire.available()) {
    byte received_flag = Wire.read();
    Serial.print("[I2C Slave(Main)] Received flag from Master: 0x");
    Serial.println(received_flag, HEX);

    if (received_flag == 0x00 && !isMaster) { 
      Serial.println("[I2C Slave(Main)] Switching to Master Mode...");
      isMaster = true;
      arbiter_flag = 0x01;

      // I2C Master 모드 전환
      Wire.end();
      Wire.begin();
    }
  }
}

// 기존 함수들
void read_response() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id != id_ecu_SFA_Main) {
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
          processing_angle_data(); 
        }
        break;

      case id_ecu_SFA_Backup:
        if (canMsg.data[1] == 0x01) {
          Serial.println("Main제어기가 제어중");
        }
        else if(canMsg.data[1] == 0x00){
          Serial.println("Backup 제어기가 제어중");
        }
        if (uxSemaphoreGetCount(binarySemaphore) == 1){
          byte response_data[8] = {id_ecu_SFA_Main, arbiter_flag, 0x00,0x00,0x00,0x00,0x00,0x00};
          send_message(id_ecu_SFA_Backup, response_data);
        }
        break;

      default:
        Serial.println("알 수 없는 CAN ID 메시지 수신");
        break;
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

void transmit_control_message_to_RWA() {
  Serial.println("transmit control message to RWA");
}

void get_road_wheel_angle_by_RWA() {
  Serial.println("get road wheel angle by RWA");
}

void send_message(unsigned int id_out, byte data[]) {
  struct can_frame canMsg1;
  canMsg1.can_id = id_out;
  canMsg1.can_dlc = 8;
  for (int i = 0; i < 8; i++) {
    canMsg1.data[i] = data[i];
  }
  
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

void checkAndProcessSemaphore() {
  if (arbiter_flag==0x00) {
    xSemaphoreGive(binarySemaphore);
  } else {
    if (xSemaphoreTake(binarySemaphore, (TickType_t)10) == pdTRUE) {
      Serial.println("세마포어 획득");
    }
  }
}


