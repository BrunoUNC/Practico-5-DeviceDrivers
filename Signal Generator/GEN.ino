#include <driver/dac.h>

// Definición de pines DAC
#define DAC1_PIN DAC_CHANNEL_1  // GPIO 25
#define DAC2_PIN DAC_CHANNEL_2  // GPIO 26

const int SAMPLE_RATE = 1000; // 1000 Hz sample rate
const int SIGNAL_FREQUENCY = 1; // 1 Hz signal frequency
const int SAMPLES_PER_CYCLE = SAMPLE_RATE / SIGNAL_FREQUENCY; // Number of samples per cycle

const float DUTY_CYCLE1 = 0.80; // Duty cycle of 80%
const float DUTY_CYCLE2 = 0.20; // Duty cycle of 20%

int sampleIndex = 0;
hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
  // Generar señal cuadrada con duty cycle del 80%
  int squareValue1 = (sampleIndex < SAMPLES_PER_CYCLE * DUTY_CYCLE1) ? 255 : 0;

  // Generar señal cuadrada con duty cycle del 20%
  int squareValue2 = (sampleIndex < SAMPLES_PER_CYCLE * DUTY_CYCLE2) ? 255 : 0;

  // Salida de valores a los canales DAC
  dac_output_voltage(DAC1_PIN, squareValue1);
  dac_output_voltage(DAC2_PIN, squareValue2);

  sampleIndex = (sampleIndex + 1) % SAMPLES_PER_CYCLE;
}

void setup() {
  // Habilitar salida DAC
  dac_output_enable(DAC1_PIN);
  dac_output_enable(DAC2_PIN);

  // Configurar temporizador
  timer = timerBegin(0, 80, true); // Timer 0, prescaler 80, count up
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true); // 1 second period
  timerAlarmEnable(timer); // Enable the alarm
}

void loop() {
  // El bucle principal no hace nada, el temporizador maneja todo
}