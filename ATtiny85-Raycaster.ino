#include <ssd1306xled.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_HEIGHT_HALF 32
#define MAP_SIZE_X 8
#define DRAW_DISTANCE 8
#define DOUBLE_PI 6.283185
#define FOV 1.0472
#define HALF_FOV 0.523599
#define RENDER_RESOLUTION 3

#define TINYJOYPAD_LEFT  (analogRead(A0)>=750)&&(analogRead(A0)<950)
#define TINYJOYPAD_RIGHT (analogRead(A0)>500)&&(analogRead(A0)<750)
#define TINYJOYPAD_DOWN (analogRead(A3)>=750)&&(analogRead(A3)<950)
#define TINYJOYPAD_UP  (analogRead(A3)>500)&&(analogRead(A3)<750)
#define BUTTON_DOWN (digitalRead(1)==0)
#define BUTTON_UP (digitalRead(1)==1)

#define PATTERN_FULL 0xff
#define PATTERN_ODD 0x55
#define PATTERN_EVEN 0xaa



class Player {
  public:
    double posX;
    double posY;
    double orientation;
    Player() {
      posX = 182.4614;
      posY = 121.0115;
      orientation = 0.7494;
    }
};

struct HitData {
  double x;
  double y;
  double dist;
  int hitIndex;
  bool isHorizontal;
  HitData(double a_x, double a_y, double a_dist, int a_hitIndex, bool a_isHorizontal) {
    x = a_x;
    y = a_y;
    dist = a_dist;
    hitIndex = a_hitIndex;
    isHorizontal = a_isHorizontal;
  }
};

// MAP
const unsigned char MAP[] {
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 1, 1, 0, 1, 1, 1,
  1, 0, 1, 0, 0, 0, 0, 1,
  1, 0, 1, 1, 0, 0, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 1, 0, 0, 0, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1
};

// VARIABLES
Player *player = new Player();
unsigned long oldTime = 0;

// METHODS
void setup() {
  SSD1306.ssd1306_init();

  pinMode(1, INPUT);
  digitalWrite(4, LOW);
  pinMode(4, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A3, INPUT);

  // vertical addressing mode
  SSD1306.ssd1306_send_command_start();
  SSD1306.ssd1306_send_byte(0x20);
  SSD1306.ssd1306_send_byte(0x01);
  SSD1306.ssd1306_send_command_stop();

  // offset horizontal
  SSD1306.ssd1306_send_command_start();
  SSD1306.ssd1306_send_byte(0x21);
  SSD1306.ssd1306_send_byte(0x01);
  SSD1306.ssd1306_send_byte(0xff);
  SSD1306.ssd1306_send_command_stop();

  // offset vertical
  SSD1306.ssd1306_send_command_start();
  SSD1306.ssd1306_send_byte(0xD3);
  SSD1306.ssd1306_send_byte(56);
  SSD1306.ssd1306_send_command_stop();
}


void loop() {
  unsigned long deltaTime = calculateDeltaTime();
  update(deltaTime);
  draw();
}

unsigned long calculateDeltaTime() {
  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - oldTime;
  oldTime = currentTime;
  return deltaTime;
}

void update(unsigned long deltaTime) {
  if (TINYJOYPAD_LEFT) {
    updatePlayerOrientation(deltaTime * -.001);
  }
  if (TINYJOYPAD_RIGHT) {
    updatePlayerOrientation(deltaTime * .001);
  }
  if (TINYJOYPAD_UP) {
    player->posX += cos(player->orientation) * deltaTime * .1;
    player->posY += sin(player->orientation) * deltaTime * .1;
  }
  else if (TINYJOYPAD_DOWN) {
    player->posX -= cos(player->orientation) * deltaTime * .1;
    player->posY -= sin(player->orientation) * deltaTime * .1;
  }
}

void updatePlayerOrientation(double radians) {
  player->orientation += radians;

  while (player->orientation > DOUBLE_PI) {
    player->orientation -= DOUBLE_PI;
  }

  while (player->orientation < 0) {
    player->orientation += DOUBLE_PI;
  }
}

void draw() {
  SSD1306.ssd1306_setpos(0, 0);
  for (uint8_t i = 0; i < 128; ++i){
    int8_t height = getRayCastedWallHeight(i);
    uint8_t pattern = PATTERN_FULL;
    
    if (height < 0) {
      height = -height;
      pattern = (i%2 == 0) ? PATTERN_EVEN : PATTERN_ODD;
    }
    
    renderVerticalRayCaster(height, pattern);
  }
}

