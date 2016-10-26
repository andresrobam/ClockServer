#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

char* ssid = "kolm-kuus";         //wifi name
char* password = "mingilambikas"; //wifi password

String url = "http://192.168.0.10:8080/getstate"; //GET url

const int laserpin = 3; //the driving the laser transistor
const int pin1x = 0;    //yaw motor pins
const int pin2x = 4;
const int pin3x = 5;
const int pin4x = 16;
int currentstepx = 1;   //tracks the current step the motor is on

const int pin1y = 15;   //pitch motor pins
const int pin2y = 13;
const int pin3y = 12;
const int pin4y = 14;
int currentstepy = 1;   //track the current step the motor is on

int motordelay = 4;     //number of milliseconds to wait after advancing one step on the motors
int requestdelay = 500; //number of milliseconds to wait between sending GET requests
int dotdelay = 100;     //number of milliseconds to wait when drawing a single dot in the colon sequence

int globalx = 0;  //tracks the current global step of the motors
int globaly = 0;

boolean work = false;   //whether to draw the digits on this pass of the loop or not
boolean dotting = true; //whether to draw the colon in the middle or not

int dig1 = 0;   //the 4 digits to draw
int dig2 = 0;
int dig3 = 0;
int dig4 = 0;

int bright = 1023;              //brightness (using PWM, 0-1023)
int stage = 0;                  //a modifier to track which line/arc of the digit the program is drawing currently
int digit = 1;                  //which digit the program is currently drawing (in the range of 1 to 6, 3 is colon, 6 is the processes to run in the end of draw sequence)
float sped = 0.02;              //speed at which the digits are drawn
float tsped = sped / 10;        //this speed is used by parts of the digits using parametric equations
float sqr = 30;                 //width of the projection of a digit (in millimeters)
float sep = 6;                 //separation of two digits (in millimeters)
float x = 0;                    //x coordinate of the current location in the function defining the line/arc the program is currently drawing
float y = 0;                    //y coordinate of the current location in the function defining the line/arc the program is currently drawing
float t = 0;                    //t parameter  of the current location in the function defining the line/arc the program is currently drawing
float dist = 300;               //distance of the phosphorescent screen from the yaw axis of the laser (in millimeters)

                                //8-Step sequence: Internal Motor alone: 5.625° (64 steps per revolution)
                                //Gear Reduction ratio: 1 / 64
                                //64*64 = 4096 steps per output shaft revolution
float scale = 651.8986469044;   //scale = (4096/2*PI), used in trigonometric calculations (we are using radian units for angles)

void setup() {

  pinMode(laserpin, OUTPUT);  //define the pins as outputs
  pinMode(pin1x, OUTPUT);
  pinMode(pin2x, OUTPUT);
  pinMode(pin3x, OUTPUT);
  pinMode(pin4x, OUTPUT);
  pinMode(pin1y, OUTPUT);
  pinMode(pin2y, OUTPUT);
  pinMode(pin3y, OUTPUT);
  pinMode(pin4y, OUTPUT);
  gotostep(1, 1);             //set motors to step 1
  relax();                    //write motor pins to low so we are not wasting power while doing nothing

  WiFiMulti.addAP(ssid, password);        //log into the defined WiFi

  while (WiFi.status() != WL_CONNECTED) { //wait while the WiFi connects
    delay(10);
  }
}

