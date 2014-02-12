// **********************************************************************************************************
// LaundryMote sketch that works with Moteinos equipped with HopeRF RFM69W/RFM69HW
// Can be adapted to use Moteinos using RFM12B
// 2013-12-26 (C) klintholmes@gmail.com 
// **********************************************************************************************************
// Uses 2 current sensors to detect wheter or not the washer/dryer are in use and 
// can be used to send alerts when washing/drying is done.
// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/3.0/
// **********************************************************************************************************

#include <RFM69.h>  //install this library in your Arduino library directory from https://github.com/LowPowerLab/RFM69
#include <SPI.h>

//*****************************************************************************************************************************
// ADJUST THE SETTINGS BELOW DEPENDING ON YOUR HARDWARE/SITUATION!
//*****************************************************************************************************************************
#define GATEWAYID               1
#define NODEID                  7
#define NETWORKID               100

#define FREQUENCY               RF69_915MHZ
#define ENCRYPTKEY              "0123456789abcdef" 
#define IS_RFM69HW
#define LED                             9
#define CURRENTSENSOR1                  A0
#define CURRENTSENSOR2                  A1
#define WASHER_CURRENT_THRESHOLD        1200
#define TIME_BETWEEN_ALERTS             130000  // 130 seconds 
#define WASHER_ALERT_TIMER              10000 // 10 seconds 

#define DRYER_CURRENT_THRESHOLD         2700 
#define CURRENT_MOVEMENT_TIME           2500 
#define SERIAL_BAUD                     115200
//*****************************************************************************************************************************

RFM69 radio;
char input;


// might rename struct to match the name like LaundryPayload / GaragePayload
typedef struct {                
    int           nodeId; //store this nodeId
    unsigned long uptime;
    boolean       washerStatus;
    boolean       dryerStatus;
    boolean       washerAlert;
    boolean       dryerAlert;
} Payload;
Payload data;

long last = -1;
long WASHER_ON;
long WASHER_OFF;
long VOLTAGE_OVER_PERIOD_WASHER;
long VOLTAGE_OVER_PERIOD_DRYER;
long LAST_VOLTAGE_OVER_PERIOD_WASHER;
long LAST_VOLTAGE_OVER_PERIOD_DRYER;
long LAST_WASHER_ALERT;
boolean LAST_WASHER_STATUS;
boolean LAST_DRYER_STATUS;
boolean REQUEST_STATUS;

