// 27MHz continuous frame sender
// Pinning: TX_PIN -> D9 (fast port toggle)
// Serial commands:
//  F - toggle Forward
//  B - toggle Reverse (Back)
//  L - toggle Left
//  R - toggle Right
//  S - Stop (all off)


#define TX_PIN 9       // пин на выход DOUT

const unsigned int LONG_HIGH = 1300;
const unsigned int LONG_LOW  = 500;
const unsigned int SHORT_HIGH = 500;
const unsigned int SHORT_LOW  = 500;

bool cmdF = false, cmdB = false, cmdL = false, cmdR = false;

int resolveShortCount() {
  if (cmdF && cmdL) return 28;
  if (cmdF && cmdR) return 34;
  if (cmdB && cmdL) return 52;
  if (cmdB && cmdR) return 46;

  if (cmdF && !cmdB && !cmdL && !cmdR) return 10;
  if (cmdB && !cmdF && !cmdL && !cmdR) return 40;
  if (cmdR && !cmdF && !cmdB && !cmdL) return 64;
  if (cmdL && !cmdF && !cmdB && !cmdR) return 58;

  return 0;
}

void sendLongPulse() {
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(LONG_HIGH);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(LONG_LOW);
}

void sendShortPulse() {
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(SHORT_HIGH);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(SHORT_LOW);
}

void sendFrame(int shortCount) {

  for (int i = 0; i < 4; ++i) sendLongPulse(); // 4 длинных импульса
  for (int i = 0; i < shortCount; ++i) sendShortPulse(); // N коротких

}

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);

  Serial.begin(115200);
  Serial.println("Hold keys: F B L R, S=stop all");
}

void loop() {
  // читаем клавиши и удержание
while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'F': case 'f': cmdF = !cmdF; break; // переключаем состояние
      case 'B': case 'b': cmdB = !cmdB; break;
      case 'L': case 'l': cmdL = !cmdL; break;
      case 'R': case 'r': cmdR = !cmdR; break;
      case 'S': case 's': cmdF = cmdB = cmdL = cmdR = false; break; // стоп
    }
}

  int shortCount = resolveShortCount();
  if (shortCount > 0) {
    sendFrame(shortCount); // кадр отправляется непрерывно
  } else {
    delay(10); // чтобы не грузить процессор
  }

}
