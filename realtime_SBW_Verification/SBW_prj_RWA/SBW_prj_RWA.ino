/*
 id_ecu3
 RWA Controller
*/
class CircularQueue {
  private:
    double* data;       // 데이터를 저장할 배열
    int front;          // 가장 오래된 데이터의 인덱스
    int rear;           // 가장 최근에 추가된 데이터의 인덱스
    int capacity;       // 큐의 크기
    int count;          // 현재 저장된 데이터 수

  public:
    CircularQueue(int size) {
      capacity = size;
      data = new double[size];
      front = 0;
      rear = -1;
      count = 0;
    }

    ~CircularQueue() {
      delete[] data;
    }

    void enqueue(double value) {
      if (count == capacity) {
        // 큐가 가득 찼다면, 가장 오래된 데이터를 덮어씌움
        front = (front + 1) % capacity;
      } else {
        count++;
      }
      rear = (rear + 1) % capacity;
      data[rear] = value;
    }

    double average() {
      if (count == 0) return 0; // 큐가 비어 있으면 평균 0 반환
      double sum = 0;
      for (int i = 0; i < count; i++) {
        sum += data[(front + i) % capacity];
      }
      return sum / count;
    }
};
#include <SPI.h>
#include <mcp2515.h>
#include <Arduino_FreeRTOS.h>

//#define CANINTF 0x2C  // CANINTF 레지스터 주소를 직접 지정
// 핀 정의
#define encdpin_L 3
#define encdpin_R 2
const int EN_R = 9;
const int IN2 = 8;
const int IN1 = 7;

const int EN_L = 6;
const int IN4 = 5;
const int IN3 = 4;

// 바퀴와 조향 관련 상수
const double wheelDiameter = 0.6; // 바퀴 지름 (m)
const double wheelBase = 1.5;     // 바퀴 간 거리 (m)
volatile int anglePerPulse = 18;  // 펄스당 각도 (degree)

// 속도 및 조향 변수
double input_by_speed = 0;      // 평균 속도 (m/s)
double input_by_steer = 0;       // 조향 입력 (degree)

// 큐 초기화
CircularQueue rpmQueueLeft(5);
CircularQueue rpmQueueRight(5);
// 인터럽트 관련 변수
volatile unsigned long lastTime1 = 0;
volatile unsigned long lastTime2 = 0;
volatile unsigned long currentTime1 = 0;
volatile unsigned long currentTime2 = 0;
volatile double deltaT1 = 0;
volatile double deltaT2 = 0;
volatile unsigned long timeAverageAngularVelocity1 = 0;
volatile unsigned long timeAverageAngularVelocity2 = 0;
volatile int sumPulses1 = 0;
volatile int sumPulses2 = 0;
volatile int averageSample = 50;

#define id_ecu_SFA_Main 0x022
#define id_ecu_SFA_Backup 0x033
#define id_ecu_RWA 0x011


#define job1 0x00 // 생존확인용 job
#define job2 0x01
#define job3 0x02
#define job4 0x03

struct can_frame canMsg;
struct can_frame canMsg1;
struct can_frame canMsg2;
volatile byte arbiter_flag = 0x01;
MCP2515 mcp2515(10);

void setup() {
  lastTime1 = millis();
  lastTime2 = millis();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(encdpin_L, INPUT);
  pinMode(encdpin_R, INPUT);

  attachInterrupt(digitalPinToInterrupt(encdpin_L), interruptFunction1, FALLING);
  attachInterrupt(digitalPinToInterrupt(encdpin_R), interruptFunction2, FALLING);

  Serial.begin(57600);
  Serial.println("start");

  // 모터 초기 설정
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  arbiter_flag = 0x01; // Take a role
  
  
  Serial.println("initialized");

  // FreeRTOS 태스크 생성
  xTaskCreate(messageTask, "messageTask", 256, NULL, 1, NULL);
  xTaskCreate(processingTask, "processingTask", 256, NULL, 1, NULL);
  //mcp2515.setFilterMask(0, true, 0x7FF); // 모든 비트 확인
  //mcp2515.setFilter(0, true, id_ecu_RWA); // RWA 메시지 필터링
}

void loop() {
  // FreeRTOS 환경에서 loop 함수는 빈 상태로 남겨둡니다.
}

