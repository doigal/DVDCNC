/*
 Forked from original code on 23rd Feb 2015 here: https://github.com/adidax/mini_cnc_plotter_firmware
 Now uses the Adafruit Motor Driver Shield V1 (or one of the ebay clones)
 Debugging code expanded
 M18 (motor release), G4 (delay) codes added
 Adapted L.Doig, May 2015

 Mini CNC Plotter firmware, based in TinyCNC https://github.com/MakerBlock/TinyCNC-Sketches
 Send GCODE to this Sketch using gctrl.pde https://github.com/damellis/gctrl
 Convert SVG to GCODE with MakerBot Unicorn plugin for Inkscape available here https://github.com/martymcguire/inkscape-unicorn

 More information about the Mini CNC Plotter here (german, sorry): http://www.makerblog.at/2015/02/projekt-mini-cnc-plotter-aus-alten-cddvd-laufwerken/

 PIN LIST
 Motorshield V1 - X Axis on M1, Y Axis on M2, Servo on Servo1
 Pin 2 reserved for Endstop
 Pin 13 reserved for Endstop
 Pins A0-A5 for LCD
 
 TO DO LIST
 Endstops and home finding
 Make LCD optional 
 
  */


// CALIBRATION VARABLES

// Servo position for Up and Down
const int penZUp = 40;
const int penZDown = 85;

// The number of motor steps to go 1 millimeter.
// Use a test sketch to go 100 steps. Measure the length of line and calculate the steps per mm.
// Can also use this sketch with verbose set to true and a manual GCODE input thru the terminal window.
float StepsPerMillimeterX = 6.7;
float StepsPerMillimeterY = 4.8;

// Drawing robot limits, in mm
float Xmin = 0;
float Xmax = 36;
float Ymin = 0;
float Ymax = 36;
float Zmin = 0;
float Zmax = 1;

// END CALIBRATION VARABLES

// Set to true to get debug output.
boolean verbose = true;

// Set to true to enable LCD
boolean lcddisp = true;


#include <Servo.h>
#include <LiquidCrystal.h>
#include <AFMotor.h>

#define LINE_BUFFER_LENGTH 512

// Servo on PWM pin 10 (servo 1 on adafruit shield)
const int penServoPin = 10;

// Should be right for DVD steppers, but is not too important here
const int stepsPerRevolutionX = 20;
const int stepsPerRevolutionY = 20;

// create servo object to control a servo
Servo penServo;

// Initialize steppers for X- and Y-axis. X is wired up to M1 & M2 (Aka X1), Y is wired up to M3 & M4 (Aka X2)
AF_Stepper myStepperX(stepsPerRevolutionX, 1);
AF_Stepper myStepperY(stepsPerRevolutionY, 2);


// Setup the variables for positioning
float Xpos = Xmin;
float Ypos = Ymin;
float Zpos = Zmax;

/* Structures, global variables    */
struct point {
  float x;
  float y;
  float z;
};

// Current position of plothead
struct point actuatorPos;

//  Drawing settings, should be OK
float StepInc = 1;
int StepDelay = 0;
int LineDelay = 50;
int penDelay = 50;

//

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
LiquidCrystal lcd(54, 55, 56, 57, 58, 59);
#else 
LiquidCrystal lcd(14,15,16,17,18,19);
#endif

int xsteptmp = 0;
int ysteptmp = 0;


// SUPPORTED GCODE
// Needs to interpret:
//  G1 for moving
//  G4 P300 (wait 150ms)
//  M300 S30 (pen down)
//  M300 S50 (pen up)
//  Discard anything with a (
//  Discard any other command!

/**********************
 * void setup() - Initialisations
 ***********************/
void setup() {
  //  Setup
  Serial.begin( 9600 );

  penServo.attach(penServoPin);
  penServo.write(penZUp);
  delay(200);

  if (lcddisp)
  {
    lcd.begin(20, 4);
    lcd.setCursor(0, 0);
    //1234567890123456
    lcd.print("Mini CNC Plotter");
    lcd.setCursor(0, 1);
    lcd.print("L.Doig July 2015");
    delay(2000);

    lcd.setCursor(0, 0);
    lcd.print("X = 00.0     0 U");
    lcd.setCursor(0, 1);
    //0123456789012345
    lcd.print("Y = 00.0     0 P");
    delay(2000);
  }

  // Decrease if necessary
  myStepperX.setSpeed(250);
  myStepperY.setSpeed(250);

  //  Set & move to initial home position
  // TBD

  //  Notifications!!!
  Serial.println("Mini CNC Plotter alive and kicking!");
  Serial.print("X range is from ");
  Serial.print(Xmin);
  Serial.print(" to ");
  Serial.print(Xmax);
  Serial.println(" mm.");
  Serial.print("Y range is from ");
  Serial.print(Ymin);
  Serial.print(" to ");
  Serial.print(Ymax);
  Serial.println(" mm.");
}

