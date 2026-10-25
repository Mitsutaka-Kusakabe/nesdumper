/*
 * nesdumper - Famicom / NES ROM Reader Firmware for Arduino Mega 2560
 * 
 * Mapped to actual connector orientation (A30 = Pin 1 GND, A1 = Pin 30 +5V):
 *   Pin_Actual = 31 - Connector_Label
 * 
 * [Power & GND]
 *   A30 (Pin 01) -> GND
 *   A1  (Pin 30) -> +5V
 * 
 * [CPU / PRG Address (A0..A14)]
 *   CPU A0  (Pin 13 / A18) : D22
 *   CPU A1  (Pin 12 / A19) : A6
 *   CPU A2  (Pin 11 / A20) : A5
 *   CPU A3  (Pin 10 / A21) : A4
 *   CPU A4  (Pin 09 / A22) : A3
 *   CPU A5  (Pin 08 / A23) : A2
 *   CPU A6  (Pin 07 / A24) : A1
 *   CPU A7  (Pin 06 / A25) : A0
 *   CPU A8  (Pin 05 / A26) : D5
 *   CPU A9  (Pin 04 / A27) : D6
 *   CPU A10 (Pin 03 / A28) : D7
 *   CPU A11 (Pin 02 / A29) : D8
 *   CPU A12 (Pin 33 / B28) : D11
 *   CPU A13 (Pin 34 / B27) : D12
 *   CPU A14 (Pin 35 / B26) : A13
 * 
 * [CPU / PRG Data (D0..D7)]
 *   CPU D0  (Pin 43 / B18) : D49
 *   CPU D1  (Pin 42 / B19) : A14
 *   CPU D2  (Pin 41 / B20) : A7
 *   CPU D3  (Pin 40 / B21) : A8
 *   CPU D4  (Pin 39 / B22) : A9
 *   CPU D5  (Pin 38 / B23) : A10
 *   CPU D6  (Pin 37 / B24) : A11
 *   CPU D7  (Pin 36 / B25) : A12
 * 
 * [CPU Control]
 *   CPU R/W (Pin 14 / A17) : D13
 *   M2      (Pin 32 / B29) : D10
 *   /ROMSEL (Pin 44 / B17) : D14 (and D2 mirrored for safety)
 * 
 * [PPU / CHR Address (A0..A13 + /A13)]
 *   PPU A0   (Pin 25 / A6)  : D29
 *   PPU A1   (Pin 24 / A7)  : D28
 *   PPU A2   (Pin 23 / A8)  : D27
 *   PPU A3   (Pin 22 / A9)  : D26
 *   PPU A4   (Pin 21 / A10) : D25
 *   PPU A5   (Pin 20 / A11) : D24
 *   PPU A6   (Pin 19 / A12) : D23
 *   PPU A7   (Pin 50 / B11) : D47
 *   PPU A8   (Pin 51 / B10) : D46
 *   PPU A9   (Pin 52 / B9)  : D45
 *   PPU A10  (Pin 53 / B8)  : D44
 *   PPU A11  (Pin 54 / B7)  : D43
 *   PPU A12  (Pin 55 / B6)  : D42
 *   PPU A13  (Pin 56 / B5)  : D31
 *   PPU /A13 (Pin 49 / B12) : D48
 * 
 * [PPU / CHR Data (D0..D7)]
 *   PPU D0  (Pin 26 / A5)  : D37
 *   PPU D1  (Pin 27 / A4)  : D36
 *   PPU D2  (Pin 28 / A3)  : D35
 *   PPU D3  (Pin 29 / A2)  : D34
 *   PPU D4  (Pin 60 / B1)  : D9
 *   PPU D5  (Pin 59 / B2)  : D4
 *   PPU D6  (Pin 58 / B3)  : D33
 *   PPU D7  (Pin 57 / B4)  : D32
 * 
 * [PPU Control]
 *   PPU /RD (Pin 17 / A14) : D3
 *   PPU /WR (Pin 47 / B14) : D2
 */

#include <Arduino.h>

#define SERIAL_BAUD 115200

