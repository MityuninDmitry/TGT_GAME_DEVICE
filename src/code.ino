/* 
1. Режим. Респ каждые *** секунд (время пронрамируется от 30 до 300 мекунд)
2. Режим. Респ после *** секунд нахождения в мертвяке ( время от 15 до 60 секунд). Подходишь в мертвяк, нажимаешь кнопку и начинается обратный отсчёт. Если в это время нажать еще раз кнопку, добавится 15 секунд. То есть если первый запустил таймер и через время еще один мёртвый нажимает, добавляется +15секунд. Но по истечению времени оба могут покинуть мертвяк
3. Режим. Выход из мертвяка по набору определённого количества человек. Что бы сделать возможность волн в некоторых режимах
4. Режим. Количество жизней на команду. К примеру от 5 до 30. При каждом нажатии число уменьшается. Проигрывает команда у которой закончились жизни и нет живых игроков
5. Режим. Скрытый счетчик. По кнопкам копим кол-во жизней. В конце игры показываем сколько скопилось. 
*/

/*
ПОДКЛЮЧЕННЫЕ МОДУЛИ: 
1. Кнопка зеленая 6 пин и GND 
2. Кнопка красная 7 пин и GND 
3. OLED 128 64 (нет) 
GND - в минус 
VСС - в плюс
SCL - А5
SDA - А4
4. TFT 240 240 на базе ST7789 
VCC - 3.3v
GND - GND 
SCK - 1kOM - D13
SDA - 1kOM - D11
RES - 1kOM - D9
DC - 1kOM - D8
BL - 3.3v
Схема тут: https://docstech.ru/arduino-st7789/

- ARDUINO модуль часового LED дисплея
(GND подключен в GND, VCC - в 5v, DIO - D4, CLK - D5)

- колонка 
Ардуино 3 пин -> резистор 1кОМ -> средняя ножка транзистора 
левая ножка транзистора -> красный провод колонки ->  колонка -> черный провод в 5v ардуино 
правая ножка транзистора -> в GND ардуино 
*/


#include <EncButton.h>
#include <TM1637Display.h>
int CLK_PIN = 5; // часы 
int DIO_PIN = 4; // часы 
TM1637Display oledTimerDisplay(CLK_PIN, DIO_PIN);
// TFT DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "FontsRus/TimesNRCyr10.h"
#define TFT_CS     -1
#define TFT_RST    9
#define TFT_DC     8
int BUZZER_PIN = 2;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// подключаем кнопку на D3
Button redBtn(7);
Button greenBtn(6);

// возможные варианты режимов работы 
enum WorkMode: int {
  MODE_INSTRUCTION,      // Режим 6: инструкция
  MODE_TIMER,           // Режим 1: респ каждые *** секунд (время пронрамируется от 30 до 300 мекунд)
  MODE_DEADZONE_TIMER,  // Режим 2: респ после *** секунд нахождения в мертвяке ( время от 15 до 60 секунд). Подходишь в мертвяк, нажимаешь кнопку и начинается обратный отсчёт. Если в это время нажать еще раз кнопку, добавится 15 секунд. То есть если первый запустил таймер и через время еще один мёртвый нажимает, добавляется +15секунд. Но по истечению времени оба могут покинуть мертвяк
  MODE_PLAYER_COUNT,    // Режим 3: выход из мертвяка по набору определённого количества человек. Что бы сделать возможность волн в некоторых режимах
  MODE_LIFE_COUNT,      // Режим 4: кол-во жизней на команду. К примеру от 5 до 30. При каждом нажатии число уменьшается. Проигрывает команда у которой закончились жизни и нет живых игроков
  MODE_COUNTER          // Режим 5: скрытый счетчик  
};


// возможные этапы игры
enum GameStage: int {
  STAGE_SELECT_MODE, // выбор режима игры 
  STAGE_PREGAME, // настройка игры 
  STAGE_GAME, // игра 
  STAGE_ENDGAME // конец игры 
};
GameStage currentStage = STAGE_SELECT_MODE; // первый режим "выбор режима"

struct WorkModeMenuItem { // пункт меню при выборе режима игры (для экрана)
  WorkMode mode; // режим игры 
  bool selected; // курсор установлен на нем или нет
  const char* name;  //String name; // наименование режима на экране 
};
// Глобальные переменные
WorkModeMenuItem availableModes[6]; // массив режимов
int currentModeIndex = 1; // текущий выбранный режим игры 
WorkMode currentMode = MODE_TIMER; // первично задаем первый режим по дефолту 

