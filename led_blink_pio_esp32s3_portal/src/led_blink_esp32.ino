
//int LED_BUILTIN = 2;

void setup(void){
  
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
}

int i = 0;

void loop(void){
  digitalWrite(LED_BUILTIN, LOW);  
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(500);

  Serial.printf("Hello %d\n", i++);
}
