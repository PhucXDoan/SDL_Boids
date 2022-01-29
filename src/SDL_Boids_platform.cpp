#include "SDL_Boids_platform.h"
#include "SDL_Boids.cpp"

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not initialize video.");
	}
	else
	{
		SDL_Window* window = SDL_CreateWindow("SDL_Boids", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

		if (window)
		{
			SDL_Renderer* window_renderer = SDL_CreateRenderer(window, -1, 0);

			if (window_renderer)
			{
				PLATFORM_Program program;
				program.is_running      = true;
				program.is_initialized  = false;
				program.delta_seconds   = 0;
				program.renderer        = window_renderer;
				program.memory          = reinterpret_cast<byteptr>(VirtualAlloc(reinterpret_cast<LPVOID>(tebibytes_of(4)), static_cast<size_t>(MEMORY_CAPACITY), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
				program.memory_capacity = MEMORY_CAPACITY;

				u64 performance_count = SDL_GetPerformanceCounter();

				while (program.is_running)
				{
					u64 new_performance_count = SDL_GetPerformanceCounter();
					program.delta_seconds     = static_cast<f32>(new_performance_count - performance_count) / SDL_GetPerformanceFrequency();
					performance_count         = new_performance_count;

					update(&program);
				}
			}
			else
			{
				fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
				ASSERT(!"SDL could not create a renderer for the window.");
			}

			SDL_DestroyRenderer(window_renderer);
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
