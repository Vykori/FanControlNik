const int PWM_PIN = 1; // Pin 1 corresponds to PB1 (Physical Pin 6)
const int BUTTON_PIN = 2;
const int LED_PIN = 0;
float min = 0.1;
float speed = 0.5;
int dir = 1;

unsigned long timer = 0;
bool isHeld = 0;

void setup() {
  timer = millis();
  pinMode(PWM_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //next we need to change some register values so the PWM frequency we provide to the fan is the expected ~25kHz
  //bruh idk how any of this bullshit works lmao
  // --- Configure Timer 1 for 25kHz PWM on PB1 (Channel A) ---
  TCCR1 = 0; // Reset registers
  GTCCR = 0;
  // 1. Set Prescaler to 8 (CS12=0, CS11=1, CS10=1)
  // 8,000,000 Hz / 8 = 1,000,000 Hz timer clock
  TCCR1 |= (1 << CS11) | (1 << CS10);
  // 2. Set TOP value in OCR1C to control the frequency
  // 1,000,000 Hz / 25,000 Hz = 40 counts (0 to 39)
  OCR1C = 39; 
  // 3. Enable PWM on Compare Match A (PB1 / Pin 6)
  // PWM1A enables PWM mode for comparator A
  // COM1A1 enables cleared-on-compare-match (non-inverted) behavior
  TCCR1 |= (1 << PWM1A) | (1 << COM1A1);
  // 4. Initialize duty cycle to 0 (Off)
  OCR1A = 0; 

  setFanSpeed(speed);  // Initialize the fan to the default speed
}

void loop() {
  setFanSpeed(speed);
  while(digitalRead(BUTTON_PIN)==1){}; //button is not pressed 
  delay(20); //debounce
  while(digitalRead(BUTTON_PIN)==0){ //button is pressed
    if(isHeld == 0){ //these things only happen once, as soon as the button is pressed
      isHeld = 1;
      dir *= -1;
    }
    speed += (0.01 * dir); //if button is held, increment fan
    if(speed > 1.0){ 
      speed = 1.0;
    }
    if(speed < min){
      speed = min;
    }
    setFanSpeed(speed);
    delay(50); //speed ramp delay
  }
  isHeld = 0;
}

void setFanSpeed(float pwm) {
  constrain(pwm, 0.0, 1.0);
  // Map onto the 0 to 39 Timer range for OCR0B
  OCR1A = (pwm * 39);
}