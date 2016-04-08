#include <LiquidCrystal.h>

#define TriggerPINA 2
#define TriggerPINB 3

// We're either in Velocity (A-B) mode, or ROF (A-A-A-A...) mode

#define VEL_MODE 0b00000001
#define ROF_MODE 0b00000010

// In vel mode, caught A already. Waiting for B.
// This is where timeouts can occur.
#define CAUGHT_A 0b00000100
// We've caught A and B, and it hasn't timed out. Therefore, we have data.
#define HAS_DATA 0b00001000
// Detector timed out.
#define TIMEOUT_ 0b10000000


// ROF flags
// Waiting for first pass. Should display a screen.
#define WAITING_ 0b00000100
// First pass has occured, start timing. When timeout occurs, clear flags back to state = ROF_MODE
#define STARTED_ 0b00001000



const float DeltaDCM = 100.0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 6, 8);

unsigned long t1 = 0;
unsigned long t2 = 0;
int count = 0;

// State stored as flags. See defines above.
// Starts in Velocity recording mode.
int state = VEL_MODE;


void triggerPinA (){
  if (digitalRead (TriggerPINA) == HIGH) {
      digitalWrite (13, HIGH);
      if (state & VEL_MODE) {
        t1 = micros();
        state |= CAUGHT_A;  // We have, self evidently, tripped sensor A.
        state &= ~TIMEOUT_; // Reset the timeout flag, since we want to leave the screen.
        state &= ~HAS_DATA; // We can't have data from this triggering yet, there's not been a second event.
      }
      if (state & ROF_MODE) { 
        t2 = micros();
        count ++;
      }
  }
}  // end of switchPressed

void triggerPinB () {
  if (digitalRead (TriggerPINB) == HIGH && state & CAUGHT_A && !(state & HAS_DATA)) {
      digitalWrite (13, HIGH);
      t2 = micros();
      state |= HAS_DATA; // Both sensors have been activated, so there must be data of SOME sort.
  }
}  // end of switchPressed

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Setup pins
  pinMode (TriggerPINA, INPUT);
  pinMode (TriggerPINB, INPUT);
  
  digitalWrite (TriggerPINA, LOW);
  digitalWrite (TriggerPINB, LOW);

  // Configure interrupts from triggers.
  attachInterrupt (digitalPinToInterrupt(TriggerPINA), triggerPinA, RISING);
  attachInterrupt (digitalPinToInterrupt(TriggerPINB), triggerPinB, RISING);
}

void MuzzleVelocityLoop (){
   // In velocity mode prior to catching A, so show a blank screen.
  if (!(state & HAS_DATA) && !(state & CAUGHT_A)) {
    
    t1 = 0;
    t2 = 0;

    lcd.setCursor(0, 0);
    lcd.print("Ready to fire....");

    lcd.setCursor(0, 1);
    if (state & TIMEOUT_) {
      lcd.print("Timeout! Retry!");
      state = VEL_MODE; // Complete reset of state.
    }
    
    digitalWrite (13, LOW);
  }

  // We haven't timed out yet. This shouldn't stay around for long.
  if (!(state & HAS_DATA) && (state & CAUGHT_A)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting for 2nd");
    lcd.setCursor(0, 1);
    lcd.print(" sensor....");
    digitalWrite (13, LOW);

    // Check for timeout condition.
    if ((micros()-t1) > 1000000 && !(state & TIMEOUT_)) {
      state  = VEL_MODE;
      state |= TIMEOUT_;
    }
  }
  
  // HAS_DATA
  // There is a result to be made!
  if (state & HAS_DATA) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Velocity in M/S:");
    lcd.setCursor(0, 1);
    digitalWrite (13, LOW);
    lcd.setCursor(0, 1);
    // MATH!
    lcd.print((DeltaDCM/100.0)/((t2-t1)/1000000.0));
  }
  delay(100);
}

void RateOfFireLoop() {
  
}

void loop() {
  if (state & VEL_MODE) MuzzleVelocityLoop();
  if (state & ROF_MODE) RateOfFireLoop();
}




