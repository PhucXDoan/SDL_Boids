#include <stdio.h>
#include <SDL.h>

const int SCREEN_WIDTH  = 640;
const int SCREEN_HEIGHT = 480;

int main(int, char**)
{
	SDL_Window*  window         = 0;
	SDL_Surface* screen_surface = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
		return 1;
	}

	window = SDL_CreateWindow("hello_sdl2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	if (!window)
	{
		fprintf(stderr, "could not create window: %s\n", SDL_GetError());
		return 1;
	}

	screen_surface = SDL_GetWindowSurface(window);

	SDL_FillRect(screen_surface, 0, SDL_MapRGB(screen_surface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);
	SDL_Delay(2000);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
