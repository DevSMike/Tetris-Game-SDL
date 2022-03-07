//--------------------------------------------------------------------------------------------------
/**
	файл   App.cpp
**/
//--------------------------------------------------------------------------------------------------

#include "App.h"
// подключаем все заголовочные файлы проекта 
#include "Debug.h" //класс дебага 
#include "Game.h" // класс игры 
#include "Renderer.h" //класс рендера 

#include "SDL.h" //SDL 2
#include "SDL_ttf.h" //SDL_ttf2

#ifdef __VCCOREVER__	
#include <GLES2/gl2.h>
#endif

#include <stdio.h>
#include <chrono>

//--------------------------------------------------------------------------------------------------

static void print_SDL_version(const char* preamble, const SDL_version& v)
{
	printf("%s %u.%u.%u\n", preamble, v.major, v.minor, v.patch);
}

static bool operator==(SDL_version& a, SDL_version& b)
{
	return (a.major == b.major) && (a.minor == b.minor) && (a.patch == b.patch);
}

#ifdef GL_ES_VERSION_2_0

static void SetGLAttribute(SDL_GLattr attr, int value)
{
	if (SDL_GL_SetAttribute(attr, value) != 0)
	{
		fprintf(stderr, "SDL_GL_SetAttr failed: %s\n", SDL_GetError());
		HP_FATAL_ERROR("SDL_GL_SetAttr failed");
	}
}

static void PrintGLString(GLenum name)
{
	const GLubyte* ret = glGetString(name);
	if (ret == 0)
	{
		fprintf(stderr, "Failed to get GL string: %d\n", name);
	}
	else
	{
		printf("%s\n", ret);
	}
}

#endif // GL_ES_VERSION_2_0

//--------------------------------------------------------------------------------------------------

App::App()
	: m_pWindow(nullptr)
	, m_pRenderer(nullptr)
	, m_pGame(nullptr)
{

}
// булева€ функци€ инициализации, возврат false, если не получилось выполнить функцию SDL_INIT_EVERYTHING
bool App::Init(bool bFullScreen, unsigned int displayWidth, unsigned int displayHeight)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
		return false;
	}

	printf("SDL initialised\n");

	SDL_version compiledVersion;
	SDL_version linkedVersion;
	SDL_VERSION(&compiledVersion);
	SDL_GetVersion(&linkedVersion);
	print_SDL_version("Compiled against SDL version", compiledVersion);
	print_SDL_version("Linking against SDL version", linkedVersion);
	SDL_assert_release((compiledVersion == linkedVersion));

	int numDisplays = SDL_GetNumVideoDisplays(); // количество доступных видеодисплеев
	printf("%d video displays\n", numDisplays);
	for (int i = 0; i < numDisplays; ++i)
	{
		SDL_DisplayMode displayMode; // переменна€ дл€ записи информации о данном дисплее 
		if (SDL_GetCurrentDisplayMode(i, &displayMode) != 0)
		{
			fprintf(stderr, "Failed to get display mode for video display %d: %s", i, SDL_GetError());
			continue;
		}

		printf("Display %d: w=%d, h=%d refresh_rate=%d\n", i, displayMode.w, displayMode.h, displayMode.refresh_rate);
	}

#ifdef GL_ES_VERSION_2_0
	SetGLAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SetGLAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SetGLAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif

	const char* title = "SDL Kursach";
	//если удалось создать окно - то созадем, иначе выводим ошибку 
	if (bFullScreen)
	{
		HP_FATAL_ERROR("Just checking");
		m_pWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else
	{
		m_pWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayWidth, displayHeight, SDL_WINDOW_SHOWN /*| SDL_WINDOW_OPENGL*/);
	}

	if (!m_pWindow)
	{
		printf("Failed to create SDL window: %s\n", SDL_GetError());
		return false;
	}

#ifdef GL_ES_VERSION_2_0
	// Let's see if we can use OpenGL ES 2 on Raspberry Pi
	SDL_GLContext gl_context = SDL_GL_CreateContext(m_pWindow);
	printf("GL_VERSION: ");
	PrintGLString(GL_VERSION);
	printf("GL_RENDERER: ");
	PrintGLString(GL_RENDERER);
	printf("GL_SHADING_LANGUAGE_VERSION: ");
	PrintGLString(GL_SHADING_LANGUAGE_VERSION);
	printf("GL_EXTENSIONS: ");
	PrintGLString(GL_EXTENSIONS);
	SDL_GL_DeleteContext(gl_context);