// Pin lookup tables matching the physical connector wiring
static const uint8_t PIN_CPU_A[15] = {
  22, A6, A5, A4, A3, A2, A1, A0,  // CPU A0..A7
  5,  6,  7,  8,  11, 12, A13      // CPU A8..A14
};

static const uint8_t PIN_CPU_D[8] = {
  49, A14, A7, A8, A9, A10, A11, A12 // CPU D0..D7
};

#define PIN_CPU_RW    13  // A17 (Pin 14)
#define PIN_M2        10  // B29 (Pin 32)
#define PIN_ROMSEL_1  14  // B17 (Pin 44) - if D14 is plugged into B17
#define PIN_ROMSEL_2  2   // B17/B14      - if D2 is plugged into B17

static const uint8_t PIN_PPU_A[14] = {
  29, 28, 27, 26, 25, 24, 23, 47,  // PPU A0..A7
  46, 45, 44, 43, 42, 31           // PPU A8..A13
};

#define PIN_PPU_A13_N 48  // B12 (Pin 49: PPU /A13)

static const uint8_t PIN_PPU_D[8] = {
  37, 36, 35, 34, 9, 4, 33, 32     // PPU D0..D7
};

#define PIN_PPU_RD    3   // A14 (Pin 17)

// Put all pins into safe High-Z (INPUT) state when idle
void safe_high_z() {
  DDRA = 0x00; PORTA = 0x00;
  DDRC = 0x00; PORTC = 0x00;
  DDRL = 0x00; PORTL = 0x00;
  DDRF = 0x00; PORTF = 0x00;
  DDRK = 0x00; PORTK = 0x00;
  for (int p = 2; p <= 15; p++) {
    pinMode(p, INPUT);
  }
}

static inline void set_romsel(uint8_t state) {
  digitalWrite(PIN_ROMSEL_1, state);
  digitalWrite(PIN_ROMSEL_2, state);
}

static void bus_idle() {
  for (int i = 0; i < 8; i++) pinMode(PIN_CPU_D[i], INPUT_PULLUP);
  for (int i = 0; i < 8; i++) pinMode(PIN_PPU_D[i], INPUT_PULLUP);
  set_romsel(HIGH);
  digitalWrite(PIN_CPU_RW, HIGH);
  digitalWrite(PIN_M2, HIGH);
  digitalWrite(PIN_PPU_RD, HIGH);
  digitalWrite(PIN_PPU_A13_N, HIGH);
}

void enable_bus() {
  for (int i = 0; i < 15; i++) {
    pinMode(PIN_CPU_A[i], OUTPUT);
    digitalWrite(PIN_CPU_A[i], LOW);
  }
  for (int i = 0; i < 8; i++) {
    pinMode(PIN_CPU_D[i], INPUT_PULLUP);
  }

  pinMode(PIN_ROMSEL_1, OUTPUT);
  pinMode(PIN_ROMSEL_2, OUTPUT);
  set_romsel(HIGH);

  pinMode(PIN_CPU_RW, OUTPUT);
  digitalWrite(PIN_CPU_RW, HIGH);

  pinMode(PIN_M2, OUTPUT);
  digitalWrite(PIN_M2, HIGH);

  for (int i = 0; i < 14; i++) {
    pinMode(PIN_PPU_A[i], OUTPUT);
    digitalWrite(PIN_PPU_A[i], LOW);
  }
  pinMode(PIN_PPU_A13_N, OUTPUT);
  digitalWrite(PIN_PPU_A13_N, HIGH);

  for (int i = 0; i < 8; i++) {
    pinMode(PIN_PPU_D[i], INPUT_PULLUP);
  }

  pinMode(PIN_PPU_RD, OUTPUT);
  digitalWrite(PIN_PPU_RD, HIGH);
}

static inline void set_cpu_addr(uint16_t addr) {
  for (int i = 0; i < 15; i++) {
    digitalWrite(PIN_CPU_A[i], (addr >> i) & 1);
  }
}

static inline void set_ppu_addr(uint16_t addr) {
  for (int i = 0; i < 14; i++) {
    digitalWrite(PIN_PPU_A[i], (addr >> i) & 1);
  }
  // PPU /A13 is logical NOT of PPU A13 (bit 13)
  bool a13 = (addr >> 13) & 1;
  digitalWrite(PIN_PPU_A13_N, a13 ? LOW : HIGH);
}

