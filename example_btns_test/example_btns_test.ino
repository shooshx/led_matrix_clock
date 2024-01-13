#include "Buttons.h"

Buttons buttons;

void setup() {
  Serial.begin(115200); 

  auto click_handler = [](int pin) {
    Serial.printf("click %d\n", pin);
  };
  auto long_press_handler = [](int pin) {
    Serial.printf("long-press %d\n", pin);
  };

  buttons.setup( {
    Button(4, click_handler, long_press_handler),
    Button(0, click_handler, long_press_handler),
    Button(17, click_handler, long_press_handler),
    Button(12, click_handler, long_press_handler),
    Button(26, click_handler, long_press_handler),
  });
  Serial.printf("did setup\n");
}


void setup_raw() {
  Serial.begin(115200);  
  
  pinMode(4, INPUT_PULLUP);
  pinMode(0, INPUT_PULLUP);
  //pinMode(1, INPUT_PULLUP);
  //pinMode(3, INPUT_PULLUP);
  pinMode(17, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
}

void doRead(int n) {
  int b = digitalRead(n);
  if (b == 0)
    Serial.printf("pushed %d : %d\n", n, b);
}

void loop() {
  buttons.scan();
  return;

  //Serial.printf("reading... ");
  doRead(4);
  doRead(0);
  //doRead(1);
  //doRead(3);
  doRead(17);
  doRead(12);
  doRead(26);
  //Serial.printf("\n");
}
