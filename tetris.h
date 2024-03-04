#ifndef TETRIS
#define TETRIS

#include <iostream>
#include <vector>

class tetris{
    public:
        std::wstring tetromino[7];
        unsigned char *pField = nullptr;
        int nFieldWidth;
        int nFieldHeight;
        bool bKey[4];
        int nCurrentPiece;
        int nCurrentRotation;
        int nCurrentX;
        int nCurrentY;

        bool down;
        bool bRotateHold;

        int speed;
	    int counter;

        int clearSpeed;
	    int clearCounter;
        std::vector<int> vLines;

        int nScore;
        
        tetris(){
            nFieldWidth = 12;
            nFieldHeight = 18;
            nCurrentPiece = 0;
            nCurrentRotation = 0;
            nCurrentX = nFieldWidth / 2;
            nCurrentY = 0;
            down = false;
            bRotateHold = true;

            speed = 20;
	        counter = 0;

            clearSpeed = 3;
	        clearCounter = 0;

            nScore = 0;

            pField = new unsigned char[nFieldWidth*nFieldHeight]; // Create play field buffer
            for (int x = 0; x < nFieldWidth; x++) // Board Boundary
                for (int y = 0; y < nFieldHeight; y++)
                    pField[y*nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;

            tetromino[0].append(L"..X...X...X...X."); // Tetronimos 4x4
            tetromino[1].append(L"..X..XX...X.....");
            tetromino[2].append(L".....XX..XX.....");
            tetromino[3].append(L"..X..XX..X......");
            tetromino[4].append(L".X...XX...X.....");
            tetromino[5].append(L".X...X...XX.....");
            tetromino[6].append(L"..X...X..XX.....");
        }
        int Rotate(int px, int py, int r);
        bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY);
        void HandleInput();
        void Draw(wchar_t* screen, int nScreenWidth, int nScreenHeight, bool second);
        void Update(bool& bGameOver, bool second = false);
};

int tetris::Rotate(int px, int py, int r){
	int pi = 0;
	switch (r % 4){
	case 0: // 0 degrees			// 0  1  2  3
		pi = py * 4 + px;			// 4  5  6  7
		break;						// 8  9 10 11
									//12 13 14 15

	case 1: // 90 degrees			//12  8  4  0
		pi = 12 + py - (px * 4);	//13  9  5  1
		break;						//14 10  6  2
									//15 11  7  3

	case 2: // 180 degrees			//15 14 13 12
		pi = 15 - (py * 4) - px;	//11 10  9  8
		break;						// 7  6  5  4
									// 3  2  1  0

	case 3: // 270 degrees			// 3  7 11 15
		pi = 3 - py + (px * 4);		// 2  6 10 14
		break;						// 1  5  9 13
	}								// 0  4  8 12

	return pi;
}

bool tetris::DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY){
	// All Field cells >0 are occupied
	for (int px = 0; px < 4; px++)
		for (int py = 0; py < 4; py++){
			// Get index into piece
			int pi = Rotate(px, py, nRotation);

			// Get index into field
			int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

			// Check that test is in bounds. Note out of bounds does
			// not necessarily mean a fail, as the long vertical piece
			// can have cells that lie outside the boundary, so we'll
			// just ignore them
			if (nPosX + px >= 0 && nPosX + px < nFieldWidth){
				if (nPosY + py >= 0 && nPosY + py < nFieldHeight){
					// In Bounds so do collision check
					if (tetromino[nTetromino][pi] != L'.' && pField[fi] != 0)
						return false; // fail on first hit
				}
			}
		}
	return true;
}

void tetris::HandleInput(){
    nCurrentX += (bKey[0] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) ? 1 : 0;
    nCurrentX -= (bKey[1] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) ? 1 : 0;		

    if(bKey[2] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)){
        nCurrentY += 1;
        counter = 0;
        down = false;
    }

    // Rotate, but latch to stop wild spinning
    if (bKey[3]){
        nCurrentRotation += (bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
		nCurrentRotation %= 4;
        bRotateHold = false;
    }
    else
        bRotateHold = true;
}

void tetris::Update(bool& bGameOver, bool second){
    if(!vLines.empty()){
		if(clearCounter == clearSpeed){
			for (auto &v : vLines){
				for (int px = 1; px < nFieldWidth - 1; px++){
					for (int py = v; py > 0; py--)
						pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
					pField[px] = 0;
				}
			}
			vLines.clear();
        }
		//if(!second)
		clearCounter++;
		return;
	}
	//if(!second)
	counter++;
	if(counter >= speed){
		counter = 0;
		down = true;
	}

	HandleInput();
		
	if(down){
		down = false;
		if(DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
			nCurrentY += 1;
		else{
			for (int px = 0; px < 4; px++)
				for (int py = 0; py < 4; py++)
					if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
						pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;

				
			// line Clear
			for (int py = 0; py < 4; py++){
				if(nCurrentY + py < nFieldHeight - 1){
					bool bLine = true;
					for (int px = 1; px < nFieldWidth - 1; px++)
						bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;

					if (bLine){
						// Remove Line, set to =
						for (int px = 1; px < nFieldWidth - 1; px++)
							pField[(nCurrentY + py) * nFieldWidth + px] = 8;
						vLines.push_back(nCurrentY + py);
					}						
				}
			}
			nScore += 25;
			clearCounter = 0;
			if(!vLines.empty())
				nScore += (1 << vLines.size()) * 100;
					
			// Pick New Piece
			nCurrentX = nFieldWidth / 2;
			nCurrentY = 0;
			nCurrentRotation = 0;
			nCurrentPiece = rand() % 7;

			// If piece does not fit straight away, game over!
			bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
		}
	}
}

void tetris::Draw(wchar_t* screen, int nScreenWidth, int nScreenHeight, bool second){

    int xSpace = second ? 2 * nFieldWidth + 2 : 2;

	// Draw Field
    for (int x = 0; x < nFieldWidth; x++)
		for (int y = 0; y < nFieldHeight; y++)
			screen[(y + 2)*nScreenWidth + (x + xSpace)] = L" ABCDEFG=#"[pField[y*nFieldWidth + x]];

	// Draw Current Piece
	for (int px = 0; px < 4; px++)
		for (int py = 0; py < 4; py++)
			if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
				screen[(nCurrentY + py + 2)*nScreenWidth + (nCurrentX + px + xSpace)] = nCurrentPiece + 65;

	if(!second)
		swprintf_s(&screen[21 * nScreenWidth + xSpace], 21, L"YOUR SCORE: %8d", nScore);
	else
		swprintf_s(&screen[21 * nScreenWidth + xSpace], 22, L"RIVAL SCORE: %8d", nScore);

    /*
    for (int y = 0; y < nFieldWidth; y++){
		for (int x = 0; x < nFieldHeight; x++){
			std::cout << (char)screen[(y + 2)*nScreenWidth + (x + 2)];
		}
		std::cout << "\n";
	}
    */
}

#endif