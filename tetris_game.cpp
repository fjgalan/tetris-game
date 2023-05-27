#include <iostream>
#include <Windows.h>
#include <thread>
#include <vector>

using namespace std;

wstring tetromino[7];
int fieldWidth = 12;
int fieldHeight = 18;
unsigned char* Field = nullptr;

int screenWidth = 80;		// Console screen size X (columns)
int screenHeight = 30;		// Console screen size Y (rows)

int Rotate(int px, int py, int r)
{
	int pi = 0;
	switch (r % 4)
	{
	case 0:							// 0º
		pi = py * 4 + px;
		break;

	case 1:							// 90º
		pi = 12 + py - (px * 4);
		break;

	case 2:							// 180º
		pi = 15 - (py * 4) - px;
		break;

	case 3:							// 270º
		pi = 3 - py + (px * 4);
		break;
	}

	return pi;
}


bool PieceFitting(int tetro, int rotation, int posX, int posY)
{
	for (int px = 0; px < 4; px++)
		for (int py = 0; py < 4; py++)
		{
			// Get index into piece
			int piece = Rotate(px, py, rotation);

			// Get index into field
			int fi = (posY + py) * fieldWidth + (posX + px);

			if (posX + px >= 0 && posX + px < fieldWidth)
			{
				if (posY + py >= 0 && posY + py < fieldHeight)
				{
					if (tetromino[tetro][piece] != L'.' && Field[fi] != 0)
						return false; // fail on first hit
				}
			}
		}
	return true;
}

