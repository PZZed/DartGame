#include <WiFi.h>
#include <WebSocketsServer.h>
#include "esp_timer.h"

const char* WIFI_SSID = "****";
const char* WIFI_PASS = "****";

WebSocketsServer webSocket = WebSocketsServer(81); // Port WebSocket

// COLS → sorties
const byte COLS[7] = {13, 12, 14, 27, 26, 25, 33};

// ROWSs → entrées INPUT_PULLUP
const byte ROWS[10] = {15, 2, 4, 16, 17, 5, 18, 19, 21, 3};

const byte COL_COUNT = 7;
const byte ROW_COUNT = 10;



// MATRIX : COLS 0->6, ROWSs 0->9
const char* MATRIX[COL_COUNT][ROW_COUNT] = {
  // C0
  {"T9","T12", "T5","T20","T10","T6","T13","T4","T18","T1"},
  // C1
  {"D9","D12","D5","D20","D10","D6","D13","D4","D18","D1"},
  // C2
  {"9","12","5","20","10","6","13","4","18","1"},
  // C3
  {"D25","25","","","","","","","",""},
  // C4
  {"14","11","8","16","15","2","17","3","19","7"},
  // C5
  {"D14","D11","D8","D16","D15","D2","D17","D3","D19","D7"},
  // C6
  {"T14","T11","T8","T16","T15","T2","T17","T3","T19","T7"}
};




// Anti-rebond
const uint32_t ROW_ON_TIME_US = 40;
const uint32_t DEBOUNCE_MS   = 200;


volatile bool hitDetected = false;
volatile uint32_t lastHitTime = 0;

uint8_t currentRow = 0;


void IRAM_ATTR onColumnInterrupt() {
  uint32_t now = esp_timer_get_time();
  if (now - lastHitTime < DEBOUNCE_MS) return;

  lastHitTime = now;
  hitDetected = true;
}

void initRows() {
  for (uint8_t r = 0; r < ROW_COUNT; r++) {
    pinMode(ROWS[r], OUTPUT);
    digitalWrite(ROWS[r], LOW);
  }
}

void initColumns() {
  for (uint8_t c = 0; c < COL_COUNT; c++) {
    pinMode(COLS[c], INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(COLS[c]),
                    onColumnInterrupt,
                    RISING);
  }
}

void scanNextRow() {
  digitalWrite(ROWS[currentRow], HIGH);
  delayMicroseconds(ROW_ON_TIME_US);
}

void releaseCurrentRow() {
  digitalWrite(ROWS[currentRow], LOW);
  currentRow = (currentRow + 1) % ROW_COUNT;
}

int readActiveColumn() {
  for (uint8_t c = 0; c < COL_COUNT; c++) {
    if (digitalRead(COLS[c]) == HIGH) {
      return c;
    }
  }
  return -1;
}

void handleHit() {
  noInterrupts();

  int row = currentRow;
  int col = readActiveColumn();
  hitDetected = false;

  interrupts();

  if (col == -1) return;
  Serial.println("Hit detected");
  Serial.println(String(row)+ " "+ String(col));
  String score = MATRIX[col][row];
  Serial.println(score);
  sendHit(row, col, score);

  delay(DEBOUNCE_MS);
}

// WIFI 

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connexion WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connecté");
  Serial.println(WiFi.localIP());
}

// Websockets
void initWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t*, size_t) {
  if (type == WStype_CONNECTED)
    Serial.printf("Client WS #%d connecté\n", num);

  if (type == WStype_DISCONNECTED)
    Serial.printf("Client WS #%d déconnecté\n", num);
}


void sendHit(int row, int col, String score) {
  String json = "{";
  json += "\"row\":" + String(row) + ",";
  json += "\"col\":" + String(col) + ",";
  json += "\"score\": \"" + score + "\",";
  json += "\"timestamp\":" + String(millis());
  json += "}";

  webSocket.broadcastTXT(json);
  Serial.println("HIT → " + json);
}

void setup() {
  Serial.begin(115200);

  initRows();
  initColumns();
  initWiFi();
  initWebSocket();

  Serial.println("🎯 Dartboard prêt");
}


void loop() {
  webSocket.loop();

  scanNextRow();

  if (hitDetected) {
    handleHit();
  }

  releaseCurrentRow();
}


