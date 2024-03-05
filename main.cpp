#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <stdio.h>
#include <time.h>

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

		bool enemyLose = false;
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
            ClientMessage<MessageType> message = messageQueue.pop();
            HandleMessage(message.message);
        }

	protected:
		void HandleMessage(Message<MessageType>& msg) override{

			if(msg.head.id == MessageType::Start){
				int seed;
				msg >> seed;
				ready = true;
				srand(seed);
			}
			else if(msg.head.id == MessageType::Win){
				enemyLose = true;
			}
			else if(msg.head.id == MessageType::Lose){
				
			}
			else if(msg.head.id == MessageType::Input){
				InputBody ib;
				msg >> ib;
				for(int i = 0; i < 4; i++){
					rival.bKey[i] = ib.inputs[i];
				}
				//rival.counter = ib.counter;
				//rival.clearCounter = ib.clearCounter;
				rival.speed = ib.speed;

				rival.Update(enemyLose);
				rival.Draw(screen, nScreenWidth, nScreenHeight, true);
				enemyUpdate = true;
			}
			else if(msg.head.id == MessageType::NewPiece){
				msg >> rival.nCurrentPiece;
				rival.Draw(screen, nScreenWidth, nScreenHeight, true);
				enemyUpdate = true;
			}
			else if(msg.head.id == MessageType::Damage){
				int damage;
				msg >> damage;
				player.nScore -= damage;
				player.Draw(screen, nScreenWidth, nScreenHeight, false);
			}
		}
};

int main(){
	HWND hWnd = GetForegroundWindow();
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
		if(cl.enemyLose){
			break;
		}

		for (int k = 0; k < 4; k++)							// R   L   D Z
        	cl.player.bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;
		
		if(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() > 50){
			begin = std::chrono::steady_clock::now();

			if(GetForegroundWindow() != hWnd){
				for (int k = 0; k < 4; k++)
					cl.player.bKey[k] = false;
			}

			speedUpCounter++;
			if(speedUpCounter == speedUp){
				speedUpCounter = 0;
				if (cl.player.speed >= 5){
					cl.player.speed--;
				} 
			}

			Message<MessageType> ms;
			ms.head = {MessageType::Input, 0};

			InputBody ib;
			for(int i = 0; i < 4; i++){
				ib.inputs[i] = cl.player.bKey[i];
			}
			ib.counter = cl.player.counter;
			ib.clearCounter = cl.player.clearCounter;
			ib.speed = cl.player.speed;

			ms << ib;

			cl.Send(ms);
			int score = cl.player.nScore;
			cl.player.Update(bGameOver);
			if(cl.player.nScore - score > 25){
				Message<MessageType> ms;
				ms.head = {MessageType::Damage, 0};
				ms << (cl.player.nScore - score - 25);
				cl.Send(ms);
				cl.rival.nScore -= (cl.player.nScore - score - 25);
				cl.rival.Draw(screen, nScreenWidth, nScreenHeight, true);
			}
			cl.player.Draw(screen, nScreenWidth, nScreenHeight, false);

			if(cl.player.counter == 0 && cl.player.clearCounter == 0){
				Message<MessageType> ms;
				ms.head = {MessageType::NewPiece, 0};
				ms << cl.player.nCurrentPiece;
				cl.Send(ms);
			}
			WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);

			if(bGameOver || cl.player.nScore < -1000){
				bGameOver = true;
				Message<MessageType> ms;
				ms.head = {MessageType::Lose, 0};
				cl.Send(ms);
			}
		}
	}

	cl.Disconnect();

	CloseHandle(hConsole);

	if(bGameOver){
		std::cout << "You Lose!!" << endl;
	}else{
		std::cout << "You Win!!" << endl;
	}
	std::this_thread::sleep_for(2000ms);

	system("pause");
	return 0;
    
}