void loop() {

  while (!work) {   //a loop the program is in while not drawing

    relax();        //save current while not drawing

    delay(requestdelay);
    
    if ((WiFiMulti.run() == WL_CONNECTED)) {  //if WiFi is connected, start sending HTTP request

      HTTPClient http;    //initiate the http client

      http.begin(url);    //begin the http request

      int httpCode = http.GET();       //we are sending a GET request

      if (httpCode == HTTP_CODE_OK) {  //if we received HTTP 200 OK

        String payload = http.getString();                   //this is the response body
        StaticJsonBuffer<2000> jsonBuffer;                   //initiate a buffer for the JSON processor
        JsonObject& root = jsonBuffer.parseObject(payload);  //parse the JSON response body

        if (root.success()) {   //if the program successfully parsed the JSON response

          bright = root["bright"];    //get the variables from JSON and map them to the variables this program uses
          dotting = root["dotting"];
          dotdelay = root["dotdelay"];
          motordelay = root["motordelay"];
          requestdelay = root["requestdelay"];
          dist = root["dist"];
          sqr = root["sqr"];
          sep = root["sep"];
          sped = root["sped"];
          tsped = sped / 10;
          laser(root["on"]);
          int offsetx = root["offsetx"];
          int offsety = root["offsety"];
          if (offsetx != 0 || offsety != 0) {         //if the received offset  coordinates are not 0, initiate offset (move laser and define the new position as 0,0)
            
            moveto(root["offsetx"], root["offsety"]);
            globalx = 0;
            globaly = 0;
          }
          moveto(root["gotox"], root["gotoy"]);       //move the laser to the position definded in the JSON response
          dig1 = root["digit1"];
          dig2 = root["digit2"];
          dig3 = root["digit3"];
          dig4 = root["digit4"];
          work = root["work"];
        }
      }
      http.end(); //end the HTTP process
    }
  }

  if (digit == 1) {                                 //drawing the digits
    draw(dig1, -((sqr + sep) + sqr / 2 + sep), 0);
  }
  else if (digit == 2) {
    draw(dig2, -(sqr / 2 + sep), 0);
  }
  else if (digit == 3) {       //digit 3 is the colon in the middle
    if (dotting) {
      draw(69, 0, 0);
    }
    else {
      digit++;
    }
  }
  else if (digit == 4) {
    draw(dig3, (sqr / 2 + sep), 0);
  }
  else if (digit == 5) {
    draw(dig4, ((sqr + sep) + sqr / 2 + sep), 0);
  }
  else if (digit == 6) {       //digit 3 is the sequence end process, reset all the necessary variables
    x = 0;
    y = 0;
    work = false;
    digit = 1;
  }
  moveto(x, y);
}

void moveto(float gotox, float gotoy) { //method to move the laser to the coordinates given in millimeters
  
  int newgotox = (int)(scale * atan2(gotox, dist));   //calculates the global steps that match the coordinates in millimeters
  int newgotoy = (int)(scale * atan2(gotoy, dist));
  int deltax = newgotox - globalx;    //the difference between the current global step and the one the motors need to move to
  int deltay = newgotoy - globaly;
  
  while (deltax != 0 || deltay != 0) {
    
    int dirx = 0;
    int diry = 0;
    if (deltax > 0) {
      dirx = 1;
    }
    else if (deltax < 0) {
      dirx = -1;
    }
    if (deltay > 0) {
      diry = 1;
    }
    else if (deltay < 0) {
      diry = -1;
    }
    nextStep(dirx, diry);
    deltax -= dirx;
    deltay -= diry;
  }
}

void nextStep(int dirx, int diry) { //method to advance the motors by one or zero steps in the given directions
  
  currentstepx += dirx;
  currentstepy -= diry;
  if (currentstepx > 8) {
    currentstepx = 1;
  }
  else if (currentstepx < 1) {
    currentstepx = 8;
  }
  if (currentstepy > 8) {
    currentstepy = 1;
  }
  else if (currentstepy < 1) {
    currentstepy = 8;
  }
  gotostep(currentstepx, currentstepy);

  globalx += dirx;  //always update the global step when advancing
  globaly += diry;
}

void relax() {    //turn all the pins to low when necessary to save power
  
  digitalWrite(pin1x, LOW);
  digitalWrite(pin2x, LOW);
  digitalWrite(pin3x, LOW);
  digitalWrite(pin4x, LOW);
  digitalWrite(pin1y, LOW);
  digitalWrite(pin2y, LOW);
  digitalWrite(pin3y, LOW);
  digitalWrite(pin4y, LOW);
}