void renderVerticalRayCaster(uint8_t height, uint8_t pattern){
  // If height is zero, we just fill it with black
  if (height == 0){
    SSD1306.ssd1306_send_data_start();
    for (uint8_t i = 0; i < 8; ++i){
      SSD1306.ssd1306_send_byte(0x00);
    }
    SSD1306.ssd1306_send_data_stop();
    return;
  }
  
  uint8_t emptySegmentsSize = (SCREEN_HEIGHT - height) / 2;
  uint8_t emptySegmentsSizePlusHeight = emptySegmentsSize + height;
  
  uint8_t startSegment = emptySegmentsSize / 8;
  uint8_t endSegment = emptySegmentsSizePlusHeight / 8;

  uint8_t startSubPixel = emptySegmentsSize % 8;
  uint8_t endSubPixel = emptySegmentsSizePlusHeight % 8;

  SSD1306.ssd1306_send_data_start();
  for (uint8_t i = 0; i < 8; ++i){
    uint8_t data = 0x00;
    
    if (i == startSegment == endSegment){
      data = ((0xff >> (8-endSubPixel)) << startSubPixel);
    }
    else if (i == startSegment){
      data = 0xff << startSubPixel;
    }
    else if (i == endSegment){
      data = 0xff >> (8-endSubPixel);
    }
    else if (i > startSegment && i < endSegment){
      data = 0xff;
    }

    SSD1306.ssd1306_send_byte(data & pattern);
  }
  SSD1306.ssd1306_send_data_stop();
}

uint8_t lastWallHeight;
int8_t getRayCastedWallHeight(uint8_t x) {
  if (x > 0 && (x % RENDER_RESOLUTION != 0)) {
    return lastWallHeight;
  }

  // Calculate the angle of the ray for this column
  double radians = ((player->orientation - HALF_FOV) + (( (double)x / SCREEN_WIDTH ) * FOV));

  while (radians < 0) {
    radians += DOUBLE_PI;
  }
  while (radians > DOUBLE_PI) {
    radians -= DOUBLE_PI;
  }

  // Calculate the angle between player orientation and the current ray angle for fish eye correction
  double fishEyeAngle = player->orientation - radians;
  while (fishEyeAngle < 0) {
    fishEyeAngle += DOUBLE_PI;
  }
  while (fishEyeAngle > DOUBLE_PI) {
    fishEyeAngle -= DOUBLE_PI;
  }

  // Cast a ray, which returns the hit position, square distance and index
  HitData *hit = rayCast(radians);

  if (hit == NULL) {
    return lastWallHeight;
  }

  double result = min(max((SCREEN_HEIGHT * 40) / (sqrt(hit->dist) * cos(fishEyeAngle)), 0), SCREEN_HEIGHT);
  result = hit->isHorizontal ? result : -result;
  free(hit);
  lastWallHeight = result;
  return result;
}

HitData* rayCast(double radians) {
  HitData *result = NULL;

  // For horizontal RayCast hit
  for (int i = 1; i < DRAW_DISTANCE; i++) {
    result = horizontalRayCast(60.0 * i, radians);

    if (result != NULL) {
      break;
    }
  }

  // For vertical RayCast hit
  for (int i = 1 ; i < DRAW_DISTANCE; i++) {
    HitData *ray = verticalRayCast(60.0 * i, radians);

    if (ray != NULL) {
      bool isCloser = (ray->dist < result->dist);
      free(isCloser ? result : ray);
      return isCloser ? ray : result;
    }
  }

  return result;
}

HitData* horizontalRayCast(double distance, double radians) {
  double playerPosMod = fmod((player->posY - 30.0), 60.0);
  double vecY = radians < PI ? (distance - playerPosMod) : (((playerPosMod * -1.0) - distance) + 59.0);

  double sL = tan(radians - HALF_PI) * vecY;

  return createHitData(player->posX - sL, player->posY + vecY, true);
}

HitData* verticalRayCast(double distance, double radians) {
  double playerPosMod = fmod((player->posX - 30.0), 60.0);
  double vecX = (radians < HALF_PI || radians > 3.0 * HALF_PI) ? (distance - playerPosMod) : (((playerPosMod * -1.0) - distance) + 59.0);

  double sL = ctg(radians - HALF_PI) * vecX;

  return createHitData(player->posX + vecX, player->posY - sL, false);
}

HitData* createHitData(double hitX, double hitY, bool isHorizontal) {
  //return new HitData(1.0, 1.0, 5.0, 1,false);
  int intX = round(hitX / 60.0);
  int intY = round(hitY / 60.0);
  int index =  (intY * MAP_SIZE_X) + intX;
  double dist = getDist(player->posX, player->posY, hitX, hitY);
  return MAP[index] == 1 ? new HitData(hitX, hitY, dist, index, isHorizontal) : NULL;
}

double getDist(double x1, double y1, double x2, double y2) {
  double a = x1 - x2;
  double b = y1 - y2;
  return a * a + b * b;
}

double ctg(double x) {
  return 1.0 / tan(x);
}
