/* Detects patterns of knocks and triggers a motor to unlock
   it if the pattern is correct.
   
   By Steve Hoefer http://grathio.com
   Version 0.1.10.20.10
   Licensed under Creative Commons Attribution-Noncommercial-Share Alike 3.0
   http://creativecommons.org/licenses/by-nc-sa/3.0/us/
   (In short: Do what you want, just be sure to include this line and the four above it, and don't sell it or use it in anything you sell without contacting me.)
   
   Analog Pin 0: Piezo speaker (connected to ground with 1M pulldown resistor)
   Digital Pin 2: Switch to enter a new code.  Short this to enter programming mode.
   Digital Pin 3: DC gear reduction motor attached to the lock. (Or a motor controller or a solenoid or other unlocking mechanisim.)
   Digital Pin 4: Red LED. 
   Digital Pin 5: Green LED. 
   
   Update: Nov 09 09: Fixed red/green LED error in the comments. Code is unchanged. 
   Update: Nov 20 09: Updated handling of programming button to make it more intuitive, give better feedback.
   Update: Jan 20 10: Removed the "pinMode(knockSensor, OUTPUT);" line since it makes no sense and doesn't do anything.
 */
 
// Pin definitions
const int knockSensor = A0;         // Piezo sensor on pin 0.
const int redLED = 4;              // Status LED
const int greenLED = 13;            // Status LED
const int lockServo = 9;
 
// Tuning constants.  Could be made vars and hoooked to potentiometers for soft configuration, etc.
const int threshold = 4;           // Minimum signal from the piezo to register as a knock
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock..
const int averageRejectValue = 15; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockTurnTime = 650;      // milliseconds that we run the motor to get it to go a half turn.

const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1200;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
int secretCode[maximumKnocks] = {100, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int unlockCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int lockCode[maximumKnocks] = {100, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
#include <Servo.h> 

Servo myservo;  // create servo object to control a servo 
 
int pos = 10;    // variable to store the servo position 

void setup() {
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(A0, INPUT);

  Serial.begin(9600);               			// Uncomment the Serial.bla lines for debugging.
  Serial.println("Started");  			// but feel free to comment them out after it's working right.
  
  // Green LED on, everything is go.
  digitalWrite(greenLED, HIGH);     
   
  // attaches the servo on pin 9 to the servo object 
  myservo.attach(lockServo);
  myservo.write(10);  
}

void loop() {
  // Listen for any knock at all.
  knockSensorValue = analogRead(knockSensor);

  if (knockSensorValue >=threshold){
    listenToSecretKnock();
  }
} 

// Records the timing of knocks.
void listenToSecretKnock(){
  Serial.println("knock starting");   

  int i = 0;
  int predelay = 0;
  int now=millis();
  int startTime=now;                 // Reference for when this knock started.
  // First lets reset the listening array.
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]=0;
  }
  
  int currentKnockNumber=0;         			// Incrementer for the array.
  
  digitalWrite(greenLED, LOW);      			// we blink the LED for a bit as a visual indicator of the knock.
  delay(knockFadeTime);                       	// wait for this peak to fade before we listen to the next one.
  digitalWrite(greenLED, HIGH);  

  do {
    //listen for the next knock or wait for it to timeout. 
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue >=threshold){                   //got another knock...
      //record the delay time.
      Serial.println("knock.");
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber++;                             //increment the counter
      startTime=now;          
      // and reset our timer for the next knock
      digitalWrite(greenLED, LOW);  
      delay(knockFadeTime);                              // again, a little delay to let the knock decay.
      digitalWrite(greenLED, HIGH);
    }

    now=millis();
    
    //did we timeout or run out of knocks?
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));
  
  //we've got our knock recorded, lets see if it's valid
  
  if (validateKnock() == true){
    triggerDoorUnlock(); 
  } else {
    Serial.println("Secret knock failed.");
    digitalWrite(greenLED, LOW);
    pinMode(A0, OUTPUT);
    Serial.println("Beep.");
    beep(0);
    for (i=0;i<maximumKnocks;i++){
        if ((predelay=knockReadings[i]) != 0) {
            beep(predelay);
        }
    }
    pinMode(A0, INPUT);
    // We didn't unlock, so blink the red LED as visual feedback.
    for (i=0;i<4;i++){
      digitalWrite(redLED, HIGH);
      delay(100);
      digitalWrite(redLED, LOW);
      delay(100);
    }
    digitalWrite(greenLED, HIGH);
  }
}

void beep(int predelay) {
  String msg;
  msg = "Delay ";
  msg += predelay;
  Serial.println(msg);
  delay(predelay);
  
  for (int i=0;i<2;i++){
    digitalWrite(A0, HIGH);
    delay(4);
    digitalWrite(A0, LOW);
    delay(4);
  }
}

// Runs the motor (or whatever) to unlock the door.
void triggerDoorUnlock(){

  int i=0;
  if (pos == 10) {
    pos = 165;
    for (i=0;i<maximumKnocks;i++){
      secretCode[i] = unlockCode[i];
    }
  }
  else {
    pos = 10;
    for (i=0;i<maximumKnocks;i++){
      secretCode[i] = lockCode[i];
    }
  }

  // turn the motor on for a bit.
  myservo.write(pos);
  // tell servo to go to position in variable 'pos'

  Serial.println("Door unlocked!");
  digitalWrite(greenLED, HIGH);
  // And the green LED too.
  
  // Blink the LED a few times for more visual feedback.
  for (i=0; i < 5; i++){   
      digitalWrite(greenLED, LOW);
      delay(100);
      digitalWrite(greenLED, HIGH);
      delay(100);
  }
}

// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
// todo: break it into smaller functions for readability.
boolean validateKnock(){
  int i=0;
  String msg="";
 
  // simplest check first: Did we get the right number of knocks?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;          			// We use this later to normalize the times.
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      msg += knockReadings[i];
      msg += " ";
      currentKnockCount++;
    }
    if (secretCode[i] > 0){  					//todo: precalculate this.
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){ 	// collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  msg += "Knocks detected: ";
  msg += currentKnockCount;
  msg += " Secret knocks: ";
  msg += secretKnockCount;
  Serial.println(msg);  
  if (currentKnockCount != secretKnockCount){
    return false; 
  }
  
  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast. 
  */
  int totaltimeDifferences=0;
  int timeDiff=0;
  int reading = 0;
  for (i=0;i<maximumKnocks;i++){ // Normalize the times
    reading = map(knockReadings[i], 0, maxKnockInterval, 0, 100);      
    timeDiff = abs(reading - secretCode[i]);
    msg = "r: ";
    msg += reading;
    msg += "s: ";
    msg += secretCode[i];
    msg += "diff: ";
    msg += timeDiff;
    Serial.println(msg);  
    if (timeDiff > rejectValue){ // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences/secretKnockCount>averageRejectValue){
    return false; 
  }
  
  return true;
  
}