/**********************
 * void loop() - Main loop
 ***********************/
void loop()
{
  delay(200);
  char line[ LINE_BUFFER_LENGTH ];
  char c;
  int lineIndex;
  bool lineIsComment, lineSemiColon;

  lineIndex = 0;
  lineSemiColon = false;
  lineIsComment = false;

  while (1) {

    // Serial reception - Mostly from Grbl, added semicolon support
    while ( Serial.available() > 0 ) {
      c = Serial.read();
      if (( c == '\n') || (c == '\r') ) {             // End of line reached
        if ( lineIndex > 0 ) {                        // Line is complete. Then execute!
          line[ lineIndex ] = '\0';                   // Terminate string
          if (verbose) {
            Serial.print( "Received : ");
            Serial.println( line );
          }
          processIncomingLine( line, lineIndex );
          lineIndex = 0;
        }
        else {
          // Empty or comment line. Skip block.
        }
        lineIsComment = false;
        lineSemiColon = false;
        Serial.println("ok");
      }
      else {
        if ( (lineIsComment) || (lineSemiColon) ) {   // Throw away all comment characters
          if ( c == ')' )  lineIsComment = false;     // End of comment. Resume line.
        }
        else {
          if ( c <= ' ' ) {                           // Throw away whitepace and control characters
          }
          else if ( c == '/' ) {                    // Block delete not supported. Ignore character.
          }
          else if ( c == '(' ) {                    // Enable comments flag and ignore all characters until ')' or EOL.
            lineIsComment = true;
          }
          else if ( c == ';' ) {
            lineSemiColon = true;
          }
          else if ( lineIndex >= LINE_BUFFER_LENGTH - 1 ) {
            Serial.println( "ERROR - lineBuffer overflow" );
            lineIsComment = false;
            lineSemiColon = false;
          }
          else if ( c >= 'a' && c <= 'z' ) {        // Upcase lowercase
            line[ lineIndex++ ] = c - 'a' + 'A';
          }
          else {
            line[ lineIndex++ ] = c;
          }
        }
      }
    }
  }
}

