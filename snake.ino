#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// TFT PINS
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8
#define TFT_MISO 2
#define TFT_MOSI 3
#define TFT_CLK 4

// JOYSTICK
#define SW 5

// ULTRASONIC
#define trigger1 5;
#define echo1 7;
#define trigger2 11;
#define echo2 12;

// CONSTANTS
#define bSize 12
#define SLOW 450
#define NORMAL 250
#define FAST 150
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define TRAJ 2
#define width 320
#define height 240
#define X 0
#define Y 1
#define ALL -1
#define HEAD 1
#define TAIL 0

// AUDIO
byte STARTAUDIO = 97;
byte EATAUDIO = 98;
byte DEATHAUDIO = 99;

// TFT panel
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// GAME VALUES
unsigned long time;
bool GAME_START;
int GAME_SPEED = NORMAL;
int SPEEDUP_MULT = 0;
int TRAJECTORY;
int FOOD[2];
int SNAKE[100][3]; // not expecting them to eat more than 100 food before dying hehe
int sensorX, sensorY;

// <----------------------------- HELPER FUNCTIONS --------------------------------->
void moveNext();
void startGame();
void gameOver();

void setGameSpeed(int game_speed);
void setTrajectory(char dir);
void setHead();
void clearTail();
void updateBody();
void cookFood();
bool elongate();
bool didCollide();

void track();
int getX();
int getY();
bool didSelect();
int calc_dist(int trigger, int echo);
int getPosX();
int getPoxY();

void drawText(int coords[][3], String lines[], int color, int numlines, int index);
// <----------------Destin is the absolute greatest person alive.------------------->


/*
  initialize pins and tft stuff, -> initialize game
*/
void setup() {
  pinMode(SW, INPUT_PULLUP);
  pinMode(trigger1, OUTPUT);
  pinMode(echo1, INPUT);
  pinMode(trigger2, OUTPUT);
  pinMode(echo2, INPUT);
  Serial.begin(9600);
  randomSeed(millis());

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  gameStart();
}

/*
  arduino loop
*/
void loop()
{
  track();
  setTrajectory();
  if ( millis() - time >= GAME_SPEED - (SPEEDUP_MULT * 5) && GAME_START ) {
    moveNext();
    time = millis();
  }
}

/*!
  @brief main game driver, determines the logic of the game (make snake longer, end game, etc...)
*/
void moveNext() {
  if (elongate()) {
    cookFood();
  } else {
    clearTail();
    updateBody();
  }
  setHead();

  if (!didCollide()) {
    tft.fillRect(SNAKE[HEAD][X], SNAKE[HEAD][Y], bSize, bSize, ILI9341_GREEN);
  } else {
    gameOver();
  }
}

/*!
  @brief initializes the game, provides option menu to change game speed
*/
void gameStart() {
  // create data for the start screen text
  int title_coords[][3] = {{95, 90, 4}, {95, 122, 2}, {95, 138, 1}};
  String title_text[] = {{"Snake!"}, {"by A1"}, {"(p.s. Destin is awesome)"}};
  int start_coords[][3] = {{100, 186, 1}, {100, 194, 1}};
  String start_text[] = {{"options"}, {"start"}};
  int cursor_coords[][3] = {{90, 186, 1}, {90, 194, 1}, {90, 202, 1}};
  String cursor_text[] = {{">"}, {">"}, {">"}};
  int options_coords[][3] = {{100, 186, 1}, {100, 194, 1}, {100, 202, 1}};
  String options_text[] = {{"slow"}, {"normal"}, {"fast"}};

  // draw the start screen text
  drawText(title_coords, title_text, ILI9341_WHITE, 3, ALL);
  drawText(start_coords, start_text, ILI9341_WHITE, 2, ALL);
  drawText(cursor_coords, cursor_text, ILI9341_WHITE, 2, 1);

  bool enter_menu = false;

  while ( !GAME_START ) {   // start game or go into option menu
    while (!enter_menu && didSelect() );
    enter_menu = true;

    if ( getY() > 0 ) {
      drawText(cursor_coords, cursor_text, ILI9341_BLACK, 2, 1);
      drawText(cursor_coords, cursor_text, ILI9341_WHITE, 2, 0);
    } else if ( getY() < 0 ) {
      drawText(cursor_coords, cursor_text, ILI9341_BLACK, 2, 0);
      drawText(cursor_coords, cursor_text, ILI9341_WHITE, 2, 1);
    }

    if ( didSelect() ) {
      if (getPosY() == cursor_coords[1][Y]) {   // start game
        drawText(title_coords, title_text, ILI9341_BLACK, 3, ALL);
        drawText(start_coords, start_text, ILI9341_BLACK, 2, ALL);
        drawText(cursor_coords, cursor_text, ILI9341_BLACK, 3, ALL);
    
        GAME_START = true;
        TRAJECTORY = RIGHT;
        Serial.write(STARTAUDIO);
        SNAKE[HEAD][X] = (width / bSize / 2) * bSize;
        SNAKE[HEAD][Y] = (height / bSize / 2) * bSize;
        tft.setCursor(SNAKE[HEAD][X], SNAKE[HEAD][Y]);
        cookFood();
        time = millis();
      } else if (getPosY() == cursor_coords[0][Y]) {    // go to options
        drawText(start_coords, start_text, ILI9341_BLACK, 2, ALL);
        drawText(cursor_coords, cursor_text, ILI9341_BLACK, 2, ALL);
        drawText(options_coords, options_text, ILI9341_WHITE, 3, ALL);

        time = millis();
        bool j_used = false;
        bool can_return = false;
        enter_menu = false;
        int option;

        if (GAME_SPEED == SLOW) {
          option = 0;
        } else if (GAME_SPEED == NORMAL) {
          option = 1;
        } else if (GAME_SPEED == FAST) {
          option = 2;
        }

        while (!can_return) {
          while (!enter_menu && didSelect());
          enter_menu = true;

          if ( getY() < 50 && getY() > -50 ) {
            j_used = false;
          } else if ( getY() > 200 && !j_used ) {
            if (option > 0) {
              drawText(cursor_coords, cursor_text, ILI9341_BLACK, 3, option);
              option--;
              j_used = true;
            }
          } else if ( getY() < 0 && !j_used) {
            if ( option < 2 ) {
              drawText(cursor_coords, cursor_text, ILI9341_BLACK, 3, option);
              option++;
              j_used = true;
            }
          }
          drawText(cursor_coords, cursor_text, ILI9341_WHITE, 3, option);

          if ( didSelect() ) {
            if (option == 0) {
              setGameSpeed(SLOW);
            } else if ( option == 1 ) {
              setGameSpeed(NORMAL);
            } else if ( option == 2 ) {
              setGameSpeed(FAST);
            }

            can_return = true;
            enter_menu = false;
            drawText(options_coords, options_text, ILI9341_BLACK, 3, ALL);
            drawText(cursor_coords, cursor_text, ILI9341_BLACK, 3, ALL);
            drawText(start_coords, start_text, ILI9341_WHITE, 2, ALL);
            drawText(cursor_coords, cursor_text, ILI9341_WHITE, 2, 0);
          }
        }
      }
    }
  }
}