void initModes() { // ф-ция инициализации режимов 
  availableModes[0].mode = MODE_INSTRUCTION;
  availableModes[0].selected = false;
  availableModes[0].name = "Инструкция";  

  availableModes[1].mode = MODE_TIMER;
  availableModes[1].selected = true;
  availableModes[1].name = "Респ по таймеру";
  
  availableModes[2].mode = MODE_DEADZONE_TIMER;
  availableModes[2].selected = false;
  availableModes[2].name = "Мертвяк";
  
  availableModes[3].mode = MODE_PLAYER_COUNT;
  availableModes[3].selected = false;
  availableModes[3].name = "Лимит игроков";
  
  availableModes[4].mode = MODE_LIFE_COUNT;
  availableModes[4].selected = false;
  availableModes[4].name = "Лимит жизней";

  availableModes[5].mode = MODE_COUNTER;
  availableModes[5].selected = false;
  availableModes[5].name = "Скрытый счетчик";  
}
const int BUZZER_MAX = LOW;
const int BUZZER_MIN = HIGH;

void piskOn() {
  digitalWrite(BUZZER_PIN, BUZZER_MAX);
}
void piskOff() {
  digitalWrite(BUZZER_PIN, BUZZER_MIN);
}
unsigned long piskTimer = 0;
unsigned long piskDuration = 0;  // добавляем переменную для хранения длительности
bool piskActive = false;

void piskOnFor(unsigned long duration) {
  piskOn();
  piskTimer = millis();
  piskDuration = duration;  // сохраняем длительность
  piskActive = true;
}

void updatePisk() {
  if (piskActive && (millis() - piskTimer >= piskDuration)) {
    piskOff();
    piskActive = false;
  }
}
// настройки игры 
struct GameSettings {
  int value; // выбранное значение (таймер или кол-вол жизней)
  int minValue; // левое ограничение 
  int maxValue; // правое ограничение 
  int step; // шаг изменения 
  String description; // описание настройки 
  String unit; // юнит (человек или жизней)
  int actualValue; // актуальное значение в текущий момент игры 
};
GameSettings currentSettings; // объявляем 

// Глобальные переменные для таймера
unsigned long timerStartTime = 0;
unsigned int timerDuration = 0;    // Длительность в секундах
bool timerActive = false;
int lastDisplayedSecond = -1;

void initGameSettings() { // инициализация настроек игры в зависимости от выбранного режима
  switch(currentMode) {
      case MODE_TIMER: 
        currentSettings.minValue = 15; 
        currentSettings.maxValue = 300;  
        currentSettings.step = 15;
        currentSettings.description = F("Циклическое возрождение по времени");
        currentSettings.unit = F("сек");
        currentSettings.value = currentSettings.minValue;
        currentSettings.actualValue = currentSettings.value;
        break;
      case MODE_DEADZONE_TIMER: 
        currentSettings.minValue = 15;
        currentSettings.maxValue = 60;
        currentSettings.step = 15;
        currentSettings.description = F("Время в мертвяке");
        currentSettings.unit = F("сек");
        currentSettings.value = currentSettings.minValue;
        currentSettings.actualValue = 0;
        break;
      case MODE_PLAYER_COUNT: 
        currentSettings.minValue = 2;
        currentSettings.maxValue = 99;
        currentSettings.step = 1;
        currentSettings.description = F("Лимит игроков");
        currentSettings.unit = F("чел.");
        currentSettings.value = currentSettings.minValue;
        currentSettings.actualValue = 0;
        break; 
      case MODE_LIFE_COUNT: 
        currentSettings.minValue = 5;  
        currentSettings.maxValue = 30; 
        currentSettings.step = 1;
        currentSettings.description = F("Жизней на команду");
        currentSettings.unit = F("жиз.");
        currentSettings.value = currentSettings.minValue;
        currentSettings.actualValue = currentSettings.value;
        break;
      case MODE_COUNTER: 
        currentSettings.minValue = 0;  
        currentSettings.maxValue = 999; 
        currentSettings.step = 1;
        currentSettings.description = F("Скрытый счетчик");
        currentSettings.unit = F("ед.");
        currentSettings.value = currentSettings.minValue;
        currentSettings.actualValue = currentSettings.value;
        break;
      case MODE_INSTRUCTION: 
        currentSettings.minValue = 0;  
        currentSettings.maxValue = 999; 
        currentSettings.step = 0;
        currentSettings.description = F("НЕТ ВЫБОРА");
        currentSettings.unit = F("ед.");
        currentSettings.value = currentSettings.minValue;
        currentSettings.actualValue = currentSettings.value;
        break;
    }
};

void actualizeCurrentSettingsActualValue() {
  switch(currentMode) {
    case MODE_TIMER: 
        currentSettings.actualValue = currentSettings.value;
        break;
      case MODE_DEADZONE_TIMER:     
        currentSettings.actualValue = 0;
        break;
      case MODE_PLAYER_COUNT:       
        currentSettings.actualValue = 0;
        break; 
      case MODE_LIFE_COUNT:         
        currentSettings.actualValue = currentSettings.value;
        break;
      case MODE_COUNTER:         
        currentSettings.actualValue = currentSettings.value;
        break;
    }
};

void setupTFT() {
  tft.init(240, 240, SPI_MODE3); 
  tft.setRotation(2); 
  tft.setFont(&TimesNRCyr10pt8b);   
}

void testTFT() {
  tft.fillScreen(ST77XX_BLACK);
  for(int i = 0; i < 15; i++) {
    tft.setCursor(0, 15 + i * 20);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(0);
    tft.print(String(i));
    tft.println(" линия " + String(10 + i * 20));
  }
}

void setupOledTimerDisplay() {
  // настройка дисплея 
  oledTimerDisplay.setBrightness(7);
  oledTimerDisplay.clear();
  showMinutesSeconds(9999);
}

void setup() {
  piskOff();
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
  
  initModes();
  

  setupTFT();
  //testTFT();
  displayGreeting();
  delay(1000);
  clearDisplay();
  setupOledTimerDisplay();
  piskOnFor(300);
}



void loop() { // работа ардуинки 
  // обновляем состояние кнопки 
  redBtn.tick();
  greenBtn.tick();
  updatePisk(); 
  switch(currentStage) { // в зависимости от стадии игры, своя логика
    case STAGE_SELECT_MODE: // выбор режима
      selectMode(); 
      if (isUserOK() || isUserCancel()) {
        currentStage = STAGE_PREGAME; // переход на этап настройки игры
        initGameSettings(); // инициируем настройки игры 
        showMinutesSeconds(9999);        
        // очищаем экран (подготовка для новых данных)
      } 
      break;
    case STAGE_PREGAME: // задаем настройки игры 
      setGameSettings();
      if (isUserOK() ) { // подтверждение начала игры     
        actualizeCurrentSettingsActualValue();
        currentStage = STAGE_GAME;
        showMinutesSeconds(9999);
      } 
      if (isUserCancel()) { // предыдущий этап               
        currentStage = STAGE_SELECT_MODE;
        showMinutesSeconds(9999);
      }
      break;
    case STAGE_GAME: // игра 
      game();
      if (isUserOK() || isUserCancel()) {
        currentStage = STAGE_ENDGAME; 
        stopTimer();
      }
      break;
    case STAGE_ENDGAME: // конец игры 
      displayTheEnd(); 
      showMinutesSeconds(currentSettings.actualValue);     
      if (isUserOK() || isUserCancel()) {
        initGameSettings();
        currentStage = STAGE_SELECT_MODE;
        showMinutesSeconds(9999);
      }    
      break;
  }
}


/*
=======================
ФУНКЦИИ ВЫВОДА НА ЭКРАН
*/

void clearDisplay() { // очищаем экран 
  tft.fillScreen(ST77XX_BLACK);
}


void updateDisplayWithArray(String lines[11]) {
  static String lastLines[11];
  static unsigned long lastUpdateTime = 0;
  const unsigned long MIN_UPDATE_INTERVAL = 50; // 20 FPS максимум
  
  // Ограничиваем частоту обновлений
  unsigned long now = millis();
  if (now - lastUpdateTime < MIN_UPDATE_INTERVAL) {
    return; // Слишком рано для обновления
  }
  
  tft.setTextSize(0);
  
  
  bool needUpdate = false;

  // Быстрая проверка: нужно ли вообще обновлять?
  for (int i = 0; i < 11; i++) {
    if (lines[i] != lastLines[i]) {
      needUpdate = true;
      break;
    }
  }
  
  if (!needUpdate) return;
  
  
  // Обновляем только измененные строки
  for (int i = 0; i < 11; i++) {
    if (lines[i] != lastLines[i]) {
      int y = 20 + i * 20;      
      
      //tft.drawRect(0, y - 14, 240, 20, ST77XX_YELLOW);  // отладка
      // Быстрая очистка строки
      tft.fillRect(0, y - 14, 240, 20, ST77XX_BLACK);
      
      // Вывод
      tft.setCursor(10, y);
      if (i == 0) {
        tft.setTextColor(ST77XX_BLUE);
      } else if (i == 9) {
        tft.setTextColor(ST77XX_GREEN);
      } else if (i == 10) {
        tft.setTextColor(ST77XX_RED);
      } else {
        tft.setTextColor(ST77XX_BLUE);
      }
      
      tft.print(lines[i]);
      
      
      lastLines[i] = lines[i];
    }
  }
  
  lastUpdateTime = now;
}

String version = "1.2.2";
// 1 0 0 - 4 режима  
// 1 1 0 - добавил 5 режим (скрытый счетчик) 
// 1 2 0 - добавил зумер
// 1 2 1 - поменял инструкции 
// 1 2 2 - убрал код динамика + добавил инструкцию
void displayGreeting() {
  clearDisplay();
  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(40, 100);
  tft.setTextSize(4);
  tft.print("TGT");
  tft.setCursor(0, 15);
  tft.setTextSize(0);
  tft.print(version);
}

void displayModeMenu() { // отображаем меню 
  int modesCount = availableModesLenght();
 
  String menuItems[11] = {
    getStageName(currentStage), //F("=== Выбор режима ===")
    F(""),
    F(""),
    F(""),
    F(""),
    F(""),
    F(""),
    F(""),
    F(""),
    F("(.) (.): навигация"),
    F("(x) (x): подтвердить")
  };

  for(int i = 0; i < modesCount; i++) {
    
    if(availableModes[i].selected) {
      menuItems[i + 2] = F(">   ");
    } else {
      menuItems[i + 2] = F("    "); 
    }
    menuItems[i + 2] += String(i + 1) + ". " + availableModes[i].name;
  }

  updateDisplayWithArray(menuItems);
  
}

void displayGameSetting() { // то, что видим на экране во время настройки игры  
  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),
    F(""),
    String("Диапазон: ") + currentSettings.minValue + "-" + currentSettings.maxValue + " " + currentSettings.unit, // возможный диапазон значений
    F(""),
    "Выбор: " + String(currentSettings.value) + " " + currentSettings.unit, // выбранное значение 
    F(""),
    F(""),
    F("(.) (.): сменить выбор"),
    F("(x) ( ): к режимам"),
    F("( ) (x): подтвердить")
  };
  updateDisplayWithArray(displayItems);
}
void displayGameDeadZoneTimer() {
  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),    
    F(""),
    F(""),
    isTimerActive() ? "Запущен   " : "Остановлен   ",
    F(""),
    String(getRemainingTime()) + "   ",
    F(""),
    F(""),
    F("(.) (.): добавить время"),
    F("(x) (x): стоп игра")
  };
  updateDisplayWithArray(displayItems);
}
void displayGamePlayerCount() {
  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),
    F(""),    
    "Сейчас в респе: " + String(currentSettings.actualValue) + "/" + String(currentSettings.value) + " " + currentSettings.unit,
    F(""),
    F(""),
    getGamePlayerCountStatus(),
    String(getRemainingTime()),
    F(""),
    F("(.) (.): добавить игрока"),
    F("(x) (x): стоп игра")
  };
  updateDisplayWithArray(displayItems);
}

void displayGameLifeCount() {
  String statusGame = "";
  if (currentSettings.actualValue == 0) {
    statusGame = F("стоп");
  } else {
    statusGame = F("играем");
  }

  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),    
    F(""),
    F("Жизней осталось:"),
    String(currentSettings.actualValue) + " из " + String(currentSettings.value),
    F(""),
    "Статус: " + statusGame,        
    F(""),
    F(""),    
    F("(.) (.): потерять жизнь"),
    F("(x) (x): стоп игра")
  };
  updateDisplayWithArray(displayItems);
}

void displayGameCounter() {
  
  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),    
    F(""),
    F("Играем"),
    F(""),
    F(""),
    F(""),        
    F(""),
    F(""),    
    F("(.) (.): отметиться"),
    F("(x) (x): стоп игра")
  };
  updateDisplayWithArray(displayItems);
}
void displayGameInstruction() {
  String displayItems[11] = {
    getModeName(currentMode),
    F("Кнопки левая и правая"),    
    F("() () - их обозначение"),
    F(""),
    F(". - это короткое нажатие"),
    F("x - это длинное нажатие"),
    F(""),        
    F(""),
    F(""),
    F(""),
    F("(x) (x): выход")
  };
  updateDisplayWithArray(displayItems);
}
void displayTimer() {
  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),    
    F(""),
    isTimerActive() ? F("Запущен") : F("Остановлен"),
    String(getRemainingTime()),
    F(""),
    F(""),
    F(""),
    F(""),
    F("(.) (.): сброс времени"),
    F("(x) (x): стоп игра")
  };
  updateDisplayWithArray(displayItems);
}
void displayTheEnd() {
  String displayItems[11] = {
    getStageName(currentStage),
    getModeName(currentMode),    
    F(""),
    F("Спасибо за игру"),
    F(""),
    F("Последнее значение:"),
    String(currentSettings.actualValue) + " " + currentSettings.unit,
    F(""),
    F(""),
    F(""),
    F("(x) (x): к режимам"),
  };
  updateDisplayWithArray(displayItems);
}
/*
=======================
РЕЖИМЫ ИГРЫ ЛОГИКА
*/
void selectMode() { // выбор режима
  displayModeMenu(); // показываем картинку 
  // реагируем на нажатия клавиш
  if (rightButtonClick()) {
    currentModeIndex += 1;
    if (currentModeIndex > availableModesLenght() - 1) {
      currentModeIndex = 0;
    }
    updateModeSelection();
  }

  if(leftButtonClick()) {
    currentModeIndex -= 1; 
    if (currentModeIndex < 0) {
      currentModeIndex = availableModesLenght() - 1;
    }
    updateModeSelection();
  }

  
};

void setGameSettings() { // задаем настройки игры 
  displayGameSetting(); // показываем экран 
  
  if (currentMode != MODE_COUNTER) {
  // реагируем 
    if (rightButtonClick()) {
        currentSettings.value += currentSettings.step;
        if (currentSettings.value > currentSettings.maxValue) {
          currentSettings.value = currentSettings.minValue;
        }
      }

      if (leftButtonClick()) {
        currentSettings.value -= currentSettings.step;
        if (currentSettings.value < currentSettings.minValue) {
          currentSettings.value = currentSettings.maxValue;
        }
      }  
  }
  
};

void game() { // игра
  switch(currentMode) {
      case MODE_TIMER: 
        gameTimer(); 
        break;
      case MODE_DEADZONE_TIMER: 
        gameDeadZoneTimer();
        break;
      case MODE_PLAYER_COUNT: 
        gamePlayerCount();
        break; 
      case MODE_LIFE_COUNT: 
        gameLifeCount();
        break;
      case MODE_COUNTER: 
        gameCounter();
        break;
      case MODE_INSTRUCTION: 
        gameInstruction();
        break;
    }
};


// ЛОГИКА ИГРЫ ТАЙМЕР
void gameTimer() {
  updateTimer();

  displayTimer(); // показываем таймер 

  if (!isTimerActive()) {
    startTimer(currentSettings.value);
  } 
  
  if (leftButtonClick() || rightButtonClick()) {
    stopTimer();    
  }
  
};

// ЛОГИКА ИГРЫ МЕРТВЯК
void gameDeadZoneTimer() {
  updateTimer();
  displayGameDeadZoneTimer();


  if (leftButtonClick() || rightButtonClick()) {
    int remainTime = getRemainingTime();
    stopTimer();
    //startTimer(remainTime + currentSettings.value);
    if (remainTime == 0) {
      startTimer(currentSettings.value);
    } else {
      startTimer(remainTime + 0.8 * remainTime); // к оставшемуся времени добавляем оставшееся
    }
    
  }
};

// ЛОГИКА ИГРЫ ЛИМИТ ИГРОКОВ
void gamePlayerCount() {
  updateTimer();
  displayGamePlayerCount();
  if (rightButtonClick() || leftButtonClick()) { // когда нажали кнопку (одну из) 
    currentSettings.actualValue += 1; // увеличиваем счетчик игроков в респе 
    stopTimer(); // останавливаем таймер, потому что в респе явно больше 0
    if (currentSettings.actualValue == currentSettings.value) { // когда достигли лимита, стартуем таймер
      startTimer(15); // стартуем обратный отсчет      
    } else if (currentSettings.actualValue > currentSettings.value) { // если еще кто-то приперся, то 
      currentSettings.actualValue = 1; // сброс счетчика до 1 игрока в респе  
      
      piskOnFor(500);
      
    }
  }

  if (!isTimerActive() && currentSettings.actualValue == currentSettings.value) { // если таймер остановлен, но при этом мы в лимите, то (типа таймер закончился)
    currentSettings.actualValue = 0; // обнули список игроков в респе до нуля
  }
  
};

// ЛОГИКА ИГРЫ ЛИМИТ ЖИЗНЕЙ
void gameLifeCount() {
  
  displayGameLifeCount();
  showMinutesSeconds(currentSettings.actualValue);
  if ((leftButtonClick() || rightButtonClick()) && currentSettings.actualValue > 0) {
    currentSettings.actualValue -= 1; 

    if (currentSettings.actualValue == 0) {
      
      piskOnFor(1500);
    }
  }
};

// ЛОГИКА ИГРЫ СКРЫТЫЙ СЧЕТЧИК
void gameCounter() {
  
  displayGameCounter();
  
  if ((leftButtonClick() || rightButtonClick())) {
    currentSettings.actualValue += 1; 
  }
};

// ПОКАЗЫВАЕМ ИНСТРУКЦИЮ
void gameInstruction() {
  displayGameInstruction();
}

/*
====================================
ВСПОМОГАТЕЛЬНАЯ ЛОГИКА 
*/
// Функция для форматирования времени в минуты и секунды
void showMinutesSeconds(unsigned long totalSeconds) {
    static unsigned long lastTotalSeconds = 99999;
    if (lastTotalSeconds == totalSeconds) { return; }
    lastDisplayedSecond = totalSeconds;
    // Специальный случай для отображения 99:99
    if (totalSeconds == 9999) {  // Можно использовать любое специальное значение
        uint8_t data[4];
        data[0] = oledTimerDisplay.encodeDigit(9);    // Первая цифра 9
        data[1] = oledTimerDisplay.encodeDigit(9) | 0x80;  // Вторая цифра 9 с двоеточием
        data[2] = oledTimerDisplay.encodeDigit(9);    // Третья цифра 9
        data[3] = oledTimerDisplay.encodeDigit(9);    // Четвертая цифра 9
        oledTimerDisplay.setSegments(data, 4, 0);
        return;
    }

    //unsigned long totalSeconds = timeMs / 1000;
    unsigned int minutes = totalSeconds / 60;
    unsigned int seconds = totalSeconds % 60;
    
    // Форматируем как MMSS (минуты и секунды)
    uint8_t data[4];
    data[0] = oledTimerDisplay.encodeDigit(minutes / 10);    // Десятки минут
    data[1] = oledTimerDisplay.encodeDigit(minutes % 10);    // Единицы минут
    data[2] = oledTimerDisplay.encodeDigit(seconds / 10);    // Десятки секунд
    data[3] = oledTimerDisplay.encodeDigit(seconds % 10);    // Единицы секунд
    
    // Добавляем двоеточие между минутами и секундами
    data[1] |= 0x80; // Устанавливаем точку (двоеточие)
    
    oledTimerDisplay.setSegments(data, 4, 0);
}

void updateModeSelection() { // в массиве режимов меняем свойство выбранности 
  // Сбрасываем все выделения
  for(int i = 0; i < availableModesLenght(); i++) {
    availableModes[i].selected = false;
  }
  // Выделяем текущий
  availableModes[currentModeIndex].selected = true;
  currentMode = availableModes[currentModeIndex].mode;
}

bool leftButtonClick() { // клик левой кнопки 
  if (greenBtn.click()) {
    
    piskOnFor(150);
    return true;

  }
  return false;
}
bool rightButtonClick() { // клик правой кнопки 
  if(redBtn.click()) {    
    
    piskOnFor(150);
    return true;
  }
  return false;
}

bool isUserOK() { // пользователь подтверждает выбор "правая кнопка долгое нажатие"
  if(redBtn.hold()) {     
    
    piskOnFor(300);
    
    return true;
  }
  return false;
} 
bool isUserCancel() { // пользователь отменяет выбор "правая кнопка долгое нажатие"
  if (greenBtn.hold()) {
    piskOnFor(300);
    return true;
  }
  return false;
}

int availableModesLenght() { // кол-во пунктов меню (режимов игры)
  return sizeof(availableModes) / sizeof(availableModes[0]);
}

String getModeName(WorkMode mode) { // Функция для преобразования enum в строку
  switch(mode) {
    case MODE_TIMER: return F("Респ по таймеру");
    case MODE_DEADZONE_TIMER: return F("Мертвяк");
    case MODE_PLAYER_COUNT: return F("Лимит игроков");
    case MODE_LIFE_COUNT: return F("Лимит жизней");
    case MODE_COUNTER: return F("Скрытый счетчик");
    case MODE_INSTRUCTION: return F("Инструкция");
    default: return F("Неизвестный");
  }
}

String getStageName(GameStage stage) { // Функция для преобразования enum в строку
  switch(stage) {
    case STAGE_SELECT_MODE: return F("=== РЕЖИМ ===");
    case STAGE_PREGAME:     return F("=== НАСТРОЙКА ===");
    case STAGE_GAME:        return F("=== ИГРА ===");
    case STAGE_ENDGAME:     return F("=== КОНЕЦ ===");
    default: return F("Неизвестный");
  }
}

String getGamePlayerCountStatus() { // статус игры 
  // статусы
  // ПОГНАЛИ - когда закончился таймер и достигнут лимит людей в респе 
  // ОБРАТНЫЙ ОТСЧЕТ - когда достигли лимита игроков в респе и идет таймер 
  // ЖДЕМ ЛЮДЕЙ - когда ждем лимита других (не достигли лимита) и таймер нас не интересует
  if (currentSettings.actualValue == currentSettings.value && !isTimerActive()) {
    return F("ПОГНАЛИ"); 
  } else if (currentSettings.actualValue == currentSettings.value && isTimerActive()) {
    return F("ОБРАТНЫЙ ОТСЧЕТ"); 
  }  else {
    return F("ЖДЕМ ЛЮДЕЙ"); 
  }
}


/* 
================================================
ФУНКЦИИ ТАЙМЕРА
*/
/**
 * Запуск таймера
 * @param seconds количество секунд для отсчета
 */
void startTimer(unsigned int seconds) {
  timerStartTime = millis();
  timerDuration = seconds;
  timerActive = true;
  lastDisplayedSecond = -1;
}

/**
 * Проверка и обновление таймера
 * @return true если таймер все еще работает, false если завершен
 */
bool updateTimer() {
  if (!timerActive) return false;
  
  unsigned long currentMillis = millis();
  unsigned long elapsedMillis = currentMillis - timerStartTime;
  unsigned int elapsedSeconds = elapsedMillis / 1000;
  
  // Оставшееся время
  int remainingSeconds = timerDuration - elapsedSeconds;
  
  // Защита от отрицательных значений
  if (remainingSeconds < 0) {
    remainingSeconds = 0;
  }
  
  // Обновляем отображение при изменении секунды
  if (remainingSeconds != lastDisplayedSecond) {
    lastDisplayedSecond = remainingSeconds;
    
    if (remainingSeconds > 0) {
      
      showMinutesSeconds(remainingSeconds);
      if (remainingSeconds < 5) {
        piskOnFor(500);
      }
    } else {
      // Таймер завершен
      timerActive = false;
      showMinutesSeconds(9999);
      piskOnFor(500);
    }
  }
  
  return timerActive;
}

/**
 * Получить оставшееся время
 * @return оставшиеся секунды (0 если таймер не активен или завершен)
 */
unsigned int getRemainingTime() {
  if (!timerActive) return 0;
  
  unsigned long currentMillis = millis();
  unsigned long elapsedMillis = currentMillis - timerStartTime;
  unsigned int elapsedSeconds = elapsedMillis / 1000;
  
  if (elapsedSeconds >= timerDuration) {
    return 0;
  }
  
  return timerDuration - elapsedSeconds;
}

/**
 * Остановить таймер досрочно
 */
void stopTimer() {
  if (timerActive) {
    timerActive = false;
  }
}

/**
 * Проверить, активен ли таймер
 */
bool isTimerActive() {
  return timerActive;
}


