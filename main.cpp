#include <iostream>
#include <thread>
#include <vector>
#include <stdio.h>
#include <Windows.h>
#include "tetris.h"
//#pragma comment(lib, "User32.lib")
using namespace std;

int nScreenWidth = 80;			// Console Screen Size X (columns)
int nScreenHeight = 30;			// Console Screen Size Y (rows)
int nFieldWidth = 12;
int nFieldHeight = 18;
unsigned char *pField = nullptr;

int main(){
	// Create Screen Buffer
	wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight];
	for (int i = 0; i < nScreenWidth*nScreenHeight; i++) screen[i] = L' ';
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

    pField = new unsigned char[nFieldWidth*nFieldHeight]; // Create play field buffer
	for (int x = 0; x < nFieldWidth; x++) // Board Boundary
		for (int y = 0; y < nFieldHeight; y++)
			pField[y*nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;
    
	int speedUp = 360;
	int speedUpCounter = 0;
    bool bGameOver = false;

	tetris player = tetris();
	//tetris player2 = tetris();

	while(!bGameOver){
		this_thread::sleep_for(50ms);

		for (int k = 0; k < 4; k++)							// R   L   D Z
        	player.bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;

		//for (int k = 0; k < 4; k++)							// R   L   D Z
        //	player2.bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;

		player.Update(bGameOver);
		//player2.Update(bGameOver);

		speedUpCounter++;
		if(speedUpCounter == speedUp){
			speedUpCounter = 0;
			if (player.speed >= 5){
				player.speed--;
				//player2.speed--;
			} 
		}

		player.Draw(screen, nScreenWidth, nScreenHeight, false);
		//player2.Draw(screen, nScreenWidth, nScreenHeight, true);
		WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	CloseHandle(hConsole);
	cout << "Game Over!!" << endl;
	system("pause");
	return 0;
    
}