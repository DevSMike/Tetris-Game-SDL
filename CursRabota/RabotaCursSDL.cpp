#include "App.h"

#include "SDL.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "Russian");
	bool bFullScreen = false;
	unsigned int displayWidth = 1280;
	unsigned int displayHeight = 720;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--fullscreen") == 0)
		{
			bFullScreen = true;
		}
		else if (strcmp(argv[i], "--width") == 0)
		{
			SDL_assert(argc > i); // убеждаемся, что у нас есть аргумент 
			displayWidth = (unsigned int)atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "--height") == 0)
		{
			SDL_assert(argc > i); // убеждаемся, что у нас есть аргумент 
			displayHeight = (unsigned int)atoi(argv[++i]);
		}
	}

	App app; //создаем элемент класса ПРИЛОЖЕНИЕ
	if (!app.Init(bFullScreen, displayWidth, displayHeight))
	{
		printf("ERROR - Не получилось инициализировать\n");
		app.ShutDown();
		return 1;
	}

	app.Run();

	app.ShutDown();

	return 0;
}