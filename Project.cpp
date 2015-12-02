//#includes up here bro
#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library



//standard GLOBAL VARIABLES
//standard U of A library settings, assuming Atmel Mega SPI pins
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

//standard joystick pins
const int VERT = 0;  // analog input
const int HORIZ = 1; // analog input
const int SEL = 9;   // digital input 

//Define the tft
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

typedef struct {//size is 50 by default
    int* x[50];
    int* y[50];
    int head = 4;
    int tail = 0;
    int length = 5;
    char delay = 'Y'; //For eating the point dot, Y is yes, N is no
} Snake;

void sendChar(char msg){
  //Start = S
  //Sync = N
  //Up = U
  //Down = D
  //Left = L
  //Right = R
  
  Serial3.write(msg);
  //debug
  //Serial.print("msg sent: "); Serial.println('msg');
}//Done

bool listen(char c){
  if(waitOnSerial3(1,1000)){
    char msg = Serial3.read();
    //debug
    //Serial.print("msg heard: "); Serial.println('msg');
    if(msg == c){
      return true;
    }
  }
  return false;
}//Done

void startUp(){
  tft.setTextColor(0xFFFF, 0x0000);
  
  //Draw boundaries
  fillRect(16, 0, 4, 128, 0xFFFF);
  fillRect(140, 0, 4, 128, 0xFFFF);
  fillRect(0, 20, 120, 4, 0xFFFF);
  fillRect(108, 20, 120, 4, 0xFFFF);
  
  //Initialize snakes
  //initSnake();
  
  //Draw player ID
  tft.setCursor(29,10);
  tft.print("HOST SNAKE");
  tft.setCursor(131,70);
  tft.print("CLIENT SNAKE");
  
  //pointDot();
  int x,y;
  pointDot(&x,&y);
  
  //3,2,1,GO
  tft.setCursor(0,62);
  tft.print("3");
  delay(900);
  fillRect(0, 62, 4, 8, 0x0000);
  tft.print("2");
  delay(900);
  fillRect(0, 62, 4, 8, 0x0000);
  tft.print("1");
  delay(900);
  tft.setCursor(0,60);
  fillRect(0, 62, 4, 8, 0x0000);
  
  //Clear player ID
  fillRect(29, 10, 40, 8, 0x0000);
  fillRect(131, 70, 48, 8, 0x0000);
  
  tft.print("GO");
  fillRect(0, 60, 8, 8, 0x0000);
  delay(50);
  
  //Start game
  snake(&x,&y);
}//Almost

int randomDotX(){
    int x = analogRead(3) * 4;
    //Loop again, but range is only 0 to 4
    x += analogRead(3)/2;
    if(x > 40){
        x = 40; 
    }
    return x;
}//Done

int randomDotY(){
    int y = analogRead(3) * 4;
    //Loop again, but range is only 0 to 4
    y += analogRead(3)/2;
    if(y > 40){
        y = 40; 
    }
    return y;
}//Done

void pointDot(int* x, int* y){//get rand spot draw dot
  *x = randomDotX();
  *y = randomDotY();
  fillCircle(((x*3))+3, ((y*3))+19, 2, 0xFFFF);
}//Done

void menuSrv(){
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setCursor(0,0);
  tft.print("host");
  tft.setCursor(54,76);
  tft.print("START");
  while(true){ //when not pressed
    if(digitalRead(SEL) == 1){
      sendChar('S');
      startUp();
    }
    delay(50);
  }
}//Done

void menuCli(){
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setCursor(0,0);
  tft.print("client");
  tft.setCursor(50,72);
  tft.print("Waiting");
  tft.setCursor(58,70);
  tft.print("For Host");
  bool start = FALSE;
  while(start == FALSE){
    start = listen('S');
  }
  startUp();
}//Done

//Winlose is called inside time and collision, val decides winner: 0 is server, 1 is client
void winLose(bool val){
  tft.fillScreen(0x0000);
  if(val == TRUE){//Host? cli?
      tft.setCursor(56,72);
      tft.print("Host");
      tft.setCursor(54,80);
      tft.print("Wins!");
  }else{
      tft.setCursor(56,72);
      tft.print("Client");
      tft.setCursor(54,80);
      tft.print("Wins!");
  }
  //Tell to press reset key
  tft.setCursor(0,152);
  tft.print("Press Reset");
}//Done

bool collision(Snake* snakeCli, Snake* snakeSrv){
    //Check for walls
    if((snakeCli->x[snakeCli->head]<0)
    ||(snakeCli->x[snakeCli->head]>39)
    ||(snakeCli->y[snakeCli->head]<0)
    ||(snakeCli->y[snakeCli->head]>39)){
        //Snake client hit a wall
        winLose(0);
        return true;
    }
    if((snakeSrv->x[snakeSrv->head]<0)
    ||(snakeSrv->x[snakeSrv->head]>39)
    ||(snakeSrv->y[snakeSrv->head]<0)
    ||(snakeSrv->y[snakeSrv->head]>39)){
        //Snake server hit a wall
        winLose(1);
        return true;
    }
    
    //Check for collision with other snake
    //Snake client's head into other snake
    int tempTail = snakeSrv->tail;
    while(tempTail!=(snakeSrv->head)+1){
        if(snakeCli->x[snakeCli->head]==snakeSrv->x[tempTail] //Checks x
        &&snakeCli->y[snakeCli->head]==snakeSrv->y[tempTail]){ //Checks y
        //If both are true, then snake collision is true
            winLose(0);
            return true;
        }
        ++tempTail;
    }
    
    //Snake server's head into other snake
    tempTail = snakeCli->tail;
    while(tempTail!=(snakeCli->head)+1){
        if(snakeSrv->x[snakeSrv->head]==snakeCli->x[tempTail] //Checks x
        &&snakeSrv->y[snakeSrv->head]==snakeCli->y[tempTail]){ //Checks y
        //If both are true, then snake collision is true
            winLose(1);
            return true;
        }
        ++tempTail;
    }
    
    //No collision
    return false;
}

