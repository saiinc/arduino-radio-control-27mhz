/*
 * Скетч для Arduino MEGA с непрерывной отправкой команд TX-6B
 * 
 * Команды отправляются непрерывно до ввода новой команды
 * Поддержка управления с физических кнопок на цифровых пинах
 * Поддержка комбинаций кнопок
 * 
 * Подключаем к DOUT микросхемы TX2S
 */

// Пин для генерации сигнала (подключаем к DOUT TX2S)
const int TX_PIN = 9;

// Пин для индикации работы
const int LED_PIN = 13;

// Пины для подключения кнопок (внешние кнопки, подтянутые к VCC через резистор)
const int BTN_FORWARD = 2;    // Кнопка "вперёд"
const int BTN_BACKWARD = 3;   // Кнопка "назад"
const int BTN_RIGHT = 4;      // Кнопка "вправо"
const int BTN_LEFT = 5;       // Кнопка "влево"
const int BTN_F1 = 6;         // Кнопка "F1"
const int BTN_F2 = 7;         // Кнопка "F2"

// Массив пинов кнопок для удобства обработки
const int BUTTON_PINS[] = {BTN_FORWARD, BTN_BACKWARD, BTN_RIGHT, BTN_LEFT, BTN_F1, BTN_F2};
const int NUM_BUTTONS = 6;

// Состояния кнопок
bool buttonStates[] = {false, false, false, false, false, false};
bool prevButtonStates[] = {false, false, false, false, false, false};

// Переменные для настройки параметров
int SYMBOL_GAP_US = 5;            // Задержка между символами (в микросекундах)
unsigned long COMMAND_INTERVAL = 1; // Интервал между командами в миллисекундах (0 для минимального интервала)
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
  CMD_F1,
  CMD_F2,
  CMD_FWD_RIGHT,
  CMD_FWD_LEFT,
  CMD_BCK_RIGHT,
  CMD_BCK_LEFT
};

// Текущая активная команда
CommandType currentCommand = CMD_NONE;
unsigned long lastCommandTime = 0;         // Время последней отправки команды

