/*
 * 예제125-1-1 (마스터코드)
 * 기존 MCP2515의 레퍼런스 코드를 기능을 강화한 V2버전을 만들기로함!
 * 첫번째로 마스터는 슬레이브가 회신한값을 확실히 받고 다음 명령을 수행하자!
 * 그러기 위해서 슬레이브가 답장을 줄때까지 일단 기다려본다!
 * 그러나 무한정 기다릴 수는 없기에 3초가 지나면 해당 슬레이브를 오프라인 처리하자!
 */

#include <SPI.h>
#include <mcp2515.h>

#define slave1 0x0F6
#define slave2 0x036

#define job1 0x00 //생존확인용 job
#define job2 0x01
#define job3 0x02
#define job4 0x03


struct can_frame canMsg2;
MCP2515 mcp2515(10);


void setup() {
  Serial.begin(9600);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS);
  mcp2515.setNormalMode();
  
  Serial.println("Example: Write to CAN");
}

void loop() {
  //Request slave1
  byte data[8] = {0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 
  byte data_2[8] = {0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00}; 
  mcp2515_send(slave1,data); //생존확인용


  //Response slave1
  byte recv[8];
  unsigned int id = mcp2515_receive(recv);

  if(id == -1){
    Serial.println("슬레이브1 오프라인!");
  }else{
    //데이터 정상수신
    //프린트
    Serial.print("[recived ID] =");
    Serial.print(id,HEX);
    Serial.print(", [ received Data ]=");
    for(int i = 0;i<8;i++){
      Serial.print(recv[i]);
      Serial.print(",");
    }
    Serial.println();
  }

  //Request slave2
  mcp2515_send(slave2,data_2);

  //Response slave2
  id = mcp2515_receive(recv);
  if(id == -1){
    Serial.println("슬레이브2 오프라인!");
  }else{
    //데이터 정상수신
    //프린트
    Serial.print("[recived ID] =");
    Serial.print(id,HEX);
    Serial.print(", [ received Data ]=");
    for(int i = 0;i<8;i++){
      Serial.print(recv[i]);
      Serial.print(",");
    }
    Serial.println();
  }

  /*
  //Response
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("[슬레이브1의 응답데이터]");
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      if(i==1 || i==2){
        Serial.print((char)canMsg.data[i]);
      }else{
        Serial.print(canMsg.data[i],HEX);
      }
      Serial.print(" ");
    }

    Serial.println();
  }
  
  /*
  //Request slave1
  mcp2515.sendMessage(&canMsg2);
  Serial.println("[마스터에서 슬레이브2로 보낸 메시지]");
  for (int i = 0; i<canMsg2.can_dlc; i++)  {  // print the data
      Serial.print(canMsg2.data[i],HEX);
      Serial.print(" ");
  }
  Serial.println();
  delay(50);
  //Response
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("[슬레이브2의 응답데이터]");
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      if(i==1 || i==2){
        Serial.print((char)canMsg.data[i]);
      }else{
        Serial.print(canMsg.data[i],HEX);
      }
      Serial.print(" ");
    }

    Serial.println();
  }
  Serial.println("Messages sent");
  
  */
  delay(100);
}


void mcp2515_send(unsigned int id, byte data[]){
  struct can_frame canMsg1;
  canMsg1.can_id  = id; //슬레이브1의 ID
  canMsg1.can_dlc = 8;
  for(int i =0;i<8;i++){
    canMsg1.data[i] = data[i];
  }
  mcp2515.sendMessage(&canMsg1);
  Serial.print("[마스터에서 슬레이브로 보낸 메시지] = ");
  Serial.print(id,HEX);
  Serial.print(" - ");
  for (int i = 0; i<canMsg1.can_dlc; i++)  {  // print the data
      Serial.print(canMsg1.data[i],HEX);
      Serial.print(" ");
  }
  Serial.println();
}
unsigned int mcp2515_receive(byte recv[]){
  struct can_frame canMsg;
  unsigned long t = millis(); //이 타이밍의 시간을 측정
  while(1){
    if(millis() - t > 3000){
      //루프 진입후 3초가 지난 시점
      //Serial.println("슬레이브1 오프라인");
      return -1;
      break;
    }
    //나 슬레이브1의 응답데이터가 있을때까지 무조건 기다릴거야!
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      for(int i =0;i<8;i++){
        recv[i] = canMsg.data[i];
      }
      return canMsg.can_id;
      break;
    }
  }
}