void setup(void){
    Serial.begin(SERIAL_BAUD);
    pinMode(CURRENTSENSOR1, INPUT);
    pinMode(CURRENTSENSOR2, INPUT);
    pinMode(LED, OUTPUT);
    WASHER_ON = 0;
    WASHER_OFF = 0;
    VOLTAGE_OVER_PERIOD_WASHER = 0;
    VOLTAGE_OVER_PERIOD_DRYER = 0;
    LAST_VOLTAGE_OVER_PERIOD_WASHER = 0;
    LAST_VOLTAGE_OVER_PERIOD_DRYER = 0;
    LAST_WASHER_ALERT = 0;
    LAST_WASHER_STATUS = false;
    LAST_DRYER_STATUS = false;
    REQUEST_STATUS = true;
    radio.initialize(FREQUENCY,NODEID,NETWORKID);
    #ifdef IS_RFM69HW
        radio.setHighPower(); //must include only for RFM69HW!
    #endif
    radio.encrypt(ENCRYPTKEY);
    char buff[50];
    sprintf(buff, "LaundryMote : %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
    Serial.println(buff);
}

int count = 0;
void loop() {
    if (Serial.available()) { 
        input = Serial.read();
    }

    if (input=='r') {
        REQUEST_STATUS = true;
        input = 0;
    }
    
    int WASHER = analogRead(CURRENTSENSOR1);
    int DRYER = analogRead(CURRENTSENSOR2);
    
    VOLTAGE_OVER_PERIOD_WASHER += WASHER;
    VOLTAGE_OVER_PERIOD_DRYER += DRYER;
    
    int current  = millis() / CURRENT_MOVEMENT_TIME;
    if (current != last) {

        // STILL NEEDS WORK
        if ((LAST_VOLTAGE_OVER_PERIOD_WASHER > WASHER_CURRENT_THRESHOLD && VOLTAGE_OVER_PERIOD_WASHER != 0) || (VOLTAGE_OVER_PERIOD_WASHER > WASHER_CURRENT_THRESHOLD && LAST_VOLTAGE_OVER_PERIOD_WASHER != 0)) {
            data.washerStatus = true;
            WASHER_ON = millis();
        } else {
            if (millis() - WASHER_ON > WASHER_ALERT_TIMER) {
               // Serial.println("WASHER: OFF");  
                data.washerStatus = false;
                WASHER_OFF = millis();
            }
            //data.washerStatus = LAST_WASHER_STATUS;
        }
        
        // DEBUG GRAPHING DATA 
         if (LAST_WASHER_STATUS  == true && data.washerStatus == false  && millis() - LAST_WASHER_ALERT > TIME_BETWEEN_ALERTS) {
            Serial.print("{ \"uptime\" : \"");
            Serial.print(millis());
            Serial.print("\", \"voltage\" : \"");
            Serial.print(VOLTAGE_OVER_PERIOD_WASHER);
            Serial.println("\", \"alert\" : 1 }");
            //data.washerAlert = true;
            //LAST_WASHER_ALERT = millis();
         } else {
            Serial.print("{ \"uptime\" : \"");
            Serial.print(millis());
            Serial.print("\", \"voltage\" : \"");
            Serial.print(VOLTAGE_OVER_PERIOD_WASHER);
            Serial.println("\", \"alert\": 0 }");
            //data.washerAlert = false;
         }
      
        if ((LAST_VOLTAGE_OVER_PERIOD_DRYER > DRYER_CURRENT_THRESHOLD && VOLTAGE_OVER_PERIOD_DRYER != 0) || (VOLTAGE_OVER_PERIOD_DRYER > DRYER_CURRENT_THRESHOLD && LAST_VOLTAGE_OVER_PERIOD_DRYER != 0)) {
            data.dryerStatus = true;
            //Serial.println("DRYER: ON");
        } else {
            data.dryerStatus = false;
            //Serial.println("DRYER: OFF");
        }
      
        if (data.washerStatus != LAST_WASHER_STATUS || data.dryerStatus != LAST_DRYER_STATUS || REQUEST_STATUS) {    
            if (LAST_WASHER_STATUS  == true && data.washerStatus == false && millis() - LAST_WASHER_ALERT > TIME_BETWEEN_ALERTS) {
                data.washerAlert = true;
                LAST_WASHER_ALERT = millis();
                //Serial.println("ALERT"); 
            } else {
                data.washerAlert = false;
            }
        
            if (LAST_DRYER_STATUS == true && data.dryerStatus == false) {
                data.dryerAlert = true;
                //Serial.println(" #######  ######   DRYER ALERT!   #######  ####### ");
            } else {
                data.dryerAlert = false; 
            }
        
            data.uptime = millis(); // number of milliseconds since last power off
            data.nodeId = NODEID; // node id
            Serial.print("Sending data (");
            Serial.print(sizeof(data));
            Serial.print(" bytes)...");
            if (radio.sendWithRetry(GATEWAYID, (const void*)(&data), sizeof(data))) {
                Serial.println("ok!");
            } else { 
                Serial.println("nothing...");
            } 
        
            LAST_WASHER_STATUS = data.washerStatus;
            LAST_DRYER_STATUS = data.dryerStatus;
            REQUEST_STATUS = false;
        }
      
        LAST_VOLTAGE_OVER_PERIOD_WASHER = VOLTAGE_OVER_PERIOD_WASHER;
        LAST_VOLTAGE_OVER_PERIOD_DRYER = VOLTAGE_OVER_PERIOD_DRYER;
      
        VOLTAGE_OVER_PERIOD_WASHER = 0;
        VOLTAGE_OVER_PERIOD_DRYER = 0;
        last = current;
    }
  
    if (radio.receiveDone()) {
        REQUEST_STATUS = false;
        Serial.println('[');
        Serial.println(radio.SENDERID, DEC);
        Serial.println("] ");

        for (byte i = 0; i < radio.DATALEN; i++) {
            Serial.print((char)radio.DATA[i]);
        }

        if (radio.DATA[0]=='S' && radio.DATA[1]=='T' && radio.DATA[2]=='S') {
            REQUEST_STATUS = true;
        }

        //Serial.print("   [RX_RSSI:");
        //Serial.print(radio.readRSSI());
        //Serial.print("]");
        
        if (radio.ACK_REQUESTED) {
            radio.sendACK();
            Serial.print(" - ACK sent.");
        }
    }
}
