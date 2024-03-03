#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <stdio.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x0500
#endif
#define ASIO_STANDALONE
#include <asio.hpp>

#include <Windows.h>
#include <chrono>

#include "tetris.h"
#include "network/client.h"
#include "common.h"
#include <fstream>

using namespace std;

int nScreenWidth = 80;			// Console Screen Size X (columns)
int nScreenHeight = 30;			// Console Screen Size Y (rows)
int nFieldWidth = 12;
int nFieldHeight = 18;
wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight];

class client : public client_interface<MessageType>{
	public:
		tetris player;
		tetris rival;
		bool ready;

		bool enemyLose;
		bool enemyUpdate;

		client(){
			player = tetris();
			rival = tetris();
		}

		void WaitOpponent(){
			Message<MessageType> newMsg;
            newMsg.head = {MessageType::Ready, 0};
			Send(newMsg);
            messageQueue.wait();
            //while(!messageQueue.empty()){
            ClientMessage<MessageType> message = messageQueue.pop();
                //Handle Message
            HandleMessage(message.message);
           // }
        }

	protected:
		void HandleMessage(const Message<MessageType>& msg) override{

			if(msg.head.id == MessageType::Start){
				//scoped_lock<mutex> lock(mtx);
				//cv.notify_one();
				ready = true;
			}
			else if(msg.head.id == MessageType::Win){

			}
			else if(msg.head.id == MessageType::Lose){
				
			}
			else if(msg.head.id == MessageType::Input){
				Message<MessageType> ms = msg;
				InputBody ib;
				//ms >> ib;
				//ms >> rival.bKey;
				ms >> ib;
				for(int i = 0; i < 4; i++){
					rival.bKey[i] = ib.inputs[i];
				}
				//rival.counter = ib.counter;
				///rival.clearCounter = ib.clearCounter;

				//cerr << rival.counter << " " << rival.clearCounter << "\n";

				/*rival.nCurrentPiece = ib.nCurrentPiece;
    			rival.nCurrentRotation = ib.nCurrentRotation;
    			rival.nCurrentX = ib.nCurrentX;
    			rival.nCurrentY = ib.nCurrentY;*/

				rival.Update(enemyLose);
				rival.Draw(screen, nScreenWidth, nScreenHeight, true);
				enemyUpdate = true;
			}
			else if(msg.head.id == MessageType::NewPiece){
				Message<MessageType> ms = msg;
				//ms >> rival.nCurrentPiece;
				enemyUpdate = true;
			}
		}
};

int main(){
	HWND hWnd = GetForegroundWindow();
	// Create Screen Buffer
	
	for (int i = 0; i < nScreenWidth*nScreenHeight; i++) screen[i] = L' ';
	DWORD dwBytesWritten = 0;
    
	int speedUp = 360;
	int speedUpCounter = 0;
    bool bGameOver = false;

	client cl;

	cl.Connect("127.0.0.1", 10000);
	std::cout << "Wait for opponent..." << endl;
	cl.WaitOpponent();
	std::cout << cl.ready << endl;
	

	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	while(!bGameOver){

		cl.Update(-1);
		if(cl.enemyUpdate){
			cl.enemyUpdate = false;
			WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
		}

		for (int k = 0; k < 4; k++)							// R   L   D Z
        	cl.player.bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;
		
		if(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() > 50){
			begin = std::chrono::steady_clock::now();

			if(GetForegroundWindow() != hWnd){
				for (int k = 0; k < 4; k++)
					cl.player.bKey[k] = false;
			}

			Message<MessageType> ms;
			ms.head = {MessageType::Input, 0};

			//ms << cl.player.bKey;
			InputBody ib;
			for(int i = 0; i < 4; i++){
				ib.inputs[i] = cl.player.bKey[i];
			}
			ib.counter = cl.player.counter;
			ib.clearCounter = cl.player.clearCounter;
			ib.nCurrentPiece = cl.player.nCurrentPiece;
			ib.nCurrentX = cl.player.nCurrentX;
			ib.nCurrentY = cl.player.nCurrentY;
			ms << ib;

			cl.Send(ms);

			cl.player.Update(bGameOver);
			//player2.Update(bGameOver);

			speedUpCounter++;
			if(speedUpCounter == speedUp){
				speedUpCounter = 0;
				if (cl.player.speed >= 5){
					cl.player.speed--;
					//player2.speed--;
				} 
			}

			cl.player.Draw(screen, nScreenWidth, nScreenHeight, false);
			//cl.rival.Draw(screen, nScreenWidth, nScreenHeight, true); // if piece change draw before this

			if(cl.player.nCurrentY == 0){
				Message<MessageType> ms;
				ms.head = {MessageType::NewPiece, 0};
				ms << cl.player.nCurrentPiece;
				cl.Send(ms);
			}

			//player2.Draw(screen, nScreenWidth, nScreenHeight, true);
			WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
		}
	}

	CloseHandle(hConsole);
	std::cout << "Game Over!!" << endl;
	system("pause");
	return 0;
    
}