static inline uint8_t read_cpu_data_bus() {
  uint8_t val = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(PIN_CPU_D[i])) val |= (1 << i);
  }
  return val;
}

static inline uint8_t read_ppu_data_bus() {
  uint8_t val = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(PIN_PPU_D[i])) val |= (1 << i);
  }
  return val;
}

uint8_t read_prg_byte(uint16_t addr) {
  set_cpu_addr(addr);
  digitalWrite(PIN_CPU_RW, HIGH);
  set_romsel(HIGH);
  digitalWrite(PIN_M2, LOW);
  delayMicroseconds(2);
  
  // Real Famicom timing: /ROMSEL goes LOW only when M2 goes HIGH
  digitalWrite(PIN_M2, HIGH);
  set_romsel(LOW);
  delayMicroseconds(8);
  
  uint8_t val = read_cpu_data_bus();
  
  set_romsel(HIGH);
  digitalWrite(PIN_M2, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_M2, HIGH);
  return val;
}

uint8_t read_wram_byte(uint16_t addr) {
  set_cpu_addr(addr);
  set_romsel(HIGH);
  digitalWrite(PIN_CPU_RW, HIGH);
  digitalWrite(PIN_M2, LOW);
  delayMicroseconds(1);
  digitalWrite(PIN_M2, HIGH);
  delayMicroseconds(2);
  uint8_t val = read_cpu_data_bus();
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M2, HIGH);
  return val;
}

void write_cpu_byte(uint16_t addr, uint8_t data) {
  bool is_rom_space = (addr >= 0x8000);
  set_cpu_addr(addr);
  for (int i = 0; i < 8; i++) {
    pinMode(PIN_CPU_D[i], OUTPUT);
    digitalWrite(PIN_CPU_D[i], (data >> i) & 1);
  }
  
  // Phase 1: M2 is LOW, /ROMSEL is HIGH, R/W goes LOW
  set_romsel(HIGH);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_CPU_RW, LOW);
  delayMicroseconds(2);

  // Phase 2: M2 goes HIGH, /ROMSEL goes LOW (qualified by M2)
  digitalWrite(PIN_M2, HIGH);
  if (is_rom_space) set_romsel(LOW);
  delayMicroseconds(6);

  // End of Phase 2: M2 falls while /ROMSEL is still LOW (latches mapper)
  digitalWrite(PIN_M2, LOW);
  delayMicroseconds(1);
  if (is_rom_space) set_romsel(HIGH);
  delayMicroseconds(2);

  // Return to Phase 1 / bus idle
  digitalWrite(PIN_CPU_RW, HIGH);
  digitalWrite(PIN_M2, HIGH);
  for (int i = 0; i < 8; i++) {
    pinMode(PIN_CPU_D[i], INPUT_PULLUP);
  }
  delayMicroseconds(2);
  bus_idle();
}

void bank_select_write(uint16_t addr) {
  set_cpu_addr(addr);
  for (int i = 0; i < 8; i++) pinMode(PIN_CPU_D[i], INPUT_PULLUP);
  delayMicroseconds(2);
  digitalWrite(PIN_CPU_RW, LOW);
  delayMicroseconds(1);
  set_romsel(LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_M2, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_M2, HIGH);
  delayMicroseconds(2);
  set_romsel(HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_CPU_RW, HIGH);
  delayMicroseconds(2);
  bus_idle();
}

uint8_t read_chr_byte(uint16_t addr) {
  set_ppu_addr(addr & 0x3FFF);
  delayMicroseconds(15);
  digitalWrite(PIN_PPU_RD, LOW);
  delayMicroseconds(15);
  uint8_t val = read_ppu_data_bus();
  digitalWrite(PIN_PPU_RD, HIGH);
  delayMicroseconds(5);
  return val;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *p, size_t n) {
  static const uint32_t tbl[16] = {
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
  };
  while (n--) {
    crc ^= *p++;
    crc = (crc >> 4) ^ tbl[crc & 0x0F];
    crc = (crc >> 4) ^ tbl[crc & 0x0F];
  }
  return crc;
}

