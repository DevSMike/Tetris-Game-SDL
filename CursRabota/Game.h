//--------------------------------------------------------------------------------------------------
/**
	файл   Game.h
**/
//--------------------------------------------------------------------------------------------------
#include "Renderer.h"
#ifndef GAME_H
#define GAME_H

//класс дл€ рендера, наследование класса дл€ рендеринга игровых объектов
class Renderer;

// структура дл€ отслеживани€ позици€ точки на экране 
struct uint2
{
	unsigned int x;
	unsigned int y;
};
// структура дл€ отслеживани€ позици€ точки на экране 
struct int2
{
	int x;
	int y;
};
// структура - игровое поле 
struct Field
{
	unsigned int width; // высота
	unsigned int height;// длина
	int* staticBlocks;		// -1 = пусто
};

struct Tetromino
{
	static const unsigned int kNumBlocks = 4;
	static const unsigned int kNumRotations = 4;
	typedef uint2 BlockCoords[kNumBlocks];

	BlockCoords blockCoord[kNumRotations];
	unsigned int rgba;
};

enum TetrominoType
{
	kTetrominoType_I = 0,
	kTetrominoType_J,
	kTetrominoType_L,
	kTetrominoType_O,
	kTetrominoType_S,
	kTetrominoType_T,
	kTetrominoType_Z,
	kNumTetrominoTypes
};

struct TetrominoInstance
{
	TetrominoType m_tetrominoType;
	int2 m_pos;
	unsigned int m_rotation;
};
// структура, содержаща€ игровой ввод 
struct GameInput
{
	bool bStart;
	bool bMoveLeft;
	bool bMoveRight;
	bool bRotateClockwise;			
	bool bRotateAnticlockwise;		
	bool bHardDrop;
	bool bSoftDrop;
	bool bPause; //+
	bool WatchHighScore; // ƒќЅј¬»Ћ +
	bool Rules; // ƒќЅј¬»Ћ +

#ifdef _DEBUG
	bool bDebugChangeTetromino;
	bool bDebugMoveLeft;
	bool bDebugMoveRight;
	bool bDebugMoveUp;
	bool bDebugMoveDown;
#endif
};

//--------------------------------------------------------------------------------------------------
/**
	\класс   Game
**/
//--------------------------------------------------------------------------------------------------

class Game
{
public:

	Game();
	~Game();

	bool			Init(); //
	void			Shutdown();//
	void			Reset();//
	void			Update(const GameInput& gameInput, float deltaTimeSeconds);//
	void			Draw(Renderer& renderer);//

private:

	void			InitPlaying();//
	void			UpdatePlaying(const GameInput& gameInput);//
	void			DrawPlaying(Renderer& renderer);//

	bool			SpawnTetronimo();//
	void			AddTetronimoToField(const Field& field, const TetrominoInstance& tetronimoInstance); //

	// положение
	float m_deltaTimeSeconds;
	Field m_field;
	TetrominoInstance m_activeTetromino;

	int m_framesUntilFall;
	int m_framesPerFallStep;	// это измен€етс€, когда ты переходишь на новый уровень 

	unsigned int m_numUserDropsForThisTetronimo;

	// счЄт
	unsigned int m_numLinesCleared;
	unsigned int m_level;
	unsigned int m_score;
	unsigned int m_hiScore;
	// объединение - состо€ине игры 
	enum GameState
	{
		kGameState_TitleScreen = 0,
		kGameState_Playing,
		kGameState_GameOver,
		kNumGameStates,
		kGamePause,
		kGameRules
	};

	GameState m_gameState;
};

#endif // GAME_H
