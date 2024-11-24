#include <Arduino_FreeRTOS.h>

bool taskFlag = false;  // 세마포어 대체 플래그

void Task1(void *pvParameters);
void Task2(void *pvParameters);

void setup() {
  Serial.begin(9600);

  // 태스크 생성
  xTaskCreate(Task1, "Task1", 128, NULL, 1, NULL);
  xTaskCreate(Task2, "Task2", 128, NULL, 1, NULL);
}

void loop() {
  // FreeRTOS 환경에서는 loop() 함수 비워둠
}

void Task1(void *pvParameters) {
  while (1) {
    if (!taskFlag) {  // 플래그가 false일 때 실행
      Serial.println("Task1 실행 중...");
      taskFlag = true;  // Task2에게 제어권 넘김
      vTaskDelay(1000 / portTICK_PERIOD_MS);  // 1초 대기
    }
  }
}

void Task2(void *pvParameters) {
  while (1) {
    if (taskFlag) {  // 플래그가 true일 때 실행
      Serial.println("Task2 실행 중...");
      taskFlag = false;  // Task1에게 제어권 넘김
      vTaskDelay(1000 / portTICK_PERIOD_MS);  // 1초 대기
    }
  }
}
