#ifndef OMEGATFT_H
#define OMEGATFT_H

#include <TFT_eSPI.h>
#include "omegaIcons.h"
#include <vector>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

#define ICON_SIZE 40
#define NUM_ICONS icon_allArray_LEN

// Definiere den Radius, in dem die Icons angezeigt werden sollen
#define ICON_RADIUS (SCREEN_WIDTH / 2 - 30) // Abstand vom Rand



enum menuType
{
    EMPTY,
    SUBMENU,
    EXIT,
    VALUE,
    FUNCTION,
    MENU_FUNCTION
};// 'charge_battery', 48x48px


class omegaTFT
{
public:    
    static TFT_eSPI *tft;
    static TFT_eSprite * menuSprite;
    static TFT_eSprite * valueSprite ;
    static TFT_eSprite * accsrySprite ;
    bool forceDraw = true;

    typedef bool (*externalFunction)(TFT_eSPI *,TFT_eSprite *);
    typedef std::vector<omegaTFT>(*extMenu)(void);
private:
    

    menuType type;
    const char * name;
    omegaTFT * subMenus;
    std::vector<omegaTFT> extSubMenus;

    omegaTFT * lastMenu = nullptr;
    omegaTFT * currentMenu;
    uint8_t subMenuCount;
    const  uint16_t * icon;
    uint8_t selectedItem = 0;
    uint8_t dataValue = 0;
    uint8_t maxMenuItems =5;
    bool exit =false;

    externalFunction function;
    extMenu extMenuFunction;

public:

    omegaTFT(menuType myType, const char * myName,externalFunction myFunction = nullptr) : 
    type(myType),
    name(myName),
    function(myFunction)
    {setDefaultIcons();}
    
    omegaTFT(menuType myType, const char * myName, uint8_t & myData) : 
    type(myType),
    name(myName),
    dataValue(myData)
    {setDefaultIcons();}

    omegaTFT(menuType myType, const char * myName, const  uint16_t* myIcon, externalFunction myFunction = nullptr) : 
    type(myType),
    name(myName), 
    icon(myIcon),
    function(myFunction){}

    omegaTFT(menuType myType, const char * myName, const  uint16_t* myIcon,    extMenu funcSubMenu ) : 
    type(myType),
    name(myName), 
    icon(myIcon),
    extMenuFunction(funcSubMenu){}

 

    omegaTFT(menuType myType, const char * myName, const  uint16_t* myIcon, uint8_t & myData) :
    type(myType),
    name(myName), 
    icon(myIcon),
    dataValue(myData){}

    omegaTFT(menuType myType, char * myName,const  uint16_t* myIcon, omegaTFT* myMenus, uint8_t myCount) : 
    type(myType),
    name(myName),
    icon(myIcon), 
    subMenus(myMenus), 
    subMenuCount(myCount){}

    void navUp()
    {
        if(type == VALUE)
            dataValue++;
        if(type==SUBMENU||type==MENU_FUNCTION)
        {   
            selectedItem++;
            selectedItem = selectedItem%subMenuCount;
        }
            
    }

    void navDown()
    {
        if(type == VALUE)
            dataValue--;
        if(type==SUBMENU||type==MENU_FUNCTION)
        {
            if (selectedItem-1<0) selectedItem=subMenuCount-1;
            else selectedItem--;
            
        }
    }

    void setDefaultIcons()
    {
        if(!icon){
            if(type == EXIT) icon = icon_cross;    
            if(type == EMPTY) icon = icon_question;
            if(type == VALUE) icon = icon_numeric;
            if(type == FUNCTION) icon = icon_function;
        }
    }
    
    const uint16_t *getIcon(){return icon;}

    void activateMenu()
    {
        currentMenu = this;
    }
    
    void setCurrentMenu(omegaTFT* mycurrentMenu)
    {
        if(lastMenu!=nullptr)lastMenu->setCurrentMenu(mycurrentMenu);
        currentMenu = mycurrentMenu;
        currentMenu->forceDraw=true;
        
    }
    