void processIncomingLine( char* line, int charNB ) {
  int currentIndex = 0;
  char buffer[ 64 ];                                 // Hope that 64 is enough for 1 parameter
  struct point newPos;

  newPos.x = 0.0;
  newPos.y = 0.0;

  //  Needs to interpret
  //  G1 for moving
  //  G4 P300 (wait 150ms)
  //  G1 X60 Y30
  //  G1 X30 Y50
  //  M300 S30 (pen down)
  //  M300 S50 (pen up)
  //  Discard anything with a (
  //  Discard any other command!

  while ( currentIndex < charNB ) {
    switch ( line[ currentIndex++ ] ) {              // Select command, if any
      case 'U':
        penUp();
        break;
      case 'D':
        penDown();
        break;
      case 'G':
        buffer[0] = line[ currentIndex++ ];          // /!\ Dirty - Only works with 2 digit commands
        //      buffer[1] = line[ currentIndex++ ];
        //      buffer[2] = '\0';
        buffer[1] = '\0';

        switch ( atoi( buffer ) ) {                  // Select G command
          case 0:                                      // G00 & G01 - Movement or fast movement. Same here
          case 1:
            {
              // /!\ Dirty - Suppose that X is before Y
              char* indexX = strchr( line + currentIndex, 'X' ); // Get X/Y position in the string (if any)
              char* indexY = strchr( line + currentIndex, 'Y' );
              if ( indexY <= 0 ) {
                newPos.x = atof( indexX + 1);
                newPos.y = actuatorPos.y;
              }
              else if ( indexX <= 0 ) {
                newPos.y = atof( indexY + 1);
                newPos.x = actuatorPos.x;
              }
              else {
                newPos.y = atof( indexY + 1);
                indexY = '\0';
                newPos.x = atof( indexX + 1);
              }
              drawLine(newPos.x, newPos.y );
              //        Serial.println("ok");
              actuatorPos.x = newPos.x;
              actuatorPos.y = newPos.y;
            }
            break;

          case 4:                                      // Pause command (assuming in format of "G4 P150")
            char* indexP = strchr( line + currentIndex, 'P' );
            int pausetime = atof( indexP + 1);
            delay(pausetime);
            Serial.print( "Delay for time in ms: " );
            Serial.print( pausetime );
            Serial.println("");
            break;

        }
        break;

      case 'M':
        buffer[0] = line[ currentIndex++ ];        // /!\ Dirty - Only works with 3 digit commands
        buffer[1] = line[ currentIndex++ ];
        buffer[2] = line[ currentIndex++ ];
        buffer[3] = '\0';
        switch ( atoi( buffer ) ) {
          case 300:  // Servo control for pen
            {
              char* indexS = strchr( line + currentIndex, 'S' );
              float Spos = atof( indexS + 1);
              //          Serial.println("ok");
              if (Spos == 30) {
                penDown();
              }
              if (Spos == 50) {
                penUp();
              }
              break;
            }

          case 114:                                // M114 - Repport position
            Serial.print( "Absolute position (mm) : X = " );
            Serial.print( actuatorPos.x );
            Serial.print( "  -  Y = " );
            Serial.println( actuatorPos.y );
            break;

          case 18:                                // M18 Disengage Drives
            {
              Serial.println( "Disengaging motors" );
              myStepperX.release();
              myStepperY.release();
              break;
            }

          default:
            Serial.print( "Command not recognized : M");
            Serial.println( buffer );
        }
    }
  }

}


/*********************************
 * Draw a line from (x0;y0) to (x1;y1).
 * Bresenham algo from https://www.marginallyclever.com/blog/2013/08/how-to-build-an-2-axis-arduino-cnc-gcode-interpreter/
 * int (x1;y1) : Starting coordinates
 * int (x2;y2) : Ending coordinates
 **********************************/
