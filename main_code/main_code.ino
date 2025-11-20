

#include <DHT11.h>
#include "Adafruit_TCS34725.h"


// ------------------ DHT11 Sensor ------------------
//CHANGE CODE
DHT11 dht11(2);

bool justRecoveredFromBoundary = false;

bool firstTimeTurned = false;


// ================== PINOUT (TB6612FNG) ==================


//CHANGE CODE
const uint8_t PWMA = 9;
const uint8_t AIN1 = 8;
const uint8_t AIN2 = 7;


const uint8_t PWMB = 3;
const uint8_t BIN1 = 5;
const uint8_t BIN2 = 4;


const uint8_t STBY = 6;


// ================== USER CONFIGURATION ==================


//CHANGE CODE
const String FOLLOW_COLORS[] = {"BLACK", "BLUE", "BLUE"};
const int NUM_FOLLOW_COLORS = sizeof(FOLLOW_COLORS) / sizeof(FOLLOW_COLORS[0]);


//CHANGE CODE
const String LEFT_BOUNDARY_COLOR  = "WHITE";
const String RIGHT_BOUNDARY_COLOR = "ORANGE";


const String LEFT_TURN_MARKER  = "GREEN";
const String RIGHT_TURN_MARKER = "RED";


// ================== SPEED CONTROLS ==================
//50

//CHANGE CODE
const uint8_t FWD_PWM = 45;
uint8_t PIVL_LEFT_PWM  = 70;
uint8_t PIVL_RIGHT_PWM = 70;
uint8_t PIVR_LEFT_PWM  = 70;
uint8_t PIVR_RIGHT_PWM = 70;


// ================== COLOR SENSOR ==================
Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_24MS,   // faster
  TCS34725_GAIN_4X
);


// ================== MOTOR HELPERS ==================
inline uint8_t clampPWM(int v){ return (uint8_t)constrain(v,0,255); }


void leftMotorForward(uint8_t pwm){ digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); analogWrite(PWMA, clampPWM(pwm)); }
void leftMotorBackward(uint8_t pwm){ digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); analogWrite(PWMA, clampPWM(pwm)); }
void leftMotorStop(){ digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW); analogWrite(PWMA, 0); }


void rightMotorForward(uint8_t pwm){ digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); analogWrite(PWMB, clampPWM(pwm)); }
void rightMotorBackward(uint8_t pwm){ digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); analogWrite(PWMB, clampPWM(pwm)); }
void rightMotorStop(){ digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW); analogWrite(PWMB, 0); }


void motorsBrake(){
  leftMotorStop();
  rightMotorStop();
}


void motorsForward(uint8_t l, uint8_t r){
  leftMotorBackward(l);
  rightMotorBackward(r);
}
void pivotLeft(uint8_t l, uint8_t r){
  leftMotorBackward(l);
  rightMotorForward(r);
}
void pivotRight(uint8_t l, uint8_t r){
  leftMotorForward(l);
  rightMotorBackward(r);
}

void correctLeftBoundary() {
  float r, g, b, lux;
  for (int i = 0; i < 10; i++) {           // a few quick nudges
    delay(50);
    pivotRight(150, 150);                   // slightly stronger turn
    delay(100);                            // short movement
    motorsBrake();
    delay(10);

    String c = readColorOnce(r, g, b, lux);

    // Still on WHITE? Keep nudging.
    if (c == LEFT_BOUNDARY_COLOR) continue;

    // We've left WHITE: could now be BLACK/BLUE/YELLOW or something else.
    // Mark that we just came off a boundary so the next forward step is gentle.
    justRecoveredFromBoundary = true;
    return;
  }
  motorsBrake();
}

void correctRightBoundary() {
  float r, g, b, lux;
  for (int i = 0; i < 10; i++) {
    delay(50);
   pivotLeft(150, 150);
    delay(100);
    motorsBrake();
    delay(10);

    String c = readColorOnce(r, g, b, lux);

    if (c == RIGHT_BOUNDARY_COLOR) continue;

    justRecoveredFromBoundary = true;
    return;
  }
  motorsBrake();
}



// ================== COLOR DETECTION ==================
String detectColor(float r, float g, float b, float lux) {
  if ((r >= 100 && r <= 120) && (g >= 75 && g <= 90) && (b >= 50 && b <= 65) && (lux >= 7 && lux <= 20)) return "BLACK";
  if ((r >= 85 && r <= 95) && (g >= 80 && g <= 87) && (b >= 55 && b <= 65)) return "WHITE";
  if ((r >= 125 && r <= 135) && (g >= 55 && g <= 65) && (b >= 50 && b <= 60)) return "PINK";
  if ((r >= 150 && r <= 180) && (g >= 35 && g <= 65) && (b >= 25 && b <= 55))   return "RED";
  if ((r >= 85  && r <= 100) && (g >= 87 && g <= 95) && (b >= 50 && b <= 60))  return "GREEN";
  if ((r >= 55  && r <= 75) && (g >= 65  && g <= 80) && (b >= 90 && b <= 120)) return "BLUE";
  if ((r >= 105 && r <= 130) && (g >= 85  && g <= 115) && (b >= 20 && b <= 50))  return "YELLOW";
  if ((r >= 95 && r <= 120) && (g >= 57   && g <= 70)  && (b >= 65 && b <= 75)) return "PURPLE";
  if ((r >= 135 && r <= 150) && (g >= 60   && g <= 70)  && (b >= 25 && b <= 40)) return "ORANGE";
  return "UNKNOWN";
}