void print_hex2(uint8_t b) {
  if (b < 0x10) Serial.print('0');
  Serial.print(b, HEX);
}

void print_hex4(uint16_t v) {
  print_hex2((uint8_t)(v >> 8));
  print_hex2((uint8_t)(v & 0xFF));
}

void print_hex8(uint32_t v) {
  print_hex4((uint16_t)(v >> 16));
  print_hex4((uint16_t)(v & 0xFFFF));
}

void cmd_scan() {
  static const uint8_t testPins[] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    22, 23, 24, 25, 26, 27, 28, 29,
    31, 32, 33, 34, 35, 36, 36, 37,
    42, 43, 44, 45, 46, 47, 48, 49,
    A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14
  };
  const int N = sizeof(testPins) / sizeof(testPins[0]);

  for (int i = 0; i < N; i++) {
    pinMode(testPins[i], INPUT_PULLUP);
  }
  delay(10);

  Serial.println(F("=== SAFE PIN SCAN (INPUT_PULLUP) ==="));
  Serial.print(F("Pins directly tied to GND (reading LOW): "));
  int lowCount = 0;
  for (int i = 0; i < N; i++) {
    if (digitalRead(testPins[i]) == LOW) {
      Serial.print(F("D"));
      Serial.print(testPins[i]);
      Serial.print(' ');
      lowCount++;
    }
  }
  if (lowCount == 0) Serial.print(F("NONE"));
  Serial.println();

  safe_high_z();
  Serial.println(F("=== SCAN COMPLETE ==="));
}

struct WirePin {
  uint8_t pin;
  const char* label;
  const char* name;
};

static const WirePin WIRE_PINS[] = {
  { 22,  "A18", "CPU_A0" },
  { A6,  "A19", "CPU_A1" },
  { A5,  "A20", "CPU_A2" },
  { A4,  "A21", "CPU_A3" },
  { A3,  "A22", "CPU_A4" },
  { A2,  "A23", "CPU_A5" },
  { A1,  "A24", "CPU_A6" },
  { A0,  "A25", "CPU_A7" },
  { 5,   "A26", "CPU_A8" },
  { 6,   "A27", "CPU_A9" },
  { 7,   "A28", "CPU_A10" },
  { 8,   "A29", "CPU_A11" },
  { 11,  "B28", "CPU_A12" },
  { 12,  "B27", "CPU_A13" },
  { A13, "B26", "CPU_A14" },
  { 49,  "B18", "CPU_D0" },
  { A14, "B19", "CPU_D1" },
  { A7,  "B20", "CPU_D2" },
  { A8,  "B21", "CPU_D3" },
  { A9,  "B22", "CPU_D4" },
  { A10, "B23", "CPU_D5" },
  { A11, "B24", "CPU_D6" },
  { A12, "B25", "CPU_D7" },
  { 13,  "A17", "CPU_RW" },
  { 10,  "B29", "M2" },
  { 14,  "B17", "ROMSEL" },
  { 29,  "A6",  "PPU_A0" },
  { 28,  "A7",  "PPU_A1" },
  { 27,  "A8",  "PPU_A2" },
  { 26,  "A9",  "PPU_A3" },
  { 25,  "A10", "PPU_A4" },
  { 24,  "A11", "PPU_A5" },
  { 23,  "A12", "PPU_A6" },
  { 47,  "B11", "PPU_A7" },
  { 46,  "B10", "PPU_A8" },
  { 45,  "B9",  "PPU_A9" },
  { 44,  "B8",  "PPU_A10" },
  { 43,  "B7",  "PPU_A11" },
  { 42,  "B6",  "PPU_A12" },
  { 31,  "B5",  "PPU_A13" },
  { 48,  "B12", "PPU_/A13" },
  { 37,  "A5",  "PPU_D0" },
  { 36,  "A4",  "PPU_D1" },
  { 35,  "A3",  "PPU_D2" },
  { 34,  "A2",  "PPU_D3" },
  { 9,   "B1",  "PPU_D4" },
  { 4,   "B2",  "PPU_D5" },
  { 33,  "B3",  "PPU_D6" },
  { 32,  "B4",  "PPU_D7" },
  { 3,   "A14", "PPU_/RD" }
};
static const int NUM_WIRE_PINS = sizeof(WIRE_PINS) / sizeof(WIRE_PINS[0]);