    void setLastMenu(omegaTFT* mylastMenu){lastMenu =mylastMenu;}
    

    void writeExtMenu()
    {
        extSubMenus = extMenuFunction();
        subMenuCount =extSubMenus.size();
        subMenus = new omegaTFT[extSubMenus.size()];
        
        for (size_t i = 0; i < extSubMenus.size(); i++)
        {
           subMenus[i] = extSubMenus[i];
           Serial.println(subMenus[i].getName());
        }
        

    }
    void selectMenu()
    {  

            if (this->type == SUBMENU||this->type == MENU_FUNCTION)
            {
                if(subMenus[selectedItem].type == EXIT )
                {   
                    exitMenu();
                    return;
                }

                if(subMenus[selectedItem].type == MENU_FUNCTION)
                {
            
                    Serial.println("MENU_FUNCTION SELECTED");
                    subMenus[selectedItem].writeExtMenu();

                    setCurrentMenu(&subMenus[selectedItem]);
                    subMenus[selectedItem].setLastMenu(this);
                        
                 
                    
       
                   return;
                }

                if(subMenus[selectedItem].type !=0 )
                {
                    setCurrentMenu(&subMenus[selectedItem]);
                    subMenus[selectedItem].setLastMenu(this);
                    return;
                }
            }

               if(this->type == VALUE || this->type == FUNCTION) // Value wurde gesetzt
            {
                
                exitMenu();
                return;
            }
        



    }
    
    omegaTFT * getCurrentMenu(){return currentMenu;}
    omegaTFT * getSubMenus(){return subMenus;}
    
    uint8_t getValue(){return dataValue;}


    void exitMenu()
    {

        if(lastMenu!= nullptr){
            
            lastMenu->setCurrentMenu(lastMenu);
            selectedItem =0;
            lastMenu->forceDraw= true;
            lastMenu=nullptr;
            return;
        }
    }
    
