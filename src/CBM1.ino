#include <LiquidCrystal.h>
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
#define outPin 8
#define s0 9
#define s1 10
#define s2 11
#define s3 12
boolean DEBUG = true;
// Variables
int red, grn, blu;
String color ="";
long startTiming = 0;
long elapsedTime =0;
void setup(){
  Serial.begin(9600);
  
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(outPin, INPUT); //out from sensor becomes input to arduino
  // Setting frequency scaling to 100%
  digitalWrite(s0,HIGH);
  digitalWrite(s1,HIGH);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor (3,0);
  lcd.print("Welcome To");
  lcd.setCursor (1,1);
  lcd.print("Color Detector");
  delay(2000);  
  lcd.clear();  
  startTiming = millis();
}
void loop(){
  getColor();
   
  if(DEBUG)printData(); 
  elapsedTime = millis()-startTiming; 
  if (elapsedTime > 1000) {
     showDataLCD();
    startTiming = millis();
  }
}
/* read RGB components */
void readRGB(){
  red = 0, grn=0, blu=0;
  
  int n = 10;
  for (int i = 0; i < n; ++i){
    //read red component
    digitalWrite(s2, LOW);
    digitalWrite(s3, LOW);
    red = red + pulseIn(outPin, LOW);
  
   //read green component
    digitalWrite(s2, HIGH);
    digitalWrite(s3, HIGH);
    grn = grn + pulseIn(outPin, LOW);
    
   //let's read blue component
    digitalWrite(s2, LOW);
    digitalWrite(s3, HIGH);
    blu = blu + pulseIn(outPin, LOW);
  }
  red = red/n;
  grn = grn/n;
  blu = blu/n;
}
/***************************************************
* Showing captured data at Serial Monitor
****************************************************/
void printData(void){
  Serial.print("red= ");
  Serial.print(red);
  Serial.print("   green= ");
  Serial.print(grn);
  Serial.print("   blue= ");
  Serial.print(blu);
  Serial.print (" - ");
  Serial.print (color);
  Serial.println (" detected!");
}
///***************************************************
//* Showing capured data at LCD
//****************************************************/
void showDataLCD(void){
lcd.clear();
lcd.setCursor (0,0);
lcd.print("R");
lcd.print(red);
lcd.setCursor (6,0);
lcd.print("G");
lcd.print(grn);
lcd.setCursor (12,0);
lcd.print("B");
lcd.print(blu);  
lcd.setCursor (0,1);
lcd.print("Color: ");  
lcd.print(color);  
}
void getColor() {
    readRGB(); 
    
    int sum = red + grn + blu;
    if (sum == 0) {
        color = "NO_COLOR";
        return;
    }

    float R_norm = (float)red / sum;
    float G_norm = (float)grn / sum;
    float B_norm = (float)blu / sum;

    if ( R_norm > 0.1 && R_norm < 0.15 && G_norm > 0.49 && G_norm < 0.53 && B_norm > 0.36 && B_norm < 0.39) {
        color = "RED";
    } else if  (R_norm > 0.42 && R_norm < 0.465 && G_norm > 0.195 && G_norm < 0.255 && B_norm > 0.32 && B_norm < 0.35) {
        color = "GREEN";
    } else if ( R_norm > 0.45 && R_norm < 0.52 && G_norm > 0.33 && G_norm < 0.36 && B_norm > 0.15 && B_norm < 0.2) {
        color = "BLUE";
    } else if ( R_norm > 0.32 && R_norm < 0.355 && G_norm > 0.34 && G_norm < 0.36 && B_norm > 0.295 && B_norm < 0.32) {
        color = "BLACK";
    } else if ( R_norm > 0.1 && R_norm < 0.15 && G_norm > 0.42 && G_norm < 0.45 && B_norm > 0.42 && B_norm < 0.46){
       color = "ORANGE";
    }
    else if ( R_norm > 0.21 && R_norm < 0.25 && G_norm > 0.46 && G_norm < 0.52 && B_norm > 0.255 && B_norm < 0.29){
       color = "PURPLE";
    }
    else {
        color = "UNKNOWN";
    }
}
