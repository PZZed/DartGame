const byte NB_COLONNES = 7;
const byte NB_LIGNES   = 10;

const byte colonnes[NB_COLONNES] = {2,3,4,5,6,7,8};
const byte lignes[NB_LIGNES]     = {9,10,11,12,13,A0,A1,A2,A3,A4};

const unsigned long DEBOUNCE_DELAY = 30; // ms

// États des boutons
bool lastState[NB_COLONNES][NB_LIGNES];
unsigned long lastDebounceTime[NB_COLONNES][NB_LIGNES];

void setup() {
  Serial.begin(115200);
  Serial.println("Début");

  // Colonnes en sortie
  for (byte c = 0; c < NB_COLONNES; c++) {
    pinMode(colonnes[c], OUTPUT);
    digitalWrite(colonnes[c], HIGH); // repos
  }

  // Lignes en entrée avec pull-up
  for (byte l = 0; l < NB_LIGNES; l++) {
    pinMode(lignes[l], INPUT_PULLUP);
  }

  // Initialisation des états
  for (byte c = 0; c < NB_COLONNES; c++) {
    for (byte l = 0; l < NB_LIGNES; l++) {
      lastState[c][l] = HIGH;
      lastDebounceTime[c][l] = 0;
    }
  }
}

void loop() {
  for (byte c = 0; c < NB_COLONNES; c++) {
    digitalWrite(colonnes[c], LOW); // active colonne

    for (byte l = 0; l < NB_LIGNES; l++) {
      bool reading = digitalRead(lignes[l]);

      // Si changement détecté
      if (reading != lastState[c][l]) {
        lastDebounceTime[c][l] = millis();
        lastState[c][l] = reading;
      }

      // Si état stable assez longtemps
      if ((millis() - lastDebounceTime[c][l]) > DEBOUNCE_DELAY) {
        // Détection d’un appui (HIGH → LOW)
        if (reading == LOW) {
          Serial.print("Contact: Ligne ");
          Serial.print(l);
          Serial.print(" - Colonne ");
          Serial.println(c);

          // Empêche répétition tant que le bouton est maintenu
          while (digitalRead(lignes[l]) == LOW);
        }
      }
    }

    digitalWrite(colonnes[c], HIGH); // désactive colonne
  }
}
