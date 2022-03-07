//--------------------------------------------------------------------------------------------------
/**
	\file    Game.cpp
**/
//--------------------------------------------------------------------------------------------------

#include "Game.h"

#include "Debug.h"
#include "Renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
static const unsigned int s_kFieldWidth = 10;
static const unsigned int s_kFieldHeight = 20;
//static const unsigned int s_kNumHiddenRows = 2;		
static const unsigned int s_initialFramesPerFallStep = 48;
static const int s_deltaFramesPerFallStepPerLevel = 2;

//--------------------------------------------------------------------------------------------------

static const Tetromino s_tetrominos[kNumTetrominoTypes] =
{
	// I
	{
		0, 1, 1, 1, 2, 1, 3, 1,
		2, 0, 2, 1, 2, 2, 2, 3,
		0, 2, 1, 2, 2, 2, 3, 2,
		1, 0, 1, 1, 1, 2, 1, 3,
		0x00ffffff,
	},
	// J
	{
		0, 0, 0, 1, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 1, 2,
		0, 1, 1, 1, 2, 1, 2, 2,
		1, 0, 1, 1, 0, 2, 1, 2,
		0x0000ffff,
	},
	// L
	{
		2, 0, 0, 1, 1, 1, 2, 1,
		1, 0, 1, 1, 1, 2, 2, 2,
		0, 1, 1, 1, 2, 1, 0, 2,
		0, 0, 1, 0, 1, 1, 1, 2,
		0xffaa00ff,
	},
	// O
	{
		1, 0, 2, 0, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 2, 1,
		0xffff00ff
	},
	// S
	{
		1, 0, 2, 0, 0, 1, 1, 1,
		1, 0, 1, 1, 2, 1, 2, 2,
		1, 1, 2, 1, 0, 2, 1, 2,
		0, 0, 0, 1, 1, 1, 1, 2,
		0x00ff00ff,
	},
	// T
	{
		1, 0, 0, 1, 1, 1, 2, 1,
		1, 0, 1, 1, 2, 1, 1, 2,
		0, 1, 1, 1, 2, 1, 1, 2,
		1, 0, 0, 1, 1, 1, 1, 2,
		0x9900ffff,
	},
	// Z
	{
		0, 0, 1, 0, 1, 1, 2, 1,
		2, 0, 1, 1, 2, 1, 1, 2,
		0, 1, 1, 1, 1, 2, 2, 2,
		1, 0, 0, 1, 1, 1, 0, 2,
		0xff0000ff,
	}
};

//--------------------------------------------------------------------------------------------------

static bool IsOverlap(const TetrominoInstance& tetronimoInstance, const Field& field)
{
	const Tetromino& tetronimo = s_tetrominos[tetronimoInstance.m_tetrominoType];
	const Tetromino::BlockCoords& blockCoords = tetronimo.blockCoord[tetronimoInstance.m_rotation];
	for (unsigned int i = 0; i < Tetromino::kNumBlocks; ++i)
	{
		const int x = tetronimoInstance.m_pos.x + blockCoords[i].x;
		const int y = tetronimoInstance.m_pos.y + blockCoords[i].y;

		// Считайте выход за пределы поля перекрытием
		if (x < 0 || x >= (int)field.width || y < 0 || y >= (int)field.height)
			return true;

		if (field.staticBlocks[x + y * field.width] != -1)
			return true;
	}

	return false;
}

static void SetBlock(Field& field, unsigned int ix, unsigned int iy, unsigned int val)
{
	HP_ASSERT(ix < field.width);
	HP_ASSERT(iy < field.height);
	field.staticBlocks[iy * field.width + ix] = val;
}

//--------------------------------------------------------------------------------------------------
// обнуляем параметры в конструкторе 
Game::Game()
	: m_deltaTimeSeconds(0.0f)
	, m_framesUntilFall(s_initialFramesPerFallStep)
	, m_framesPerFallStep(s_initialFramesPerFallStep)
	, m_numUserDropsForThisTetronimo(0)
	, m_numLinesCleared(0)
	, m_level(0)
	, m_score(0)
	, m_hiScore(0)
	, m_gameState(kGameState_TitleScreen)
{
	m_field.staticBlocks = nullptr;
}

Game::~Game()
{
}

bool Game::Init()
{
	return true;
}

void Game::Shutdown()
{
	delete[] m_field.staticBlocks;
	m_field.staticBlocks = nullptr;
}

void Game::Reset()
{
	/* не потребовалс*/
}

