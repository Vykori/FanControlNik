const uint8_t PWM_PIN = 1; // Pin 1 corresponds to PB1 (Physical Pin 6)
const uint8_t BUTTON_PIN = 2;
const uint8_t LED_PIN = 0;
const float minimumSpeed = 0.1; //minimum fan speed that doesn't cause issues. TODO: add a method to adjust this at runtime with the button
      float speed = 0.5; //default speed for startup
const uint8_t debounce = 10; // milliseconds to ignore changes in button state
const uint16_t pressAndHoldTime = 500; // milliseconds you have to hold the button to register as a hold, and also the miliseconds between two down-strokes to consider 2 clicks as a double-click
// TODO: consider separating pressAndHoldTime and doubleClickTime into 2 vars instead of 1

enum buttonState {
  IDLE,         // button is up, hasn't been pressed in a bit
  PRESSED,      // button is down, but not long enough to be held.
  HELD,         // button has been held down. After a state is moved to "HELD", once physical button is released it will go straight back to IDLE.
  RELEASED,     // button is up, but it was first pushed less than pressAndHoldTime ago, so another click might be coming.
};

enum mode {
  NORMAL,
};

uint8_t clicks = 0; //similarly, don't click the button more than 255 times
bool buttonIsDown = false;
uint16_t buttonPressedAt = 0;
uint16_t buttonReleasedAt = 0;
bool debouncingActive = false; // if true, ignore button state changes until it is false again

mode currentMode = NORMAL;

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

  if (currentMode == NORMAL) { // as of writing, NORMAL is the only mode that is used (or even exists) but I'll probably forget to remove this comment when that is no longer true
    if (lastState == RELEASED && state == IDLE) {
      blinkLED(clicks);
    }
    else if (state == HELD) {
      digitalWrite(LED_PIN, HIGH);
    }
    else if (state == IDLE) {
      digitalWrite(LED_PIN, LOW);
    }
  }
}

buttonState updateButtonState(buttonState state) {
  uint16_t now = millis(); // 16 bits means the highest value is 65.535 seconds before rollover, which is plenty as long as I don't implement a behavior triggered on holding the button for a really long time.

  if ( digitalRead(BUTTON_PIN) == 0) { buttonIsDown = true; }
  else { buttonIsDown = false; }

  if (debouncingActive && now - buttonPressedAt >= debounce && now - buttonReleasedAt >= debounce) {
    debouncingActive = false; // this method of debouncing is a little hard to follow but doesn't require an extra timer
  }

  if (state == IDLE && buttonIsDown && !debouncingActive) {
    buttonPressedAt = now;
    clicks = 1; // start of a new click sequence.
    debouncingActive = true;
    return PRESSED;
  }
  else if (state == RELEASED && buttonIsDown && !debouncingActive) {
    buttonPressedAt = now;
    clicks += 1;
    debouncingActive = true;
    return PRESSED;
  }
  else if (state == PRESSED && now - buttonPressedAt >= pressAndHoldTime && buttonIsDown) {
    return HELD;
  }
  else if (state == PRESSED && !buttonIsDown && !debouncingActive) {
    buttonReleasedAt = now;
    debouncingActive = true;
    return RELEASED;
  }
  else if (state == HELD && !buttonIsDown && !debouncingActive) {
    buttonReleasedAt = now;
    debouncingActive = true;
    return IDLE;
  }
  else if (state == RELEASED && !buttonIsDown && now - buttonPressedAt >= pressAndHoldTime ) { // TODO: consider pros and cons of using buttonReleasedAt instead, which would require different timings
    return IDLE;
  }

  return state; // state didn't change, nothing to do.
}

void setFanSpeed(float pwm) {
  pwm = constrain(pwm, minimumSpeed, 1.0);
  // Map onto the 0 to 39 Timer range for OCR1A
  OCR1A = static_cast<uint8_t>(pwm * 39);
}

void blinkLED(uint8_t number) { // used for debugging
  digitalWrite(LED_PIN, HIGH);
  delay(2);
  digitalWrite(LED_PIN, LOW);
  delay(250);
  delay(125);
  for(; number > 0; number--) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }
  delay(250);
  digitalWrite(LED_PIN, HIGH);
  delay(2);
  digitalWrite(LED_PIN, LOW);
}