/*!
  @brief ends the game, resets game values
*/
void gameOver() {
  Serial.write(DEATHAUDIO);
  GAME_START = false;
  tft.fillRect(FOOD[X], FOOD[Y], bSize, bSize, ILI9341_BLACK);

  // draw game over
  int game_over_coords[][3] = {{52, 93, 4}, {55, 90, 4}};
  String game_over_text[] = {{"GAME OVER"}, {"GAME OVER"}};
  drawText(game_over_coords, game_over_text, 0x6000, 2, 0);
  drawText(game_over_coords, game_over_text, ILI9341_RED, 2, 1);
  delay(3000);

  // remove everything on screen and reset values
  drawText(game_over_coords, game_over_text, ILI9341_BLACK, 2, ALL);
  for ( int i = 0; i < HEAD + 1; i++) {
    tft.fillRect(SNAKE[i][X], SNAKE[i][Y], bSize, bSize, ILI9341_BLACK);
    SNAKE[i][X] = 0;
    SNAKE[i][Y] = 0;
  }
  HEAD = 1;
  SPEEDUP_MULT = 0;

  gameStart();
}

/*!
  @brief sets the game speed
  @param game_speed desired speed (SLOW, NORMAL, FAST)
*/
void setGameSpeed(int game_speed) {
  GAME_SPEED = game_speed;
}

/*!
  @brief sets the trajectory of the snake depending on where the joystick is pointed
*/
void setTrajectory() {
  int x = getX();
  int y = getY();
  if ( y > 200 && SNAKE[HEAD][TRAJ] != DOWN ) {
    TRAJECTORY = UP;
  } else if ( y < -200 && SNAKE[HEAD][TRAJ] != UP ) {
    TRAJECTORY = DOWN;
  } else if ( x > 200 && SNAKE[HEAD][TRAJ] != LEFT ) {
    TRAJECTORY = RIGHT;
  } else if ( x < -200 && SNAKE[HEAD][TRAJ] != RIGHT ) {
    TRAJECTORY = LEFT;
  }
}

/*!
  @brief sets the next location for the snake to move
*/
void setHead() {
  if (TRAJECTORY == UP) {
    SNAKE[HEAD][Y] = getPosY() - bSize;
  } else if ( TRAJECTORY == DOWN ) {
    SNAKE[HEAD][Y] = getPosY() + bSize;
  } else if ( TRAJECTORY == RIGHT ) {
    SNAKE[HEAD][X] = getPosX() + bSize;
  } else if ( TRAJECTORY == LEFT ) {
    SNAKE[HEAD][X] = getPosX() - bSize;
  }
  SNAKE[HEAD][TRAJ] = TRAJECTORY;
  tft.setCursor(SNAKE[HEAD][X], SNAKE[HEAD][Y]);
}

/*!
  @brief clears the tail when the snake moves
*/
void clearTail() {
  tft.fillRect(SNAKE[TAIL][X], SNAKE[TAIL][Y], bSize, bSize, ILI9341_BLACK);
}