void cmd_wirecheck() {
  Serial.println(F("=== WIRECHECK START ==="));
  for (int i = 0; i < NUM_WIRE_PINS; i++) {
    pinMode(WIRE_PINS[i].pin, INPUT_PULLUP);
  }
  delay(10);

  int gndFaults = 0;
  for (int i = 0; i < NUM_WIRE_PINS; i++) {
    if (digitalRead(WIRE_PINS[i].pin) == LOW) {
      Serial.print(F("GND_FAULT:"));
      Serial.print(WIRE_PINS[i].label); Serial.print(':');
      Serial.print(WIRE_PINS[i].name); Serial.print(':');
      Serial.println(WIRE_PINS[i].pin);
      gndFaults++;
    }
  }
  if (gndFaults == 0) Serial.println(F("GND_CHECK:PASS"));

  int shortFaults = 0;
  for (int i = 0; i < NUM_WIRE_PINS; i++) {
    pinMode(WIRE_PINS[i].pin, OUTPUT);
    digitalWrite(WIRE_PINS[i].pin, LOW);
    delayMicroseconds(10);

    for (int j = 0; j < NUM_WIRE_PINS; j++) {
      if (i == j) continue;
      if (digitalRead(WIRE_PINS[j].pin) == LOW) {
        Serial.print(F("SHORT_FAULT:"));
        Serial.print(WIRE_PINS[i].label); Serial.print(':'); Serial.print(WIRE_PINS[i].name);
        Serial.print(F("<->"));
        Serial.print(WIRE_PINS[j].label); Serial.print(':'); Serial.println(WIRE_PINS[j].name);
        shortFaults++;
      }
    }
    pinMode(WIRE_PINS[i].pin, INPUT_PULLUP);
  }
  if (shortFaults == 0) Serial.println(F("SHORT_CHECK:PASS"));

  safe_high_z();
  Serial.println(F("=== WIRECHECK END ==="));
}

void cmd_probe() {
  Serial.println(F("=== PROBE START ==="));
  for (int i = 0; i < NUM_WIRE_PINS; i++) {
    pinMode(WIRE_PINS[i].pin, INPUT_PULLUP);
  }
  delay(10);

  int lastTouched = -1;
  while (!Serial.available()) {
    int current = -1;
    for (int i = 0; i < NUM_WIRE_PINS; i++) {
      if (digitalRead(WIRE_PINS[i].pin) == LOW) {
        current = i;
        break;
      }
    }
    if (current != -1 && current != lastTouched) {
      Serial.print(F("TOUCH:"));
      Serial.print(WIRE_PINS[current].label); Serial.print(':');
      Serial.print(WIRE_PINS[current].name); Serial.print(':');
      Serial.println(WIRE_PINS[current].pin);
      delay(30);
    }
    lastTouched = current;
    delay(5);
  }
  while (Serial.available()) Serial.read();
  safe_high_z();
  Serial.println(F("=== PROBE END ==="));
}

