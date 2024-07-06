#ifndef MENU_H
#define MENU_H

#include <WiFi.h>
#include "esp_wifi.h"
#include <esp_now.h>

#include <vector>

#include <omegaTFT.h>
#include <omegaWireless.h>
#include <omegaNOW.h>
#include <omegaPlant.h>

#include "NotoSansMonoSCB20.h"
#include "NotoSansBold15.h"
#include "mqttManager.h"

volatile int gyroy = 0;
volatile int gyrop = 0;
volatile int gyror = 0;
uint8_t *test;
omegaWireless wirelessManager = omegaWireless("PlantPal");
EspNowManager espNOW;
PlantProfile newProfile; 

omegaPlant myPlant = omegaPlant(newProfile);

extern sensorDataPacket curData;
extern PlantSaveData curSave;
 bool espNowActive= false;

TFT_eSPI tft =TFT_eSPI();  // Create object "tft"
TFT_eSprite menuSprite(&tft);

struct Arc {
    int cx;
    int cy;
    int radius;
    int width;
    int startAngle;
    int endAngle;
    uint16_t color;
    int value;
    char unit;
    int min;
    int max;
};



std::vector<MacAddress> connectedMacAddresses;
/*########################################## Menu Functions ##################################################*/

std::vector<MacAddress> getConnectedMacAddresses() {
    std::vector<MacAddress> macAddresses;
  // Get the number of connected stations
  int numStations = WiFi.softAPgetStationNum();
  Serial.print("Number of connected devices: ");
  Serial.println(numStations);

  // Get the list of connected stations
  wifi_sta_list_t stationList;
  tcpip_adapter_sta_list_t adapterList;
 

  if (esp_wifi_ap_get_sta_list(&stationList) == ESP_OK) {
    if (tcpip_adapter_get_sta_list(&stationList, &adapterList) == ESP_OK) {
      for (int i = 0; i < adapterList.num; i++) {

        tcpip_adapter_sta_info_t station = adapterList.sta[i];
        Serial.print("Device ");
        Serial.print(i + 1);
        Serial.print(": MAC Address: ");
        Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                      station.mac[0], station.mac[1], station.mac[2],
                      station.mac[3], station.mac[4], station.mac[5]);
        MacAddress macAddress;
        memcpy(macAddress.mac, adapterList.sta[i].mac, sizeof(macAddress.mac)); // Copy MAC address
        macAddresses.push_back(macAddress); // Add MAC address to vector
      }
    }
  }
  return macAddresses;
}

//TODO: Hier Plant Objekt reinpassen um gegenstände zu platzieren
void animateAvatar(TFT_eSPI* tft,TFT_eSprite * avatarSprite, uint8_t frame)
{
  TFT_eSprite itemSprite = TFT_eSprite(tft);
  itemSprite.createSprite(50,10);
  itemSprite.pushImage(0,0,50,10,icon_sunglasses);
  if(frame>3) return;

  uint8_t avatarInt = curData.hum%3;
  avatarSprite->pushImage(0,0,120,120,plantArray[avatarInt]);
  itemSprite.pushToSprite(avatarSprite,40+frame*4,30-frame*3,0);
}


void createArc(TFT_eSprite * arcSprite,Arc arcData)
{
  arcSprite->fillScreen(0);
  arcSprite->drawSmoothArc(16,16,arcData.radius,arcData.width,arcData.startAngle,arcData.endAngle,arcData.color,TFT_TRANSPARENT,true);
  arcSprite->drawFastVLine(16,0,7,TFT_WHITE);
  arcSprite->setCursor(5,28);
  arcSprite->setTextColor(arcData.color);
  arcSprite->printf("%d",arcData.value);
  arcSprite->print(arcData.unit);

}

void calculate_arc_positions(Arc arcs[], int numArcs) {
const int displayRadius = 120;  // Half of the display's diameter (240/2)
    const int arcRadius = 16;       // Radius of the arcs
    const int centerOffset = 30;    // Distance from the edge of the display to the center of each arc
    const int arcAngle = 360;       // Angle span of each arc
    const int arcWidth = 14;        // Width of each arc
    const uint16_t arcColor = TFT_GOLD;  // Color of the arcs

    const uint8_t startAngle = 180+45;
    const uint8_t endAngle = 360-startAngle;

    float stepAngle = (endAngle - startAngle) / (numArcs - 1);  // Calculate the step angle

    for (int i = 0; i < numArcs; ++i) {
        // Calculate the angle for each arc's center point based on the start and end angles
        float angle = startAngle + i * stepAngle;
        float radians = (angle-45) * PI / 180.0;  // Convert to radians and mirror to the left side

        // Calculate the center position of the arc
        int cx = displayRadius + (displayRadius - centerOffset) * cos(radians);
        int cy = displayRadius + (displayRadius - centerOffset) * sin(radians);

     
        // Store the arc properties in the array
        arcs[i] = {cx, cy, arcRadius, arcWidth, 45, arcAngle, arcColor};
    }
}

