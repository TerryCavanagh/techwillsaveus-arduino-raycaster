//Function reference:
// cls()                                - Clear screen
// point(x, y, a)                       - Set pixel x, y to 1 or 0
// line(x1, y1, x2, y2, a)              - Draw a line
// rect(x1, y1, x2, y2, a)              - Draw a rect
// fillrect(x1, y1, x2, y2, a)          - Draw a fillrect

// beep(t)                              - Beep for t frames

// game.isHeld()                        - True if button is held
// game.isPressed()                     - True if button has been pressed
// UP, DOWN, LEFT, RIGHT, START, LDR

#include "gamelib.h"

int raymap[] = {
  1,1,1,1,1,1,1,1,1,1,
  1,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,1,
  1,0,0,1,0,0,1,0,0,1,
  1,0,0,0,0,0,1,0,0,1,
  1,0,0,1,1,0,1,1,0,1,
  1,0,0,0,1,0,1,0,0,1,
  1,0,0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1,1,1};
  
//Raycasting variables
double cameraX, rayPosX, rayPosY, rayDirX, rayDirY;
double sideDistX, sideDistY, deltaDistX, deltaDistY, perpWallDist;
double distWall, distPlayer, currentDist;
double currentFloorX, currentFloorY;

int mapX, mapY;
int stepX, stepY, hit, side;
int rayheight, drawStart, drawEnd;


//Raycast coordinates
double posX, posY, dirX, dirY, planeX, planeY;
double oldDirX, oldDirY;
double oldPlaneX, oldPlaneY;

void raycast(){
  int x, y, d;
  
  for(x=0; x<8; x++){
    //calculate ray position and direction
    cameraX = 2 * x / 8.0 - 1; //x-coordinate in camera space
    rayPosX = posX;
    rayPosY = posY;
    rayDirX = dirX + planeX * cameraX;
    rayDirY = dirY + planeY * cameraX;
		
    //which box of the map we're in
    mapX = int(rayPosX);
    mapY = int(rayPosY);		
    
    //length of ray from one x or y-side to next x or y-side
    deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
    deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));
    
    hit = 0; //was there a wall hit?
    
    //calculate step and initial sideDist
    if (rayDirX < 0){
      stepX = -1;
      sideDistX = (rayPosX - mapX) * deltaDistX;
    }else{
      stepX = 1; 
      sideDistX = (mapX + 1.0 - rayPosX) * deltaDistX;
    }
    
    if (rayDirY < 0){
      stepY = -1;
      sideDistY = (rayPosY - mapY) * deltaDistY;
    }else{
      stepY = 1;
      sideDistY = (mapY + 1.0 - rayPosY) * deltaDistY;
    }
    
    //perform DDA
    while (hit == 0){
      //jump to next map square, OR in x-direction, OR in y-direction
      if (sideDistX < sideDistY){
	sideDistX += deltaDistX;
	mapX += stepX;
	side = 0;
      }else{
	sideDistY += deltaDistY;
	mapY += stepY;
	side = 1;
      }
      //Check if ray has hit a wall
      if (raymap[int(mapX) + (10 * int(mapY))] > 0) hit = 1;
      
      //Calculate distance of perpendicular ray (oblique distance will give fisheye effect!)
      if (side == 0) {
	perpWallDist = abs((mapX - rayPosX + (1 - stepX) / 2) / rayDirX);
      }else {
	perpWallDist = abs((mapY - rayPosY + (1 - stepY) / 2) / rayDirY);
      }
      			
      //Calculate height of line to draw on screen
      rayheight = abs(int(8.0 / perpWallDist));
    }
    
    //calculate lowest and highest pixel to fill in current stripe
    drawStart = -rayheight / 2 + 8.0 / 2;
    drawStart-=1;
    if(drawStart < 0) drawStart = 0;
    drawEnd = rayheight / 2 + 8.0 / 2;
    if (drawEnd >= 8) drawEnd = 8.0 - 1;
				
    for (y = drawStart; y < drawEnd; y ++) {
      fillrect(x,y,x+1,y+1,1);
    }
  }
}

void setup(){             
  gamer.begin();
  gamesetup();
}

void loop(){
  render();
  input();
  logic();
  
  libupdate();
  delay(refreshrate);
}


void gamesetup(){
  //Game variables here
  refreshrate = 15;
  
  posX = 5.7; posY = 5;
  dirX = -1; dirY = 0;
  planeX = 0; planeY = 0.66;
}

void render(){
  cls();
  
  raycast();
  
  updatescreen();
}

void logic(){
}

void input(){
  if (gamer.isHeld(LEFT)) {
    //both camera direction and camera plane must be rotated
    oldDirX = dirX;
    dirX =dirX * cos(.15) - dirY * sin(.15);
    dirY = oldDirX * sin(.15) + dirY * cos(.15);
    
    oldPlaneX = planeX;
    planeX = planeX * cos(.15) - planeY * sin(.15);
    planeY = oldPlaneX * sin(.15) + planeY * cos(.15);
  }
  
  if (gamer.isHeld(RIGHT)) {
    //both camera direction and camera plane must be rotated
    oldDirX = dirX;
    dirX = dirX * cos(-.15) - dirY * sin(-.15);
    dirY = oldDirX * sin(-.15) + dirY * cos(-.15);
    
    oldPlaneX = planeX;
    planeX = planeX * cos(-.15) - planeY * sin(-.15);
    planeY = oldPlaneX * sin(-.15) + planeY * cos(-.15);
  }
  
  if (gamer.isHeld(UP)) {
    if (raymap[int(posX + dirX * .2) + (10 * int(posY))] == 0) posX += dirX * .2;
    if (raymap[int(posX) + (10 * int(posY + dirY * .2))] == 0) posY += dirY * .2;
  }
	
  if (gamer.isHeld(DOWN)) {
    if (raymap[int(posX - dirX * .2) + (10 * int(posY))] == 0) posX -= dirX * .2;
    if (raymap[int(posX) + (10 * int(posY - dirY * .2))] == 0) posY -= dirY * .2;
  }
}