// 첫 번째 태스크: CAN 메시지 송수신 및 세마포어 관리
void messageTask(void *pvParameters) {
  while (true) {
    //Serial.println(uxSemaphoreGetCount(binarySemaphore)==1);
    
    byte send_data[8] = {id_ecu_SFA_Backup, arbiter_flag, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    //send_message(id_ecu_SFA_Main, send_data);

     // 25ms 대기
    read_response();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    //clearBufferOverflow();
    //vTaskDelay(100 / portTICK_PERIOD_MS); // 500ms 대기

  }
}

// 두 번째 태스크: 세마포어 상태에 따른 처리 작업
void processingTask(void *pvParameters) {
  while (true){
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      double double_input= input.toDouble();
      input_by_speed = double_input;
      Serial.print("New Speed Reference: ");
      Serial.println(input_by_speed);
      input_by_speed = input_by_speed;
      
    }
    
  // 속도 및 조향 제어
    controlSteeringAndSpeed();
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
    // 수신된 메시지의 ID 확인
    if (canMsg.can_id != id_ecu_RWA){
      //내것이 아니면 넘어가 Serial.print("id가잘못됐습니다");
      return;
    } 
    // 수신된 메시지 출력 (디버깅용)
    Serial.print("[get msg] id = ");
    Serial.print(canMsg.can_id, HEX);
    Serial.print(" - 데이터 = ");
    for (int i = 0; i < canMsg.can_dlc; i++) {
      if (i==1){
        input_by_steer=canMsg.data[i];
        input_by_steer=90-input_by_steer;
      }
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    byte avgRPMLeft = rpmQueueLeft.average();
    byte avgRPMRight = rpmQueueRight.average();
    byte response_data2[8] = {id_ecu_RWA, avgRPMLeft, avgRPMRight, 0x00, 0x00, 0x00, 0x00, 0x00};
      
    if ( canMsg.data[0] == id_ecu_SFA_Main){
      send_message(id_ecu_SFA_Main, response_data2);
    }
    else if (canMsg.data[0] == id_ecu_SFA_Backup){
      send_message(id_ecu_SFA_Backup, response_data2);
    }
    else{
      Serial.println("알 수 없는 CAN ID 메시지 수신"+String(canMsg.data[0]));
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
  Serial.println("[Timeout] No response received within"+ String(timeout_ms) + "second.");
  Serial.println("백업 ecu 제어권한 획득");
  arbiter_flag=0x01;
  //return false;
}
void controlSteeringAndSpeed() {
  // 평균 RPM 계산
  double avgRPMLeft = rpmQueueLeft.average();
  double avgRPMRight = rpmQueueRight.average();

  // 조향각에 따른 속도 차이 계산
  double deltaSpeed = (wheelBase / 2.0) * tan(input_by_steer * M_PI / 180.0)*10;

  // 좌우 바퀴 속도 계산
  double speedLeft = input_by_speed + deltaSpeed;
  double speedRight = input_by_speed - deltaSpeed;

  // 속도 -> PWM 변환
  double leftPWM = speedLeft * (255.0 / 16.0);  // 최대 속도 16 m/s 기준
  double rightPWM = speedRight * (255.0 / 16.0);

  // PWM 값 제한
  leftPWM = constrain(leftPWM, 50, 255);
  rightPWM = constrain(rightPWM, 50, 255);
  if (input_by_speed==0){
    leftPWM=0;
    rightPWM=0;
  }
  // 모터 제어
  analogWrite(EN_L, (int)leftPWM);
  analogWrite(EN_R, (int)rightPWM);

  // 결과 출력
  Serial.print("Reference Vehicle Speed: ");
  Serial.print(input_by_speed);
  Serial.print(", Reference Steering Angle: ");
  Serial.print(input_by_steer);
  Serial.print(", Speed Left: ");
  Serial.print(speedLeft);
  Serial.print(", Speed Right: ");
  Serial.print(speedRight);
  Serial.print(", Left PWM: ");
  Serial.print(leftPWM);
  Serial.print(", Right PWM: ");
  Serial.println(rightPWM);
}
void interruptFunction1() {
  currentTime1 = millis();
  deltaT1 = currentTime1 - lastTime1;
  lastTime1 = currentTime1;

  sumPulses1++;
  timeAverageAngularVelocity1 += deltaT1;

  if (sumPulses1 >= averageSample) {
    double angularVelocity1 = 1000.0 / 6 * ((double)(sumPulses1 * anglePerPulse)) / ((double)timeAverageAngularVelocity1);

    rpmQueueLeft.enqueue(angularVelocity1);

    sumPulses1 = 0;
    timeAverageAngularVelocity1 = 0;
  }
}

void interruptFunction2() {
  currentTime2 = millis();
  deltaT2 = currentTime2 - lastTime2;
  lastTime2 = currentTime2;

  sumPulses2++;
  timeAverageAngularVelocity2 += deltaT2;

  if (sumPulses2 >= averageSample) {
    double angularVelocity2 = 1000.0 / 6 * ((double)(sumPulses2 * anglePerPulse)) / ((double)timeAverageAngularVelocity2);

    rpmQueueRight.enqueue(angularVelocity2);

    sumPulses2 = 0;
    timeAverageAngularVelocity2 = 0;
  }
}


