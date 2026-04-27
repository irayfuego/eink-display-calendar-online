#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <time.h>
#include "display_utils.h"
#include "renderer.h"
#include "_locale.h"
#include FONT_HEADER


//MQTT configuration
#define MQTT_HOST IPAddress(192, 168, 1, 200)
#define MQTT_PORT 1883
#define mqtt_user "hass"
#define mqtt_password "mqtt"
#define mqtt_topic_publish "homeassistant/sensor/epaper_display/state"
#define mqtt_topic_subscribe "epaper_weather/data"

AsyncMqttClient mqttClient;

//JSON to store values to send over MQTT
//StaticJsonDocument<256> JSONReceived;
JsonDocument JSONReceived;

#define num_max_calendar_entries  4 //Max number of calendar entries that can be displayed
#define calendar_line_gap  70 // gap between calendar entry rows

typedef struct calendar_event
{
  String start;
  String end;
  String summary;
};

calendar_event calendar_entries [30]; // No more than 20 calendar entries are expected.


tm *timeCurrent= {};
char* MQTTpayload;

void connectToMqtt() {
  mqttClient.connect();
  Serial.println("Connecting to MQTT...");
  
}

byte   calcDayOfWeek(int d, int m, int y)                                    // 1 <= m <= 12,  y > 1752 (in the U.K.)
    {                                                                        // https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
        static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        y -= m < 3;
        return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;                   // Sun=0, Mon=1, Tue=2, Wed=3, Thu=4, Fri=5, Sat=6
    }


// Función para eliminar tildes de una cadena
String removeAccents(const String& input) {
    String output = input;
    output.replace("á", "a");
    output.replace("é", "e");
    output.replace("í", "i");
    output.replace("ó", "o");
    output.replace("ú", "u");
    output.replace("Á", "A");
    output.replace("É", "E");
    output.replace("Í", "I");
    output.replace("Ó", "O");
    output.replace("Ú", "U");
    return output;
}

void drawCalendarEntries(){


  const int xPos0 = 80; //320
  const int xPos1 = DISP_WIDTH - 30;
  const int yPos0 = 250;
  const int yPos1 = DISP_HEIGHT - 30;

  const char *CONS_WEEKDAY[7]    = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab" };

  // x axis
  //display.drawLine(xPos0, yPos1    , xPos1, yPos1    , GxEPD_BLACK);
  //display.drawLine(xPos0, yPos1 - 1, xPos1, yPos1 - 1, GxEPD_BLACK);

  display.setFont(&FONT_12pt8b);

  if (!calendar_entries[0].start.isEmpty())
  {
    for(int i = 0; i<(num_max_calendar_entries); i++){
      if (!calendar_entries[i].start.isEmpty())
      {
         
          int year,month,day,hour,minute;
          hour=0;
          minute=0;
          float second;
          sscanf(calendar_entries[i].start.c_str(), "%d-%d-%dT%d:%d:%fZ", &year, &month, &day, &hour, &minute, &second);

          
          int weekday = calcDayOfWeek(day, month, year) ;  
          String shortweekday = CONS_WEEKDAY[weekday];
          String datetodraw = (String)day + "/" + (String) month;
          String timetodraw = (String)hour + ":" + (String) minute;
          if (minute ==0){
            timetodraw = timetodraw + "0";
          }
            Serial.print("Hora extraida: ");
            Serial.println((String)hour );

          if (month == (timeCurrent->tm_mon +1) && day == (timeCurrent->tm_mday) ) {
            shortweekday = "HOY";
            display.setFont(&FONT_16pt8b);
            drawString(xPos0, yPos0 + i*calendar_line_gap, shortweekday , RIGHT);
          }
          else
          { 
            display.setFont(&FONT_16pt8b);
            drawString(xPos0, yPos0 -13 + i*calendar_line_gap, shortweekday , RIGHT);
            drawString(xPos0, yPos0 +13 + i*calendar_line_gap, datetodraw , RIGHT);
          
          }
        
            display.setFont(&FONT_22pt8b);
            drawString(xPos0 + 120, yPos0 + i*calendar_line_gap, timetodraw , RIGHT);

            // Obtener el título del evento sin tildes
            String eventTitle = removeAccents(calendar_entries[i].summary);

            display.setFont(&FONT_26pt8b);
            drawString(xPos0 + 140, yPos0 + i*calendar_line_gap, eventTitle.c_str() , LEFT);
            

      }

    }
  }
}

void callback(char* topic, char* payload, unsigned int length) {
 
   DeserializationError errorDes = deserializeJson(JSONReceived, payload, length);
    if (errorDes) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(errorDes.f_str());
        return;
    }
    else
    {     

      JsonArray array = JSONReceived["calendar.familia"]["events"].as<JsonArray>();

      int i = 0;
      for (JsonObject calendar_array : JSONReceived["calendar.familia"]["events"].as<JsonArray>())
      {
  
            calendar_entries[i].start = calendar_array["start"]       .as<const char* >();
            calendar_entries[i].end = calendar_array["end"]       .as<const char *>();
            calendar_entries[i].summary = calendar_array["summary"]       .as<const char *>();
            
            /*Serial.print("ARRAY extraido: ");
            Serial.println(calendar_entries[i].summary );
            Serial.print("i: ");
            Serial.println(i);  
            */

        if (i == array.size() - 1)
        {
          break;
        }
        ++i;
      }


    } 
}



void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  uint16_t packetIdSub = mqttClient.subscribe(mqtt_topic_subscribe, 1);
  uint16_t packetIdPubR = mqttClient.publish(mqtt_topic_publish, 1, true, MQTTpayload);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");

}


void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  
  
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  Serial.println(payload);

  callback(topic, payload, len);

}

void onMqttPublish(uint16_t packetId) {
/*
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
*/
}


void MQTTSetup(tm *timeInfo, char* payload)
{
  
  MQTTpayload = payload;

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(mqtt_user, mqtt_password);
  mqttClient.connect();


  timeCurrent = timeInfo;

  String HoraActual;
  getDateStr(HoraActual, timeCurrent);
  Serial.print("  Hora actual: ");
  Serial.println(HoraActual);

  int yearT,monthT,dayT,hourT,minuteT;
  yearT= timeCurrent->tm_year;
  monthT= timeCurrent->tm_mon;
  dayT=timeCurrent->tm_mday;
                  printf("Year:  %d\n",yearT);
                  printf("Month:  %d\n",monthT);
                  printf("Day:  %d\n",dayT);
                  //printf("Hour:  %d\n",hourT);
                  //printf("Minute:  %d\n",minuteT);
 

}