/*!
  @brief updates the data of each "block" of the snake body (x-y coord, trajectory)
*/
void updateBody() {
  for (int i = 0; i < HEAD; i++) {
    SNAKE[i][X] = SNAKE[i + 1][X];
    SNAKE[i][Y] = SNAKE[i + 1][Y];
    SNAKE[i][TRAJ] = SNAKE[i + 1][TRAJ];
  }
}

/*!
  @brief generates a new food block in a random location
*/
void cookFood() {
  bool willnotwork;
  do {
    FOOD[X] = random(0, width / bSize) * bSize;
    FOOD[Y] = random(0, height / bSize) * bSize;
    willnotwork = false;
    for (int i = 0; i < HEAD + 1; i++) {
      if (FOOD[X] == SNAKE[i][X] || FOOD[Y] == SNAKE[i][Y]) {
        willnotwork = true;
      }
    }
  }
  while (willnotwork);
  tft.fillRect(FOOD[X], FOOD[Y], bSize, bSize, ILI9341_WHITE);
}

/*!
  @brief makes the snake longer
*/
bool elongate() {
  if ( getPosX() == FOOD[0] && getPosY() == FOOD[1] ) {
    HEAD++;
    SPEEDUP_MULT++;
    SNAKE[HEAD][X] = getPosX();
    SNAKE[HEAD][Y] = getPosY();
    return true;
  }
  return false;
}

/*!
  @brief sees if the snake has collided with the screen boundaries or with itself
*/
bool didCollide() {
  if ( ( SNAKE[HEAD][X] >= width || SNAKE[HEAD][X] <= -bSize ) || ( SNAKE[HEAD][Y] >= height || SNAKE[HEAD][Y] <= -bSize ) ) {
    return true;
  }
  for (int i = 0; i < HEAD; i++) {
    if ( SNAKE[HEAD][X] == SNAKE[i][X] && SNAKE[HEAD][Y] == SNAKE[i][Y] ) {
      return true;
    }
  }
  return false;
}

/*!
  @brief tracks and updates the x and y values for the ultrasonic sensors
*/
void track() {
  float dist=81, theta, d1, d2;   // dist -> distance between two sensors

  // pins for sensor 1 need to be uploaded
  digitalWrite(5, LOW);
  delayMicroseconds(2);
  digitalWrite(5, HIGH);
  delayMicroseconds(10);
  digitalWrite(5, LOW);
  d1 = pulseIn(7, HIGH)*343/2000;
  delay(10);

  // pins for sensor 2 need to be uploaded
  digitalWrite(11, LOW);
  delayMicroseconds(2);
  digitalWrite(11, HIGH);
  delayMicroseconds(10);
  digitalWrite(11, LOW);
  d2 = pulseIn(12, HIGH)*343/2000;

  theta=acos((((d1*d1)+(dist*dist)-(d2*d2)))/(2*d1*dist));

  if( theta < 3 && theta > 0 ) {    // to avoid impossible values
    sensorY=d1*cos(theta) + dist/2; // y coordinate 
    sensorX=d1*sin(theta);          // X coordinate
  } else {
    Serial.println("bad track");
  }
  delay(10);
}

/*!
  @brief returns the adjusted x-value of the sensor
*/
int getX() {
  return map(sensorX, 0, 1023, -512, 512);
}

/*!
  @brief returns the adjusted y-value of the sensor
*/
int getY() {
  return map(sensorY, 0, 1023, -512, 512);
}

/*!
  @brief returns if the user moves closer to the sensors (in order to select something)
*/
bool didSelect() {
  int distL = calc_dist(trigger1, echo1);
  int distR = calc_dist(trigger2, echo2);

  if ((distL >40 && distR>40) && (distL <60 && distR<60)) {   // detect both hands
    return true;
  }
  return false;
}

/*!
  @brief calculates the distance from the object to the sensor
  @param trigger sound emitter
  @param echo sound listener
*/
int calc_dist(int trigger, int echo) {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);

  long time_taken = pulseIn(echo, HIGH);
  int dist = time_taken*0.034/2;
  if (dist > 60) {
    return 60;
  }
  return dist;
}

/*!
  @brief gets the current screen cursor position on the x-axis
*/
int getPosX() {
  return tft.getCursorX();
}

/*!
  @brief gets the current screen cursor position on the y-axis
*/
int getPosY() {
  return tft.getCursorY();
}

/*!
  @brief helper function to draw text because doing it all by hand is a pain
  @param coords 2d array with x-y coords and text size
  @param lines array of strings to use
  @param color color to make the text
  @param numlines number of lines present in coords and lines
  @param index optional indicator if user wants to only draw a specific text from the arrays
*/
void drawText(int coords[][3], String lines[], int color, int numlines, int index) {
  if ( index == ALL ) {
    for (int i = 0; i < numlines; i++) {
      tft.setTextColor(color);
      tft.setTextSize(coords[i][2]);
      tft.setCursor(coords[i][X], coords[i][Y]);
      tft.print(lines[i]);
    }
  } else {
    tft.setTextColor(color);
    tft.setTextSize(coords[index][2]);
    tft.setCursor(coords[index][X], coords[index][Y]);
    tft.print(lines[index]);
  }
}