int main()
{
	srand((unsigned int)time(NULL));

	wchar_t* screen = new wchar_t[screenWidth * screenHeight];
	for (int i = 0; i < screenWidth * screenHeight; i++)
		screen[i] = L' ';
	HANDLE Console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(Console);
	DWORD bytesWritten = 0;

	// Create assets
	// X: part of the shape
	// Tetronimos 4x4
	tetromino[0].append(L"..X...X...X...X.");
	tetromino[1].append(L"..X..XX...X.....");
	tetromino[2].append(L".....XX..XX.....");
	tetromino[3].append(L"..X..XX..X......");
	tetromino[4].append(L".X...XX...X.....");
	tetromino[5].append(L".X...X...XX.....");
	tetromino[6].append(L"..X...X..XX.....");

	Field = new unsigned char[fieldWidth * fieldHeight];
	for (int i = 0; i < fieldWidth; i++)
		for (int j = 0; j < fieldHeight; j++)
			Field[j * fieldWidth + i] = (i == 0 || i == fieldWidth - 1 || j == fieldHeight - 1) ? 9 : 0;

	// Game logics
	bool gameOver = false;
	int currentPiece = rand() % 7, nextPiece = rand() % 7;
	int currentRotation = 0;
	int currentX = fieldWidth / 2; // the first piece will be in he middle
	int currentY = 0;

	bool key[4];
	bool rotateHold = true;

	int speed = 20;
	int speedCounter = 0;
	bool forceSpeed = false;
	int pieceCount = 0;
	int score = 0;
	int numberLines = 0;

	vector<int> vLines;

	while (!gameOver)
	{
		// Game Timing
		this_thread::sleep_for(50ms);	// game tick
		speedCounter++;
		forceSpeed = (speedCounter == speed);

		// Inpunt
		for (int k = 0; k < 4; k++)
			key[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;

		// Game logic
		currentX += (key[0] && PieceFitting(currentPiece, currentRotation, currentX + 1, currentY)) ? 1 : 0;
		currentX -= (key[1] && PieceFitting(currentPiece, currentRotation, currentX - 1, currentY)) ? 1 : 0;
		currentY += (key[2] && PieceFitting(currentPiece, currentRotation, currentX, currentY + 1)) ? 1 : 0;

		if (key[3])
		{
			currentRotation += (rotateHold && PieceFitting(currentPiece, currentRotation + 1, currentX, currentY)) ? 1 : 0;
			rotateHold = false;
		}
		else
			rotateHold = true;

		if (forceSpeed)
		{
			speedCounter = 0;
			pieceCount++;
			if (pieceCount % 50 == 0)
				if (speed >= 10)
					speed--;

			// Test if piece can be moved down
			if (PieceFitting(currentPiece, currentRotation, currentX, currentY + 1))
				currentY++;
			else
			{
				// Lock current piece in the field
				for (int px = 0; px < 4; px++)
					for (int py = 0; py < 4; py++)
						if (tetromino[currentPiece][Rotate(px, py, currentRotation)] != L'.')
							Field[(currentY + py) * fieldWidth + (currentX + px)] = currentPiece + 1;

				// Check if we have lines
				for (int py = 0; py < 4; py++)
					if (currentY + py < fieldHeight - 1)
					{
						bool line = true;
						for (int px = 1; px < fieldWidth - 1; px++)
							line &= (Field[(currentY + py) * fieldWidth + px]) != 0;

						if (line)
						{
							for (int px = 1; px < fieldWidth - 1; px++)
								Field[(currentY + py) * fieldWidth + px] = 8;

							vLines.push_back(currentY + py);
							numberLines++;
						}
					}

				score += 50;
				if (!vLines.empty())
					score += (1 << vLines.size()) * 100;

				// Choose next piece
				currentX = fieldWidth / 2;
				currentY = 0;
				currentRotation = 0;

				currentPiece = nextPiece;
				nextPiece = rand() % 7;

				// Clear current next piece
				for (int px = 0; px < 4; px++)
					for (int py = 0; py < 4; py++)
						screen[(py + 12) * screenWidth + (fieldWidth + 12 + px)] = L' ';

				// If piece does not fit
				gameOver = !PieceFitting(currentPiece, currentRotation, currentX, currentY);
			}
		}

		// Draw Field
		for (int i = 0; i < fieldWidth; i++)
			for (int j = 0; j < fieldHeight; j++)
				screen[(j + 2) * screenWidth + (i + 2)] = L" ABCDEFG*#"[Field[j * fieldWidth + i]]; // + 2 offset so we draw inside the board

		// Draw current piece
		for (int px = 0; px < 4; px++)
			for (int py = 0; py < 4; py++)
				if (tetromino[currentPiece][Rotate(px, py, currentRotation)] == L'X')
					screen[(currentY + py + 2) * screenWidth + (currentX + px + 2)] = currentPiece + 65; // for ABCDEFG

		// Draw frame around next piece
		for (int px = 0; px < 6; px++)
		{
			screen[(11) * screenWidth + (fieldWidth + 11 + px)] = L'=';      // Top frame
			screen[(16) * screenWidth + (fieldWidth + 11 + px)] = L'=';      // Bottom frame
		}
		for (int py = 0; py < 4; py++)
		{
			screen[(py + 12) * screenWidth + (fieldWidth + 11)] = L'|';      // Left frame
			screen[(py + 12) * screenWidth + (fieldWidth + 16)] = L'|';      // Right frame
		}

		// Draw new next piece
		for (int px = 0; px < 4; px++)
			for (int py = 0; py < 4; py++)
				if (tetromino[nextPiece][Rotate(px, py, 0)] == L'X')
					screen[(py + 12) * screenWidth + (fieldWidth + 12 + px)] = nextPiece + 65;


		// Draw Score
		swprintf_s(&screen[2 * screenWidth + fieldWidth + 6], 16, L"SCORE: %8d", score);
		swprintf_s(&screen[2 * screenWidth + fieldWidth + 30], 16, L"LINES: %8d", numberLines);

		if (!vLines.empty())
		{
			// Display Frame
			WriteConsoleOutputCharacter(Console, screen, screenWidth * screenHeight, { 0, 0 }, &bytesWritten);
			this_thread::sleep_for(400ms);

			for (auto& v : vLines)
				for (int px = 1; px < fieldWidth - 1; px++)
				{
					for (int py = v; py > 0; py--)
						Field[py * fieldWidth + px] = Field[(py - 1) * fieldWidth + px];
					Field[px] = 0;
				}
			vLines.clear();
		}

		// Display Frame
		WriteConsoleOutputCharacter(Console, screen, screenWidth * screenHeight, { 0, 0 }, &bytesWritten);
	}

	CloseHandle(Console);
	cout << "GAME OVER! Score: " << score << endl;
	system("pause");
	return 0;
}