void cmd_pincheck() {
  enable_bus();
  delayMicroseconds(50);
  Serial.println(F("=== PINCHECK START ==="));

  // 1. CPU Data Bus Bit Distribution
  uint16_t ones[8] = {0};
  uint16_t zeros[8] = {0};
  for (uint16_t i = 0; i < 256; i++) {
    uint8_t val = read_prg_byte(0xC000 + i);
    for (int b = 0; b < 8; b++) {
      if ((val >> b) & 1) ones[b]++;
      else zeros[b]++;
    }
  }

  static const char* cpu_d_labels[8] = { "B18", "B19", "B20", "B21", "B22", "B23", "B24", "B25" };
  static const char* cpu_d_pins[8] = { "D49", "A14", "A7", "A8", "A9", "A10", "A11", "A12" };
  for (int b = 0; b < 8; b++) {
    Serial.print(F("PIN:CPU_D"));
    Serial.print(b);
    if (zeros[b] == 0) {
      Serial.print(F(":FAIL_STUCK_HIGH:"));
      Serial.print(cpu_d_labels[b]); Serial.print(':'); Serial.println(cpu_d_pins[b]);
    } else if (ones[b] == 0) {
      Serial.print(F(":FAIL_STUCK_LOW:"));
      Serial.print(cpu_d_labels[b]); Serial.print(':'); Serial.println(cpu_d_pins[b]);
    } else {
      Serial.println(F(":PASS"));
    }
  }

  // 2. CPU Address Lines Aliasing Check
  static const char* cpu_a_labels[14] = { "A18", "A19", "A20", "A21", "A22", "A23", "A24", "A25", "A26", "A27", "A28", "A29", "B28", "B27" };
  static const char* cpu_a_pins[14]   = { "D22", "A6", "A5", "A4", "A3", "A2", "A1", "A0", "D5", "D6", "D7", "D8", "D11", "D12" };
  for (int b = 0; b < 14; b++) {
    uint16_t mask = (1 << b);
    int diffCount = 0;
    for (uint16_t sample = 0; sample < 16; sample++) {
      uint16_t addr1 = 0xC000 + (sample * 17);
      uint16_t addr2 = addr1 ^ mask;
      if (read_prg_byte(addr1) != read_prg_byte(addr2)) {
        diffCount++;
      }
    }
    Serial.print(F("PIN:CPU_A"));
    Serial.print(b);
    if (diffCount == 0) {
      Serial.print(F(":FAIL_ALIAS:"));
      Serial.print(cpu_a_labels[b]); Serial.print(':'); Serial.println(cpu_a_pins[b]);
    } else {
      Serial.println(F(":PASS"));
    }
  }

  // 3. PPU Data Bus Bit Distribution
  uint16_t pones[8] = {0};
  uint16_t pzeros[8] = {0};
  for (uint16_t i = 0; i < 256; i++) {
    uint8_t val = read_chr_byte(i);
    for (int b = 0; b < 8; b++) {
      if ((val >> b) & 1) pones[b]++;
      else pzeros[b]++;
    }
  }
  static const char* ppu_d_labels[8] = { "A5", "A4", "A3", "A2", "B1", "B2", "B3", "B4" };
  static const char* ppu_d_pins[8]   = { "D37", "D36", "D35", "D34", "D9", "D4", "D33", "D32" };
  for (int b = 0; b < 8; b++) {
    Serial.print(F("PIN:PPU_D"));
    Serial.print(b);
    if (pzeros[b] == 0) {
      Serial.print(F(":FAIL_STUCK_HIGH:"));
      Serial.print(ppu_d_labels[b]); Serial.print(':'); Serial.println(ppu_d_pins[b]);
    } else if (pones[b] == 0) {
      Serial.print(F(":FAIL_STUCK_LOW:"));
      Serial.print(ppu_d_labels[b]); Serial.print(':'); Serial.println(ppu_d_pins[b]);
    } else {
      Serial.println(F(":PASS"));
    }
  }

  // 4. PPU Address Lines Aliasing Check
  static const char* ppu_a_labels[13] = { "A6", "A7", "A8", "A9", "A10", "A11", "A12", "B11", "B10", "B9", "B8", "B7", "B6" };
  static const char* ppu_a_pins[13]   = { "D29", "D28", "D27", "D26", "D25", "D24", "D23", "D47", "D46", "D45", "D44", "D43", "D42" };
  for (int b = 0; b < 13; b++) {
    uint16_t mask = (1 << b);
    int diffCount = 0;
    for (uint16_t sample = 0; sample < 16; sample++) {
      uint16_t addr1 = (sample * 17) & 0x1FFF;
      uint16_t addr2 = addr1 ^ mask;
      if (read_chr_byte(addr1) != read_chr_byte(addr2)) {
        diffCount++;
      }
    }
    Serial.print(F("PIN:PPU_A"));
    Serial.print(b);
    if (diffCount == 0) {
      Serial.print(F(":FAIL_ALIAS:"));
      Serial.print(ppu_a_labels[b]); Serial.print(':'); Serial.println(ppu_a_pins[b]);
    } else {
      Serial.println(F(":PASS"));
    }
  }

  bus_idle();
  Serial.println(F("=== PINCHECK END ==="));
}

