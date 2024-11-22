#include "Arduino.h"
#include "TFT_eSPI.h" /* Please use the TFT library provided in the library. */
#include "img_logo.h"
#include "pin_config.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <string.h>
#include <stdio.h>




// Define your custom service and characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-abcd-abcd-abcd-abcd1234abcd"

// Create a BLE server
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

String currentValue = "Hello BLE";
int counter = 0;
int last_cut = 0;
String heap[5];



/* The product now has two screens, and the initialization code needs a small change in the new version. The LCD_MODULE_CMD_1 is used to define the
 * switch macro. */
#define LCD_MODULE_CMD_1

TFT_eSPI tft = TFT_eSPI();
#define WAIT 1000
unsigned long targetTime = 0; // Used for testing draw times

#if defined(LCD_MODULE_CMD_1)
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
};
#endif
// Custom BLECharacteristicCallback to handle read and write events
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    Serial.println("Characteristic Read!");
    Serial.print("Current value: ");
    Serial.println(currentValue);
  }

  void onWrite(BLECharacteristic *pCharacteristic) override {
    Serial.println("Characteristic Written!");
    // Get the written value
    String writtenValue = pCharacteristic->getValue().c_str();
    Serial.print("Written value: ");
    Serial.println(writtenValue);
    // First we test them with a background colour set

  String to_print = writtenValue;
   if(to_print.length() < currentValue.length()){
    last_cut = 0;
    counter = 0;
    tft.fillScreen(TFT_BLACK);
  }
  currentValue = writtenValue;
  Serial.println(to_print);
  Serial.println(to_print[last_cut]);
  

   if((to_print.length() - last_cut) / 20 > 0){
      int index = last_cut + 20;



        while(!isspace(to_print[index])){
          index --;
        }
        index ++;

      if(counter < 4){
        counter ++;
      }
      // use a queue for this to make it faster
      else{
        for (int i = 0; i < 4; i++) {
          heap[i] = heap[i+1];
       }
       tft.fillScreen(TFT_BLACK);
       for (int i=0; i < 4; i++){
      tft.drawString(heap[i], 20, i * 33, 2);
    }   
      }
      last_cut = index;
   }
   heap[counter] = (to_print.substring(last_cut, to_print.length()));
   tft.drawString(heap[counter], 20, counter * 33, 2);

  



  }
};

// Custom BLEServerCallbacks to handle connection and disconnection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("Client connected!");
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("Client disconnected!");
  }
};


void setup(){
    Serial.begin(9600);


    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    Serial.println("Hello T-Display-S3");

    tft.begin();
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);

#if defined(LCD_MODULE_CMD_1)
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7f); j++) {
            tft.writedata(lcd_st7789v[i].data[j]);
        }

        if (lcd_st7789v[i].len & 0x80) {
            delay(120);
        }
    }
#endif

 
   tft.setRotation(3);
    tft.setSwapBytes(true);
   /* tft.pushImage(0, 0, 320, 170, (uint16_t *)img_logo);
    delay(2000);
  */

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    ledcSetup(0, 2000, 8);
    ledcAttachPin(PIN_LCD_BL, 0);
    ledcWrite(0, 255);
#else
    ledcAttach(PIN_LCD_BL, 200, 8);
    ledcWrite(PIN_LCD_BL, 255);
#endif

  Serial.println("Serial initialized");
  // Initialize BLE
  BLEDevice::init("ESP32 BLE Server");

  // Create the BLE server
  pServer = BLEDevice::createServer();
  
  // Create the service and characteristic using your custom UUIDs
  pService = pServer->createService(SERVICE_UUID);
  pServer->setCallbacks(new MyServerCallbacks());  // Set the callback for connection events

  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  Serial.println(SERVICE_UUID);
  // Set the initial value of the characteristic
  pCharacteristic->setValue(currentValue.c_str());

  // Set the characteristic callbacks to handle read/write events
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Set min advertising interval to 100ms
  pAdvertising->setMaxPreferred(0x12);  // Set max advertising interval to 200ms
  pAdvertising->start();
    if (!pServer) {
  Serial.println("Failed to create BLE server");
}
if (!pService) {
  Serial.println("Failed to create service");
}
if (!pCharacteristic) {
  Serial.println("Failed to create characteristic");
}
    tft.fillScreen(TFT_BLACK);

}

void loop()
{




}


// TFT Pin check
#if PIN_LCD_WR  != TFT_WR || \
    PIN_LCD_RD  != TFT_RD || \
    PIN_LCD_CS    != TFT_CS   || \
    PIN_LCD_DC    != TFT_DC   || \
    PIN_LCD_RES   != TFT_RST  || \
    PIN_LCD_D0   != TFT_D0  || \
    PIN_LCD_D1   != TFT_D1  || \
    PIN_LCD_D2   != TFT_D2  || \
    PIN_LCD_D3   != TFT_D3  || \
    PIN_LCD_D4   != TFT_D4  || \
    PIN_LCD_D5   != TFT_D5  || \
    PIN_LCD_D6   != TFT_D6  || \
    PIN_LCD_D7   != TFT_D7  || \
    PIN_LCD_BL   != TFT_BL  || \
    TFT_BACKLIGHT_ON   != HIGH  || \
    170   != TFT_WIDTH  || \
    320   != TFT_HEIGHT
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif