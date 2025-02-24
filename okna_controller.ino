//#include <GyverDS18.h>
//GyverDS18Single ds(7);

#include <microDS18B20.h>
MicroDS18B20 <7> ds;

#include <GyverTM1637.h>
GyverTM1637 disp(6, 5);

#include <EncButton.h>
#define EB_CLICK_TIME 700
EncButton enc(4, 3, 2);

//classes
class Timer {
  public:
    Timer () {}
    Timer (uint32_t nprd) {
      start(nprd);
    }
    void start(uint32_t nprd) {
      prd = nprd;
      start();
    }
    void start() {
      tmr = millis();
      if (!tmr) tmr = 1;
    }
    void stop() {
      tmr = 0;
    }
    bool ready() {
      if (tmr && millis() - tmr >= prd) {
        start();
        return 1;
      }
      return 0;
    }

  private:
    uint32_t tmr = 0, prd = 0;
};

//таймеры
Timer disp_tmr(100);
Timer ds_tmr(1000);

// потом eeprom добавлю


//пременные состояния
//float curr_temp;
struct able {
  bool can_open;
  bool can_close;
  bool can_full_open;
  bool can_full_close;
};

able can = {true, false, true, false};


struct {
  //температура, нужная для активации указанной логики
  int8_t open_temp = 50;
  int8_t close_temp = -70;
  int8_t full_open_temp = 50;
  int8_t full_close_temp = -70;
  bool use_full_open = true;
  bool use_full_close = true;

  //Время открытия и закрытия
  uint32_t open_time = 10000;
  uint32_t close_time = 6000;
  uint32_t full_time = 30000;

} settings;

bool go_to_begining = false;
bool out_of_settings = false;

float cur_temp;

//пины
#define IN1 A0 // реле 1
#define IN2 A1 // реле 2
#define WORKING 12 //состояние работы
#define SETTING 13 //состояние настройки
#define OPENING 8 // настройка открывания
#define CLOSING 9 //настройка закрывания
#define FULL_OPENING 10 // настройка полного открывания
#define FULL_CLOSING 11 //настройка полного закрывания


void setup() {
  //реле
  pinMode(IN2, OUTPUT);
  pinMode(IN1, OUTPUT);

  //светодиоды
  pinMode(WORKING, OUTPUT);
  pinMode(SETTING, OUTPUT);
  pinMode(OPENING, OUTPUT);
  pinMode(CLOSING, OUTPUT);
  pinMode(FULL_OPENING, OUTPUT);
  pinMode(FULL_CLOSING, OUTPUT);

  //энкодер
  enc.counter = 0;

  //дисплей
  disp.clear();
  disp.brightness(7);

  //термодатчик
  //  ds.setParasite(false);
  ds.requestTemp();
  while (!ds_tmr.ready());
  ds.readTemp();
  cur_temp = ds.getTemp();

  //епром

}

void loop() {
  digitalWrite(WORKING, HIGH);

  if (enc.tick()) {
    if (enc.click(2)) settingMode();
    if (enc.rightH()) handyClosing();
    if (enc.leftH()) handyOpening();
  }

  tempRead();
  actions();
  showTemp();


}

void tempRead() {
  //  if (ds.ready()) if (ds.readTemp()) cur_temp = ds.getTemp();
  if (ds.readTemp()) cur_temp = ds.getTemp();

  //  if (ds_tmr.ready() && !ds.isWaiting()) ds.requestTemp();
  if (ds_tmr.ready()) ds.requestTemp();
}

void showTemp() {
  if (disp_tmr.ready()) {
    disp.clear();
    disp.displayInt(round(cur_temp));
  }
}

void handyClosing() {
  uint32_t stp = millis();
  disp.clear();
  disp.displayByte(_C, _L, _O, _S);

  while (millis() - stp < settings.full_time) {
    if (enc.tick()) if (enc.click(2)) break;
    digitalWrite(IN2, HIGH);
  }

  digitalWrite(IN2, LOW);
}

void handyOpening() {
  uint32_t stp = millis();
  disp.clear();
  disp.displayByte(_O, _P, _E, _n);

  while (!enc.click() && millis() - stp < settings.full_time) {
    if (enc.tick()) if (enc.click(2)) break;
    digitalWrite(IN1, HIGH);
  }

  digitalWrite(IN1, LOW);
}

void actions() {
  uint32_t stp = millis();

  // призакрыть
  if (cur_temp < settings.close_temp && can.can_close) {
    can = (able) {
      true, false, true, true
    };
    disp.clear();
    disp.displayByte(_C, _L, _O, _S);
    while (millis() - stp < settings.close_time) {
      if (enc.tick()) if (enc.click(2)) break;
      digitalWrite(IN2, HIGH);
    }
    digitalWrite(IN2, LOW);
  }

  //приоткрыть
  else if (cur_temp > settings.open_temp && can.can_open) {
    can = (able) {
      false, true, true, true
    };
    disp.clear();
    disp.displayByte(_O, _P, _E, _n);
    while (millis() - stp < settings.close_time) {
      if (enc.tick()) if (enc.click(2)) break;
      digitalWrite(IN1, HIGH);
    }
    digitalWrite(IN1, LOW);
  }

  //закрыть полностью
  else if (cur_temp < settings.full_close_temp && can.can_full_close && settings.use_full_close) {
    can = (able) {
      true, false, true, false
    };
    disp.clear();
    disp.displayByte(_C, _L, _O, _S);
    while (millis() - stp < settings.full_time) {
      if (enc.tick()) if (enc.click(2)) break;
      digitalWrite(IN2, HIGH);
    }
    digitalWrite(IN2, LOW);
  }

  //открыть полностью
  else if (cur_temp > settings.full_open_temp && can.can_full_open && settings.use_full_open) {
    can = (able) {
      false, true, false, true
    };
    disp.clear();
    disp.displayByte(_O, _P, _E, _n);
    while (millis() - stp < settings.full_time) {
      if (enc.tick()) if (enc.click(2)) break;
      digitalWrite(IN1, HIGH);
    }
    digitalWrite(IN1, LOW);
  }
}