#endif

	// SDL2_ttf
	//провер€ем подключенность библиотеки SDL_ttf
	if (TTF_Init() == -1)
	{
		fprintf(stderr, "Failed to initialise SDL2_ttf: %s\n", TTF_GetError());
		return false;
	}

	printf("SDL_ttf initialised\n");

	SDL_TTF_VERSION(&compiledVersion);
	const SDL_version *pLinkedVersion = TTF_Linked_Version();
	print_SDL_version("Compiled against SDL_ttf version", compiledVersion);
	print_SDL_version("Linking against SDL_ttf version", *pLinkedVersion);
	// значение дл€ окна (1280 * 720) 
	unsigned int logicalWidth = 1280;
	unsigned int logicalHeight = 720;
	m_pRenderer = new Renderer(*m_pWindow, logicalWidth, logicalHeight); // рендерим окно данного размера 

	m_pGame = new Game(); // создаем элемент класса »гра 

	if (!m_pGame->Init()) //если не получилось инициализировать , то выдаем ошибку 
	{
		fprintf(stderr, "ERROR - Game failed to initialise\n");
		return false;
	}

	return true;
}
// метод закрывани€ игры 
void App::ShutDown()  
{
	if (m_pGame)
	{
		m_pGame->Shutdown();
		delete m_pGame;
		m_pGame = nullptr;
	}

	delete m_pRenderer;
	m_pRenderer = nullptr;

	TTF_Quit();	// SDL2_TTF

	SDL_DestroyWindow(m_pWindow); // уничтожаем окно 
	SDL_Quit(); // выодим из SDL2
}
// метод запуска игры 
void App::Run()
{
	Uint32 lastTimeMs = SDL_GetTicks();	// только точность мс, SDL_GetTricks - ѕолучите количество миллисекунд с момента инициализации библиотеки SDL.
	auto lastTime = std::chrono::high_resolution_clock::now();	// высокоточное врем€

	bool bDone = false;
	while (!bDone)
	{
		GameInput gameInput = {};

		// —„»“џ¬ј≈ћ ƒ≈…—“¬»я ѕќЋ№«ќ¬ј“≈Ћя 
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				bDone = true;
			}

			// считываем все символы 
			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					bDone = true;
				}
				else if (event.key.keysym.sym == SDLK_SPACE)
				{
					gameInput.bStart = true;
				}
				else if (event.key.keysym.sym == SDLK_LEFT)
				{
					gameInput.bMoveLeft = true;
				}
				else if (event.key.keysym.sym == SDLK_RIGHT)
				{
					gameInput.bMoveRight = true;
				}
				else if (event.key.keysym.sym == SDLK_z)
				{
					gameInput.bRotateClockwise = true;
				}
				else if (event.key.keysym.sym == SDLK_x)
				{
					gameInput.bRotateAnticlockwise = true;
				}
				else if (event.key.keysym.sym == SDLK_UP)
				{
					gameInput.bHardDrop = true;
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					gameInput.bSoftDrop = true;
				}
				else if (event.key.keysym.sym == SDLK_p)
				{
					gameInput.bPause = true;
				}
				else if (event.key.keysym.sym == SDLK_h)
				{
					gameInput.WatchHighScore = true;
				}
				else if (event.key.keysym.sym == SDLK_r)
				{
					gameInput.Rules = true;
				}
#ifdef _DEGBUG
				else if (event.key.keysym.sym == SDLK_t)
				{
					gameInput.bDebugChangeTetromino = true;
				}
				else if (event.key.keysym.sym == SDLK_a)
				{
					gameInput.bDebugMoveLeft = true;
				}
				else if (event.key.keysym.sym == SDLK_d)
				{
					gameInput.bDebugMoveRight = true;
				}
				else if (event.key.keysym.sym == SDLK_w)
				{
					gameInput.bDebugMoveUp = true;
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					gameInput.bDebugMoveDown = true;
				}
#endif
			}
		}

		Uint32 currentTimeMs = SDL_GetTicks();
		Uint32 deltaTimeMs = currentTimeMs - lastTimeMs;
		lastTimeMs = currentTimeMs;
		HP_UNUSED(deltaTimeMs);		

		auto currentTime = std::chrono::high_resolution_clock::now();	
		auto deltaTime = currentTime - lastTime;
		std::chrono::microseconds deltaTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(deltaTime);
		float deltaTimeSeconds = 0.000001f * (float)deltaTimeMicroseconds.count();
		lastTime = currentTime;

		m_pGame->Update(gameInput, deltaTimeSeconds);

		m_pRenderer->Clear();
		m_pGame->Draw(*m_pRenderer);
		m_pRenderer->Present();
	}
}