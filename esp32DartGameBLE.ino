/**
 * @file esp32DartGame_BLE.ino
 * @brief Cible de fléchettes connectée via ESP32 + Bluetooth Low Energy (BLE)
 *
 * Architecture :
 *  - DartMatrix   : gestion du scan matriciel et anti-rebond (inchangée)
 *  - BleManager   : serveur GATT BLE — notifie les impacts via une caractéristique
 *
 * Protocole BLE :
 *  Service UUID         : DART_SERVICE_UUID
 *  Caractéristique UUID : DART_HIT_CHAR_UUID  (NOTIFY | READ)
 *  Payload notifié      : JSON UTF-8, ex. {"row":2,"col":4,"score":"T9","timestamp":12345}
 *
 * Librairie requise : ESP32 BLE Arduino (incluse dans le board package esp32)
 *
 * Branchement :
 *  COLS (sorties)  → pins 13,12,14,27,26,25,33
 *  ROWS (entrées)  → pins 15,2,4,16,17,5,18,19,21,3  (INPUT_PULLDOWN)
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_timer.h"

// ===========================================================================
// Configuration BLE
// ===========================================================================
namespace Config {
  constexpr char DEVICE_NAME[]       = "DartBoard";
  constexpr char DART_SERVICE_UUID[] = "12345678-1234-1234-1234-1234567890ab";
  constexpr char DART_HIT_CHAR_UUID[]= "12345678-1234-1234-1234-1234567890cd";
}

// ===========================================================================
// class DartMatrix  (identique à la version WebSocket)
// ===========================================================================
/**
 * @brief Gère le scan ligne/colonne de la cible et détecte les impacts.
 *
 * Principe de fonctionnement :
 *  - Les ROWS sont activées tour à tour (sortie HIGH pendant ROW_ON_TIME_US µs).
 *  - Les COLS sont en INPUT_PULLDOWN avec interruption RISING.
 *  - Un flag volatile signale un impact ; readHit() lit la position exacte.
 */
class DartMatrix {
public:
  // -------------------------------------------------------------------------
  // Constantes de la matrice
  // -------------------------------------------------------------------------
  static constexpr uint8_t  COL_COUNT      = 7;
  static constexpr uint8_t  ROW_COUNT      = 10;
  static constexpr uint32_t ROW_ON_TIME_US = 40;   ///< Durée d'activation d'une ligne (µs)
  static constexpr uint32_t DEBOUNCE_MS    = 200;  ///< Anti-rebond minimum entre deux impacts

  // -------------------------------------------------------------------------
  // Table de correspondance position → score
  // -------------------------------------------------------------------------
  static constexpr const char* SCORE_TABLE[COL_COUNT][ROW_COUNT] = {
    //         R0     R1     R2     R3     R4     R5     R6     R7     R8     R9
    /* C0 */ {"T9",  "T12", "T5",  "T20", "T10", "T6",  "T13", "T4",  "T18", "T1"},
    /* C1 */ {"D9",  "D12", "D5",  "D20", "D10", "D6",  "D13", "D4",  "D18", "D1"},
    /* C2 */ {"9",   "12",  "5",   "20",  "10",  "6",   "13",  "4",   "18",  "1"},
    /* C3 */ {"D25", "25",  "",    "",    "",    "",    "",    "",    "",    ""},
    /* C4 */ {"14",  "11",  "8",   "16",  "15",  "2",   "17",  "3",   "19",  "7"},
    /* C5 */ {"D14", "D11", "D8",  "D16", "D15", "D2",  "D17", "D3",  "D19", "D7"},
    /* C6 */ {"T14", "T11", "T8",  "T16", "T15", "T2",  "T17", "T3",  "T19", "T7"},
  };

  // -------------------------------------------------------------------------
  // Structure représentant un impact
  // -------------------------------------------------------------------------
  struct Hit {
    uint8_t     row;
    uint8_t     col;
    const char* score;
    uint32_t    timestamp;
  };