void drawLine(float x1, float y1) {

  if (verbose)
  {
    Serial.print("Moving fm position (mm):  ");
    Serial.print(Xpos / StepsPerMillimeterX);
    Serial.print(", ");
    Serial.print(Ypos / StepsPerMillimeterY);
    Serial.println("");

    Serial.print("Moving fm position (st):  ");
    Serial.print(Xpos);
    Serial.print(", ");
    Serial.print(Ypos);
    Serial.println("");
  }

  //  Bring instructions within limits
  if (x1 >= Xmax) {
    x1 = Xmax;
  }
  if (x1 <= Xmin) {
    x1 = Xmin;
  }
  if (y1 >= Ymax) {
    y1 = Ymax;
  }
  if (y1 <= Ymin) {
    y1 = Ymin;
  }

  if (verbose)
  {
    Serial.print("Moving to position (mm): ");
    Serial.print(x1);
    Serial.print(", ");
    Serial.print(y1);
    Serial.println("");
  }

  //  Convert coordinates to steps
  x1 = (int)(x1 * StepsPerMillimeterX);
  y1 = (int)(y1 * StepsPerMillimeterY);
  float x0 = Xpos;
  float y0 = Ypos;

  if (verbose)
  {
    Serial.print("Moving to position (st): ");
    Serial.print(x1);
    Serial.print(", ");
    Serial.print(y1);
    Serial.println("");
  }


  //  Let's find out the change for the coordinates
  long dx = abs(x1 - x0);
  long dy = abs(y1 - y0);
  int sx = x0 < x1 ? StepInc : -StepInc;
  int sy = y0 < y1 ? StepInc : -StepInc;

  long i;
  long over = 0;

  if (verbose)
  {
    Serial.print("Delta in Steps: ");
    Serial.print(dx);
    Serial.print(", ");
    Serial.print(dy);
    Serial.println("");

    Serial.print("Direction vector: ");
    Serial.print(sx);
    Serial.print(", ");
    Serial.print(sy);
    Serial.println("");
  }

  xsteptmp = x0;
  ysteptmp = y0;

  if (dx > dy) {
    for (i = 0; i < dx; ++i) {
      if (sx > 0) {
        myStepperX.step(1, FORWARD, DOUBLE);
        xsteptmp = xsteptmp + 1;
        if (lcddisp)
        {
          LCDUpdateMM(xsteptmp, ysteptmp);
        }
      }
      else {
        myStepperX.step(1, BACKWARD, DOUBLE);
        xsteptmp = xsteptmp - 1;
        if (lcddisp)
        {
          LCDUpdateMM(xsteptmp, ysteptmp);
        }
      }
      over += dy;
      if (over >= dx) {
        over -= dx;
        if (sy > 0) {
          myStepperY.step(1, FORWARD, DOUBLE);
          ysteptmp = ysteptmp + 1 ;
          if (lcddisp)
          {

            LCDUpdateMM(xsteptmp, ysteptmp);
          }
        }
        else {
          myStepperY.step(1, BACKWARD, DOUBLE);
          ysteptmp = ysteptmp - 1;
          if (lcddisp)
          {
            LCDUpdateMM(xsteptmp, ysteptmp);
          }
        }
      }
      delay(StepDelay);
    }
  }
  else {
    for (i = 0; i < dy; ++i) {
      if (sy > 0) {
        myStepperY.step(1, FORWARD, DOUBLE);
        ysteptmp = ysteptmp + 1 ;
        if (lcddisp)
        {
          LCDUpdateMM(xsteptmp, ysteptmp);
        }
      }
      else {
        myStepperY.step(1, BACKWARD, DOUBLE);
        ysteptmp = ysteptmp - 1 ;
        if (lcddisp)
        {
          LCDUpdateMM(xsteptmp, ysteptmp);
        }
      }
      over += dx;
      if (over >= dy) {
        over -= dy;
        if (sx > 0) {
          myStepperX.step(1, FORWARD, DOUBLE);
          xsteptmp = xsteptmp + 1;
          if (lcddisp)
          {
            LCDUpdateMM(xsteptmp, ysteptmp);
          }
        }
        else {
          myStepperX.step(1, BACKWARD, DOUBLE);
          xsteptmp = xsteptmp - 1;
          if (lcddisp)
          {
            LCDUpdateMM(xsteptmp, ysteptmp);
          }
        }
      }
      delay(StepDelay);
    }
  }


  //  Delay before any next lines are submitted
  delay(LineDelay);
  //  Update the positions
  Xpos = x1;
  Ypos = y1;
}

//  Raises pen and updates display
void penUp() {
  penServo.write(penZUp);
  delay(LineDelay);
  Zpos = Zmax;

  if (lcddisp)
  {
    lcd.setCursor(15, 0);
    lcd.print("U");
    lcd.setCursor(15, 1);
    lcd.print("P");
  }

  if (verbose) {
    Serial.println("Pen up!");
  }
}
//  Lowers pen and updates display
void penDown() {
  penServo.write(penZDown);
  delay(LineDelay);
  Zpos = Zmin;

  if (lcddisp)
  {
    lcd.setCursor(15, 0);
    lcd.print("D");
    lcd.setCursor(15, 1);
    lcd.print("N");
  }

  if (verbose) {
    Serial.println("Pen down.");
  }
}

/*****************************************
 * Updates the 16x2 LCD with the location
 * int (xstep;ystep) : Current position
 *****************************************/

void LCDUpdateMM(int xsteps, int ysteps) {
  float xmm = ((xsteps / StepsPerMillimeterX) * 10) / 10;
  float ymm = (ysteps / StepsPerMillimeterY);


  lcd.setCursor(4, 0);
  if ( xmm < 10 ) {
    lcd.print(" ");
  }
  lcd.print(xmm);
  lcd.setCursor(11, 0);
  if ( xsteps < 100 ) {
    lcd.print(" ");
    if ( xsteps < 10 ) {
      lcd.print(" ");
    }
  }
  lcd.print(xsteps);

  lcd.setCursor(4, 1);
  if ( ymm < 10 ) {
    lcd.print(" ");
  }
  lcd.print(ymm);
  lcd.setCursor(11, 1);
  if ( ysteps < 100 ) {
    lcd.print(" ");
    if ( ysteps < 10 ) {
      lcd.print(" ");
    }
  }
  lcd.print(ysteps);
}

