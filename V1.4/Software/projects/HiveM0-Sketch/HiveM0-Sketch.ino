//###################################################
// Include the libraries we need

#include "OneWire.h"
#include "DallasTemperature.h"
#include "ADS1231.h"
#include "Sodaq_nbIOT.h"
#include "Sodaq_wdt.h"
#include "FlashStorage.h"
#include "EasyHive.h"
#include "rtttl.h"
#include "MemoryFree.h"

char *startup = "OneMoreT:d=16,o=5,b=250:3a7,4p,3a7,4p,3a7,4p,3b7,4p,3b7,4p";

extern int BoardID;
extern int datalogtime;
extern int datasendtime;
int loopcounter;
int sendcounter;

extern Sodaq_nbIOT nbiot;
// Server variables
// define Server variables
extern const char* apn;
extern const char* cdp;
extern unsigned char cid;
extern const char* forceOperator; // optional - depends on SIM / network

int8_t SIGNAL;
uint8_t BER;


// only for debugging reasons:
extern int lengthSent;
extern size_t sizeStrBuffer;

/*
   The setup function. We only start the sensors here and initialize the board state
*/

// Reserve a portion of flash memory to store an "bool" variable
// and call it "calibrated".
//FlashStorage(test, float);

void setup(void)
{
  sendcounter = 0;
  loopcounter = 0;
  // play_rtttl(startup);
  tone(BUZZER, 3950);
  delay(400);
  noTone(BUZZER);
  tone(BUZZER, 4000);
  delay(380);
  noTone(BUZZER);
  tone(BUZZER, 4080);
  delay(370);
  noTone(BUZZER);
  // sodaq_wdt_safe_delay(1000);

  init_RTC();
  init_server_data();
  init_BoardID();
  init_Temp();
  init_Weight();

  float w_calib;
  float w_offset;
  float w_cweight;
  get_Weight_calib(&w_calib, &w_offset, &w_cweight);
  //SerialUSB.println("calib1");
  //SerialUSB.println(w_calib);
  //SerialUSB.println(w_offset);

  float w_calib2;
  float w_offset2;
  float w_cweight2;
  get_Weight_calib2(&w_calib2, &w_offset2, &w_cweight2);
  //SerialUSB.println("calib2");
  //SerialUSB.println(w_calib2);
  //SerialUSB.println(w_offset2);


  // TEST
  pinMode(LC_ON, OUTPUT);
  digitalWrite(LC_ON, LOW);

  //this code is needed to setup watchdogtimer and to make MCU sleep
  sodaq_wdt_enable(WDT_PERIOD_8X); // watchdog expires in ~8 seconds
  sodaq_wdt_reset(); // restting the watchdog
  initSleep();


  //param1 = calib_val (at 1000g); param2 = offset_val(at 0g);
  //set_Weight_calib(1622203.0, -329233);
  //set_Weight_calib(12345.0, 42.0);

  //test.write(1.23);

  //enable_USB_in();

  /****** nbiot init *******/


  DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
  DEBUG_STREAM.println("Initializing and connecting... ");

  MODEM_STREAM.begin(nbiot.getDefaultBaudrate());
  nbiot.setDiag(DEBUG_STREAM);
  nbiot.init(MODEM_STREAM, powerPin, enablePin, -1, cid);

  if (!nbiot.connect(apn, cdp, forceOperator, 8)) {
    DEBUG_STREAM.println("Failed to connect to the modem!");
  }

  // check Signal quality
  nbiot.getRSSIAndBER(&SIGNAL, &BER);

  // read Battery voltage
  float VOLT = getBatteryVoltage();

  //send Message and check answer
  // [FLOAT], [FLOAT], [FLOAT], [FLOAT], [INT8_T], [INTEGER], [LONG]
  // [WEIGHT1],[WEIGHT2],[TEMP],[VOLT],[SIGNALQUALITY],[BOARDID],[PAKETNR/EPOCH]
  String startmsg = "Hello!,,," + String(VOLT) + "," + String(SIGNAL) + "," + String(BoardID);
  if (sendMessageThroughUDP(startmsg.c_str())) {
    DEBUG_STREAM.println("FINE!");
  }
  else
  {
    DEBUG_STREAM.println("NO RESPONSE");
  }

  // DEBUG_STREAM.println(sendMessageThroughUDP("Hello!"));
}

