/*
 * Скетч для Arduino MEGA с непрерывной отправкой команд TX-6B
 * 
 * Исправлена структура команды с учетом двух неиспользуемых битов (N)
 * Команды отправляются непрерывно до ввода новой команды
 * 
 * Подключаем к DOUT микросхемы TX2S
 */

// Пин для генерации сигнала (подключаем к DOUT TX2S)
const int TX_PIN = 9;

// Пин для индикации работы
const int LED_PIN = 13;

// Переменные для настройки параметров
int SYMBOL_GAP_US = 5;            // Задержка между символами (в микросекундах)
bool even_parity = true;          // Тип паритета: true = четный, false = нечетный

// Для F1: 75% скважность (1.5 мс HIGH, 0.5 мс LOW)
const int F1_HIGH_US = 1500;   // 1.5 мс HIGH для F1
const int F1_LOW_US = 500;     // 0.5 мс LOW для F1

// Для F0: 25% скважность (0.5 мс HIGH, 1.5 мс LOW)
const int F0_HIGH_US = 500;    // 0.5 мс HIGH для F0
const int F0_LOW_US = 1500;    // 1.5 мс LOW для F0

// Тип команды
enum CommandType {
  CMD_NONE,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_RIGHT,
  CMD_LEFT,
  CMD_TURBO,
  CMD_F1,
  CMD_F2,
  CMD_FWD_TURBO,
  CMD_BCK_TURBO,
  CMD_FWD_RIGHT,
  CMD_FWD_LEFT,
  CMD_BCK_RIGHT,
  CMD_BCK_LEFT
};

// Текущая активная команда
CommandType currentCommand = CMD_NONE;
unsigned long lastCommandTime = 0;         // Время последней отправки команды
unsigned long COMMAND_INTERVAL = 1;       // Интервал между командами в миллисекундах (это видно на осциллографе)

