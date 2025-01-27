// Basic ESP8266 Packet Monitor Interface for the SH1106
// github.com/HakCat-Tech/ESP8266-Packet-Monitor

#include "./esppl_functions.h"

#include "SH1106Wire.h"
SH1106Wire display(0x3C, SDA, SCL); // use builtin i2C

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels {1, D8, NEO_GRB + NEO_KHZ800 }; // NeoPixel pin

#define ltBtn D7 // left button
#define rtBtn D5 // right button

// button states and previous states
int lState = 0; int plState = 1; 
int rState = 0; int prState = 1;

String packet[7];
String devices[100][3]; int devCnt = 0;
String srcMac, ssid, src, dest;
char srcOctet[2], destOctet[2];
int addr, fst, ft;
String pktType;

int filter = 0; // 0 = ALL, 1 = DEAUTH, 2 = PROBE REQ

void cb(esppl_frame_info *info) { /*--- WiFi Scanner Function ---*/
  ssid = "";
    src = "";  // source 
  
  Serial.print("\n");
  Serial.print("FT: ");  
  Serial.print((int) info->frametype);
  
  Serial.print(" FST: ");  
  Serial.print((int) info->framesubtype);
  
  Serial.print(" SRC: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02x", info->sourceaddr[i]);
      for (int i= 0; i< 6; i++) { 
      sprintf(srcOctet, "%02x", info->sourceaddr[i]); 
      src+=srcOctet;
    }
  
  Serial.print(" DEST: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02x", info->receiveraddr[i]);
    dest = "";   // dest MAC
    for (int i= 0; i< 6; i++) { 
      sprintf(destOctet, "%02x", info->receiveraddr[i]); dest+=destOctet;
    }
  
  Serial.print(" RSSI: ");
  Serial.print(info->rssi);
  
  Serial.print(" SEQ: ");
  Serial.print(info->seq_num);
  
  Serial.print(" CHNL: ");
  Serial.print(info->channel);
  
  if (info->ssid_length > 0) {
    Serial.print(" SSID: ");
    for (int i = 0; i < info->ssid_length; i++) Serial.print((char) info->ssid[i]);    
  }
  if (info->ssid_length > 0) {
     for (int i= 0; i< info->ssid_length; i++) { ssid+= (char) info->ssid[i]; }
    }

   // append packets metadata to packet list
    packet[0] = (String) info->frametype;
    packet[1] = (String) info->framesubtype;
    packet[2] = src;
    packet[3] = dest;
    packet[4] = (String) info->rssi;
    packet[5] = (String) info->channel;
    packet[6] = ssid;
    ft = packet[0].toInt(); fst = packet[1].toInt();     
}

void printPacket() { // function to print wifi packets to the screen

  // flag packet w/ frame + subframe type
  if (filter==0 || (filter==1 && ft == 0 and fst == 12) || (filter==2 && ft == 0 and fst == 4 )) {
    if      (ft == 0 and (fst == 0 or fst == 1)) pktType = "Association Req.";
    else if (ft == 0 and (fst == 2 or fst == 3)) pktType = "Re-Assoc";
    else if (ft == 0 and fst == 4) pktType = "Probe Request";
    else if (ft == 0 and fst == 8 ) pktType = "Beacon";
    else if (ft == 0 and fst == 10) pktType = "Disassociation";
    else if (ft == 0 and fst == 11) pktType = "Authentication";
    else if (ft == 0 and fst == 12) pktType = "De-Authentication";
    else if (ft == 0) pktType = "Management";
    else if (ft == 1) pktType = "Control";
    else if (ft == 2) pktType = "Data";
    else pktType = "Extension";

    srcMac = packet[2];
    display.drawString(0,0,"PKT: "); display.drawString(30,0,pktType);
    display.drawString(0,8,"SRC: "); display.drawString(30,8,srcMac); 
    display.drawString(0,16,"DST: "); display.drawString(30,16, packet[3]);
    display.drawString(0,24,"RSS: "); display.drawString(30,24,packet[4]); 
    display.drawString(0,32,"CH: "); display.drawString(30,32, packet[5]);
    display.drawString(0,40,"SSID: ");
    if (packet[6].length() < 18) { display.drawString(30,40,packet[6]); }
    else if (packet[6].length() > 1) { display.drawString(30,40,packet[6].substring(0, 17 ) + "..."); }
    
  }
}

// check if button is pressed 
void checkForPress() {
  lState = digitalRead(ltBtn);
  rState = digitalRead(rtBtn);
  
  if (lState == 0 && lState!=plState && filter>0) {filter--;}
  else if (rState ==0 && rState!=prState && filter<2) {filter++;}
  else if(lState == 0 && lState!=plState) {filter = 2;}
  else if(rState ==0 && rState!=prState) {filter = 0;}
  
  plState = lState;
  prState = rState;
  
  
  
}


void updateMenu() { // update scroll menu and packet type selection
  display.drawLine(0,54, 127,54);
  display.drawLine(20,54, 20,63);
  display.fillTriangle(8, 59, 11, 56, 11, 62);
  display.drawLine(107,54, 107,63); 
  display.fillTriangle(119, 59, 116, 56, 116, 62);
  
  if (filter == 0) {
   display.drawString(55,54,"ALL");
   pixels.setPixelColor(0, pixels.Color(0, 150, 0)); pixels.show();
  }
  else if (filter == 1) {
    display.drawString(42,54,"DEAUTH");
    pixels.setPixelColor(0, pixels.Color(150, 0, 0)); pixels.show();
  }
  else {
    display.drawString(45,54,"PROBE");
    pixels.setPixelColor(0, pixels.Color(0, 0, 150)); pixels.show();
  }
}

void setup() {
  pinMode(ltBtn, INPUT_PULLUP);
  pinMode(rtBtn, INPUT_PULLUP);
  
  delay(500);
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  pixels.begin(); pixels.clear();
  esppl_init(cb);
}

void loop() {
  esppl_sniffing_start();
  while (true) {
    for (int i = 1; i < 15; i++ ) {
      esppl_set_channel(i);
      while (esppl_process_frames()) {
        //
      }
    }
    checkForPress();
    display.clear();
    updateMenu();
    printPacket();
    display.display();
    //if (filter>0) delay(600); //dumb delay to display packets longer
    delay(0);
  }  
}
