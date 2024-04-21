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
#include <list>

using namespace std;

int reconcilationFailCount = 0;
ofstream debugFile;
bool debugOn = false;

int nScreenWidth = 80;			// Console Screen Size X (columns)
int nScreenHeight = 30;			// Console Screen Size Y (rows)
int nFieldWidth = 12;
int nFieldHeight = 18;
wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight];

struct EnemyState{
	PlayerStateField psf;
	bool intermediate;
};

class client : public client_interface<MessageType>{
	public:
		tetris player;
		tetris rival;
		bool ready;
		list<InputBody> inputRequests;
		queue<EnemyState> enemyStates;

		int inputRequest = 0;

		bool enemyLose = false;
		bool playerLose = false;
		bool rivalUpdate = false;
		bool playerUpdate = false;

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
				ready = true;
			}
			else if(msg.head.id == MessageType::Win){
				enemyLose = true;
			}
			else if(msg.head.id == MessageType::Lose){
				playerLose = true;
			}
			else if(msg.head.id == MessageType::RivalStateInBetween){
				PlayerStateInBetween ps;
				msg >> ps;

				EnemyState es;
				es.intermediate = true;
				es.psf.playerState.nCurrentRotation = ps.nCurrentRotation;
				es.psf.playerState.nCurrentX = ps.nCurrentX;
				es.psf.playerState.nCurrentY = ps.nCurrentY;

				enemyStates.push(es);
			}
			else if(msg.head.id == MessageType::GState){
				PlayerState ps;
				PlayerState rs;

				msg >> rs;
				msg >> ps;

				EnemyState es;
				es.psf.playerState = rs;
				es.intermediate = true;
				enemyStates.push(es);

				if(ps.lastRequest == -1){
					SetPlayer(ps);
					player.Draw(screen, nScreenWidth, nScreenHeight, false);
					playerUpdate = true;
				}
				else if(!Reconcilation(ps)){
					SetPlayer(ps);
					for (auto input : inputRequests){
						for(int j = 0; j < 4; j++){
							player.bKey[j] = input.inputs[j];
						}
						player.HandleInput();
					}
					player.Draw(screen, nScreenWidth, nScreenHeight, false);
					playerUpdate = true;
				}
				
			}
			else if(msg.head.id == MessageType::GStateField_1_0){
				PlayerStateField ps;
				PlayerState rs;

				msg >> rs;
				msg >> ps;

				EnemyState es;
				es.psf.playerState = rs;
				es.intermediate = true;
				enemyStates.push(es);

				SetPlayerField(ps);
				player.Draw(screen, nScreenWidth, nScreenHeight, false);
				playerUpdate = true;

			}
			else if(msg.head.id == MessageType::GStateField_0_1){
				PlayerState ps;
				PlayerStateField rs;

				msg >> rs;
				msg >> ps;

				EnemyState es;
				es.psf.playerState = rs.playerState;
				for(int i = 0; i < nFieldWidth * nFieldHeight; i++){
					es.psf.field[i] = rs.field[i];
				}
				es.intermediate = false;
				enemyStates.push(es);

				if(ps.lastRequest == -1){
					SetPlayer(ps);
					player.Draw(screen, nScreenWidth, nScreenHeight, false);
					playerUpdate = true;
				}
				else if(!Reconcilation(ps)){
					SetPlayer(ps);
					for (auto input : inputRequests){
						for(int j = 0; j < 4; j++){
							player.bKey[j] = input.inputs[j];
						}
						player.HandleInput();
					}
					player.Draw(screen, nScreenWidth, nScreenHeight, false);
					playerUpdate = true;
				}
			}
			else if(msg.head.id == MessageType::GStateField){

				PlayerStateField ps;
				PlayerStateField rs;

				msg >> rs;
				msg >> ps;

				EnemyState es;
				es.psf.playerState = rs.playerState;
				for(int i = 0; i < nFieldWidth * nFieldHeight; i++){
					es.psf.field[i] = rs.field[i];
				}
				es.intermediate = false;
				enemyStates.push(es);

				SetPlayerField(ps);
				player.Draw(screen, nScreenWidth, nScreenHeight, false);
				playerUpdate = true;
			}
		}
	private:
		void SetPlayer(PlayerState ps){
			player.nCurrentPiece = ps.nCurrentPiece;
			player.nCurrentRotation = ps.nCurrentRotation;
			player.nCurrentX = ps.nCurrentX;
			player.nCurrentY = ps.nCurrentY;
			player.nScore = ps.nScore;
			player.bRotateHold = ps.bRotateHold;
		}
		void SetPlayerField(PlayerStateField ps){
			SetPlayer(ps.playerState);
			for(int i = 0; i < nFieldWidth * nFieldHeight; i++){
				player.pField[i] = ps.field[i];
			}
		}
		bool Reconcilation(PlayerState ps){
			// ps is server state

			// RECONCILATION
			int tempCurrentPiece = ps.nCurrentPiece;
			int tempCurrentRotation = ps.nCurrentRotation;
			int tempCurrentX = ps.nCurrentX;
			int tempCurrentY = ps.nCurrentY;
			bool tempRotateHold = ps.bRotateHold;

			std::list<InputBody>::iterator i = inputRequests.begin();

			while(i != inputRequests.end()){
				if(i->requestOrder <= ps.lastRequest)
					inputRequests.pop_front();
				else
					break;
				i = inputRequests.begin();
			}

			for (auto input : inputRequests){
				for(int j = 0; j < 4; j++){
					player.bKey[j] = input.inputs[j];
				}
				player.HandleInputVirtually(tempCurrentPiece, tempCurrentX, tempCurrentY, tempCurrentRotation, tempRotateHold);
			}

			// CHECK IF SAME
			bool reconcilated = true;

			if(tempCurrentPiece != player.nCurrentPiece)
				reconcilated = false;
			if(tempCurrentRotation != player.nCurrentRotation)
				reconcilated = false;
			if(tempCurrentX != player.nCurrentX)
				reconcilated = false;
			if(tempCurrentY != player.nCurrentY)
				reconcilated = false;

			if(reconcilated == true){
				player.DrawInfo(screen, nScreenWidth, nScreenHeight, L"       RECONCILATED", 19, reconcilationFailCount);
				return true;
			}	
			reconcilationFailCount++;
			player.DrawInfo(screen, nScreenWidth, nScreenHeight, L"CANNOT RECONCILATED", 19, reconcilationFailCount);

			if(debugOn){
				debugFile << "input size: " << inputRequests.size() << "\n";
				for (auto input : inputRequests){
					for(int j = 0; j < 4; j++){
						debugFile << input.inputs[j] <<  " ";
					}
					debugFile << "\n";
				}	
				debugFile << "Server State: "<< ps.nCurrentRotation << " " << ps.nCurrentX << " " << ps.nCurrentY << "\n";
				debugFile << "Current Play State: "<< player.nCurrentRotation << " " << player.nCurrentX << " " << player.nCurrentY << "\n";
				debugFile << "Temp State: "<< tempCurrentRotation << " " << tempCurrentX << " " << tempCurrentY << "\n";
			}
			
			return false;
		}
};