void setup() {
  // Инициализируем пины для передачи сигнала
  pinMode(TX_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Устанавливаем начальное состояние
  digitalWrite(TX_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  // Инициализируем пины кнопок как входы с подтяжкой
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  
  Serial.begin(9600);
  while (!Serial) {
    ; // Ждем подключения Serial
  }
  
  Serial.println("TX-6B Button-Controlled Command Generator with Combinations Started");
  Serial.println("Physical buttons connected to pins:");
  Serial.println("Pin 2: Forward");
  Serial.println("Pin 3: Backward");
  Serial.println("Pin 4: Right");
  Serial.println("Pin 5: Left");
  Serial.println("Pin 6: F1");
  Serial.println("Pin 7: F2");
  Serial.println("Combinations supported: Forward+Right, Forward+Left, Backward+Right, Backward+Left");
  Serial.println("");
  Serial.println("Serial commands:");
  Serial.println("A - Forward");
  Serial.println("B - Backward");
  Serial.println("R - Right");
  Serial.println("G - Left");
  Serial.println("C - F1");
  Serial.println("H - F2");
  Serial.println("V - Forward+Right");
  Serial.println("W - Forward+Left");
  Serial.println("X - Backward+Right");
  Serial.println("Y - Backward+Left");
  Serial.println("Z - Stop (No Command)");
  Serial.println("F[value] - set SYMBOL_GAP_US (e.g., F5)");
  Serial.println("I[value] - set COMMAND_INTERVAL in ms (e.g., I100 for 100ms)");
  Serial.println("P - toggle parity");
  Serial.println("J - Info");
  Serial.println("");
  Serial.println("Current values:");
  Serial.println("SYMBOL_GAP_US=" + String(SYMBOL_GAP_US) + "us, COMMAND_INTERVAL=" + String(COMMAND_INTERVAL) + "ms, Parity=" + String(even_parity ? "Even" : "Odd"));
  Serial.println("Current command: Stop");
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
      case CMD_F1:
        sendCommand(false, false, false, false, false, true, false);
        break;
      case CMD_F2:
        sendCommand(false, false, false, false, false, false, true);
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
    case CMD_F1: return "F1";
    case CMD_F2: return "F2";
    case CMD_FWD_RIGHT: return "Forward+Right";
    case CMD_FWD_LEFT: return "Forward+Left";
    case CMD_BCK_RIGHT: return "Backward+Right";
    case CMD_BCK_LEFT: return "Backward+Left";
    case CMD_NONE: return "Stop";
    default: return "Unknown";
  }
}

// Функция проверки комбинаций кнопок и установки соответствующей команды
CommandType checkButtonCombinations() {
  bool fwd = buttonStates[0];  // Forward
  bool bck = buttonStates[1];  // Backward
  bool rgt = buttonStates[2];  // Right
  bool lft = buttonStates[3];  // Left
  bool f1 = buttonStates[4];   // F1
  bool f2 = buttonStates[5];   // F2

  // Проверяем приоритетные комбинации сначала
  if (fwd && rgt && !bck && !lft) {
    return CMD_FWD_RIGHT;
  } else if (fwd && lft && !bck && !rgt) {
    return CMD_FWD_LEFT;
  } else if (bck && rgt && !fwd && !lft) {
    return CMD_BCK_RIGHT;
  } else if (bck && lft && !fwd && !rgt) {
    return CMD_BCK_LEFT;
  } 
  // Затем одиночные команды
  else if (fwd && !bck && !rgt && !lft) {
    return CMD_FORWARD;
  } else if (bck && !fwd && !rgt && !lft) {
    return CMD_BACKWARD;
  } else if (rgt && !fwd && !bck && !lft) {
    return CMD_RIGHT;
  } else if (lft && !fwd && !bck && !rgt) {
    return CMD_LEFT;
  } else if (f1 && !f2 && !fwd && !bck && !rgt && !lft) {
    return CMD_F1;
  } else if (f2 && !f1 && !fwd && !bck && !rgt && !lft) {
    return CMD_F2;
  }
  
  // Если ни одна комбинация не подошла, проверяем на одиночные нажатия
  // Важно: приоритет у комбинаций, а не у отдельных кнопок
  if (fwd) return CMD_FORWARD;
  if (bck) return CMD_BACKWARD;
  if (rgt) return CMD_RIGHT;
  if (lft) return CMD_LEFT;
  if (f1) return CMD_F1;
  if (f2) return CMD_F2;
  
  // Если ничего не нажато
  return CMD_NONE;
}

// Функция обновления команды на основе состояния кнопок
void updateCommandFromButtons() {
  CommandType newCommand = checkButtonCombinations();
  
  if (newCommand != currentCommand) {
    currentCommand = newCommand;
    if (currentCommand != CMD_NONE) {
      Serial.println("Button combination changed: " + getCommandName(currentCommand));
    } else {
      Serial.println("All buttons released: Stop");
    }
    Serial.println("Current command: " + getCommandName(currentCommand));
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
            
          case 'J':
          case 'j': // Используем 'J' вместо 'I', чтобы не путать с командой установки интервала
            Serial.println("Current values:");
            Serial.println("SYMBOL_GAP_US=" + String(SYMBOL_GAP_US) + "us, COMMAND_INTERVAL=" + String(COMMAND_INTERVAL) + "ms, Parity=" + String(even_parity ? "Even" : "Odd"));
            Serial.println("Current command: " + getCommandName(currentCommand));
            commandProcessed = true;
            break;
        }
      }
      
      // Выводим текущие параметры после успешной обработки команды
      if (commandProcessed) {
        if (cmd != 'J' && cmd != 'j') {
          Serial.println("Current values: SYMBOL_GAP_US=" + String(SYMBOL_GAP_US) + "us, COMMAND_INTERVAL=" + String(COMMAND_INTERVAL) + "ms, Parity=" + String(even_parity ? "Even" : "Odd"));
          Serial.println("Current command: " + getCommandName(currentCommand));
        }
      } else {
        Serial.println("Unknown command: " + String(cmd));
        Serial.println("Use A, B, R, G, C, H, V, W, X, Y, Z, F[value], L[value], I[value], P, J");
      }
      
      // Очищаем буфер
      while(Serial.available()) {
        Serial.read();
      }
    }
  }
  
  // Обновляем состояние кнопок
  for (int i = 0; i < NUM_BUTTONS; i++) {
    prevButtonStates[i] = buttonStates[i];
    buttonStates[i] = !digitalRead(BUTTON_PINS[i]); // LOW при нажатии, HIGH при отпускании
  }
  
  // Обновляем команду на основе комбинации кнопок
  updateCommandFromButtons();
  
  // Отправляем активную команду с учетом интервала
  bool shouldSend = (COMMAND_INTERVAL == 0) || ((millis() - lastCommandTime) >= COMMAND_INTERVAL);
  
  if (shouldSend && currentCommand != CMD_NONE) {
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