void gotostep(int stepnumberx, int stepnumbery) { //write the in to the values matchin the steps (stepper motors work according to the gray code)
  
  if (stepnumberx == 8) {
    digitalWrite(pin1x, LOW);
    digitalWrite(pin2x, LOW);
    digitalWrite(pin3x, LOW);
    digitalWrite(pin4x, HIGH);
  }
  else if (stepnumberx == 7) {
    digitalWrite(pin1x, LOW);
    digitalWrite(pin2x, LOW);
    digitalWrite(pin3x, HIGH);
    digitalWrite(pin4x, HIGH);
  }
  else if (stepnumberx == 6) {
    digitalWrite(pin1x, LOW);
    digitalWrite(pin2x, LOW);
    digitalWrite(pin3x, HIGH);
    digitalWrite(pin4x, LOW);
  }
  else if (stepnumberx == 5) {
    digitalWrite(pin1x, LOW);
    digitalWrite(pin2x, HIGH);
    digitalWrite(pin3x, HIGH);
    digitalWrite(pin4x, LOW);
  }
  else if (stepnumberx == 4) {
    digitalWrite(pin1x, LOW);
    digitalWrite(pin2x, HIGH);
    digitalWrite(pin3x, LOW);
    digitalWrite(pin4x, LOW);
  }
  else if (stepnumberx == 3) {
    digitalWrite(pin1x, HIGH);
    digitalWrite(pin2x, HIGH);
    digitalWrite(pin3x, LOW);
    digitalWrite(pin4x, LOW);
  }
  else if (stepnumberx == 2) {
    digitalWrite(pin1x, HIGH);
    digitalWrite(pin2x, LOW);
    digitalWrite(pin3x, LOW);
    digitalWrite(pin4x, LOW);
  }
  else if (stepnumberx == 1) {
    digitalWrite(pin1x, HIGH);
    digitalWrite(pin2x, LOW);
    digitalWrite(pin3x, LOW);
    digitalWrite(pin4x, HIGH);
  }
  
  if (stepnumbery == 8) {
    digitalWrite(pin1y, LOW);
    digitalWrite(pin2y, LOW);
    digitalWrite(pin3y, LOW);
    digitalWrite(pin4y, HIGH);
  }
  else if (stepnumbery == 7) {
    digitalWrite(pin1y, LOW);
    digitalWrite(pin2y, LOW);
    digitalWrite(pin3y, HIGH);
    digitalWrite(pin4y, HIGH);
  }
  else if (stepnumbery == 6) {
    digitalWrite(pin1y, LOW);
    digitalWrite(pin2y, LOW);
    digitalWrite(pin3y, HIGH);
    digitalWrite(pin4y, LOW);
  }
  else if (stepnumbery == 5) {
    digitalWrite(pin1y, LOW);
    digitalWrite(pin2y, HIGH);
    digitalWrite(pin3y, HIGH);
    digitalWrite(pin4y, LOW);
  }
  else if (stepnumbery == 4) {
    digitalWrite(pin1y, LOW);
    digitalWrite(pin2y, HIGH);
    digitalWrite(pin3y, LOW);
    digitalWrite(pin4y, LOW);
  }
  else if (stepnumbery == 3) {
    digitalWrite(pin1y, HIGH);
    digitalWrite(pin2y, HIGH);
    digitalWrite(pin3y, LOW);
    digitalWrite(pin4y, LOW);
  }
  else if (stepnumbery == 2) {
    digitalWrite(pin1y, HIGH);
    digitalWrite(pin2y, LOW);
    digitalWrite(pin3y, LOW);
    digitalWrite(pin4y, LOW);
  }
  else if (stepnumbery == 1) {
    digitalWrite(pin1y, HIGH);
    digitalWrite(pin2y, LOW);
    digitalWrite(pin3y, LOW);
    digitalWrite(pin4y, HIGH);
  }
  delay(motordelay);  //wait a bit
}

void laser(boolean state) {   //turns the laser on or off

  if (state) {
    analogWrite(laserpin, bright);
  }
  else {
    analogWrite(laserpin, 0);
  }
}

void one(float zerox, float zeroy) {    //how the program draws the number one
  if (stage == 0) {
    x = zerox - sqr / 4;
    y = zeroy;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    x += sped / 2;
    y = 2 * (x - zerox + sqr / 4) + zeroy;
    if (x >= zerox + sqr / 2 - sqr / 4) {
      stage++;
    }
  }
  else if (stage == 2) {
    y -= sped;
    if (y <= zeroy - sqr) {
      digit++;
      stage = 0;
      laser(false);
    }
  }
}

