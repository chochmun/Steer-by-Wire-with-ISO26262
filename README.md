###MCU redundancy 실시간(RT) 구현
![realtime SBW( steer by wire) with MCU redundancy 0-14 screenshot](https://github.com/user-attachments/assets/aba77c9c-0642-477f-a638-bb9b284e5876)


### 시스템설계_전체
![sbw_semi_complete](https://github.com/user-attachments/assets/107f6f1b-5fe9-4412-a992-dd66a443cf6d)



### 차속에따른 가변조향비 모델링 검증그래프
![차속에따른 가변조향비 모델링 검증그래프](https://github.com/user-attachments/assets/62fb6b4b-4d4b-43c8-916b-a0468a7c2a7a)

### CAN 통신 구현 검증래프
![can통신구현](https://github.com/user-attachments/assets/b54a87a1-e397-42d8-a33b-05cedcce5dde)

ECU 이중화 _ 중재제어 영상
https://youtu.be/oTrxxru4FOQ
![SbW 프로젝트 중- ECU 이중화 _ 중재제어 0-21 screenshot](https://github.com/user-attachments/assets/be7e6c27-36bd-4c03-a00f-3b446a89b9e7)

1. 아이템 식별 (Item Identification)
•	아이템 이름: Steer-by-Wire (SbW) 시스템
•	아이템 설명: Steer-by-Wire 시스템은 물리적인 조향 장치(스티어링 칼럼)를 제거하고 전기 신호를 사용해 차량 바퀴를 조향하는 시스템입니다. 운전자의 핸들 조작을 센서로 감지하고 이를 기반으로 **전자 제어 장치(ECU)**가 조향각을 결정, **Road Wheel Actuator (RWA)**를 통해 실제로 바퀴를 회전시키며, Steer Force Actuator (SFA)를 통해 운전자에게 조향 피드백을 제공합니다.
(자세한 내용은 SbW_ItemDefifnition.docx 파일을 확인)

## 안전 케이스 도출
### HARA
![image](https://github.com/user-attachments/assets/dd0b58f7-72e8-4309-a47d-68ca20e9d506)
### Safety Goal
![image](https://github.com/user-attachments/assets/2311ae4d-ac84-448f-a3cf-45fe4ceb7cd6)
### FSR
![image](https://github.com/user-attachments/assets/74b6b434-f214-4eaa-be4e-3b687b0cd7d0)
### 예비 아키텍쳐
![image](https://github.com/user-attachments/assets/fccf64b9-80c2-4815-bb80-4f452490753b)

### TSR (개발 가능한 범위의 4개의 요구사항을 집중함)
![image](https://github.com/user-attachments/assets/29e1c734-687e-4032-a3bf-829160d5388e)
