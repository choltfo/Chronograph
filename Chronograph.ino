#include <LiquidCrystal.h>

///////////////////////////////////////////////////////////
// FIX THESE PIN NUMBERS
///////////////////////////////////////////////////////////

#define TriggerPINA 3
#define TriggerPINB 2

#define MODE_PIN 8
#define UNITS_PIN 9

// We're either in Velocity (A-B) mode, or ROF (A-A-A-A...) mode

#define VEL_MODE 0b00000001
#define IMP_MODE 0b00000010
 
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

#define FULL_ 0b11111111
#define NONE_ 0b00000000



const float DeltaDCM = 10.0;
const float DeltaDFt = 0.328084;

///////////////////////////////////////////////////////////
// Pin number assignemnts for LCD
// https://www.arduino.cc/en/Tutorial/LiquidCrystalDisplay
///////////////////////////////////////////////////////////

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 10, 6, 5, 4, 7);

unsigned long t1 = 0;
unsigned long t2 = 0;
int count = 0;

// State stored as flags. See defines above.
// Starts in Velocity recording mode.
int state = VEL_MODE; //VEL_MODE;


void triggerPinA (){
      digitalWrite (13, HIGH);
      if (state & VEL_MODE) {
        // Velocity mode
        t1 = micros();
        state |= CAUGHT_A;  // We have, self evidently, tripped sensor A.
        state &= ~TIMEOUT_; // Reset the timeout flag, since we want to leave the screen.
        state &= ~HAS_DATA; // We can't have data from this triggering yet - there's not been a second event.
      } else {
        // ROF Mode.
        if (state & STARTED_) {
          t2 = micros();
          count ++;
        } else {
          count = 0;
          t1 = micros();
          state = 0;
          state |= STARTED_;
        }
      }
}  // end of TriggerPinA

void triggerPinB () {
  if (state & CAUGHT_A && !(state & HAS_DATA)) {
      digitalWrite (13, HIGH);
      t2 = micros();
      state |= HAS_DATA; // Both sensors have been activated, so there must be data of SOME sort.
  }
}  // end of switchPressed

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  Serial.begin(9600);

  // Setup pins
  pinMode (TriggerPINA, INPUT);
  pinMode (TriggerPINB, INPUT);
  
  pinMode (MODE_PIN, INPUT);

  pinMode (10, OUTPUT);


  // Leave triggers floating.
  //digitalWrite (TriggerPINA, LOW);
  //digitalWrite (TriggerPINB, HIGH);
  digitalWrite (MODE_PIN, HIGH);

  // Configure interrupts from triggers.
  attachInterrupt (digitalPinToInterrupt(TriggerPINA), triggerPinA, FALLING);
  attachInterrupt (digitalPinToInterrupt(TriggerPINB), triggerPinB, FALLING);
}

void MuzzleVelocityLoop (){
   // In velocity mode prior to catching A.
  if (!(state & HAS_DATA) && !(state & CAUGHT_A)) {
    
    t1 = 0;
    t2 = 0;

    lcd.setCursor(0, 0);
    if (digitalRead(UNITS_PIN)) {
      lcd.print("fps mode");
    } else {
      lcd.print("M/S mode");
    }

    
    if (state & TIMEOUT_) {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Timeout! Retry!");
      state = VEL_MODE; // Complete reset of state.
    }
    
    digitalWrite (13, LOW);
  }

  // We haven't timed out yet. This shouldn't stay around for long.
  // If it does, then the shot missed.
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
    digitalWrite (13, LOW);
    // MATH!
    //if (state & IMP_MODE) {
    if (digitalRead(UNITS_PIN)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Velocity in fps:");
      lcd.setCursor(0, 1);
      lcd.print((DeltaDFt)/((t2-t1)/1000000.0));
      Serial.println((DeltaDFt)/((t2-t1)/1000000.0), DEC);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Velocity in M/S:");
      lcd.setCursor(0, 1);
      lcd.print((DeltaDCM/100.0)/((t2-t1)/1000000.0));
      Serial.println((DeltaDCM/100.0)/((t2-t1)/1000000.0), DEC);
    }
    //Serial.println(" M/S");
  }
  delay(100);
}

void RateOfFireLoop() {
  if (state & STARTED_) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ROF mode.");
    lcd.setCursor(0, 1);
    // Number / DeltaT
    Serial.println((60*count) / ((t2-t1)/1000000.0), DEC);
    lcd.print((60*count) / ((t2-t1)/1000000.0));
    lcd.setCursor(12, 1);
    lcd.print("RPM");

    // Needs a manual reset instead. Like, say, a button.
    /*if ((micros()) > 10000000 && !(state & TIMEOUT_)) {
      state  = ROF_MODE;
      t1 = 0;
      t2 = 0;
      count = 0;
    }*/
  } else {  // Not waitng - currently counting shots.
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ROF mode.");
    lcd.setCursor(0, 1);
    lcd.print("Ready!");
  }
  digitalWrite (13, LOW);
  delay(100);
}

// Goes through either individual function as the case may be.
void loop() {
  if (state & VEL_MODE) MuzzleVelocityLoop();
  else RateOfFireLoop();

  //digitalWrite (11, digitalRead(2));
  
  /*if (digitalRead(MODE_PIN) != (state & VEL_MODE)) { 
    state = 0;
    state += digitalRead(MODE_PIN)*VEL_MODE;
    lcd.clear();
  }

  // Read unit selector pin.

  // If current switch state doesn't match the state defined therein:
  if (digitalRead(UNITS_PIN) != ((state & IMP_MODE) != 0)) {
    state = state & (~IMP_MODE);
    //if (digitalRead(UNITS_PIN)) state |= IMP_MODE;
    lcd.clear();
  }*/
}