int main(int argc, char* argv[]){

	if(argc == 2){
		debugFile = ofstream(argv[1]);
		debugOn = true;
	}else{
		debugOn = false;
	}

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

	while(!cl.playerLose && !cl.enemyLose){

		cl.Update(-1);

		if(cl.playerUpdate){
			cl.playerUpdate = false;
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

			if(debugOn){
				debugFile << "Pressed input:  ";
				for(int k = 0; k < 4; ++k){
					debugFile << cl.player.bKey[k] << " ";
				}
				debugFile << "\n";
			}

			// PREDICTION
			cl.player.HandleInput();
			cl.player.Draw(screen, nScreenWidth, nScreenHeight, false);


			if(!cl.enemyStates.empty()){
				EnemyState es = cl.enemyStates.front();
				if(es.intermediate){
					cl.rival.nCurrentRotation = es.psf.playerState.nCurrentRotation;
					cl.rival.nCurrentX = es.psf.playerState.nCurrentX;
					cl.rival.nCurrentY = es.psf.playerState.nCurrentY;
				}else{
					cl.rival.nCurrentPiece = es.psf.playerState.nCurrentPiece;
					cl.rival.nCurrentRotation = es.psf.playerState.nCurrentRotation;
					cl.rival.nCurrentX = es.psf.playerState.nCurrentX;
					cl.rival.nCurrentY = es.psf.playerState.nCurrentY;
					cl.rival.nScore =es.psf.playerState.nScore;
					for(int i = 0; i < nFieldWidth * nFieldHeight; i++){
						cl.rival.pField[i] = es.psf.field[i];
					}
				}
				cl.enemyStates.pop();
				cl.rival.Draw(screen, nScreenWidth, nScreenHeight, true);
			}
			WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);

			// SEND REQUEST TO SERVER
			Message<MessageType> ms;
			ms.head = {MessageType::Input, 0};

			InputBody ib;
			for(int i = 0; i < 4; i++){
				ib.inputs[i] = cl.player.bKey[i];
			}
			ib.requestOrder = cl.inputRequest++;
			cl.inputRequests.push_back(ib);
			ms << ib;
			cl.Send(ms);
		}
	}

	cl.Disconnect();

	CloseHandle(hConsole);

	debugFile.close();

	if(cl.playerLose){
		std::cout << "You Lose!!" << endl;
	}else{
		std::cout << "You Win!!" << endl;
	}
	std::this_thread::sleep_for(2000ms);

	system("pause");
	return 0;
    
}