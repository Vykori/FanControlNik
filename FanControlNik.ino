const uint8_t PWM_PIN = 1; // Pin 1 corresponds to PB1 (Physical Pin 6)
const uint8_t BUTTON_PIN = 2;
const uint8_t LED_PIN = 0;
static float min = 0.1;
uint8_t debounce = 10; // milliseconds to ignore changes in button state
uint16_t pressAndHoldTime = 300; // milliseconds you have to hold the button to register as a hold, and also the miliseconds between two down-strokes to consider 2 clicks as a double-click
float speed = 0.5;
int dir = 1;
bool wasAlreadyPressed = false;
uint16_t buttonPressedAt = 0;
uint16_t buttonReleasedAt = 0;
bool debounceActive = false;
uint16_t now = 0; //I will be casting millis() to this vavlue, 16 bits means the highest value is 65.535 seconds, so uhh, don't hold the button for over a minute and expect it to work I guess
uint8_t clicks = 0; //similarly, don't click the button more than 255 times

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

void loop() { // main loop interprets button presses, holds, double-clicks, etc.
// I am also avoiding using any delays so this loop can run as many times per second as possible
// this means debouncing needs to be taken care of with timers
  now = millis();

  if ((digitalRead(BUTTON_PIN)==0 || (debounceActive && now - buttonPressedAt < debounce)) &&
                                    (!debounceActive || !(now - buttonReleasedAt < debounce))) { // button is depressed (poor button, get well soon)
    if (wasAlreadyPressed == false) { // button was just pressed on this loop
      wasAlreadyPressed = true;
      debounceActive = true;
      // buttonPressedAt = now; // we actually need to do this 1 step later so we can use the old value for 1 more check
      if (now - buttonPressedAt < pressAndHoldTime) { // this is not the first click of this sequence
        clicks += 1;
        buttonPressedAt = now;
        if (clicks == 255) { bullyUser(); } //ok come on, are you trying to break my shit?
      }
      else { // this is the first click of a new sequence
        clicks = 1;
        buttonPressedAt = now;
      }
    }
    else { 
      if (now - buttonPressedAt >= pressAndHoldTime) { // button is now being held down, this should trigger something
        digitalWrite(LED_PIN, HIGH);
      }
    }
  }
  else { // button is not depressed
    if (wasAlreadyPressed == true) { // button was just released this loop
      wasAlreadyPressed = false;
      debounceActive = true;
      buttonReleasedAt = now;
      if (now - buttonPressedAt >= pressAndHoldTime) { // button was just released after a hold, this should trigger something
        clicks = 0;
        digitalWrite(LED_PIN, LOW);
      }
    }
    else if (now - buttonReleasedAt >= pressAndHoldTime && clicks > 0) { // user has stopped clicking, this should trigger something
      blinkLED(clicks);
      clicks = 0;
    }
    if (now - buttonReleasedAt >= debounce || now - buttonPressedAt >= debounce) { // if the state hasn't changed for [debounce] milliseconds
      debounceActive = false;
    }
  }
}

void setFanSpeed(float pwm) {
  pwm = constrain(pwm, 0.0, 1.0);
  // Map onto the 0 to 39 Timer range for OCR1A
  OCR1A = (pwm * 39);
}

void blinkLED(int number) { // used for debugging
  digitalWrite(LED_PIN, HIGH);
  delay(10);
  digitalWrite(LED_PIN, LOW);
  delay(250);
  delay(125);
  for(int i = number; i > 0; i--) {
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