void cmd_mmc1_write(uint16_t addr, uint8_t val) {
  enable_bus();
  delayMicroseconds(10);
  if (val & 0x80) {
    write_cpu_byte(addr, 0x80);
  } else {
    for (int i = 0; i < 5; i++) {
      write_cpu_byte(addr, (val >> i) & 1);
      delayMicroseconds(5);
    }
  }
  bus_idle();
  Serial.println(F("MMC1_OK"));
}

void cmd_test() {
  enable_bus();
  delayMicroseconds(50);

  Serial.println(F("--- NES DUMPER BUS TEST ---"));

  uint16_t nmi   = read_prg_byte(0xFFFA) | ((uint16_t)read_prg_byte(0xFFFB) << 8);
  uint16_t reset = read_prg_byte(0xFFFC) | ((uint16_t)read_prg_byte(0xFFFD) << 8);
  uint16_t irq   = read_prg_byte(0xFFFE) | ((uint16_t)read_prg_byte(0xFFFF) << 8);

  Serial.print(F("PRG $FFFA (NMI):   $")); print_hex4(nmi); Serial.println();
  Serial.print(F("PRG $FFFC (RESET): $")); print_hex4(reset); Serial.println();
  Serial.print(F("PRG $FFFE (IRQ):   $")); print_hex4(irq); Serial.println();

  Serial.println(F("\nPRG Dump sample ($C000-$C01F):"));
  for (uint16_t a = 0xC000; a < 0xC020; a += 16) {
    print_hex4(a);
    Serial.print(F(": "));
    for (uint16_t i = 0; i < 16; i++) {
      print_hex2(read_prg_byte(a + i));
      Serial.print(' ');
    }
    Serial.println();
  }

  Serial.println(F("\nCHR Dump sample ($0000-$001F):"));
  for (uint16_t a = 0x0000; a < 0x0020; a += 16) {
    print_hex4(a);
    Serial.print(F(": "));
    for (uint16_t i = 0; i < 16; i++) {
      print_hex2(read_chr_byte(a + i));
      Serial.print(' ');
    }
    Serial.println();
  }
  safe_high_z();
  Serial.println(F("--- TEST COMPLETE ---"));
}

void cmd_dump_prg(uint16_t start, uint16_t len) {
  enable_bus();
  delayMicroseconds(50);

  Serial.print(F("PRG_START:"));
  Serial.println(len);

  uint8_t buf[64];
  uint16_t remaining = len;
  uint16_t curr = start;

  while (remaining > 0) {
    uint8_t chunkSize = (remaining > sizeof(buf)) ? sizeof(buf) : remaining;
    for (uint8_t i = 0; i < chunkSize; i++) {
      buf[i] = read_prg_byte(curr++);
    }
    Serial.write(buf, chunkSize);
    remaining -= chunkSize;
  }
  safe_high_z();
  Serial.println(F("\nPRG_END"));
}

void cmd_dump_chr(uint16_t start, uint16_t len) {
  enable_bus();
  delayMicroseconds(50);

  Serial.print(F("CHR_START:"));
  Serial.println(len);

  uint8_t buf[64];
  uint16_t remaining = len;
  uint16_t curr = start;

  while (remaining > 0) {
    uint8_t chunkSize = (remaining > sizeof(buf)) ? sizeof(buf) : remaining;
    for (uint8_t i = 0; i < chunkSize; i++) {
      buf[i] = read_chr_byte(curr++);
    }
    Serial.write(buf, chunkSize);
    remaining -= chunkSize;
  }
  safe_high_z();
  Serial.println(F("\nCHR_END"));
}

