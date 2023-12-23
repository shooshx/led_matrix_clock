void setup() {
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