bool drawHomeScreen(TFT_eSPI* tft,TFT_eSprite * mainSprite)
{
  static int frameCounter = 0;
  bool doAnimate = true;
  static bool loadFont = true;
  static uint8_t animationIndex=0;
  static TFT_eSprite txtSprite = TFT_eSprite(tft);//120x120
  static TFT_eSprite avatarSprite = TFT_eSprite(tft); //180x120
  static TFT_eSprite arcSprite = TFT_eSprite(tft);
  static TFT_eSprite iconSprite = TFT_eSprite(tft);

    static bool spritesInitialized = false;
    if (!spritesInitialized) {
        txtSprite.createSprite(180, 35);
        txtSprite.loadFont(NotoSansBold15);

        arcSprite.createSprite(34,44);
        arcSprite.loadFont(NotoSansBold15);

        iconSprite.createSprite(40,40);

        avatarSprite.createSprite(120, 120, 1);
        spritesInitialized = true;
    }
  

  frameCounter = frameCounter%100;

  if(frameCounter<20)animationIndex =0;
  else if(frameCounter<21)animationIndex =1;
  else if(frameCounter<23)animationIndex =2;
  else if(frameCounter<25)animationIndex =1;
  else if(frameCounter<100)animationIndex =0;
  

  uint16_t offset = 50;
  uint16_t startAngl = 45;
  uint16_t endAngl = 315;

  Arc valArcs[4];
  calculate_arc_positions(valArcs,4);  

  Serial.printf("Rangetemp: %d", curProfile.range_temp);

  uint16_t tempc_angle = map(curData.tempc,18, (curProfile.tempc+curProfile.range_temp),startAngl,endAngl);
  uint16_t hum_angle = map(curData.hum,(curProfile.hum-curProfile.range_hum),(curProfile.hum+curProfile.range_hum),startAngl,endAngl);
  uint16_t light_angle = map(curData.light,(curProfile.light-curProfile.range_light), (curProfile.light+curProfile.range_light),startAngl,endAngl);
  uint16_t soil_angle = map(curData.moist,(curProfile.soil_moisture-curProfile.range_soil_moisture), (curProfile.soil_moisture+curProfile.range_soil_moisture),startAngl,endAngl);  
  
  uint16_t soil_mood = map(curData.moist,(curProfile.soil_moisture-curProfile.range_soil_moisture), (curProfile.soil_moisture+curProfile.range_soil_moisture),startAngl,endAngl);  
  uint16_t soil_xp = map(curData.moist,(curProfile.soil_moisture-curProfile.range_soil_moisture), (curProfile.soil_moisture+curProfile.range_soil_moisture),startAngl,endAngl);  
  
    
  if(tempc_angle>endAngl) tempc_angle = endAngl;
  if(hum_angle>endAngl) hum_angle = endAngl;
  if(light_angle>endAngl) light_angle = endAngl;
  if(soil_angle>endAngl) soil_angle = endAngl;
  

  //Arcs


  valArcs[0].endAngle = light_angle;
  valArcs[1].endAngle = tempc_angle;
  valArcs[2].endAngle = hum_angle;
  valArcs[3].endAngle = soil_angle;

  valArcs[0].value = curData.light;
  valArcs[1].value = curData.tempc;
  valArcs[2].value = curData.hum;
  valArcs[3].value = curData.moist;

  //valArcs[0].unit = "C";
  //valArcs[1].unit = "%";
  //valArcs[2].unit = "%";
  //valArcs[3].value ="";
  

  valArcs[0].color = COLOR_YELLOW;  
  valArcs[1].color = COLOR_RED;
  valArcs[2].color = COLOR_PURPLE;
  valArcs[3].color = COLOR_BROWN;

  // Bottom Text
    
  mainSprite->setSwapBytes(1);
  mainSprite->fillScreen(0);

  txtSprite.fillScreen(TFT_BLACK);
  txtSprite.setCursor(0,0);
  txtSprite.setTextColor(TFT_WHITE);
  //txtSprite.printf("Age: %d Days\n",val1);  
  uint8_t level = myPlant.calculateLevel(curData.xp);
 uint8_t xp_progress = abs(curData.xp - (level-1)*2);
  
  

  txtSprite.setTextColor(0x3F29);
txtSprite.printf("Mood\n  %d%%",curData.mood);

  txtSprite.setSwapBytes(1);
  txtSprite.setTextColor(TFT_WHITE);
  txtSprite.pushToSprite(mainSprite,155,180);

  txtSprite.fillScreen(0);
  txtSprite.setTextColor(TFT_WHITE);
  txtSprite.setCursor(0,0);

  
  txtSprite.setTextColor(TFT_SILVER);
  txtSprite.printf("Level\n    %d\n",level);
  txtSprite.pushToSprite(mainSprite,180,90);

  txtSprite.fillScreen(0);
  txtSprite.setTextColor(TFT_GOLD);
  txtSprite.setCursor(0,0);
  txtSprite.printf(" %dXP",xp_progress);
  txtSprite.pushToSprite(mainSprite,180,125);


  uint16_t mood_angle = map(curData.mood,-1,101,360-offset,180+offset);  
  uint16_t xp_angle = map(xp_progress,0,level*2,360-offset,180+offset); //Real magic numbers here! 
  
  if (mood_angle > 360-offset) mood_angle = 180+offset;  
  if (xp_angle > 360-offset) xp_angle = 360-offset;
  if (xp_angle <  180+offset) xp_angle =  180+offset;

  mainSprite->drawSmoothArc(120,120,118,110,mood_angle,360-offset,0x3F29,TFT_TRANSPARENT,true);

  mainSprite->drawSmoothArc(120,120,106,102,xp_angle,360-offset,TFT_GOLD,TFT_TRANSPARENT,true);
  mainSprite->drawFastHLine(190,120,40,TFT_WHITE);
  int iPlant =(frameCounter/(100/plantArrLen))%plantArrLen;
  //mainSprite->drawFastHLine(0,120,35,TFT_WHITE);
  mainSprite->setCursor(80,10);
  
    char msb = (char)((curData.id >> 8) & 0xFF); // Most significant byte (MSB)
    char lsb = (char)(curData.id & 0xFF);        // Least significant byte (LSB)
  mainSprite->printf("Plant %c%d",msb,lsb);

  animateAvatar(tft,&avatarSprite,animationIndex);


  avatarSprite.pushToSprite(mainSprite,60,60,TFT_BLACK);



  

  for (int i = 0; i < 4;i++) {
      createArc(&arcSprite,valArcs[i]);
        arcSprite.pushToSprite(mainSprite,valArcs[i].cx-16, valArcs[i].cy-16);
    }

  if (frameCounter <50 || frameCounter >75){
  

  mainSprite->fillCircle(120,160,5,TFT_SILVER);
  mainSprite->fillCircle(120,160,4,COLOR_BROWN);

  mainSprite->fillCircle(80,145,5,TFT_SILVER);
  mainSprite->fillCircle(80,145,4,COLOR_PURPLE);

  mainSprite->fillCircle(65,105,5,TFT_SILVER);
  mainSprite->fillCircle(65,105,4,COLOR_RED);

  mainSprite->fillCircle(85,65,5,TFT_SILVER);
  mainSprite->fillCircle(85,65,4,COLOR_YELLOW);

}else{
  

if(curData.light<20){
  mainSprite->fillCircle(80,86,2,TFT_WHITE);

  if (frameCounter>55) mainSprite->fillCircle(70,80,5,TFT_WHITE);
  //if (frameCounter>58)mainSprite->drawCircle(40,70,22, TFT_WHITE);
  if (frameCounter>58) mainSprite->fillCircle(40,70,22,TFT_WHITE);

  if (frameCounter>62) iconSprite.pushImage(0,0,40,40,icon_light);
  if (frameCounter>62) iconSprite.pushToSprite(mainSprite,20,50,TFT_BLACK);
} 
}
  

  //mainSprite->fillCircle()
  mainSprite->pushSprite(0,0);
  //mainSprite->pushSprite(0,0);

  frameCounter++;
  

  return false; //bleibe im Menu



}