void two(float zerox, float zeroy) {    //how the program draws the number two
  if (stage == 0) {
    x = zerox - sqr / 2;
    y = zeroy + sqr / 2;
    t = PI;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 2 + (sqr / 2) * sin(t);
    if (t <= 0) {
      stage++;
    }
  }
  else if (stage == 2) {
    x -= 2 * sped / 3;
    y = zeroy - sqr + 3 * (x - zerox + sqr / 2) / 2;
    if (x <= zerox - sqr / 2) {
      stage++;
      t = PI;
    }
  }
  else if (stage == 3) {
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr + (sqr / 2) * sin(t) / 3;
    if (t <= 0) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void three(float zerox, float zeroy) {    //how the program draws the number three
  if (stage == 0) {
    x = zerox - sqr / 2;
    y = zeroy + sqr / 2;
    t = PI;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 2 + (sqr / 2) * sin(t);
    if (t <= -PI / 2) {
      stage++;
      t = PI / 2;
    }
  }
  else if (stage == 2) {
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    if (t <= -PI) {
      digit++;
      stage = 0;
      laser(false);
    }
  }
}

void four(float zerox, float zeroy) {    //how the program draws the number four
  if (stage == 0) {
    x = zerox - sqr / 4;
    y = zeroy + sqr;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    x -= sped / 4;
    y = zeroy + sqr + 4 * (x - (zerox - sqr / 4));
    if (x <= zerox - sqr / 2) {
      stage++;
    }
  }
  else if (stage == 2) {
    x += sped;
    if (x >= zerox + sqr / 2) {
      stage++;
      laser(false);
      x = zerox + sqr / 4;
      y = zeroy + sqr / 4;
    }
  }
  else if (stage == 3) {
    laser(true);
    y -= sped;
    if (y <= zeroy - sqr) {
      digit++;
      stage = 0;
      laser(false);
    }
  }
}

void five(float zerox, float zeroy) {    //how the program draws the number five
  if (stage == 0) {
    t = PI + .4;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t += tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    if (t >= 2 * PI) {
      stage++;
    }
  }
  else if (stage == 2) {
    y += sped;
    if (y >= zeroy - sqr / 2 + (sqr / 2) / 3) {
      stage++;
      t = 0;
    }
  }
  else if (stage == 3) {
    t += tsped;
    x = zerox + (sqr / 2) * cos(t);
    if (t < PI / 2) {
      y = zeroy - (sqr / 3) + (sqr / 2) * sin(t);
    }
    else {
      y = zeroy + (sqr / 2) * sin(t) / 3;
    }
    if (t >= PI) {
      stage++;
    }
  }
  else if (stage == 4) {
    x += sped / 4;
    y = 4 * (x - zerox + sqr / 2) + zeroy;
    if (y >= zeroy + sqr) {
      stage++;
    }
  }
  else if (stage == 5) {
    x += sped;
    if (x >= zerox + sqr / 2) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void six(float zerox, float zeroy) {    //how the program draws the number six
  if (stage == 0) {
    x = zerox - sqr / 2;
    y = zeroy - sqr / 6;
    t = PI;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 6 + (sqr / 2) * sin(t) / 3;
    if (t <= PI / 2) {
      stage++;
      t = PI / 2;
    }
  }
  else if (stage == 2) {
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    if (t <= -PI) {
      stage++;
    }
  }
  else if (stage == 3) {
    y += sped;
    if (y >= zeroy + sqr / 2) {
      stage++;
      t = PI;
    }
  }
  else if (stage == 4) {
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 2 + (sqr / 2) * sin(t);
    if (t <= .5) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void seven(float zerox, float zeroy) {    //how the program draws the number seven
  if (stage == 0) {
    x = zerox - sqr / 2;
    y = zeroy + sqr;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    x += sped;
    if (x >= zerox + sqr / 2) {
      stage++;
    }
  }
  else if (stage == 2) {
    x -= sped / 2;
    y = 2 * (x - zerox) + zeroy;
    if (y <= zeroy - sqr) {
      stage++;
      laser(false);
      x = zerox - sqr / 4;
      y = zeroy;
    }
  }
  else if (stage == 3) {
    laser(true);
    x += sped;
    if (x >= zerox + sqr / 4) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void eight(float zerox, float zeroy) {    //how the program draws the number eight
  if (stage == 0) {
    x = zerox;
    y = zeroy;
    t = PI / 2;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    if (t <= -3 * PI / 2) {
      stage++;
      t = -PI / 2;
    }
  }
  else if (stage == 2) {
    t += tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 2 + (sqr / 2) * sin(t);
    if (t >= 3 * PI / 2) {
      digit++;
      stage = 0;
      laser(false);
    }
  }
}

void nine(float zerox, float zeroy) {    //how the program draws the number nine
  if (stage == 0) {
    t = PI + .5;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t += tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    if (t >= 2 * PI) {
      stage++;
    }
  }
  else if (stage == 2) {
    y += sped;
    if (y >= zeroy + sqr / 2) {
      stage++;
      t = 0;
    }
  }
  else if (stage == 3) {
    t += tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 2 + (sqr / 2) * sin(t);
    if (t >= 3 * PI / 2) {
      stage++;
      t = 3 * PI / 2;
    }
  }
  else if (stage == 4) {
    t += tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 6 + (sqr / 2) * sin(t) / 3;
    if (t >= 2 * PI) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void zero(float zerox, float zeroy) {    //how the program draws the number zero
  if (stage == 0) {
    x = zerox - sqr / 2;
    y = zeroy + sqr / 2;
    t = PI;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy + sqr / 2 + (sqr / 2) * sin(t);
    if (t <= 0) {
      stage++;
    }
  }
  else if (stage == 2) {
    y -= sped;
    if (y <= zeroy - sqr / 2) {
      stage++;
      t = 0;
    }
  }
  else if (stage == 3) {
    t -= tsped;
    x = zerox + (sqr / 2) * cos(t);
    y = zeroy - sqr / 2 + (sqr / 2) * sin(t);
    if (t <= -PI) {
      stage++;
    }
  }
  else if (stage == 4) {
    y += sped;
    if (y >= zeroy + sqr / 2) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void dots(float zerox, float zeroy) {    //how the program draws the colon between the digits
  if (stage == 0) {
    x = zerox;
    y = zeroy + sqr / 2;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    delay(dotdelay);
    stage++;
  }
  else if (stage == 2) {
    laser(false);
    y = zeroy - sqr / 2;
    stage++;
  }
  else if (stage == 3) {
    laser(true);
    delay(dotdelay);
    stage++;
  }
  else if (stage == 4) {
    digit++;
    stage = 0;
    laser(false);
  }
}

void border(float zerox, float zeroy) {    //how the program draws the bounding boxes of the digits
  if (stage == 0) {
    x = zerox - sqr / 2;
    y = zeroy + sqr;
    stage++;
  }
  else if (stage == 1) {
    laser(true);
    x += sped;
    if (x >= zerox + sqr / 2) {
      stage++;
    }
  }
  else if (stage == 2) {
    y -= sped;
    if (y <= zeroy - sqr) {
      stage++;
    }
  }
  else if (stage == 3) {
    x -= sped;
    if (x <= zerox - sqr / 2) {
      stage++;
    }
  }
  else if (stage == 4) {
    y += sped;
    if (y >= zeroy + sqr) {
      stage = 0;
      digit++;
      laser(false);
    }
  }
}

void draw(int number, float zerox, float zeroy) {   //method that takes the number to draw and the location as arguments and handles the necessary processes

  if (number == 1) {
    one(zerox, zeroy);
  }
  else if (number == 2) {
    two(zerox, zeroy);
  }
  else if (number == 3) {
    three(zerox, zeroy);
  }
  else if (number == 4) {
    four(zerox, zeroy);
  }
  else if (number == 5) {
    five(zerox, zeroy);
  }
  else if (number == 6) {
    six(zerox, zeroy);
  }
  else if (number == 7) {
    seven(zerox, zeroy);
  }
  else if (number == 8) {
    eight(zerox, zeroy);
  }
  else if (number == 9) {
    nine(zerox, zeroy);
  }
  else if (number == 69) {  //69 draws a colon
    dots(zerox, zeroy);
  }
  else if (number == 96) {  //96 draws a bounding box
    border(zerox, zeroy);
  } 
  else {
    zero(zerox, zeroy);
  }
}
