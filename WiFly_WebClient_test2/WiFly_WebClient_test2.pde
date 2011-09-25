#include "WiFly.h"
#include "Credentials.h"
#include <LiquidCrystal.h>

Client client(server, 80);
LiquidCrystal lcd(2, 3, 4, 5, 6, 8);

void setup() {

    
  lcd.begin(20, 4);
  lcd.clear();
  delay(2);
  lcd.setCursor(0, 0);
  lcd.print("SSID: ");
  lcd.print(ssid); 
  lcd.setCursor(0, 1);

  lcd.print("Host: ");
  lcd.print(server);
  lcd.setCursor(0, 2);
  
  delay(2);
  
  Serial.begin(9600);

  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);

  delay(5000);

  Serial.println("starting...");
  delay(10000);
  digitalWrite(9, HIGH);
  



}
int cnt = 1;
void loop() {
  
  Serial.print("loop(");
  Serial.print(cnt);
  Serial.println(") start:");

  WiFly.begin();

  Serial.println("join");
  if (!WiFly.join(ssid, passphrase)) {
    Serial.println("Association failed.");
    while (1) {
      // Hang on failure.
    }
  }  

  Serial.println("connect");

  if (client.connect()) {
    Serial.println("connected");
    client.println("GET /cgi-bin/status.cgi?id=2");
    client.println();
  } 
  else {
    Serial.println("connection failed");
  }

  for(;;) {
    while(client.available()) {
      char c = client.read();
      Serial.print(c);
      lcd.print(c);
    }

    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
      break;
    }
  }

  Serial.print("loop(");
  Serial.print(cnt);
  Serial.println(") end:");

  lcd.print("loop(");
  lcd.print(cnt);
  lcd.println(") end:");

  delay(3000);
  Serial.println("OFF");  
  digitalWrite(9, LOW);
  delay(10000);
  Serial.println("ON");
  digitalWrite(9, HIGH);
  //craps out after 3 at most iterations
  //when connection failed, need to retry, client.connect
//doesn't comeback from 3rd reboot on .begin()  
//pinging helps stabililty from server to wifly - reached 12 loops and still going... html response might be messed up, need to turn off printing extra info to be sure not intermixed; wifly became unreachable in ping after 13 loops, but restarting ping seemed to help - weird, go saints!
//reached loop 17 with actually hitting webmail for status and printing to lcd; todo get format and stuff - look nice, etc... then troubleshoot loooping stall
   cnt++;
 
}






