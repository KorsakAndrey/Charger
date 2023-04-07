#include <GyverButton.h>

#define LOAD_OUT 13
#define CHARGE_OUT 3  //Timer2_B
#define SWITCH 5
#define ADC_IN A0
#define CURRENT 0
#define VOLTAGE 1
#define LED 13

#define VREF 5        //V
#define DIV_R1 23800  //betwen "+" and POWER_SENS
#define DIV_R2 9920   //betwen "-" and POWER_SENS
#define CUR_R  0.15   //Om

//Steps
#define CHARGE_1 0
#define REST 1
#define CHARGE_2 2
#define DISCHARGE 3

#define CHARGE_TIME 28800000   //8 hours
#define REST_TIME 115200000    //24 hours
#define VOLTAGE_LIMIT 9.0      //Dischrge threshold
#define CURRENT_1 1.0  //A
#define CURRENT_2 2.0  //A


constexpr float vFactor = ((((float)DIV_R1 + DIV_R2) / DIV_R2) * VREF) / 1024;
constexpr float cFactor = (float)VREF / 1024 / CUR_R;

byte curState = CHARGE_1;
unsigned long lastTime = 0;
unsigned long showTime = 0;
bool change = false;

GButton btn(SWITCH);


void startPWM() {
  DDRD |= 1 << 3;
}

void stopPWM() {
  DDRD &= !(1 << 3);
}

void upPWM() {
  OCR2B == 255 ? 0 : OCR2B++;
}

void downPWM() {
  OCR2B ? OCR2B-- : 0 ;
}

void setup() {
  Serial.begin(9600);

  //Config timer
  OCR2B = 0;
  TCCR2A |= 1 << COM2B1 | 0 << COM2B0 | 1 << WGM21 | 1 << WGM20;
  TCCR2B |= 0 << WGM22 | 1 << CS22 | 1 << CS21 | 0 << CS20;


  analogReference(DEFAULT);  //Set VREF 5v

  pinMode(LOAD_OUT, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(LOAD_OUT, 0);
  startPWM();
}




void loop() {
  unsigned long curTime  = millis();  //Get current time;
  float voltage;
  float current;
  btn.tick();

  if (btn.isClick()) {
    change = true;
    lastTime = curTime;
  }

  if (curState == DISCHARGE) {
    voltage = (float)analogRead(ADC_IN) * vFactor;
  } else {
    current = (float)analogRead(ADC_IN) * cFactor;

  }
  if (curTime - showTime > 2000) {
    showTime = curTime;
    uint32_t sec = curTime / 1000ul;      // полное количество секунд

    Serial.print("MODE : ");
    Serial.println(curState == CHARGE_1 ? "Charge 1" : curState == REST ? "REST" : curState == CHARGE_2 ? "Charge 2" : "Discharge");
    Serial.print("Time : ");  // Время
    Serial.print((sec / 3600ul));  // h
    Serial.print(":");
    Serial.print((sec % 3600ul) / 60ul); // m
    Serial.print(":");
    Serial.print((sec % 3600ul) % 60ul); // s
    sec = sec - (lastTime / 1000);
    Serial.print("    Mode time : ");  // Время
    Serial.print((sec / 3600ul));  // h
    Serial.print(":");
    Serial.print((sec % 3600ul) / 60ul); // m
    Serial.print(":");
    Serial.println((sec % 3600ul) % 60ul); // s

    if (curState == DISCHARGE) {
      Serial.print("vFactor : ");
      Serial.println(vFactor);
      Serial.print("Voltage : ");
      Serial.println(voltage);
    } else {
      Serial.print("cFactor : ");
      Serial.println(cFactor);
      Serial.print("Current : ");
      Serial.println(current);
      Serial.print("PWM % : ");
      Serial.println(OCR2B * 100 / 255);
    }
    Serial.println("********************");
  }

  /*Serial.print("Status : ");
    Serial.println(curState);
    Serial.print("vFactor : ");
    Serial.println(vFactor);
    Serial.print("Voltage : ");
    Serial.println(voltage);
    Serial.print("Current : ");
    Serial.println(current);
    Serial.print("Time : ");
    Serial.println(curTime / 1000);
    Serial.print("Compare register : ");
    Serial.println(OCR2B);
    Serial.println("********************");
    Serial.println("********************");
    delay(1000);*/

  switch (curState) {
    case CHARGE_1: {
        if (current < CURRENT_1) {
          upPWM();
        } else {
          downPWM();
        }
        if (curTime - lastTime >  CHARGE_TIME) {
          change = true;
          lastTime = curTime;
        }
        if (change) {
          change = false;
          curState = REST;
          stopPWM();
        }
        break;
      }

    case REST: {
        //do nothing
        if (curTime - lastTime >  REST_TIME) {
          change = true;
          lastTime = curTime;
        }
        if (change) {
          change = false;
          curState = CHARGE_2;
          startPWM();
        }
        break;
      }

    case CHARGE_2: {
        if (current < CURRENT_2) {
          upPWM();
        } else {
          downPWM();
        }
        if (curTime - lastTime >  CHARGE_TIME) {
          change = true;
          lastTime = curTime;
        }
        if (change) {
          change = false;
          curState = DISCHARGE;
          stopPWM();
        }
        break;
      }

    case DISCHARGE: {
        //do something
        digitalWrite(LOAD_OUT, 1);//Discharge on
        if (voltage <  VOLTAGE_LIMIT) {
          change = true;
          lastTime = curTime;
          }
        if (change) {
          change = false;
          curState = CHARGE_1;
          digitalWrite(LOAD_OUT, 0); //Discharge off
          startPWM();
        }
        break;
      }
  }
}
