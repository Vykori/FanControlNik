const uint8_t PWM_PIN = 1; // Pin 1 corresponds to PB1 (Physical Pin 6)
const uint8_t BUTTON_PIN = 2;
const uint8_t LED_PIN = 0;
const float minimumSpeed = 0.1; 
      float speed = 0.5; //default speed for startup
const uint8_t debounce = 10; // milliseconds to ignore changes in button state
const uint16_t pressAndHoldTime = 300; // milliseconds you have to hold the button to register as a hold, and also the miliseconds between two down-strokes to consider 2 clicks as a double-click

enum buttonState {
  IDLE,         // button is up, hasn't been pressed in a bit
  PRESSED,      // button is down, but not long enough to be held.
  HELD,         // button has been held down. After a state is moved to "HELD", once physical button is released it will go straight back to IDLE.
  RELEASED,     // button is up, but it was first pushed less than pressAndHoldTime ago, so another click might be coming.
};

uint16_t now = 0; //I will be casting millis() to this vavlue, 16 bits means the highest value is 65.535 seconds, so uhh, don't hold the button for over a minute and expect it to work I guess
uint8_t clicks = 0; //similarly, don't click the button more than 255 times
bool buttonIsDown = false;
uint16_t buttonPressedAt = 0;

// enum Mode {
//   NORMAL,
// };
// Mode currentMode = NORMAL;

int dir = 1;
buttonState state = IDLE;
buttonState lastState = IDLE;


void setup() {
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

  lastState = state;
  state = updateButtonState(state);

//  if (currentMode == NORMAL) { // as of writing, NORMAL is the only mode that is used (or even exists) but I'll probably forget to remove this comment when that is no longer true
    if (lastState == RELEASED && state == IDLE) {
      blinkLED(clicks);
    }
//  }

  if (state == HELD) {
    digitalWrite(LED_PIN, HIGH);
  }
  else if (state == IDLE) {
    digitalWrite(LED_PIN, LOW);
  }

  delay(debounce); //TODO: do it for real, in a non-blocking way

}

buttonState updateButtonState(buttonState state) {
  now = millis();
  if ( digitalRead(BUTTON_PIN) == 0) { buttonIsDown = true; }
  else { buttonIsDown = false; }

  if (state == IDLE && buttonIsDown) {
    buttonPressedAt = now;
    clicks++;
    return PRESSED;
  }
  else if (state == PRESSED && now - buttonPressedAt >= pressAndHoldTime && buttonIsDown) {
    return HELD;
  }
  else if (state == PRESSED && !buttonIsDown) {
    return RELEASED;
  }
  else if (state == HELD && !buttonIsDown) {
    clicks = 0;
    return IDLE;
  }
  else if (state == RELEASED && buttonIsDown) {
    buttonPressedAt = now;
    return PRESSED;
  }
  else if (state == RELEASED && !buttonIsDown && now - buttonPressedAt >= pressAndHoldTime ) {
    clicks = 0;
    return IDLE;
  }

  return state; // state didn't change
}

void setFanSpeed(float pwm) {
  pwm = constrain(pwm, minimumSpeed, 1.0);
  // Map onto the 0 to 39 Timer range for OCR1A
  OCR1A = (pwm * 39);
}

void blinkLED(uint8_t number) { // used for debugging
  digitalWrite(LED_PIN, HIGH);
  delay(10);
  digitalWrite(LED_PIN, LOW);
  delay(250);
  delay(125);
  for(; number > 0; number--) {
    digitalWrite(LED_PIN, HIGH);
    delay(125);
    digitalWrite(LED_PIN, LOW);
    delay(125);
  }
  delay(250);
  digitalWrite(LED_PIN, HIGH);
  delay(10);
  digitalWrite(LED_PIN, LOW);
}

void bullyUser() { // call this function if you think the end user deserves punishment for something.
  while(true){
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}