    void drawSubMenu()
    {
        if(!subMenus) {
            exitMenu();
            return;}

        int itemAngle = 40;
        int selectedIndent = 0;
        int maxItems =5;

        static int oldIndex=0;
        if(oldIndex==selectedItem && !forceDraw)return;
        forceDraw = false;
        //
        int currentX[subMenuCount];
        int currentY[subMenuCount];
        float currentAngles[subMenuCount];

        // Calculate Fixed positions
               //0
             //1 
            //2 CurrentItem
             //3
               //4
        for (int i = 0; i < subMenuCount; i++) {
            //Wenn Erstes oder Zweites element der Liste ausgewählt überschreibe den letzten und vorletzten winkel 
            
            if(subMenuCount>3){
                if(oldIndex==0 &&  ((i==subMenuCount-1 && subMenuCount>3)|| (i==subMenuCount-2&&  subMenuCount>4)))
                    currentAngles[i]= radians(180) - radians((i - subMenuCount) * itemAngle);
                
                else if(oldIndex==1 && subMenuCount>4 && i==subMenuCount-1)
                    currentAngles[i]= radians(180) - radians(-2 * itemAngle);

                else if(oldIndex==subMenuCount-1&&(i<2)) 
                    currentAngles[i] = radians(180) - radians((i+1) * itemAngle);
                
                else if(oldIndex==subMenuCount-2 && i==0)
                    currentAngles[i] = radians(180) - radians(2 * itemAngle);
                     
                else currentAngles[i] = radians(180) - radians((i - oldIndex) * itemAngle);
            }

            else currentAngles[i] = radians(180) - radians((i - oldIndex) * itemAngle);

            currentX[i] = SCREEN_WIDTH / 2 + (cos(currentAngles[i]) * ICON_RADIUS - (ICON_SIZE/2));
            currentY[i] = SCREEN_HEIGHT / 2 + (sin(currentAngles[i]) * ICON_RADIUS - (ICON_SIZE/2));
            if (i==oldIndex)currentX[i]+=selectedIndent;
        }
        


        // Bewege die Icons schrittweise zur neuen Position
        int steps = 3; // Anzahl der Schritte für die Animation
        for (int step = 1; step <= steps; step++) {
            // Lösche den Bildschirm

            menuSprite->fillScreen(TFT_BLACK);

            //menuSprite->pushImage(30-15*(step/steps),105,32,32,icon_diamond_with_a_dot_1f4a0, TFT_BLACK);
            
            // Zeichne die Icons an ihren aktuellen Positionen mit einer Interpolation zur neuen Position
            for (int i = 0; i < subMenuCount; i++) {
            
            
            float newAngle;
            if(subMenuCount>3){
                
            if(selectedItem==0 && ((i==subMenuCount-1 && subMenuCount>3)|| (i==subMenuCount-2&& subMenuCount>4)))
                newAngle= radians(180) - radians((i - subMenuCount) * itemAngle);
            else if(selectedItem==1 && subMenuCount>4 && i==subMenuCount-1)
                newAngle= radians(180) - radians(-2 * itemAngle);

            else if(selectedItem==subMenuCount-1&&(i<2)) 
                newAngle = radians(180) - radians((i+1) * itemAngle);
            
            else if(selectedItem==subMenuCount-2 && i==0)
                newAngle = radians(180) - radians(2 * itemAngle);

            else newAngle = radians(180) - radians((i - selectedItem) * itemAngle);
            }

            else newAngle = radians(180) - radians((i - selectedItem) * itemAngle);

             
       
            // Interpoliere die Position zwischen alt und neu

            
            
            int x = currentX[i] + (SCREEN_WIDTH / 2 + (cos(newAngle) * ICON_RADIUS - (ICON_SIZE/2)) - currentX[i]) * step / steps;
            int y = currentY[i] + (SCREEN_HEIGHT / 2 + (sin(newAngle) * ICON_RADIUS - (ICON_SIZE/2)) - currentY[i]) * step / steps;
            
            
            
            // Zeichne das Icon an seiner interpolierten Position
            menuSprite->setSwapBytes(1);
            

            if(newAngle<radians(180)+radians(90) && newAngle>radians(180)-radians(90) && x<SCREEN_WIDTH/2)
                menuSprite->pushImage(x, y, ICON_SIZE, ICON_SIZE, subMenus[i].getIcon());
            }
            
            menuSprite->setSwapBytes(1);
            //menuSprite->pushImage(30-10*(step/steps),105,32,32,icon_diamond);
        //Serial.println("Before Push!");
        /*
            //tft->setSwapBytes(1);
                menuSprite->setCursor(110,113);
                menuSprite->print("Item: ");
                menuSprite->print(selectedItem);
                
                //menuSprite->drawSmoothRoundRect(40,90,10,8,220,60,TFT_WHITE);
            */
            menuSprite->setCursor(selectedIndent+ICON_SIZE+16,110);
            menuSprite->printf(subMenus[selectedItem].getName());

            if(subMenus[selectedItem].type==VALUE)
            {
                
                menuSprite->setCursor(200,110);
                menuSprite->print(subMenus[selectedItem].getValue());
                
            }
           
            //enuSprite->drawFastHLine(55,132,30-random(15),TFT_WHITE);
            //menuSprite->drawSmoothArc(SCREEN_WIDTH/2,SCREEN_HEIGHT/2,120,116,20,160,TFT_DARKCYAN,TFT_TRANSPARENT, true);
            //menuSprite->drawFastHLine(0,120,240,TFT_WHITE);
            //menuSprite->drawFastVLine(120,0,240,TFT_WHITE);
            menuSprite->pushSprite(0,0);

            }
            
oldIndex = selectedItem;
}
    