void settingMode() {
  digitalWrite(WORKING, LOW);
  digitalWrite(SETTING, HIGH);

  while (true) {
    digitalWrite(OPENING, HIGH);

    if (enc.tick()) {
      if (enc.right()) settings.open_temp = constrain(settings.open_temp + 1, -99, 99);
      if (enc.left()) settings.open_temp = constrain(settings.open_temp - 1, -99, 99);
      if (enc.click()) closing();
      if (enc.click(2)) break;
      if (enc.click(3)) saveSettings();
      if (enc.rightH()) openTime();
    }

    if (go_to_begining) {
      go_to_begining = false;
      led_off();
    }
    if (out_of_settings) {
      out_of_settings = false;
      break;
    }
    showOpenTemp();
  }

  led_off();
  digitalWrite(WORKING, HIGH);
  digitalWrite(SETTING, LOW);
}

void openTime() {
  while (true) {
    if (enc.tick()) {
      if (enc.right()) settings.open_time = constrain(settings.open_time + 1000, 1000, settings.full_time);
      if (enc.left()) settings.open_time = constrain(settings.open_time - 1000, 1000, settings.full_time);
      if (enc.click()) closing();
      if (enc.click(2)) closeSettings();
      if (enc.click(3)) saveSettings();
      if (enc.leftH()) break;
    }
    if (go_to_begining) break;
    showOpenTime();
  }
}

void showOpenTemp() {
  if (disp_tmr.ready()) {
    disp.clear();
    disp.displayInt(settings.open_temp);
  }
}

void showOpenTime() {
  if (disp_tmr.ready()) {
    disp.clear();
    disp.displayInt(settings.open_time / 1000);
  }
}

void closing() {
  led_off();
  digitalWrite(CLOSING, HIGH);

  while (true) {
    if (enc.tick()) {
      if (enc.right()) settings.close_temp = constrain(settings.close_temp + 1, -99, 99);
      if (enc.left()) settings.close_temp = constrain(settings.close_temp - 1, -99, 99);
      if (enc.click()) fullOpen();
      if (enc.click(2)) closeSettings();
      if (enc.click(3)) saveSettings();
      if (enc.rightH()) closeTime();
    }
    if (go_to_begining) break;
    showClosingTemp();
  }
}

void closeTime() {
  while (true) {
    if (enc.tick()) {
      if (enc.right()) settings.close_time = constrain(settings.close_time + 1000, 1000, settings.full_time);
      if (enc.left()) settings.close_time = constrain(settings.close_time - 1000, 1000, settings.full_time);
      if (enc.click()) fullOpen();
      if (enc.click(2)) closeSettings();
      if (enc.click(3)) saveSettings();
      if (enc.leftH()) break;
    }
    if (go_to_begining) break;
    showClosingTime();
  }
}

void showClosingTemp() {
  if (disp_tmr.ready()) {
    disp.clear();
    disp.displayInt(settings.close_temp);
  }
}

void showClosingTime() {
  if (disp_tmr.ready()) {
    disp.clear();
    disp.displayInt(settings.close_time / 1000);
  }
}

void fullOpen() {
  led_off();
  digitalWrite(FULL_OPENING, HIGH);

  while (true) {
    if (enc.tick()) {
      if (enc.right()) settings.full_open_temp = constrain(settings.full_open_temp + 1, -99, 99);
      if (enc.left()) settings.full_open_temp = constrain(settings.full_open_temp - 1, -99, 99);
      if (enc.click()) fullClose();
      if (enc.click(2)) closeSettings();
      if (enc.click(3)) saveSettings();
      if (enc.rightH()) settings.use_full_open = !settings.use_full_open;
    }
    if (go_to_begining) break;
    showFOpenTemp();
  }
}

void showFOpenTemp() {
  if (disp_tmr.ready()) {
    disp.clear();
    if (settings.use_full_open) disp.displayInt(settings.full_open_temp);
    else disp.displayByte(_O, _F, _F, _empty);

  }
}

void fullClose() {
  led_off();
  digitalWrite(FULL_CLOSING, HIGH);

  while (true) {
    if (enc.tick()) {
      if (enc.right()) settings.full_close_temp = constrain(settings.full_close_temp + 1, -99, 99);
      if (enc.left()) settings.full_close_temp = constrain(settings.full_close_temp - 1, -99, 99);
      if (enc.click()) go_to_begining = true;
      if (enc.click(2)) closeSettings();
      if (enc.click(3)) saveSettings();
      if (enc.rightH()) settings.use_full_close = !settings.use_full_close;
    }
    if (go_to_begining) break;
    showFCloseTemp();
  }
}

void showFCloseTemp() {
  if (disp_tmr.ready()) {
    disp.clear();
    if (settings.use_full_close) disp.displayInt(settings.full_close_temp);
    else disp.displayByte(_O, _F, _F, _empty);
  }
}

void closeSettings() {
  led_off();
  go_to_begining = true;
  out_of_settings = true;
}

void saveSettings() {
  closeSettings();
}

void led_off() {
  digitalWrite(OPENING, LOW);
  digitalWrite(CLOSING, LOW);
  //  digitalWrite(SETTING, LOW);
  //  digitalWrite(WORKING, LOW);
  digitalWrite(FULL_OPENING, LOW);
  digitalWrite(FULL_CLOSING, LOW);
}