//Bool time FeelsBadMan
//Returns true if time is up, otherwise return false
bool time(int* iTime){
    int tempTime = millis();
    if((tempTime-iTime)>90000){ //Constant timeout of 1 minute and 30 seconds
        //TODO: Point system
        return true;
    }
    return false;
}

//Main game function, runs the entire game
void snake(int* dotX, int* dotY){//up = N down = S left = W right = E
  
  //Create snakes - 2d array
  //call with snakeCli[i][j]
  Snake* snakeCli = malloc(sizeof(Snake));
  Snake* snakeSrv = malloc(sizeof(Snake));
  assert(snakeCli != NULL);
  assert(snakeSrv != NULL);
  //Position snakes
  snakeSrv->x = {2,3,4,5,6};
  snakeSrv->y = {2,2,2,2,2};
  snakeCli->x = {37,36,35,34,33};
  snakeCli->y = {37,37,37,37,37};
  
  //Spawn random dot, give coordinates
  int dotX;
  int dotY;
  
  randomDot(&dotX, &dotY);
  //3 2 1 GO! Enter startup function
  
  int iTime = millis(); //Initial time
  
  //Start snakes
  while(!collision(snakeCli,snakeSrv)&&!time(&iTime)){
       //Put snake code in here - That means moving snakes
       
       //Check for point dot function here (prevents tie/player priority)
       
       //If haven't eaten dot, delete tail and undraw
       //Client's Tail
       if(snakeCli->delay == 'N'){
            //Delete tail on display
            fillRect((snakeCli->x[snakeCli->tail]*3)+3,(snakeCli->y[snakeCli->tail]*3)+19,3,3,0); //0 = black
            //Move tail by one
            snakeCli->tail = (snakeCli->tail + 1)%50;
       }else{
            snakeCli->delay = 'N';
       }
       
       //Server's Tail
       if(snakeSrv->delay == 'N'){
            //Delete tail on display
            fillRect((snakeSrv->x[snakeSrv->tail]*3)+3,(snakeSrv->y[snakeSrv->tail]*3)+19,3,3,0); //0 = black
            //Move tail by one
            snakeSrv->tail = (snakeSrv->tail + 1)%50;
       }else{
            snakeSrv->delay = 'N';
       }
       
       //Placeholder for synch() to be used with head;
       
       //Client
       if(){//Up
            //Transfer coordinates to new head
            snakeCli->y[(snakeCli->head+1)%50] += snakeCli->y[snakeCli->head] + 1;
            snakeCli->x[(snakeCli->head+1)%50] += snakeCli->x[snakeCli->head];
       }
       if(){//Right
            //Transfer coordinates to new head
            snakeCli->x[(snakeCli->head+1)%50] += snakeCli->x[snakeCli->head] + 1;
            snakeCli->y[(snakeCli->head+1)%50] += snakeCli->y[snakeCli->head];
       }
       if(){//Down
            //Transfer coordinates to new head
            snakeCli->y[(snakeCli->head+1)%50] += snakeCli->y[snakeCli->head] - 1;
            snakeCli->x[(snakeCli->head+1)%50] += snakeCli->x[snakeCli->head];
       }
       if(){//Left
            //Transfer coordinates to new head
            snakeCli->x[(snakeCli->head+1)%50] += snakeCli->x[snakeCli->head] - 1;
            snakeCli->y[(snakeCli->head+1)%50] += snakeCli->y[snakeCli->head];
       }
       snakeCli->head = (snakeCli->head + 1)%50;
       //Draw new client head
       fillRect((snakeCli->x[snakeCli->head]*3)+3,(snakeCli->y[snakeCli->head]*3)+19,3,3,0xFFFF); //0 = black
       
       //Server
       //To be copy pasted once client is finished
       
  }
  
}

//Function to start 
int main(){
  //stuff go here
  
  init();
  
  tft.initR(INITR_BLACKTAB);
  
  Serial.begin(9600);
  
  randomSeed(analogRead(4));
  
  //srv/cli part, needs to be in the main
  //setup pin
  int srvCliPin = 13;
  pinMode(srvCliPin, INPUT);
  digitalWrite(srvCliPin, LOW);
  
  if(digitalRead(srvCliPin) == HIGH){ // read pin / determine srv or cli
    Serial.println("pin HIGH Srv");
    function(); // call appropriate functions
  }else{
    Serial.println("pin low cli");
    function(); // call appropriate functions
  }
  
  Serial.end();
}