bool myFunc(TFT_eSPI* tft,TFT_eSprite * mainSprite)
{
   
  uint16_t angly = map(gyroy,-18000, 18000,41,319 );
  uint16_t anglp = map(gyrop,-18000, 18000,41,319 );
  uint16_t anglr = map(gyror,-18000, 18000,41,319 );
  
  Serial.print(angly/100);

  mainSprite->fillScreen(TFT_BLACK);
  mainSprite->loadFont(NotoSansMonoSCB20);

  mainSprite->setCursor(80,100);
  mainSprite->setTextColor(TFT_GOLD);
  mainSprite->print("Yaw: ");
  mainSprite->print(gyroy/100);

  mainSprite->setCursor(80,120);
  mainSprite->setTextColor(TFT_SILVER);
  mainSprite->print("Pitch: ");
  mainSprite->print(gyrop/100);
  
  mainSprite->setCursor(80,140);
  mainSprite->setTextColor(TFT_VIOLET);
  mainSprite->print("Roll: ");
  mainSprite->print(gyror/100);
  

  mainSprite->drawSmoothArc(120,120,118,114,40,angly,TFT_GOLD,TFT_TRANSPARENT,true);
  mainSprite->drawSmoothArc(120,120,110,106,40,anglp,TFT_SILVER,TFT_TRANSPARENT,true);
  mainSprite->drawSmoothArc(120,120,102,98,40,anglr,TFT_VIOLET,TFT_TRANSPARENT,true);
  
  mainSprite->pushSprite(0,0);
  return 0; // Bleibe im Menu
}

