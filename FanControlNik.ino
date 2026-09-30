const uint8_t PWM_PIN = 1; // Pin 1 corresponds to PB1 (Physical Pin 6)
const uint8_t BUTTON_PIN = 2;
const uint8_t LED_PIN = 0;
const uint8_t debounce = 10; // milliseconds to ignore changes in button state
const uint16_t pressAndHoldTime = 500; // milliseconds you have to hold the button to register as a hold
const uint16_t doubleClickTime = 300; // on button release, miliseconds to wait for another click
const uint8_t incrementDelay = (float)5000/255; // milliseconds between incrementing speed during slow speed adjustment

enum ButtonState {
  IDLE,         // button is up, hasn't been pressed in a bit
  PRESSED,      // button is down, but not long enough to be held.
  HELD,         // button has been held down. After a state is moved to "HELD", once physical button is released it will go straight back to IDLE.
  WAITING,      // button is up, but still waiting in case another click might be coming.
};

enum Mode {
  NORMAL,
};

uint8_t clicks = 0; // quantity of button presses within 1 click sequence. reminder not to implement a behavior that requires more than 255 button presses, because this will overflow, and also that is not very user friendly...!
bool buttonIsDown = false;
uint16_t buttonPressedAt = 0;
uint16_t buttonReleasedAt = 0;
uint16_t lastIncrement = 0;
bool debouncingActive = false; // if true, ignore button state changes until it is false again

Mode currentMode = NORMAL;

ButtonState state = IDLE;
ButtonState lastState = IDLE;

uint8_t speeds[4] = {25, 0, 0, 255}; // first value defines minimum speed, last value defines maximum speed, the rest are dynamically corrected to be equal increments
uint8_t speed = speeds[0]; //default speed for startup

uint16_t now = millis(); // 16 bits means the highest value is 65.535 seconds before rollover, which is plenty, as long as I don't implement a behavior triggered on holding the button for a really long time.

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

  redistributeSpeeds();
  setFanSpeed(speed);
}

void loop() { 
  now = millis();
  lastState = state;
  state = updateButtonState(state);

  if (currentMode == NORMAL) { // as of writing, NORMAL is the only mode that is used (or even exists) but I'll probably forget to remove this comment when that is no longer true
    if (lastState == WAITING && state == IDLE) {
      if (clicks == 1) { //if 1 click, figure out what speed increment is next, and go to it
        for (uint8_t i = (sizeof(speeds) / sizeof(speeds[0])) - 1; true; i--) { //dangerous! loop will go forever if speed is less than speeds[0] (which is supposed to be the minimum anyway)
          if (speed >= speeds[i]) {
            if (i+1 == sizeof(speeds) / sizeof(speeds[0])) {
              speed = speeds[0]; // wrap around to zero
              break;
            }
            else {
              speed = speeds[i+1];
              break;
            }
          }
        }
      }
      else if (clicks == 2) { //if 2 clicks, figure out what speed increment is previous, and go to it
        for (uint8_t i = 0; true; i++) { //dangerous! loop will go forever if speed is more than speeds[highest] (which is supposed to be the maximum anyway)
          if (speed <= speeds[i]) {
            if (i == 0) {
              speed = speeds[(sizeof(speeds) / sizeof(speeds[0]) - 1)]; //wrap around to max
              break;
            }
            else {
              speed = speeds[i-1];
              break;
            }
          }
        }
      }

      setFanSpeed(speed);
    }
    else if (state == HELD) {
      digitalWrite(LED_PIN, HIGH);
      if (clicks == 1) {
        if (now - lastIncrement >= incrementDelay) {
          lastIncrement = now;
          speed += 1;
          if (speed > speeds[(sizeof(speeds) / sizeof(speeds[0])) - 1] || speed == 0) {
            speed =   speeds[(sizeof(speeds) / sizeof(speeds[0])) - 1];
          }
          setFanSpeed(speed);
        }
      }
      if (clicks == 2) {
        if (now - lastIncrement >= incrementDelay) {
          lastIncrement = now;
          speed -= 1;
          if (speed < speeds[0] || speed == 255) {
            speed =   speeds[0];
          }
          setFanSpeed(speed);
        }
      }
    }
    else if (lastState == HELD && state == IDLE) {
      digitalWrite(LED_PIN, LOW);
    }
  }
}

ButtonState updateButtonState(ButtonState state) {
  if (digitalRead(BUTTON_PIN) == LOW) { buttonIsDown = true; }
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
  else if (state == WAITING && buttonIsDown && !debouncingActive) {
    buttonPressedAt = now;
    clicks += 1;
    debouncingActive = true;
    return PRESSED;
  }
  else if (state == PRESSED && buttonIsDown && now - buttonPressedAt >= pressAndHoldTime) {
    return HELD;
  }
  else if (state == PRESSED && !buttonIsDown && !debouncingActive) {
    buttonReleasedAt = now;
    debouncingActive = true;
    return WAITING;
  }
  else if (state == HELD && !buttonIsDown && !debouncingActive) {
    buttonReleasedAt = now;
    debouncingActive = true;
    return IDLE;
  }
  else if (state == WAITING && !buttonIsDown && now - buttonReleasedAt >= doubleClickTime ) {
    return IDLE;
  }

  return state; // state didn't change, nothing to do.
}

void setFanSpeed(uint8_t pwm) {
  // Map onto the 0 to 39 Timer range for OCR1A
  // OCR1A = static_cast<uint8_t>(static_cast<float>(pwm) / 255 * OCR1C);
  OCR1A = (static_cast<uint16_t>(pwm) * OCR1C) / 255; //this method avoids floats and gives same result
}

void redistributeSpeeds() {
  uint8_t min = speeds[0];
  uint8_t increments = sizeof(speeds) / sizeof(speeds[0]);
  uint8_t max = speeds[increments - 1];

  for (uint8_t i = 1; i < increments - 1; i++) { // iterate through all elements of speeds[] except the first and the last
    speeds[i] = (((max - min) / (increments-1)) * i) + min; // evenly space all increments (the math theoretically work for first and last, too)
  }
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