  // -------------------------------------------------------------------------
  // Méthodes publiques
  // -------------------------------------------------------------------------

  /** @brief Initialise les GPIO et attache les interruptions. */
  void begin() {
    _initRows();
    _initColumns();
  }

  /**
   * @brief À appeler dans loop() : active la ligne courante.
   *        Doit être suivi d'un appel à releaseRow() après traitement.
   */
  void scanRow() {
    digitalWrite(ROWS[_currentRow], HIGH);
    delayMicroseconds(ROW_ON_TIME_US);
  }

  /** @brief Désactive la ligne courante et passe à la suivante. */
  void releaseRow() {
    digitalWrite(ROWS[_currentRow], LOW);
    _currentRow = (_currentRow + 1) % ROW_COUNT;
  }

  /** @return true si un impact est en attente de traitement. */
  bool hasHit() const { return _hitDetected; }

  /**
   * @brief Lit la position de l'impact détecté.
   * @param[out] hit  Rempli avec les informations de l'impact.
   * @return true si l'impact est valide (colonne active trouvée).
   */
  bool readHit(Hit& hit) {
    noInterrupts();
    const uint8_t row = _currentRow;
    const int8_t  col = _readActiveColumn();
    _hitDetected = false;
    interrupts();

    if (col < 0) return false;

    hit.row       = row;
    hit.col       = static_cast<uint8_t>(col);
    hit.score     = SCORE_TABLE[col][row];
    hit.timestamp = millis();
    return true;
  }

  /** @brief ISR déléguée — appelée depuis le wrapper global dartMatrixISR(). */
  void IRAM_ATTR onInterrupt() {
    const uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    if ((now - _lastHitTime) < DEBOUNCE_MS) return;
    _lastHitTime = now;
    _hitDetected = true;
  }

private:
  static constexpr uint8_t COLS[COL_COUNT] = {13, 12, 14, 27, 26, 25, 33};
  static constexpr uint8_t ROWS[ROW_COUNT] = {15,  2,  4, 16, 17,  5, 18, 19, 21, 3};

  volatile bool     _hitDetected = false;
  volatile uint32_t _lastHitTime = 0;
  uint8_t           _currentRow  = 0;

  void _initRows() {
    for (uint8_t r = 0; r < ROW_COUNT; r++) {
      pinMode(ROWS[r], OUTPUT);
      digitalWrite(ROWS[r], LOW);
    }
  }

  void _initColumns();  // Défini après le wrapper ISR

  int8_t _readActiveColumn() const {
    for (uint8_t c = 0; c < COL_COUNT; c++) {
      if (digitalRead(COLS[c]) == HIGH) return static_cast<int8_t>(c);
    }
    return -1;
  }
};

// ===========================================================================
// Wrapper ISR global (une méthode de classe ne peut pas être une ISR directe)
// ===========================================================================
static DartMatrix* g_dartMatrix = nullptr;

void IRAM_ATTR dartMatrixISR() {
  if (g_dartMatrix) g_dartMatrix->onInterrupt();
}

void DartMatrix::_initColumns() {
  for (uint8_t c = 0; c < COL_COUNT; c++) {
    pinMode(COLS[c], INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(COLS[c]), dartMatrixISR, RISING);
  }
}

// Définition des membres statiques constexpr
constexpr uint8_t     DartMatrix::COLS[];
constexpr uint8_t     DartMatrix::ROWS[];
constexpr const char* DartMatrix::SCORE_TABLE[DartMatrix::COL_COUNT][DartMatrix::ROW_COUNT];

// ===========================================================================
// class BleManager
// ===========================================================================
/**
 * @brief Serveur GATT BLE qui notifie les impacts aux clients abonnés.
 *
 * Cycle de vie :
 *  1. begin()      → initialise BLE, démarre l'advertising
 *  2. loop()       → relance l'advertising si le client se déconnecte
 *  3. notifyHit()  → envoie un JSON via la caractéristique NOTIFY
 *
 * Un seul client peut être connecté à la fois (comportement standard BLE).
 */