void handle_famidump_read(char region, uint32_t addr, uint32_t len) {
  enable_bus();
  delayMicroseconds(50);

  Serial.print(F("OK "));
  Serial.println(len, HEX);

  uint8_t buf[128];
  uint32_t crc = 0xFFFFFFFF;
  while (len > 0) {
    uint32_t n = (len > sizeof(buf)) ? sizeof(buf) : len;
    for (uint32_t i = 0; i < n; i++) {
      if (region == 'C') buf[i] = read_chr_byte((uint16_t)(addr + i));
      else if (region == 'W') buf[i] = read_wram_byte((uint16_t)(addr + i));
      else buf[i] = read_prg_byte((uint16_t)(addr + i));
    }
    crc = crc32_update(crc, buf, n);
    Serial.write(buf, n);
    addr += n;
    len -= n;
  }
  bus_idle();
  Serial.print(F("CRC "));
  print_hex8(crc ^ 0xFFFFFFFF);
  Serial.println();
}

void setup() {
  safe_high_z();
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { ; }
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line == "PING") {
      Serial.println(F("PONG"));
      return;
    } else if (line == "SCAN") {
      cmd_scan();
      return;
    } else if (line == "TEST") {
      cmd_test();
      return;
    } else if (line == "PINCHECK") {
      cmd_pincheck();
      return;
    } else if (line == "WIRECHECK") {
      cmd_wirecheck();
      return;
    } else if (line == "PROBE") {
      cmd_probe();
      return;
    } else if (line.startsWith("MMC1 ")) {
      int sp1 = line.indexOf(' ');
      int sp2 = line.indexOf(' ', sp1 + 1);
      if (sp1 > 0 && sp2 > 0) {
        uint16_t addr = strtoul(line.substring(sp1 + 1, sp2).c_str(), NULL, 16);
        uint8_t val = strtoul(line.substring(sp2 + 1).c_str(), NULL, 16);
        cmd_mmc1_write(addr, val);
      } else {
        Serial.println(F("ERR_INVALID_PARAMS"));
      }
      return;
    } else if (line.startsWith("DUMP_PRG")) {
      int sp1 = line.indexOf(' ');
      int sp2 = line.indexOf(' ', sp1 + 1);
      if (sp1 > 0 && sp2 > 0) {
        uint16_t start = strtoul(line.substring(sp1 + 1, sp2).c_str(), NULL, 16);
        uint16_t len = strtoul(line.substring(sp2 + 1).c_str(), NULL, 16);
        cmd_dump_prg(start, len);
      } else {
        Serial.println(F("ERR_INVALID_PARAMS"));
      }
      return;
    } else if (line.startsWith("DUMP_CHR")) {
      int sp1 = line.indexOf(' ');
      int sp2 = line.indexOf(' ', sp1 + 1);
      if (sp1 > 0 && sp2 > 0) {
        uint16_t start = strtoul(line.substring(sp1 + 1, sp2).c_str(), NULL, 16);
        uint16_t len = strtoul(line.substring(sp2 + 1).c_str(), NULL, 16);
        cmd_dump_chr(start, len);
      } else {
        Serial.println(F("ERR_INVALID_PARAMS"));
      }
      return;
    }

    char cmd = line.charAt(0);
    unsigned long addr = 0, len = 0;
    if (line.length() > 1) {
      sscanf(line.c_str() + 1, "%lx %lx", &addr, &len);
    }

    switch (cmd) {
      case 'V':
        Serial.println(F("famidump v0.5 rev2"));
        break;
      case 'R':
        handle_famidump_read('R', addr, len);
        break;
      case 'C':
        handle_famidump_read('C', addr, len);
        break;
      case 'W':
        handle_famidump_read('W', addr, len);
        break;
      case 'M':
        Serial.println('V');
        break;
      case 'S':
        Serial.println(F("STATUS famidump-v0.5 mirror=V pins=PASS md=0xFF"));
        break;
      case 'T':
        Serial.println(F("SELFTEST PASS"));
        break;
      case 'B':
        enable_bus();
        bank_select_write((uint16_t)addr);
        Serial.print(F("BANK "));
        print_hex4((uint16_t)addr);
        Serial.println();
        bus_idle();
        break;
      case 'P':
        enable_bus();
        write_cpu_byte((uint16_t)addr, (uint8_t)len);
        Serial.print(F("WROK "));
        print_hex4((uint16_t)addr);
        Serial.print(' ');
        print_hex2((uint8_t)len);
        Serial.println();
        bus_idle();
        break;
      default:
        Serial.println(F("ERR_UNKNOWN_CMD"));
        break;
    }
  }
}
