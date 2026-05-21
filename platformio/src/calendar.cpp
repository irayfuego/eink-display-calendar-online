#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>
#include "calendar.h"
#include "config.h"
#include "display_utils.h"
#include "renderer.h"
#include "_locale.h"
#include FONT_HEADER
#ifndef USE_HTTP
  #include <WiFiClientSecure.h>
#endif

#define NUM_MAX_CALENDAR_ENTRIES 4
#define CALENDAR_LINE_GAP        70
#define GCAL_TIMEOUT_MS          8000

typedef struct {
  String start;
  String end;
  String summary;
} calendar_event_t;

static calendar_event_t calendar_entries[NUM_MAX_CALENDAR_ENTRIES];
static tm *timeCurrent;

static byte calcDayOfWeek(int d, int m, int y)
{
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7; // Sun=0, Mon=1, ..., Sat=6
}

static String removeAccents(const String& input)
{
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

void drawCalendarEntries()
{
  const int xPos0 = 80;
  const int yPos0 = 250;
  const char *CONS_WEEKDAY[7] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

  display.setFont(&FONT_12pt8b);

  if (calendar_entries[0].start.isEmpty()) return;

  for (int i = 0; i < NUM_MAX_CALENDAR_ENTRIES; i++) {
    if (calendar_entries[i].start.isEmpty()) break;

    int year, month, day, hour, minute;
    float second;
    hour = 0;
    minute = 0;
    sscanf(calendar_entries[i].start.c_str(), "%d-%d-%dT%d:%d:%fZ",
           &year, &month, &day, &hour, &minute, &second);

    int weekday = calcDayOfWeek(day, month, year);
    String shortweekday = CONS_WEEKDAY[weekday];
    String datetodraw = (String)day + "/" + (String)month;
    String timetodraw = (String)hour + ":" + (minute < 10 ? "0" : "") + (String)minute;

    Serial.println("Hora extraida: " + (String)hour);

    if (month == (timeCurrent->tm_mon + 1) && day == timeCurrent->tm_mday) {
      shortweekday = "HOY";
      display.setFont(&FONT_16pt8b);
      drawString(xPos0, yPos0 + i * CALENDAR_LINE_GAP, shortweekday, RIGHT);
    } else {
      display.setFont(&FONT_16pt8b);
      drawString(xPos0, yPos0 - 13 + i * CALENDAR_LINE_GAP, shortweekday, RIGHT);
      drawString(xPos0, yPos0 + 13 + i * CALENDAR_LINE_GAP, datetodraw, RIGHT);
    }

    display.setFont(&FONT_22pt8b);
    drawString(xPos0 + 120, yPos0 + i * CALENDAR_LINE_GAP, timetodraw, RIGHT);

    String eventTitle = removeAccents(calendar_entries[i].summary);
    display.setFont(&FONT_26pt8b);
    drawString(xPos0 + 140, yPos0 + i * CALENDAR_LINE_GAP, eventTitle.c_str(), LEFT);
  }
}

#ifdef USE_HTTP
int getGoogleCalendar(WiFiClient &client, tm *timeInfo)
#else
int getGoogleCalendar(WiFiClientSecure &client, tm *timeInfo)
#endif
{
  timeCurrent = timeInfo;

  HTTPClient http;
  http.setTimeout(GCAL_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Accept-Encoding", "identity");
  http.begin(client, GCAL_SCRIPT_URL);

  int httpResponse = http.GET();
  Serial.println("Google Calendar HTTP: " + String(httpResponse));

  if (httpResponse == HTTP_CODE_OK) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    if (err) {
      Serial.println("Calendar JSON error: " + String(err.f_str()));
    } else {
      int i = 0;
      for (JsonObject ev : doc["events"].as<JsonArray>()) {
        if (i >= NUM_MAX_CALENDAR_ENTRIES) break;
        calendar_entries[i].start   = ev["start"]   | "";
        calendar_entries[i].end     = ev["end"]     | "";
        calendar_entries[i].summary = ev["summary"] | "";
        i++;
      }
    }
  }

  http.end();
  return httpResponse;
}
