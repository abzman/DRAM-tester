//Tweaked by Evan Allen October 27th 2020
//added 4116 support, extra input for device selection, and LEDs are now active high

#define DI 15   // PC1
#define DO 8    // PB0
#define CAS 9   // PB1
#define RAS 17  // PC3
#define WE 16   // PC2

#define XA0 18  // PC4
#define XA1 2   // PD2
#define XA2 19  // PC5
#define XA3 6   // PD6
#define XA4 5   // PD5
#define XA5 4   // PD4
#define XA6 7   // PD7
#define XA7 3   // PD3
#define XA8 14  // PC0

#define R_LED 13   // PB5 active high
#define G_LED 12   // PB4 active high
#define Mode_0 11  // PB3 jumper pulls down
#define Mode_1 10  // PB2 jumper pulls down

#define RXD 0  // PD0
#define TXD 1  // PD1

#define BUS_SIZE 9

volatile int bus_size;

const unsigned int a_bus[BUS_SIZE] = {
  XA0, XA1, XA2, XA3, XA4, XA5, XA6, XA7, XA8
};

void setBus(unsigned int a) {
  int i;
  for (i = 0; i < BUS_SIZE; i++) {
    digitalWrite(a_bus[i], a & 1);
    a /= 2;
  }
}

void writeAddress(unsigned int r, unsigned int c, int v) {
  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* rw */
  digitalWrite(WE, LOW);

  /* val */
  digitalWrite(DI, (v & 1) ? HIGH : LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  digitalWrite(WE, HIGH);
  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);
}

int readAddress(unsigned int r, unsigned int c) {
  int ret = 0;

  /* row */
  setBus(r);
  digitalWrite(RAS, LOW);

  /* col */
  setBus(c);
  digitalWrite(CAS, LOW);

  /* get current value */
  ret = digitalRead(DO);

  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);

  return ret;
}

void error(int r, int c) {
  unsigned long a = ((unsigned long)c << bus_size) + r;
  digitalWrite(R_LED, HIGH);
  green(LOW);
  interrupts();
  Serial.print(" FAILED $");
  Serial.println(a, HEX);
  Serial.flush();
  while (1)
    ;
}

void ok(void) {
  red(LOW);
  green(HIGH);
  interrupts();
  Serial.println(" OK!");
  Serial.flush();
  while (1)
    ;
}

void green(int v) {
  digitalWrite(G_LED, v);
}

void red(int v) {
  digitalWrite(R_LED, v);
}
void fill(int v) {
  int r, c, g = 0;
  v %= 1;
  for (c = 0; c < (1 << bus_size); c++) {
    green(g ? HIGH : LOW);
    for (r = 0; r < (1 << bus_size); r++) {
      writeAddress(r, c, v);
      if (v != readAddress(r, c))
        error(r, c);
    }
    g ^= 1;
  }
}

void fillx(int v) {
  int r, c, g = 0;
  v %= 1;
  for (c = 0; c < (1 << bus_size); c++) {
    green(g ? HIGH : LOW);
    for (r = 0; r < (1 << bus_size); r++) {
      writeAddress(r, c, v);
      if (v != readAddress(r, c))
        error(r, c);
      v ^= 1;
    }
    g ^= 1;
  }
}

void setup() {
  int i;

  Serial.begin(115200);
  while (!Serial)
    ; /* wait */

  Serial.println();
  Serial.print("DRAM TESTER ");

  for (i = 0; i < BUS_SIZE; i++)
    pinMode(a_bus[i], OUTPUT);

  pinMode(CAS, OUTPUT);
  pinMode(RAS, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(DI, OUTPUT);

  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);

  pinMode(Mode_0, INPUT_PULLUP);
  pinMode(Mode_1, INPUT_PULLUP);
  pinMode(DO, INPUT);

  digitalWrite(WE, HIGH);
  digitalWrite(RAS, HIGH);
  digitalWrite(CAS, HIGH);

  green(LOW);
  red(LOW);

  if ((digitalRead(Mode_0) == 1) && (digitalRead(Mode_1) == 1)) {
    /* neither jumper set - 41256 */
    bus_size = BUS_SIZE;
    Serial.print("256Kx1 ");
  } else if ((digitalRead(Mode_0) == 0) && (digitalRead(Mode_1) == 1)) {
    /* one jumper set - 4164 */
    bus_size = BUS_SIZE - 1;
    Serial.print("64Kx1 ");
  } else if ((digitalRead(Mode_0) == 1) && (digitalRead(Mode_1) == 0)) {
    /* other jumper set - 4164 */
    bus_size = BUS_SIZE - 1;
    Serial.print("64Kx1 ");
  } else if ((digitalRead(Mode_0) == 0) && (digitalRead(Mode_1) == 0)) {
    /* both jumpers set - 4116 */
    bus_size = BUS_SIZE - 2;
    Serial.print("16Kx1 ");
  }
  Serial.flush();

  green(HIGH);
  red(HIGH);

  noInterrupts();
  for (i = 0; i < (1 << BUS_SIZE); i++) {
    digitalWrite(RAS, LOW);
    digitalWrite(RAS, HIGH);
  }
  green(LOW);
  red(LOW);
}

void loop() {
  interrupts();
  Serial.print(".");
  Serial.flush();
  noInterrupts();
  fillx(0);
  interrupts();
  Serial.print(".");
  Serial.flush();
  noInterrupts();
  fillx(1);
  interrupts();
  Serial.print(".");
  Serial.flush();
  noInterrupts();
  fill(0);
  interrupts();
  Serial.print(".");
  Serial.flush();
  noInterrupts();
  fill(1);
  ok();
}
