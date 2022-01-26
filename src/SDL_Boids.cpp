#include <stdio.h>
#include <SDL.h>
#include "unified.h"

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not initialize video.");
	}
	else
	{
		constexpr i32 WINDOW_WIDTH  = 1280;
		constexpr i32 WINDOW_HEIGHT = 720;

		SDL_Window* window = SDL_CreateWindow("SDL_Boids", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

		if (window)
		{
			SDL_Surface* window_surface = SDL_GetWindowSurface(window);

			if (window_surface)
			{
				constexpr strlit TEST_IMAGE_FILE_PATH = "./ralph-quarter.bmp";
				SDL_Surface* test_surface = SDL_LoadBMP(TEST_IMAGE_FILE_PATH);

				if (test_surface)
				{
					bool32 running = true;

					while (running)
					{
						{
							SDL_Event event;
							while (SDL_PollEvent(&event))
							{
								switch (event.type)
								{
									case SDL_QUIT:
									{
										running = false;
									} break;
								}
							}
						}

						SDL_FillRect(window_surface, 0, SDL_MapRGB(window_surface->format, 64, 16, 8));

						SDL_BlitSurface(test_surface, 0, window_surface, 0);

						SDL_UpdateWindowSurface(window);
					}
				}
				else
				{
					printf("SDL_Error: '%s'\n", SDL_GetError());
					ASSERT(!"Could not load the bitmap.");
				}

				SDL_FreeSurface(test_surface);
			}
			else
			{
				printf("SDL_Error: '%s'\n", SDL_GetError());
				ASSERT(!"SDL could not create a surface for the window.");
			}

			SDL_FreeSurface(window_surface);
		}
		else
		{
			printf("SDL_Error: '%s'\n", SDL_GetError());
			ASSERT(!"SDL could not create window.");
		}

		SDL_DestroyWindow(window);
	}

	SDL_Quit();

	return 0;
}