// возвращает true, если есть место для спавна 
bool Game::SpawnTetronimo()
{
	m_activeTetromino.m_tetrominoType = (TetrominoType)(rand() % kNumTetrominoTypes);
	m_activeTetromino.m_rotation = 0;
	m_activeTetromino.m_pos.x = (m_field.width - 4) / 2;	// ширина блока tetronimo приблизительно = 4
	m_activeTetromino.m_pos.y = 0;

	if (IsOverlap(m_activeTetromino, m_field))
	{
		return false;
	}

	m_framesUntilFall = s_initialFramesPerFallStep;
	m_numUserDropsForThisTetronimo = 0;
	return true;
}

void Game::Update(const GameInput& gameInput, float deltaTimeSeconds)
{
	m_deltaTimeSeconds = deltaTimeSeconds;

	switch (m_gameState)
	{
	case kGameState_TitleScreen:
		if (gameInput.bStart)
		{
			InitPlaying();
			m_gameState = kGameState_Playing;
		}
		if (gameInput.WatchHighScore)
		{
			
			m_gameState = kNumGameStates;
			
		}
		if (gameInput.Rules)
		{
			m_gameState = kGameRules;
		}
		break;
	case kGameState_Playing:
	{
		UpdatePlaying(gameInput);
		if (gameInput.bPause)
			m_gameState = kGamePause;
		break;
	}
	case kGameState_GameOver:
		if (gameInput.bStart)
		{
			m_gameState = kGameState_TitleScreen;
		}
		break;
	case kNumGameStates:
	{
		
		if (gameInput.bStart)
		{
			m_gameState = kGameState_TitleScreen;
		}
		break;
	}
	case kGamePause :
	{
		if (gameInput.bStart)
			m_gameState = kGameState_Playing;

		break;
	}
	case kGameRules:
	{
		if (gameInput.bStart)
		{
			m_gameState = kGameState_TitleScreen;
		}
		break;
	}
	default:
		HP_FATAL_ERROR("Unhandled case");
	}
}

void Game::InitPlaying()
{
	m_field.width = s_kFieldWidth;
	m_field.height = s_kFieldHeight;
	delete[] m_field.staticBlocks;
	m_field.staticBlocks = new int[m_field.width * m_field.height];

	for (unsigned int iy = 0; iy < m_field.height; ++iy)
	{
		for (unsigned int ix = 0; ix < m_field.width; ++ix)
		{
			m_field.staticBlocks[iy * m_field.width + ix] = -1;
		}
	}

	srand((unsigned int)time(NULL));

	SpawnTetronimo(); // создает фигуру в тетрисе (рандомную)

	m_numLinesCleared = 0;
	m_level = 0;
	m_framesPerFallStep = s_initialFramesPerFallStep;
	m_score = 0;
}


