#include <Arduino.h>
#include <SPI.h>

// AS5048 config from Arduino-FOC
#define AS5048_ANGLE_REGISTER 0x3FFF

// Change these if your wiring differs
const int CS_PIN = 5; // chip select used in your project
const int MISO_PIN = 19; // default ESP32 VSPI MISO - change if needed
SPISettings settings(1000000, MSBFIRST, SPI_MODE1);

void setup() {
  Serial.begin(115200);
  while (!Serial) ;
  Serial.println("Sensor test start");
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  pinMode(MISO_PIN, INPUT);
  SPI.begin();
  delay(100);
}

uint16_t readRegister(uint16_t reg) {
  uint16_t command = reg;
  // set read bit for AS5048 (bit 14)
  command |= (1 << 14);
  // parity bit (bit 15)
  auto calcParity = [](uint16_t v){
    uint8_t cnt = 0;
    for (int i=0;i<16;i++) { if (v & 0x1) cnt++; v >>= 1; }
    return (cnt & 0x1);
  };
  command |= ((uint16_t)calcParity(command) << 15);

  // diagnostic: MISO level before transaction (CS high)
  int miso_idle = digitalRead(MISO_PIN);

  SPI.beginTransaction(settings);
  digitalWrite(CS_PIN, LOW);
  // small delay to let device see CS low
  delayMicroseconds(2);
  SPI.transfer16(command);
  digitalWrite(CS_PIN, HIGH);
  delayMicroseconds(50);
  digitalWrite(CS_PIN, LOW);
  uint16_t resp = SPI.transfer16(0x0000);
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();

  int miso_after = digitalRead(MISO_PIN);
  Serial.print("MISO idle="); Serial.print(miso_idle);
  Serial.print(" MISO after transfer="); Serial.println(miso_after);

  return resp;
}

void loop() {
  uint16_t raw = readRegister(AS5048_ANGLE_REGISTER);
  // For AS5048 (14-bit) shift right by (1 + data_start_bit - bit_resolution)
  // data_start_bit=13, bit_resolution=14 => shift = 0
  uint16_t data_mask = 0x3FFF; // 14-bit
  uint16_t value = raw & data_mask;
  float angle = (value / 16384.0f) * TWO_PI;
  Serial.print("raw=0x");
  Serial.print(raw, HEX);
  Serial.print(" masked=0x");
  Serial.print(value, HEX);
  Serial.print(" value=");
  Serial.print(value);
  Serial.print(" angle=");
  Serial.println(angle, 6);
  delay(300);
}