class BleManager : public BLEServerCallbacks {
public:
  /** @brief Initialise le périphérique BLE et démarre l'advertising. */
  void begin() {
    BLEDevice::init(Config::DEVICE_NAME);

    _server = BLEDevice::createServer();
    _server->setCallbacks(this);

    BLEService* service = _server->createService(Config::DART_SERVICE_UUID);

    _hitCharacteristic = service->createCharacteristic(
      Config::DART_HIT_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    // Descripteur obligatoire pour activer NOTIFY côté client (BT spec 0x2902)
    _hitCharacteristic->addDescriptor(new BLE2902());

    service->start();
    _startAdvertising();

    Serial.println("[BLE] Advertising démarré — nom : " + String(Config::DEVICE_NAME));
  }

  /**
   * @brief À appeler dans loop() : relance l'advertising après déconnexion.
   */
  void loop() {
    if (_clientDisconnected) {
      _clientDisconnected = false;
      delay(500);  // Laisse le stack BLE se stabiliser
      _startAdvertising();
      Serial.println("[BLE] Re-advertising…");
    }
  }

  /**
   * @brief Notifie le client connecté d'un impact.
   * @param hit Impact à transmettre.
   */
  void notifyHit(const DartMatrix::Hit& hit) {
    if (!_clientConnected) {
      Serial.println("[BLE] Aucun client connecté, impact ignoré.");
      return;
    }

    String json;
    json.reserve(80);
    json  = "{";
    json += "\"row\":"       + String(hit.row)       + ",";
    json += "\"col\":"       + String(hit.col)       + ",";
    json += "\"score\":\""   + String(hit.score)     + "\",";
    json += "\"timestamp\":" + String(hit.timestamp);
    json += "}";

    _hitCharacteristic->setValue(json.c_str());
    _hitCharacteristic->notify();

    Serial.println("[BLE] Notif → " + json);
  }

  /** @return true si un client BLE est actuellement connecté. */
  bool isConnected() const { return _clientConnected; }

private:
  // -------------------------------------------------------------------------
  // Callbacks BLEServerCallbacks
  // -------------------------------------------------------------------------
  void onConnect(BLEServer* /*server*/) override {
    _clientConnected = true;
    Serial.println("[BLE] Client connecté");
  }

  void onDisconnect(BLEServer* /*server*/) override {
    _clientConnected    = false;
    _clientDisconnected = true;
    Serial.println("[BLE] Client déconnecté");
  }

  // -------------------------------------------------------------------------
  // Méthodes privées
  // -------------------------------------------------------------------------
  void _startAdvertising() {
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(Config::DART_SERVICE_UUID);
    advertising->setScanResponse(true);
    // Améliore la détection par iOS/Android
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
  }

  // -------------------------------------------------------------------------
  // État interne
  // -------------------------------------------------------------------------
  BLEServer*          _server              = nullptr;
  BLECharacteristic*  _hitCharacteristic   = nullptr;
  bool                _clientConnected     = false;
  bool                _clientDisconnected  = false;
};

// ===========================================================================
// Instances globales
// ===========================================================================
static DartMatrix g_matrix;
static BleManager g_ble;

// ===========================================================================
// setup / loop
// ===========================================================================
void setup() {
  Serial.begin(115200);

  g_dartMatrix = &g_matrix;  // Lie l'ISR à l'instance
  g_matrix.begin();

  g_ble.begin();

  Serial.println("[App] Cible de fléchettes prête 🎯");
}

void loop() {
  g_ble.loop();
  g_matrix.scanRow();

  if (g_matrix.hasHit()) {
    DartMatrix::Hit hit;
    if (g_matrix.readHit(hit)) {
      Serial.printf("[Hit] row=%d col=%d score=%s\n",
                    hit.row, hit.col, hit.score);
      g_ble.notifyHit(hit);
      delay(DartMatrix::DEBOUNCE_MS);
    }
  }

  g_matrix.releaseRow();
}