//метод считывания действий во время игры
void Game::UpdatePlaying(const GameInput& gameInput)
{
#ifdef _DEBUG
	if (gameInput.bDebugChangeTetromino)
	{
		m_activeTetromino.m_tetrominoType = (TetrominoType)(((unsigned int)m_activeTetromino.m_tetrominoType + 1) % (unsigned int)kNumTetrominoTypes);
	}
	if (gameInput.bDebugMoveLeft)
	{
		--m_activeTetromino.m_pos.x;
	}
	if (gameInput.bDebugMoveRight)
	{
		++m_activeTetromino.m_pos.x;
	}
	if (gameInput.bDebugMoveUp)
	{
		--m_activeTetromino.m_pos.y;
	}
	if (gameInput.bDebugMoveDown)
	{
		++m_activeTetromino.m_pos.y;
	}
#endif

	// двигается горизонтально 
	if (gameInput.bMoveLeft)
	{
		//попробуйте пошевелится
		TetrominoInstance testInstance = m_activeTetromino;
		--testInstance.m_pos.x;
		if (!IsOverlap(testInstance, m_field))
			m_activeTetromino.m_pos.x = testInstance.m_pos.x;
	}
	if (gameInput.bMoveRight)
	{
		//попробуйте пошевелится
		TetrominoInstance testInstance = m_activeTetromino;
		++testInstance.m_pos.x;
		if (!IsOverlap(testInstance, m_field))
			m_activeTetromino.m_pos.x = testInstance.m_pos.x;
	}


	// вращение 
	if (gameInput.bRotateClockwise)
	{
		TetrominoInstance testInstance = m_activeTetromino;
		if (testInstance.m_rotation == 0)
		{
			testInstance.m_rotation = 3;
		}
		else
		{
			--testInstance.m_rotation;
		}

		// проверьте наличие перекрытий
		if (IsOverlap(testInstance, m_field))
		{
			// Простой удар по стене, попробуйте 1 влево и 1 вправо
			testInstance.m_pos.x = m_activeTetromino.m_pos.x - 1;
			if (!IsOverlap(testInstance, m_field))
			{
				m_activeTetromino = testInstance;
			}
			else
			{
				testInstance.m_pos.x = m_activeTetromino.m_pos.x + 1;
				if (!IsOverlap(testInstance, m_field))
				{
					m_activeTetromino = testInstance;
				}
			}
		}
		else
		{
			// никакого перекрытия
			m_activeTetromino = testInstance;
		}
	}

	if (gameInput.bRotateAnticlockwise)
	{
		m_activeTetromino.m_rotation = (m_activeTetromino.m_rotation + 1) % Tetromino::kNumRotations;
	}

	// падает 
	m_framesUntilFall -= 1;
	if (m_framesUntilFall <= 0)
	{
		m_framesUntilFall = m_framesPerFallStep;

		// попробуйте пошевелится 
		TetrominoInstance testInstance = m_activeTetromino;
		testInstance.m_pos.y += 1;
		if (IsOverlap(testInstance, m_field))
		{
			AddTetronimoToField(m_field, m_activeTetromino);
			if (!SpawnTetronimo())
				m_gameState = kGameState_GameOver;
		}
		else
		{
			m_activeTetromino.m_pos.y = testInstance.m_pos.y;
		}
	}

	// мягкое падение (медленное)
	if (gameInput.bSoftDrop)
	{
		// попробуй двигаться 
		TetrominoInstance testInstance = m_activeTetromino;
		++testInstance.m_pos.y;
		if (!IsOverlap(testInstance, m_field))
		{
			m_activeTetromino.m_pos.y = testInstance.m_pos.y;
			++m_numUserDropsForThisTetronimo;
		}
	}

	// быстрое падение 
	if (gameInput.bHardDrop)
	{
		TetrominoInstance testInstance = m_activeTetromino;
		while (!IsOverlap(testInstance, m_field))
		{
			++testInstance.m_pos.y;
			++m_numUserDropsForThisTetronimo;
		}
		--testInstance.m_pos.y;	// резервное копирование одного
		--m_numUserDropsForThisTetronimo;
		AddTetronimoToField(m_field, testInstance);
		if (!SpawnTetronimo())
			m_gameState = kGameState_GameOver;
	}
	
}

void Game::AddTetronimoToField(const Field& field, const TetrominoInstance& tetronimoInstance)
{
	const Tetromino& tetronimo = s_tetrominos[tetronimoInstance.m_tetrominoType];
	const Tetromino::BlockCoords& blockCoords = tetronimo.blockCoord[tetronimoInstance.m_rotation];
	for (unsigned int i = 0; i < Tetromino::kNumBlocks; ++i)
	{
		const int x = tetronimoInstance.m_pos.x + blockCoords[i].x;
		const int y = tetronimoInstance.m_pos.y + blockCoords[i].y;

		//  выход за пределы поля -- перекрытие
		HP_ASSERT((x >= 0) && (x < (int)field.width) && (y >= 0) && (y < (int)field.height))

			field.staticBlocks[x + y * field.width] = (unsigned int)tetronimoInstance.m_tetrominoType;
	}

	// очистите все полные строки 
	unsigned int numLinesCleared = 0;
	for (unsigned int y = 0; y < field.height; ++y)
	{
		bool bRowFull = true;
		for (unsigned int x = 0; x < field.width; ++x)
		{
			if (field.staticBlocks[x + y * field.width] == -1)
			{
				bRowFull = false;
				break;
			}
		}

		if (bRowFull)
		{
			++numLinesCleared;

			//пройдите назад назад наверх копируя данные вниз
			for (unsigned int yy = y; yy > 0; --yy)
			{
				for (unsigned int x = 0; x < field.width; ++x)
				{
					field.staticBlocks[x + yy * field.width] = field.staticBlocks[x + (yy - 1) * field.width];
				}
			}
		}
	}

	unsigned int previousLevel = m_numLinesCleared / 10;
	m_numLinesCleared += numLinesCleared;
	m_level = m_numLinesCleared / 10;

	if (m_level != previousLevel)
	{
		// скорость обновления
		m_framesPerFallStep -= s_deltaFramesPerFallStepPerLevel;
		if (m_framesPerFallStep < 1)
		{
			m_framesPerFallStep = 1;
		}
	}

	// счёт
	if (numLinesCleared > 0)
	{
		unsigned int multiplier = 0;
		switch (numLinesCleared)
		{
		case 1:
			multiplier = 40;
			break;
		case 2:
			multiplier = 100;
			break;
		case 3:
			multiplier = 300;
			break;
		case 4:
			multiplier = 1200;
		}

		unsigned int score = multiplier * (previousLevel + 1);
		score += m_numUserDropsForThisTetronimo;
		m_score += score;
		if (m_score > m_hiScore)
			m_hiScore = m_score;
	}

}

