#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>

// Funciones públicas
void performOTA(String otaUrl);
// Función principal para configurar y usar MQTT
bool MQTTSetup(const char* payload);

// Función para cargar la configuración desde Preferences
void loadConfiguration();

// Función para guardar la configuración en Preferences
void saveConfiguration();
#endif // MQTT_H