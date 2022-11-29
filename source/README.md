# Project#3 : OOP for Billiard Game  
This repository is for OOP class assignment in Chung-Ang Univ.  

## Student Infromation  
*Name*        : Wonbeom Jang 장원범  
*Student ID*  : 20182592  
*Depertment*  : School of Computer Science and Engineering  

## System Encironment
OS          : Window10  
IDE         : Visual Studio 2019  
compliter   : MSC 18.0.21005.1  

## How to run
1. double click click BilliardBoardGame.exe  
2. Move blue with left cursor drag  
3. shoot the ball with space bar  

The white ball takes first, then the yellow ball takes its turn.

## What I modified
#### Implement following function
virtualLego.cpp  
```c++
boolean CSphere::asIntersected(CSphere& ball)
void CSphere::hitBy(CSphere& ball) 

boolean CWall::asIntersected(CSphere& ball)  
void CWall::hitBy(CSphere& ball) 
```

*boolean CSphere::asIntersected(CSphere& ball)*  
We compute distance between two point of center of ball and radius. Set relation between them.  

*void CSphere::hitBy(CSphere& ball)*  
We compute the velocity of two ball based on the current velocity of ball. We use momentum conservation law. 

*boolean CWall::asIntersected(CSphere& ball)*  
We compute distance base on one coordinate of center of ball and wall.  

*boolean CWall::asIntersected(CSphere& ball)*  
Since Wall's position is fixed, We just change the velocity of ball of the vertical component of the wall.

#### Add the factor of controll the FPS of the game
d3dUtility.h  
```c++
#define SPEEDUPFACT 10
```

virtualLego.cpp  
```c++
void CShpare::ballUpdate(float timeDiff) {
    ...
	double rate = 1 -  (1 - DECREASE_RATE)*timeDiff * 400 * SPEEDUPFACT;
    ...
}
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ...
    g_sphere[2 + tern].setPower(distance * cos(theta) * SPEEDUPFACT, distance * sin(theta) * SPEEDUPFACT);
    ...
}
```
d3dUtility.cpp  
```c++
int d3d::EnterMsgLoop( bool (*ptr_display)(float timeDelta) ) {
    double timeDelta = (currTime - lastTime)*0.0007/SPEEDUPFACT;
}
```

By adding *SPEEDUPFACT* we can modify the computational period of interection of ball and wall.

#### Modified function
virtualLego.cpp  
```c++
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)  

...
static tern = 1;
...
D3DXVECTOR3	whitepos = g_sphere[2 + tern].getCenter();
...

g_sphere[2 + tern].setPower(distance * cos(theta) * SPEEDUPFACT, distance * sin(theta) * SPEEDUPFACT); // edit
tern++;
tern %= 2;
```

By modifying function, we can change the tern of roll.