bool startAP(TFT_eSPI* tft,TFT_eSprite * mainSprite)
{
 wirelessManager.startAP();
  return true;
}

bool getMAC(TFT_eSPI* tft,TFT_eSprite * mainSprite)
{
  wirelessManager.getConnectedMacAddresses();
  return true;
}

bool stopAP(TFT_eSPI* tft,TFT_eSprite * mainSprite)
{
  wirelessManager.stopAP();
  return true;
}

void sendData()
{
//wirelessManager.sendESPNOW();


}

bool restartESP(TFT_eSPI* tft,TFT_eSprite * mainSprite)
{
  TFT_eSprite txtSprite = TFT_eSprite(tft);//120x120
  
  txtSprite.createSprite(180,120);
  mainSprite->fillScreen(TFT_BLACK);
  

  txtSprite.fillScreen(TFT_BLACK);
  txtSprite.setTextColor(TFT_RED);
  txtSprite.setCursor(0,0);
  txtSprite.print("Restarting System...");
  txtSprite.pushToSprite(mainSprite,60,100);
  mainSprite->pushSprite(0,0);



  ESP.restart();
  return 0;

}



/*########################################## Menu Structure ##################################################*/
char* macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return macStr;
}


std::vector<omegaTFT> macMenu(void)
{
  Serial.println("MACMENU");

  std::vector<omegaTFT> items;
  items.push_back(omegaTFT(EXIT, "Back",icon_cross));

  std::vector<MacAddress> macList = wirelessManager.getConnectedMacAddresses();
  uint8_t i = 0; 
      for (const MacAddress&mac : macList) {
        items.push_back(omegaTFT(VALUE,"Device:",icon_numeric,macList[i].mac[5]));
        i++;

    }
    
 return items;

}




omegaTFT plantParams[]{
omegaTFT(VALUE,"Temperature", curProfile.tempc),
omegaTFT(VALUE,"Humidity", curProfile.hum),
omegaTFT(VALUE,"Soil",curProfile.soil_moisture),
omegaTFT(VALUE,"Light", curProfile.light),

omegaTFT(VALUE,"Temp Range", curProfile.range_temp),
omegaTFT(VALUE,"Hum Range", curProfile.range_hum),
omegaTFT(VALUE,"Soil Range",curProfile.range_soil_moisture),
omegaTFT(VALUE,"Light Range", curProfile.range_light),

omegaTFT(EXIT,"Back")

};

omegaTFT plantMenu[]{
omegaTFT(SUBMENU,"Plant 1", icon_potted_plant,plantParams,sizeof(plantParams)/sizeof(omegaTFT)),
omegaTFT(SUBMENU,"Plant 2", icon_potted_plant,plantParams,sizeof(plantParams)/sizeof(omegaTFT)),
omegaTFT(EXIT,"Back")

};


omegaTFT wifiMenu[]{
omegaTFT(FUNCTION,"Start AP", icon_satellite_antenna,startAP),
omegaTFT(FUNCTION,"Stop AP", icon_satellite_antenna,stopAP),
omegaTFT(MENU_FUNCTION,"Get MAC",icon_satellite_antenna,macMenu),
omegaTFT(EXIT,"Back")


};

omegaTFT settingsMenu [] = {
  omegaTFT(SUBMENU,"Plant Profiles", icon_potted_plant,plantMenu,sizeof(plantMenu)/sizeof(omegaTFT)),
  omegaTFT(SUBMENU,"WiFi", icon_wifi,wifiMenu,sizeof(wifiMenu)/sizeof(omegaTFT)),
  omegaTFT(EMPTY,"BLE", icon_bluetooth),
  omegaTFT(EXIT,"Back")
};

omegaTFT submenus [] = {
  omegaTFT(FUNCTION, "Home Screen",icon_potted_plant,drawHomeScreen),
  omegaTFT(SUBMENU, "Settings",icon_gear,settingsMenu, sizeof(settingsMenu)/sizeof(omegaTFT)),
  omegaTFT(FUNCTION,"Restart Device",icon_cross,restartESP)
};

omegaTFT myMenu = omegaTFT(SUBMENU,"Main Menu",nullptr,submenus,sizeof(submenus)/sizeof(omegaTFT));

#endif