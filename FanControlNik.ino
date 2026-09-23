const unsigned int PWM_PIN = 1; // Pin 1 corresponds to PB1 (Physical Pin 6)
const unsigned int BUTTON_PIN = 2;
const unsigned int LED_PIN = 0;
static float min = 0.1;
static unsigned int debounce = 10; // milliseconds to ignore changes in button state
static unsigned int pressAndHoldTime = 300; // milliseconds you have to hold the button to register as a hold, and also the miliseconds between two down-strokes to consider 2 clicks as a double-click
float speed = 0.5;
int dir = 1;
bool wasAlreadyPressed = false;
unsigned long buttonPressedAt = 0;
unsigned long buttonReleasedAt = 0;
unsigned int clicks = 0;
unsigned long timer = 0;
bool isHeld = 0;
unsigned long now = 0;

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

void loop() { // main loop interprets button presses, holds, double-clicks, etc.
// I am also avoiding using any delays so this loop can run as many times per second as possible
// this means debouncing needs to be taken care of with timers
  now = millis();

  if (digitalRead(BUTTON_PIN)==0 || buttonPressedAt > now - debounce) { // button is depressed (poor button, get well soon)
    if (wasAlreadyPressed == false) { // button was just pressed on this cycle
      wasAlreadyPressed = true;
      // buttonPressedAt = now; // we actually need to do this 1 step later so we can use the old value for 1 more check
      if (buttonPressedAt > now - pressAndHoldTime) { // this is a double/triple/quadruple click
        clicks += 1;
        buttonPressedAt = now;
      }
      else { // this is the first click of a new sequence
        clicks = 1;
        buttonPressedAt = now;
      }
    }
    else { 
      if (buttonPressedAt > now - pressAndHoldTime) { // button is now being held down, this should trigger something

      }
    }
  }
  else if (digitalRead(BUTTON_PIN)==1 || buttonReleasedAt > now - debounce) { // button is not depressed
    if (wasAlreadyPressed == true) { // button was just released this cycle
      wasAlreadyPressed = false;
      buttonReleasedAt = now;
      if (buttonPressedAt < now - pressAndHoldTime) { // button was just released after a hold, this should trigger something
        clicks = 0;
      }
    }
    else if (buttonReleasedAt < now - pressAndHoldTime && clicks > 0) { // user has stopped clicking, this should trigger something
      blinkLED(clicks);
      clicks = 0;
    }
  }
}

void setFanSpeed(float pwm) {
  constrain(pwm, 0.0, 1.0);
  // Map onto the 0 to 39 Timer range for OCR0B
  OCR1A = (pwm * 39);
}

void blinkLED(int number) { // used for debugging
  delay(250);
  for(int i = number; i > 0; i--) {
    digitalWrite(LED_PIN, HIGH);
    delay(125);
    digitalWrite(LED_PIN, LOW);
    delay(125);
  }
  delay(250);
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}
