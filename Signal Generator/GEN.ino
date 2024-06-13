const int pin1 = 8;
const int pin2 = 9;
const int interval = 500; // Intervalo en milisegundos para una frecuencia de 1 Hz (1 ciclo por segundo)

void setup() {
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
}

void loop() {
  digitalWrite(pin1, HIGH);  // Establecer pin1 en alto (5V)
  digitalWrite(pin2, LOW);   // Establecer pin2 en bajo (0V)
  delay(interval);           // Esperar 500 ms

  digitalWrite(pin1, LOW);   // Establecer pin1 en bajo (0V)
  digitalWrite(pin2, HIGH);  // Establecer pin2 en alto (5V)
  delay(interval);           // Esperar 500 ms
}
