#include <stdio.h>
#include <SDL.h>
#include "unified.h"

flag_struct (ArrowKey, u32)
{
	left  = 1 << 0,
	right = 1 << 1,
	down  = 1 << 2,
	up    = 1 << 3
};

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
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
				constexpr strlit TEST_IMAGE_FILE_PATHS[] =
					{
						"./ralph-0-quarter.bmp",
						"./ralph-paperball-down.bmp",
						"./ralph-paperball-up.bmp",
						"./ralph-water-down.bmp",
						"./ralph-water-up.bmp"
					};

				SDL_Surface* test_surfaces[ARRAY_CAPACITY(TEST_IMAGE_FILE_PATHS)];

				FOR_ELEMS(test_image_file_path, TEST_IMAGE_FILE_PATHS)
				{
					test_surfaces[test_image_file_path_index] = SDL_LoadBMP(*test_image_file_path);

					if (!test_surfaces[test_image_file_path_index])
					{
						fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
						ASSERT(!"Could not load this bitmap.");
						// @TODO@ How should the case of failing be handled?
						// Doing `return -1` or such skips the deallocation or whatever, could be bad.
					}
				}

				bool32   running            = true;
				ArrowKey pressed_arrow_keys = {};

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

								case SDL_KEYDOWN:
								case SDL_KEYUP:
								{
									switch (event.key.keysym.sym)
									{
										case SDLK_LEFT:
										case SDLK_RIGHT:
										case SDLK_DOWN:
										case SDLK_UP:
										{
											ArrowKey arrow_key =
												event.key.keysym.sym == SDLK_LEFT  ? ArrowKey::left  :
												event.key.keysym.sym == SDLK_RIGHT ? ArrowKey::right :
												event.key.keysym.sym == SDLK_DOWN  ? ArrowKey::down  : ArrowKey::up;

											if (event.key.state == SDL_PRESSED && !event.key.repeat)
											{
												pressed_arrow_keys |= arrow_key;
											}
											else if (event.key.state == SDL_RELEASED)
											{
												pressed_arrow_keys &= ~arrow_key;
											}
										} break;
									}
								} break;
							}
						}
					}

					SDL_FillRect(window_surface, 0, SDL_MapRGB(window_surface->format, 64, 16, 8));

					SDL_BlitSurface
					(
						test_surfaces
						[
							+(pressed_arrow_keys & ArrowKey::left ) ? 1 :
							+(pressed_arrow_keys & ArrowKey::right) ? 2 :
							+(pressed_arrow_keys & ArrowKey::down ) ? 3 :
							+(pressed_arrow_keys & ArrowKey::up   ) ? 4 : 0
						],
						0,
						window_surface,
						0
					);

					SDL_UpdateWindowSurface(window);
				}

				FOR_ELEMS(test_surface, test_surfaces)
				{
					SDL_FreeSurface(*test_surface);
				}
			}
			else
			{
				fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
				ASSERT(!"SDL could not create a surface for the window.");
			}

			SDL_FreeSurface(window_surface);
		}
		else
		{
			fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
			ASSERT(!"SDL could not create window.");
		}

		SDL_DestroyWindow(window);
	}

	SDL_Quit();

	return 0;
}
