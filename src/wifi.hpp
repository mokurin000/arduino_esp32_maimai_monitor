#include <WiFi.h>
#include <WiFiClientSecure.h>

#define WIFI_DISCONNECTED (WiFi.status() != WL_CONNECTED)

void spawn_wifi_task();

void connect_wifi();