//maybe something here can change the speed of the color sensor?
inline String readColorOnce(float &r, float &g, float &b, float &lux) {
    // Get nice 0–255 RGB for your thresholds
    tcs.setInterrupt(false);
    delay(30);
    tcs.getRGB(&r, &g, &b);
    tcs.setInterrupt(true);

    // Optional: also compute lux from raw data
    uint16_t c, ri, gi, bi;
    tcs.getRawData(&ri, &gi, &bi, &c);
    lux = tcs.calculateLux(ri, gi, bi);

    return detectColor(r, g, b, lux);   // lux is not used now, but ok
}



// ================== SETUP ==================
void setup() {
  Serial.begin(9600);


  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  motorsBrake();


  if (!tcs.begin()) {
    Serial.println("No TCS34725 found!");
    while(1);
  }


  Serial.println("System Ready: Color Sensor + DHT11");
}


// ================== MAIN LOOP ==================
void loop() {


  /************** READ DHT11 SENSOR **************/
  int temp = 0, hum = 0;
  int result = dht11.readTemperatureHumidity(temp, hum);


  if (result == 0) {
    Serial.print("TEMP: "); Serial.print(temp);
    Serial.print("°C  HUM: "); Serial.print(hum);
    Serial.println("%");
  } else {
    Serial.println(DHT11::getErrorString(result));
  }


  /************** READ COLOR SENSOR **************/
  float r, g, b, lux;
  String color = readColorOnce(r, g, b, lux);


  Serial.print("RGB: ");
  Serial.print(int(r)); Serial.print(",");
  Serial.print(int(g)); Serial.print(",");
  Serial.print(int(b)); Serial.print(",");
  Serial.print(int(lux));

  Serial.print(" -> ");
  Serial.println(color);


   if (color == LEFT_TURN_MARKER) {
    motorsBrake(); delay(100);
    correctRightBoundary();
    
    return;
  }



  if (color == RIGHT_TURN_MARKER) {
    motorsBrake(); delay(100);
    correctLeftBoundary();
    
    return;
  }


  bool isFollowColor = false;
  for (int i = 0; i < NUM_FOLLOW_COLORS; i++)
    if (color == FOLLOW_COLORS[i]) isFollowColor = true;


  if (isFollowColor) {
    uint8_t pwm = FWD_PWM;
    delay(500);
    uint16_t step = 15;

    if (justRecoveredFromBoundary) {
      // Be more gentle on the first step after a boundary correction
      pwm = 40;          // a bit slower
      step = 15;         // shorter burst
      justRecoveredFromBoundary = false;
    }

    motorsForward(pwm, pwm);  // move
    delay(150);              // for a short time (step)
    motorsBrake();            // stop
    delay(500);              // small pause (step)

  }



  if (color == LEFT_BOUNDARY_COLOR) {
    motorsBrake(); delay(100);
    correctLeftBoundary();
    return;
  }

  if (color == RIGHT_BOUNDARY_COLOR) {
    motorsBrake(); delay(100);
    correctRightBoundary();
    
    return;
  }


  // Recovery sequence
  motorsForward(FWD_PWM, FWD_PWM); delay(70);
  motorsBrake(); delay(100);


  float rN, gN, bN, luxN;
  String colorN = readColorOnce(rN, gN, bN, luxN);
  for (int i = 0; i < NUM_FOLLOW_COLORS; i++)
    if (colorN == FOLLOW_COLORS[i]) {
      motorsForward(FWD_PWM, FWD_PWM);
      //delay(step);   //edited
      return;
    }


  pivotLeft(70, 70); delay(600);
  motorsBrake(); delay(400);



  float rL, gL, bL, luxL;
  String colorL = readColorOnce(rL, gL, bL, luxN);
  for (int i = 0; i < NUM_FOLLOW_COLORS; i++)
    if (colorL == FOLLOW_COLORS[i]) {
      motorsForward(FWD_PWM, FWD_PWM);
      //delay(step) ;  //edited
      return;
    }


  pivotRight(70, 70); delay(600);
  motorsBrake(); delay(400);

  float rR, gR, bR, luxR;
  String colorR = readColorOnce(rR, gR, bR, luxN);
  for (int i = 0; i < NUM_FOLLOW_COLORS; i++)
    if (colorR == FOLLOW_COLORS[i]) {
      motorsForward(FWD_PWM, FWD_PWM);
      //delay(step);   //edited
      return;
    }

  pivotRight(70, 70); delay(600);
  motorsBrake(); delay(400);

  colorR = readColorOnce(rR, gR, bR, luxN);
  for (int i = 0; i < NUM_FOLLOW_COLORS; i++)
    if (colorR == FOLLOW_COLORS[i]) {
      motorsForward(FWD_PWM, FWD_PWM);
      //delay(step);   //edited
      return;
    }

  pivotLeft(70, 70); delay(600);
  motorsBrake(); delay(400);

  colorN = readColorOnce(rN, gN, bN, luxN);
  for (int i = 0; i < NUM_FOLLOW_COLORS; i++)
    if (colorN == FOLLOW_COLORS[i]) {
      motorsForward(FWD_PWM, FWD_PWM);
      //delay(step);   //edited
      return;
    }


  motorsBrake();
  delay(10);
}
