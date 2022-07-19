#include <Arduino.h>
#include <MsTimer2.h>

/*
   Arduino UNO/NANO specs say following things:

  The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.

  The base frequency for pins 5 and 6 is 62500 Hz.

  The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64, 256, and 1024.

  The divisors available on pins 3 and 11 are: 1, 8, 32, 64, 128, 256, and 1024.

  PWM frequencies are tied together in pairs of pins. If one in a pair is changed, the other is also changed to match.

  Pins 5 and 6 are paired on timer0.

  Pins 9 and 10 are paired on timer1.

  Pins 3 and 11 are paired on timer2.

  Changes on pins 3, 5, 6, or 11 may cause the delay() and millis() functions to stop working. Other timing-related functions may also be affected.

  shareimprove this answer
*/
volatile long start;
volatile long val0, val1;
volatile long _val0, _val1;
volatile long max_val0 = 0, max_val1 = 0;
volatile long min_val0 = 10000, min_val1 = 10000;
volatile int i ;

volatile  boolean IN1_value = false;
volatile  boolean IN2_value = false;
volatile  boolean IN3_value = false;
volatile  boolean IN4_value = false;

volatile  boolean val0_dir = false;
volatile  boolean val1_dir = false;
//TODO регистры предыдущих значений семафоров (на предыдущем шаге цикла)
volatile  boolean _IN1_value = false;
volatile  boolean _IN2_value = false;
volatile  boolean _IN3_value = false;
volatile  boolean _IN4_value = false;

volatile int timer1 = 0;
volatile int timer2 = 0;
//TODO константы временных тиков
#define time1 110   // константа задающая число тиков таймера для перехода семафора разрешения в состояние разрешения 1 колеса
#define time2 170   // константа задающая число тиков таймера для перехода семафора разрешения в состояние разрешения 2 колеса
#define _time1 70
#define _time2 170
//TODO константы отсечек значений считанных с аналоговых входов
#define minval 270
#define maxval 415

#define LED 13
#define IN1 2
#define IN2 4
#define EN1 9 //pwm output

#define IN3 7
#define IN4 8
#define EN2 10 //pwm output

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
//
//TODO таймер срабатывает примерно раз в 10 микросекунд
//TODO ведется обратный отсчет по таймерам колес истечение таймера, значение ==0, означает превышение периода 
//TODO отсчитанного с момента наступление события реинициализации счетчика таймера и тогда мы обрабатываем превышение периода

void flash()
{
  //  digitalWrite(LED, !digitalRead(LED));
  if (timer1 != 0) timer1--;
  if (timer2 != 0) timer2--;
}

void setup() {
  /*
    default
    111 `128
     ADCTEST: 111 msec (1000 calls)
    100 16
    ADCTEST: 15 msec (1000 calls)
    011 8
    ADCTEST: 8 msec (1000 calls)
    010 4
    ADCTEST: 5 msec (1000 calls)
    101
  */

  cbi(ADCSRA, ADPS2) ;
  sbi(ADCSRA, ADPS1) ;
  sbi(ADCSRA, ADPS0) ;

  pinMode(LED, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);


/* TODO отключаем весь вывод в COM
  Serial.begin(9600);

  Serial.print("ADCTEST: ") ;
  start = millis() ;
  for (i = 0 ; i < 128 ; i++)
    //  analogRead(0) ;
    val0 += analogRead(0) ;
  val0 = val0 >> 8;
  for (i = 0 ; i < 128 ; i++)
    //  analogRead(1) ;
    val1 += analogRead(1) ;
  val1 = val1 >> 8;
  Serial.print(millis() - start) ;
  Serial.println(" msec (128X2 calls)") ;
  Serial.print(val0) ;
  Serial.print(" ") ;
  Serial.println(val1) ;
*/
  MsTimer2::set(10, flash); // 100ms period
  MsTimer2::start();

}

