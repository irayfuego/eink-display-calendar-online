#ifndef __CALENDAR_H__
#define __CALENDAR_H__

#include <Arduino.h>
#include <time.h>
#include "config.h"
#ifdef USE_HTTP
  #include <WiFiClient.h>
#else
  #include <WiFiClientSecure.h>
#endif

#ifdef USE_HTTP
  int getGoogleCalendar(WiFiClient &client, tm *timeInfo);
#else
  int getGoogleCalendar(WiFiClientSecure &client, tm *timeInfo);
#endif

void drawCalendarEntries();

#endif // __CALENDAR_H__
