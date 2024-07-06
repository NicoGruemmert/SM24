
#include <freertos/FreeRTOS.h>

#include <TFT_eSPI.h>
#include <RotaryEncoder.h>

#include "omegaMotion.h"
#include <omegaTFT.h>
#include <omegaButton.h>

#include <Wire.h>
#include <vector>
#include "myMenu.h"
#include "mqttManager.h"

#include "WiFi.h"


#ifdef DEBUG_ENABLED
  #define DEBUG_PRINT(message) Serial.print(message)
  #define DEBUG_PRINTLN(message) Serial.println(message)
#else
  #define DEBUG_PRINT(message)
  #define DEBUG_PRINTLN(message)
#endif

#define PIN_SW D6
#define PIN_DT D1
#define PIN_CLK D3

volatile int swCount = 0;
volatile byte encoderPos = 0;
volatile int lastCLKstate = 0;





TaskHandle_t dpTaskHandle;
TaskHandle_t gyroTaskHandle;
TaskHandle_t inputTaskHandle;
TaskHandle_t wifiTaskHandle;
       // Include the graphics library


SemaphoreHandle_t xSemaphore4tft;
SemaphoreHandle_t xCalibrationMutex;
SemaphoreHandle_t xWirelessMutex;

omegaButton myButton = omegaButton(PIN_SW);
RotaryEncoder myRotor = RotaryEncoder( PIN_DT,PIN_CLK, RotaryEncoder::LatchMode::FOUR3);

uint8_t mypercent=0;
volatile buttonEvent currentEvent;


void inputTask(void * paParameters)
{
  static int lastswcount;
  static int lastenc;
  int rotaryState;

  long rpm;
  while (1)
  {
    
    if( xSemaphoreTake( xSemaphore4tft, ( TickType_t ) 100 ) == pdTRUE )
    {
        
        if(currentEvent!= NOTHING)
        {
          
          if(currentEvent == CLICK)
          myMenu.getCurrentMenu()->selectMenu();
          currentEvent = NOTHING;
        }
       
        rotaryState = (int)myRotor.getDirection();
        switch (rotaryState)
        {
        case 1:
            myMenu.getCurrentMenu()->navUp();
            encoderPos++;
            break;
        case -1:
            myMenu.getCurrentMenu()->navDown();
            encoderPos--;
        default:
        
            break;
        }

     xSemaphoreGive(xSemaphore4tft);

    }
    vTaskDelay(10);

  }
}

void dpTask(void * pvParameters)
{


    omegaTFT::tft =&tft;
    omegaTFT::menuSprite = &menuSprite;


    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    menuSprite.loadFont(NotoSansMonoSCB20);

    
    myMenu.activateMenu();
    
    while (1)
    {   
        
        if(xSemaphoreTake( xSemaphore4tft, ( TickType_t ) 100 ) == pdTRUE)
        {   

            myMenu.getCurrentMenu()->runMenu();
            //myMenu->getCurrentMenu()->navUp();

            //myMenu->roundGauge(mypercent, arcSprite);
            xSemaphoreGive( xSemaphore4tft );
        }
        vTaskDelay(1);
        
         
    }
      




}

void wifiTask(void * params){

  
  //wifiMQTTManager.setup();
  setup_wifi();
  vTaskDelay(200);
  client.setKeepAlive(60);
  client.setCallback(callback);
  client.setServer(mqtt_server, 1883);
  vTaskDelay(200);
  while (1)
  {
     if (!client.connected()) {
    vTaskDelay(2000);
    reconnect();
  }
  client.loop();
  vTaskDelay(1);
  // Publish sensor data every 10 seconds

  



}
}


void updateEncoder(){myRotor.tick();}

void updateSW(){
  //myMenu->getCurrentMenu()->selectMenu();
        if (digitalRead(PIN_SW)) currentEvent = CLICK;
  };

void setup(void) {
  Serial.begin(115200);
  Wire.begin();
  
 
  menuSprite.createSprite(240,240);

    
    pinMode(PIN_SW, INPUT_PULLUP); // Rotary Encoder Schalter Pin
    pinMode(PIN_DT, INPUT_PULLUP); // Rotary Encoder DT Pin
    pinMode(PIN_CLK, INPUT_PULLUP); // Rotary Encoder CLK Pin

    attachInterrupt(digitalPinToInterrupt(PIN_CLK), updateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_DT), updateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_SW), updateSW,CHANGE);

    xCalibrationMutex = xSemaphoreCreateMutex();
    xSemaphore4tft =xSemaphoreCreateBinary();


    if (xSemaphore4tft == NULL) {Serial.println("Semaphore Fail");}
    if (xCalibrationMutex == NULL) {Serial.println("CalibrationMutex Fail");}

    xSemaphoreGive(xCalibrationMutex);
    xSemaphoreGive(xSemaphore4tft);
    
   

    xTaskCreate(
        dpTask,      // Function name of the task
        "Display_Task",   // Name of the task (e.g. for debugging)
        8192,        // Stack size (bytes)
        NULL,        // Parameter to pass
        2,           // Task priority
        &dpTaskHandle         // Task handle
    );
  
    xTaskCreate(
        inputTask,      // Function name of the task
        "Input_Task",   // Name of the task (e.g. for debugging)
        2048,        // Stack size (bytes)
        NULL,        // Parameter to pass
        2,           // Task priority
        &inputTaskHandle         // Task handle
    );

      xTaskCreate(
        wifiTask,      // Function name of the task
        "Wifi_Task",   // Name of the task (e.g. for debugging)
        8192,        // Stack size (bytes)
        NULL,        // Parameter to pass
        5,           // Task priority
        &wifiTaskHandle         // Task handle
    );

    


}

void loop() {}