void loop() {
// --------------------------------------------------------------------------------------
//считаем значения аналогового входа 1 128 раз
//получаем среднее значение обрезанное по младшему биту ( делим на 256)
  _val0 = val0;
  for (i = 0 ; i < 128 ; i++)
    val0 += analogRead(0) ;

  val0 = val0 >> 8;
// --------------------------------------------------------------------------------------
//TODO считаем значения аналогового входа 1 128 раз
//TODO получаем среднее значение обрезанное по младшему биту ( делим на 256)
  _val1 = val1;
  for (i = 0 ; i < 128 ; i++)
    val1 += analogRead(1) ;

  val1 = val1 >> 8;
// --------------------------------------------------------------------------------------
  //  val0_dir = _val0 <= val0;
  //  val1_dir = _val1 <= val1;

  //  if ( _val0 <= val0 ) {
  //    IN1_value = true;
  //  }
  //  else {
  //    IN1_value = false;
  //  }
// --------------------------------------------------------------------------------------
//включение разрешения на колесо 1

//TODO если ппроизошел переход включения магнита на притяжение начинаем отсчет занова
  if ( _IN1_value == LOW && IN1_value == HIGH ) timer1 = time1;
//TODO если произошло переключение магнита на отталкивание проверяем значения счетчиков
  if (_IN2_value == HIGH && IN2_value == LOW)
  {
//    если таймер истек и разрешение было отключено тогда включаем разрешение 
//    скорость упала ниже заданной в константе timer1
    if (timer1 == 0 && digitalRead(EN1) == LOW) digitalWrite(EN1, HIGH);
//    если остаток таймера больше чем time1 - _time1 и разрешение включено 
//    это означает что скорость достигнута и надо отключить разрешение
    if (timer1 > (time1 - _time1) && digitalRead(EN1) == HIGH) digitalWrite(EN1, LOW);
  }
// --------------------------------------------------------------------------------------

  if ( _IN3_value == LOW && IN3_value == HIGH ) timer2 = time2;

  if (_IN4_value == HIGH && IN4_value == LOW)
  {
    if (timer2 == 0 && digitalRead(EN2) == LOW) digitalWrite(EN2, HIGH);
    if (timer2 > (time2 - _time2) && digitalRead(EN2) == HIGH) digitalWrite(EN2, LOW);
  }
// --------------------------------------------------------------------------------------
// TODO сохраяем семафоры направления магнитов в регистры предыдущих состояний
  _IN1_value = IN1_value;
  _IN2_value = IN2_value;
  _IN3_value = IN3_value;
  _IN4_value = IN4_value;
// --------------------------------------------------------------------------------------
// здесь определяются какие направления магнита надо включить
  if ( minval < val0 && maxval > val0)
  {
    IN1_value = val0_dir ? true : false;
    IN2_value = !val0_dir ? true : false;
  }
  else if ( minval >= val0)
  {
    val0_dir = _val0 <= val0; // выставляем семафор направления фронта аналогового сигнала с датчика (нарастание или спад)
    IN1_value = false;
    IN2_value = false;
  }
  else
  {
    IN1_value = false;
    IN2_value = false;
    val0_dir = false;
  }
// --------------------------------------------------------------------------------------

  if ( minval < val1 && maxval > val1)
  {
    IN3_value = val1_dir ? true : false;
    IN4_value = !val1_dir ? true : false;
  }
  else if ( minval >= val1)
  {
    val1_dir = _val1 <= val1;
    IN3_value = false;
    IN4_value = false;
  }
  else
  {
    IN3_value = false;
    IN4_value = false;
    val1_dir = false;
  }
// --------------------------------------------------------------------------------------

  digitalWrite(IN1, IN1_value);
  digitalWrite(IN2, IN2_value);
  digitalWrite(IN3, IN3_value);
  digitalWrite(IN4, IN4_value);

  //  delayMicroseconds(62);
// --------------------------------------------------------------------------------------

  if (max_val0 < val0) max_val0 = val0;
  if (max_val1 < val1) max_val1 = val1;
  if (min_val0 > val0) min_val0 = val0;
  if (min_val1 > val1) min_val1 = val1;
// --------------------------------------------------------------------------------------

//        Serial.print(val0) ;
//        Serial.print(" ") ;
//        Serial.print(val1) ;
//        Serial.print(" ") ;
//        Serial.print(max_val0) ;
//        Serial.print(" ") ;
//        Serial.print(max_val1) ;
//        Serial.print(" ") ;
//        Serial.print(min_val0) ;
//        Serial.print(" ") ;
//        Serial.println(min_val1) ;
//    delay(1);
}