//наслед
void Game::Draw(Renderer& renderer)
{
	//setlocale(LC_ALL, "Rus");
	switch (m_gameState)
	{
	case kGameState_TitleScreen: //МЕНЮ
		char text[128];
		renderer.DrawText("Нажимте ПРОБЕЛ,чтобы начать игру  ", renderer.GetLogicalWidth() / 3.5 - 50, renderer.GetLogicalHeight() / 3.3, 0xffffffff);
		renderer.DrawText("Нажмите ESC,чтобы выйти", renderer.GetLogicalWidth() / 3.5 - 50, renderer.GetLogicalHeight() / 2, 0xffffffff);
		renderer.DrawText("Нажмите H, чтобы посмотреть лучший счёт", renderer.GetLogicalWidth() / 3.5 - 50, renderer.GetLogicalHeight() / 2.5, 0xffffffff);
		renderer.DrawText("Нажмите R, чтобы посмотреть правила", renderer.GetLogicalWidth() / 3.5 - 50, renderer.GetLogicalHeight() / 2.8, 0xffffffff);

		renderer.DrawText("Выполнил - Лукашев Михаил И507Б ", renderer.GetLogicalWidth() / 2.1 - 50, renderer.GetLogicalHeight() / 1.2, 0xffffffff);

		break;
	case kGameState_Playing:
		DrawPlaying(renderer);
		break;
	case kGameState_GameOver:
		DrawPlaying(renderer);
		renderer.DrawText("ИГРА ОКОНЧЕНА", renderer.GetLogicalWidth() / 2 - 100, renderer.GetLogicalHeight() / 2, 0xffffffff);
		
		snprintf(text, sizeof(text), "ПРОБЕЛ - ВЕРНУТСЯ В МЕНЮ ", m_framesPerFallStep);
		renderer.DrawText(text, 0, 300, 0x404040ff);
		break;
	case kNumGameStates :
	{
		char text[128];
		snprintf(text, sizeof(text), "Лучший счёт: %u", m_hiScore); //добавить в меню пункт с просмотром
		renderer.DrawText(text, renderer.GetLogicalWidth() / 2.5 - 50, renderer.GetLogicalHeight() / 2.5, 0xffffffff);
		snprintf(text, sizeof(text), "ПРОБЕЛ - НАЗАД ", m_framesPerFallStep);
		renderer.DrawText(text, 0, 400, 0x404040ff);
		break;
	}
	case kGamePause :
	{
		DrawPlaying(renderer);
		renderer.DrawText("ПАУЗА", renderer.GetLogicalWidth() / 2 - 100, renderer.GetLogicalHeight() / 2, 0xffffffff);
		char text[128];
		snprintf(text, sizeof(text), "ПРОБЕЛ - ПРОДОЛЖИТЬ ", m_framesPerFallStep);
		renderer.DrawText(text, 0, 300, 0x404040ff);
		break;
	}
	case kGameRules:
	{
		char text[128];
		snprintf(text, sizeof(text), "Правила", m_framesPerFallStep);
		renderer.DrawText(text,500, 100, 0xffffffff);
		renderer.DrawText("Собирайте целые линии из фигур, чем больше линий - тем больше очков", renderer.GetLogicalWidth() / 2 - 635, renderer.GetLogicalHeight() / 3.5, 0xffffffff);
		renderer.DrawText("Выстроенная полностью линия - исчезает, Вам начисляются очки ", renderer.GetLogicalWidth() / 2 - 635, renderer.GetLogicalHeight() / 3.1, 0xffffffff);
		renderer.DrawText("Игра не завершится до тех пор, пока на поле есть свободное место ", renderer.GetLogicalWidth() / 2 - 635, renderer.GetLogicalHeight() / 2.7, 0xffffffff);
		renderer.DrawText("Если свободного места для фигуры нет - Вы проиграли ", renderer.GetLogicalWidth() / 2 - 635, renderer.GetLogicalHeight() / 2.4, 0xffffffff);
		renderer.DrawText("Ваш лучиший счёт будет записан и его можно будет посмотреть ", renderer.GetLogicalWidth() / 2 - 635, renderer.GetLogicalHeight() / 2.2, 0xffffffff);
		renderer.DrawText("Приятной Игры! ", renderer.GetLogicalWidth() / 2 - 190, renderer.GetLogicalHeight() / 1.5, 0xffffffff);
		snprintf(text, sizeof(text), "ПРОБЕЛ - НАЗАД", m_framesPerFallStep);
		renderer.DrawText(text, 0, 400, 0x404040ff);
		break;
	}
	default:
		HP_FATAL_ERROR("Unhandled case"); 
	}

	//#ifdef _DEBUG
	//трекер кадров в секунду
	float fps = 1.0f / m_deltaTimeSeconds;
	char text[128];
	snprintf(text, sizeof(text), "FPS: %.1f", fps);
	renderer.DrawText(text, 0, 0, 0x8080ffff);
	//#endif
}
// метод рисования во время игры 
void Game::DrawPlaying(Renderer& renderer)
{
	static unsigned int blockSizePixels = 32;

	//Поле	

	unsigned int fieldWidthPixels = m_field.width * blockSizePixels;
	unsigned int fieldHeightPixels = m_field.height * blockSizePixels;

	unsigned int fieldOffsetPixelsX = 0;
	if (renderer.GetLogicalWidth() > fieldWidthPixels)
	{
		fieldOffsetPixelsX = (renderer.GetLogicalWidth() - fieldWidthPixels) / 2;
	}

	unsigned int fieldOffsetPixelsY = 0;
	if (renderer.GetLogicalHeight() > fieldHeightPixels)
	{
		fieldOffsetPixelsY = (renderer.GetLogicalHeight() - fieldHeightPixels) / 2;
	}

	for (unsigned int iy = 0; iy < m_field.height; ++iy)
	{
		const unsigned int y = fieldOffsetPixelsY + iy * blockSizePixels;

		for (unsigned int ix = 0; ix < m_field.width; ++ix)
		{
			const unsigned int x = fieldOffsetPixelsX + ix * blockSizePixels;

			const int blockState = m_field.staticBlocks[iy * m_field.width + ix];
			unsigned int blockRgba = 0x202020ff;
			if (blockState != -1)
			{
				HP_ASSERT(blockState < kNumTetrominoTypes);
				blockRgba = s_tetrominos[blockState].rgba;
			}

			renderer.DrawSolidRect(x, y, blockSizePixels, blockSizePixels, blockRgba);
			renderer.DrawRect(x, y, blockSizePixels, blockSizePixels, 0x404040ff);
		}
	}


	// нарисуйте активный тетромино
	for (unsigned int i = 0; i < 4; ++i)
	{
		const Tetromino& tetromino = s_tetrominos[m_activeTetromino.m_tetrominoType];
		const Tetromino::BlockCoords& blockCoords = tetromino.blockCoord[m_activeTetromino.m_rotation];
		unsigned int tetrominoRgba = tetromino.rgba;
		const unsigned int x = fieldOffsetPixelsX + (m_activeTetromino.m_pos.x + blockCoords[i].x) * blockSizePixels;
		const unsigned int y = fieldOffsetPixelsY + (m_activeTetromino.m_pos.y + blockCoords[i].y) * blockSizePixels;
		renderer.DrawSolidRect(x, y, blockSizePixels, blockSizePixels, tetrominoRgba);
	}

	char text[128];
	snprintf(text, sizeof(text), "Линии: %u", m_numLinesCleared);
	renderer.DrawText(text, 0, 100, 0xffffffff);
	snprintf(text, sizeof(text), "Уровень: %u", m_level);
	renderer.DrawText(text, 0, 140, 0xffffffff);
	snprintf(text, sizeof(text), "Счёт: %u", m_score);
	renderer.DrawText(text, 0, 180, 0xffffffff);
	snprintf(text, sizeof(text), "Лучший счёт: %u", m_hiScore); //добавить в меню пункт с просмотром
	renderer.DrawText(text, 0, 220, 0xffffffff);

#ifdef _DEBUG
	snprintf(text, sizeof(text), "ESC - ВЫХОД", m_framesPerFallStep);
	renderer.DrawText(text, 0, 400, 0x404040ff);
	snprintf(text, sizeof(text), "P - ПАУЗА", m_framesPerFallStep);
	renderer.DrawText(text, 0, 500, 0x404040ff);
#endif
}
