const char SSID[] = "<WIFI name>";
const char WIFI_PWD[] = "<WIFI pwd>";
const char HOSTNAME[] = "buttplug";

const unsigned int HTTP_REST_PORT = 80;

unsigned int PUMP_PIN = 5;
unsigned int VALVE_PIN = 0;
unsigned int LED_PIN = 13;

unsigned int samples = 10;
unsigned int targetDelta = 20;
unsigned int denialDelta = 10;
unsigned int minTarget = 0;
unsigned int maxTarget = 1000;