/*
   Main function, get and show the temperature, weight and sleep, send data via NB-IoT
*/
void loop(void){
  unsigned long loopstarttime = millis();
  loopcounter++;

  //SerialUSB.println("==============================================");
  //SerialUSB.print("Free RAM@start loop ");
  int freemem = freeMemory();
  //SerialUSB.println(freemem);

  // start loadcellstuff
  digitalWrite(LC_ON, HIGH);
  digitalWrite(PDWN, HIGH);
  // test delay; let LC-voltage settle...
  // sodaq_wdt_safe_delay(5000);

  float temp_val = get_Temp();
  //SerialUSB.print("Temp: ");
  //SerialUSB.println(temp_val);


  digitalWrite(SELECT, LOW);
  sodaq_wdt_safe_delay(300);
  get_Weight();//dummy weight measure for settlement
  sodaq_wdt_safe_delay(150);
  get_Weight();
  sodaq_wdt_safe_delay(150);

  float weight_val1 = get_Weight();
  //SerialUSB.print("Weight: ");
  //SerialUSB.println(weight_val1);

  // long weight_val_raw1 = get_Weight_raw();
  //SerialUSB.print("Weight Raw: ");
  //SerialUSB.println(weight_val_raw1, DEC);

  digitalWrite(SELECT, HIGH); // read the other side
  sodaq_wdt_safe_delay(300);
  //dummy weight measure for settlement
  get_Weight2();
  sodaq_wdt_safe_delay(150);
  get_Weight2();
  sodaq_wdt_safe_delay(150);

  float weight_val2 = get_Weight2();
  //SerialUSB.print("Weight2: ");
  //SerialUSB.println(weight_val2);

  // long weight_val_raw2 = get_Weight_raw();
  //SerialUSB.print("Weight Raw: ");
  //SerialUSB.println(weight_val_raw2, DEC);

  //Stop LoadecellStuff
  digitalWrite(LC_ON, LOW);
  digitalWrite(SELECT, LOW);
  digitalWrite(PDWN, LOW);


  // check Signal quality
  nbiot.getRSSIAndBER(&SIGNAL, &BER);

  // read Battery voltage
  float VOLT = getBatteryVoltage();
  //read Time from rtc
  long epochtime = get_time();

  // [FLOAT], [FLOAT], [FLOAT], [FLOAT], [INT8_T], [INTEGER], [ULONG]
  // [WEIGHT1],[WEIGHT2],[TEMP],[VOLT],[SIGNALQUALITY],[BOARDID],[Package/EPOCH]

  save_sens_value(weight_val1, weight_val2, temp_val, VOLT, SIGNAL, epochtime );
  sendcounter++; 
  
  // String msg = String(weight_val1) + "," + String(weight_val2) + "," + String(temp_val) + "," + String(VOLT) + "," + String(SIGNAL) + "," + String(BoardID) + "," + String(package) ;

  //float w_calib;
  //float w_offset;
  //get_Weight_calib(&w_calib, &w_offset);
  //SerialUSB.println(w_calib);
  //SerialUSB.println(w_offset);

  if (loopcounter >= datasendtime / datalogtime) {
    // if connection to nbIoT exists or can be established
    if (nbiot.isConnected() || nbiot.connect(apn, cdp, forceOperator, 8)) {
      
      //FIFO - first in first out 
      while (sendcounter > 0) {
       if (sendcounter > SENSOR_BUFFER_LEN) {
         sendcounter = SENSOR_BUFFER_LEN;   // we have only stored SENSOR_BUFFER_LEN amount of data in the buffer.
       }
        // SENSOR_BUFFER_LEN is defined in easyhive.h
        float weight1 = 0;
        float weight2 = 0;
        float temp = 0;
        float volt = 0;
        int8_t csq = 0;
        long epoch = 0;
      
        int pointer_pos = get_sens_pointer(-(sendcounter-1)); // with sendcounter 1 = Offset 0
        read_sens_value(&weight1, &weight2, &temp, &volt, &csq, &epoch, pointer_pos);
        SerialUSB.print("sendcounter: ");
        SerialUSB.println(sendcounter);

        SerialUSB.print("loopcounter: ");
        SerialUSB.println(loopcounter);

        SerialUSB.print("pointer_pos: ");
        SerialUSB.println(pointer_pos);

        // SerialUSB.print("new String: ");
        // SerialUSB.println(result);
        //String msg = String(weight1) + "," + String(weight2) + "," + String(temp) + "," + String(volt) + "," + String(csq) + "," + String(BoardID) + "," + String(epoch) ;       
        
        // changed only for debugging reasons
        // pointer_pos 
        // sendcounter: how often was it not possbile to send?
        // freemem: free memory
        String msg = String(pointer_pos) + "," + String(sendcounter) + "," + String(sizeStrBuffer) + "," + String(lengthSent) + "," + String(csq) + "," + String(freemem) + "," + String(epoch) ;

        SerialUSB.print("manipulated String msg: ");
        SerialUSB.println(msg);       

        // SerialUSB.print("old String: ");
        // SerialUSB.println(msg);

        // try to send the message.
        int success = sendMessageThroughUDP(msg.c_str());

        SerialUSB.print("success of sending message through UDP: ");
        SerialUSB.println(success);

        // if sending data to the server was successfull -> success = 1
        if(success){
          //delay(20);
          //SerialUSB.println("Message was sent. sendcounter -1.");
          sendcounter --; 
        }
        // else if sending data to the server was not possible -> success = 0
        else{
         //SerialUSB.println("Message could not be sent. Reboot SARA Modul and leave loop");
         nbiot.connect(apn, cdp, forceOperator, 8);
         break; // leave for loop. Sleep and try to send the data next time.
        }  
      } // end of while loop
      loopcounter=0; 
    } // end of if (nbiot is connected)
    else {
      DEBUG_STREAM.println("Failed to connect to the modem and/or network!");
    }
  } // end of: if (loopcounter >= datasendtime / datalogtime)

/*
  else {  // loopcounter < datasendtime/datalogtime
    // if data is measured more often then sent, the data is stored in the sensobuffer arrays
    //SerialUSB.print("logging data...");
    float weight1 = 0;
    float weight2 = 0;
    float temp = 0;
    float volt = 0;
    int8_t csq = 0;
    long epoch = 0;
    int pointer_pos = get_sens_pointer(0);
    read_sens_value(&weight1, &weight2, &temp, &volt, &csq, &epoch, pointer_pos);
  } // end of else (loopcounter < datasendtime/datalogtime)
*/
  
  //Stop LoadecellStuff
  digitalWrite(LC_ON, LOW);
  digitalWrite(SELECT, LOW);
  digitalWrite(PDWN, LOW);

  
  // calculate sleeptime
  int sleeptime = datalogtime - (millis() - loopstarttime) / 1000;
  //SerialUSB.print("Sleeptime[s]:");
  //SerialUSB.println(sleeptime);
  if (sleeptime > 0) {
    powerdownfor(sleeptime);
  }

}