    void roundGauge(uint8_t percent,TFT_eSprite *arcSprite)
{
    static uint8_t percentage;
    percentage =percent;
    static bool down;
    uint16_t fg_color = TFT_BLUE;     // Farbe des Bogens
    uint16_t bg_color = TFT_BLACK;    // Hintergrundfarbe für das Glätten (Anti-Aliasing)

    uint16_t x = tft->width() / 2;     // X-Position des Mittelpunkts des Bogens
    uint16_t y = tft->height() / 2;    // Y-Position des Mittelpunkts des Bogens

    uint8_t radius = 120;             // Radius des Bogens (Durchmesser 240 Pixel)
    uint8_t thickness = 10;           // Dicke des Bogens

    uint8_t inner_radius = radius - thickness; // Berechnung des inneren Radius (kann 0 für einen vollständigen Kreis sein)

    uint16_t start_angle = 20;          // Startwinkel des Bogens (0 Grad)
    uint16_t end_angle = map(percentage, -1, 100, start_angle, 360-start_angle); // Endwinkel des Bogens (von 0 bis 100 Prozent)

    bool arc_end = true;               // True = runde Enden, False = eckige Enden

    // Erstelle ein Sprite-Objekt
    arcSprite->createSprite(tft->width(), tft->height());

    // Lösche das Sprite und den Bildschirm nur, wenn sich der Prozentsatz ändert


    // Zeichne den Bogen auf das Sprite
    arcSprite->drawSmoothArc(x, y, radius, inner_radius, start_angle, end_angle, colorwheel(percentage), bg_color, arc_end);

    // Zeichne das Sprite auf den Bildschirm


    // Schreibe die aktuelle Prozentszahl in die Mitte des Kreises
    arcSprite->setTextSize(2);
    arcSprite->setTextColor(TFT_WHITE);
    arcSprite->setCursor(x - 12, y - 10); // Zentriere den Text
    arcSprite->print(percentage);
    //arcSprite.pushRotated(end_angle,0);
    arcSprite->pushSprite(0, 0);
    // Lösche das Sprite
    arcSprite->deleteSprite();


}

    void runMenu()
    {
        if(!tft || !menuSprite) return;

        switch (type)
        {
        case SUBMENU:
            drawSubMenu();
            break;      

            case MENU_FUNCTION:
            drawSubMenu();
            break;

        case VALUE:
            if (&this->dataValue != nullptr)
            {       
                valueSprite->fillScreen(TFT_BLACK);     
                valueSprite->setCursor(0,valueSprite->height()/2 -10);        
                valueSprite->println(dataValue);
                valueSprite->pushSprite(220,SCREEN_HEIGHT *.25);
            }
            break;
        case FUNCTION:
            runFunction();
            //exitMenu();
        default:
            break;
        }

    }

    void runFunction()
    {
        if(function)
        {
            if(function(tft,menuSprite))exitMenu();
        }
        
    }
    const char* getName(){return name;}
    omegaTFT(){}
    ~omegaTFT(){
        if(this->type ==MENU_FUNCTION)
        delete subMenus;
        }

    uint16_t colorwheel(uint8_t WheelPos) 
    {
    uint8_t r, g, b;

    if(WheelPos < 85) {
        r = 255 - WheelPos * 3;
        g = 0;
        b = WheelPos * 3;
    } else if(WheelPos < 170) {
        WheelPos -= 85;
        r = 0;
        g = WheelPos * 3;
        b = 255 - WheelPos * 3;
    } else {
        WheelPos -= 170;
        r = WheelPos * 3;
        g = 255 - WheelPos * 3;
        b = 0;
    }

    // Kombinieren der RGB-Werte zu einem 16-Bit Integer-Wert
    uint16_t rgb_int = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return rgb_int;
    }
};


TFT_eSPI * omegaTFT::tft = nullptr;
TFT_eSprite * omegaTFT::menuSprite =nullptr;
TFT_eSprite * omegaTFT::valueSprite =nullptr;
TFT_eSprite * omegaTFT::accsrySprite =nullptr;

#endif