void setup() {
  // Инициализируем пины
  pinMode(TX_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Устанавливаем начальное состояние
  digitalWrite(TX_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  Serial.begin(9600);
  while (!Serial) {
    ; // Ждем подключения Serial
  }
  
  Serial.println("TX-6B Continuous Command Generator Started");
  Serial.println("Commands will be sent continuously until a new command is received:");
  Serial.println("A - Forward");
  Serial.println("B - Backward");
  Serial.println("R - Right");
  Serial.println("G - Left");
  Serial.println("T - Turbo");
  Serial.println("C - F1");
  Serial.println("H - F2");
  Serial.println("M - Forward + Turbo");
  Serial.println("N - Backward + Turbo");
  Serial.println("V - Forward + Right");
  Serial.println("W - Forward + Left");
  Serial.println("X - Backward + Right");
  Serial.println("Y - Backward + Left");
  Serial.println("Z - Stop (No Command)");
  Serial.println("F[value] - set SYMBOL_GAP_US (e.g., F5)");
  Serial.println("I[value] - set COMMAND_INTERVAL in ms (e.g., I100 for 100ms)");
  Serial.println("P - toggle parity");
  Serial.println("I - Info");
  Serial.println("");
  Serial.println("Current values:");
  Serial.println("SYMBOL_GAP_US=" + String(SYMBOL_GAP_US) + "us, COMMAND_INTERVAL=" + String(COMMAND_INTERVAL) + "ms, Parity=" + String(even_parity ? "Even" : "Odd"));
  Serial.println("Current command: Forward");
}

// Функция корректной задержки для значений больше 16383 микросекунд
void accurateDelayMicroseconds(unsigned long us) {
  const unsigned long MAX_DELAY = 16383;
  
  while (us > MAX_DELAY) {
    delayMicroseconds(MAX_DELAY);
    us -= MAX_DELAY;
  }
  
  if (us > 0) {
    delayMicroseconds((unsigned int)us);
  }
}

// Функция генерации F1 символа (500 Гц, 75% скважность)
void sendF1() {
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(F1_HIGH_US);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(F1_LOW_US);
}

// Функция генерации F0 символа (500 Гц, 25% скважность)
void sendF0() {
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(F0_HIGH_US);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(F0_LOW_US);
}

// Функция генерации одного бита данных
void sendBit(bool bit) {
  if (bit) {
    sendF1();  // F1 для логической 1
  } else {
    sendF0();  // F0 для логического 0
  }
  // Задержка между символами
  delayMicroseconds(SYMBOL_GAP_US);
}

// Функция генерации закодированного сигнала с заданными битами
void sendCommand(bool forward, bool backward, bool turbo, bool right, bool left, bool f1, bool f2) {
  // Старт-код: F1 F1 F1 F1 F0
  sendF1(); // F1
  sendF1(); // F1
  sendF1(); // F1
  sendF1(); // F1
  sendF0(); // F0
  
  // Данные: F B T R L F1 N N F2 (согласно даташиту)
  sendBit(forward);  // F (Forward)
  sendBit(backward); // B (Backward)
  sendBit(turbo);    // T (Turbo)
  sendBit(right);    // R (Right)
  sendBit(left);     // L (Left)
  sendBit(f1);       // F1
  sendF0();          // N (неиспользуемый)
  sendF0();          // N (неиспользуемый)
  sendBit(f2);       // F2
  
  // Паритетный код: считаем количество единиц в данных (без учета N битов)
  int ones_count = (forward ? 1 : 0) + (backward ? 1 : 0) + (turbo ? 1 : 0) + 
                   (right ? 1 : 0) + (left ? 1 : 0) + (f1 ? 1 : 0) + (f2 ? 1 : 0);
  
  // Если четный паритет: нечетное число единиц -> добавляем 1, четное -> добавляем 0
  // Если нечетный паритет: нечетное число единиц -> добавляем 0, четное -> добавляем 1
  if ((even_parity && (ones_count % 2 == 1)) || (!even_parity && (ones_count % 2 == 0))) {
    sendF1();  // Добавляем 1 для четности или 0 для нечетности
  } else {
    sendF0();  // Добавляем 0 для четности или 1 для нечетности
  }
  
  // Конечный код (просто F0)
  sendF0();
}

// Функция отправки активной команды
void sendActiveCommand() {
  if (currentCommand != CMD_NONE) {
    switch(currentCommand) {
      case CMD_FORWARD:
        sendCommand(true, false, false, false, false, false, false);
        break;
      case CMD_BACKWARD:
        sendCommand(false, true, false, false, false, false, false);
        break;
      case CMD_RIGHT:
        sendCommand(false, false, false, true, false, false, false);
        break;
      case CMD_LEFT:
        sendCommand(false, false, false, false, true, false, false);
        break;
      case CMD_TURBO:
        sendCommand(false, false, true, false, false, false, false);
        break;
      case CMD_F1:
        sendCommand(false, false, false, false, false, true, false);
        break;
      case CMD_F2:
        sendCommand(false, false, false, false, false, false, true);
        break;
      case CMD_FWD_TURBO:
        sendCommand(true, false, true, false, false, false, false);
        break;
      case CMD_BCK_TURBO:
        sendCommand(false, true, true, false, false, false, false);
        break;
      case CMD_FWD_RIGHT:
        sendCommand(true, false, false, true, false, false, false);
        break;
      case CMD_FWD_LEFT:
        sendCommand(true, false, false, false, true, false, false);
        break;
      case CMD_BCK_RIGHT:
        sendCommand(false, true, false, true, false, false, false);
        break;
      case CMD_BCK_LEFT:
        sendCommand(false, true, false, false, true, false, false);
        break;
      case CMD_NONE:
        // Нет активной команды - ничего не отправляем
        break;
    }
  }
}

// Функция получения названия команды
String getCommandName(CommandType cmd) {
  switch(cmd) {
    case CMD_FORWARD: return "Forward";
    case CMD_BACKWARD: return "Backward";
    case CMD_RIGHT: return "Right";
    case CMD_LEFT: return "Left";
    case CMD_TURBO: return "Turbo";
    case CMD_F1: return "F1";
    case CMD_F2: return "F2";
    case CMD_FWD_TURBO: return "Forward+Turbo";
    case CMD_BCK_TURBO: return "Backward+Turbo";
    case CMD_FWD_RIGHT: return "Forward+Right";
    case CMD_FWD_LEFT: return "Forward+Left";
    case CMD_BCK_RIGHT: return "Backward+Right";
    case CMD_BCK_LEFT: return "Backward+Left";
    case CMD_NONE: return "Stop";
    default: return "Unknown";
  }
}

void loop() {
  // Обрабатываем команды из Serial-порта
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    
    if (command.length() > 0) {
      char cmd = command.charAt(0);
      
      bool commandProcessed = false;
      
      // Обработка числовых параметров для F, L, I команд
      if ((cmd == 'F' || cmd == 'f') && command.length() > 1) {
        int new_value = command.substring(1).toInt();
        if (new_value >= 0 && new_value <= 16383) {
          SYMBOL_GAP_US = new_value;
          Serial.println("SYMBOL_GAP_US set to: " + String(SYMBOL_GAP_US) + " us");
        } else {
          Serial.println("Invalid SYMBOL_GAP_US. Must be 0-16383");
        }
        commandProcessed = true;
      }
      else if ((cmd == 'I' || cmd == 'i') && command.length() > 1) {
        // Обработка команды установки интервала
        unsigned long new_value = command.substring(1).toInt();
        COMMAND_INTERVAL = new_value;
        Serial.println("COMMAND_INTERVAL set to: " + String(COMMAND_INTERVAL) + " ms");
        commandProcessed = true;
      }
      else {
        // Обработка команд без параметров
        switch(cmd) {
          case 'P':
          case 'p':
            even_parity = !even_parity;
            Serial.println("Parity set to: " + String(even_parity ? "Even" : "Odd"));
            commandProcessed = true;
            break;
            
          case 'A':
          case 'a':
            currentCommand = CMD_FORWARD;
            Serial.println("Command changed to: Forward");
            commandProcessed = true;
            break;
            
          case 'B':
          case 'b':
            currentCommand = CMD_BACKWARD;
            Serial.println("Command changed to: Backward");
            commandProcessed = true;
            break;
            
          case 'R':
          case 'r':
            currentCommand = CMD_RIGHT;
            Serial.println("Command changed to: Right");
            commandProcessed = true;
            break;
            
          case 'G':
          case 'g':
            currentCommand = CMD_LEFT;
            Serial.println("Command changed to: Left");
            commandProcessed = true;
            break;
            
          case 'T':
          case 't':
            currentCommand = CMD_TURBO;
            Serial.println("Command changed to: Turbo");
            commandProcessed = true;
            break;
            
          case 'C':
          case 'c':
            currentCommand = CMD_F1;
            Serial.println("Command changed to: F1");
            commandProcessed = true;
            break;
            
          case 'H':
          case 'h':
            currentCommand = CMD_F2;
            Serial.println("Command changed to: F2");
            commandProcessed = true;
            break;
            
          case 'M':
          case 'm':
            currentCommand = CMD_FWD_TURBO;
            Serial.println("Command changed to: Forward+Turbo");
            commandProcessed = true;
            break;
            
          case 'N':
          case 'n':
            currentCommand = CMD_BCK_TURBO;
            Serial.println("Command changed to: Backward+Turbo");
            commandProcessed = true;
            break;
            
          case 'V':
          case 'v':
            currentCommand = CMD_FWD_RIGHT;
            Serial.println("Command changed to: Forward+Right");
            commandProcessed = true;
            break;
            
          case 'W':
          case 'w':
            currentCommand = CMD_FWD_LEFT;
            Serial.println("Command changed to: Forward+Left");
            commandProcessed = true;
            break;
            
          case 'X':
          case 'x':
            currentCommand = CMD_BCK_RIGHT;
            Serial.println("Command changed to: Backward+Right");
            commandProcessed = true;
            break;
            
          case 'Y':
          case 'y':
            currentCommand = CMD_BCK_LEFT;
            Serial.println("Command changed to: Backward+Left");
            commandProcessed = true;
            break;
            
          case 'Z':
          case 'z':
            currentCommand = CMD_NONE;
            Serial.println("Command changed to: Stop");
            commandProcessed = true;
            break;
            
          case 'i': // Отдельно обрабатываем команду 'i' для информации
            Serial.println("Current values:");
            Serial.println("SYMBOL_GAP_US=" + String(SYMBOL_GAP_US) + "us, COMMAND_INTERVAL=" + String(COMMAND_INTERVAL) + "ms, Parity=" + String(even_parity ? "Even" : "Odd"));
            Serial.println("Current command: " + getCommandName(currentCommand));
            commandProcessed = true;
            break;
        }
      }
      
      // Выводим текущие параметры после успешной обработки команды
      if (commandProcessed) {
        if (cmd != 'I' || (command.length() == 1 && (cmd == 'I' || cmd == 'i'))) {  // Не выводим при команде I с параметром
          if (cmd != 'I' && cmd != 'i') {
            Serial.println("Current values: SYMBOL_GAP_US=" + String(SYMBOL_GAP_US) + "us, COMMAND_INTERVAL=" + String(COMMAND_INTERVAL) + "ms, Parity=" + String(even_parity ? "Even" : "Odd"));
            Serial.println("Current command: " + getCommandName(currentCommand));
          }
        }
      } else {
        Serial.println("Unknown command: " + String(cmd));
        Serial.println("Use A, B, R, G, T, C, H, M, N, V, W, X, Y, Z, F[value], L[value], I[value], P, I");
      }
      
      // Очищаем буфер
      while(Serial.available()) {
        Serial.read();
      }
    }
  }
  
  // Отправляем активную команду с интервалом
  if ((millis() - lastCommandTime) >= COMMAND_INTERVAL) {
    sendActiveCommand();
    
    // Мигаем светодиодом при отправке команды
    digitalWrite(LED_PIN, HIGH);
    delay(5); // короткий импульс
    digitalWrite(LED_PIN, LOW);
    
    lastCommandTime = millis();
  }
  
  // Небольшая задержка для стабильности
  delay(1);
}