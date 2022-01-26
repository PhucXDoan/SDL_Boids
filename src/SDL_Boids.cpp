#include <stdio.h>
#include <SDL.h>

// @TODO@ Render boids using textures?
// @TODO@ Should resizing of window be allowed?
// @TODO@ Make grid centered?
// @TODO@ Flip the rendering upside down.

#define DEBUG 1
#include "unified.h"

flag_struct (ArrowKey, u32)
{
	left  = 1 << 0,
	right = 1 << 1,
	down  = 1 << 2,
	up    = 1 << 3
};

struct Boid
{
	f32 angular_acceleration;
	f32 angular_velocity;
	f32 angle;
	f32 acceleration;
	f32 velocity;
	vf2 position;
};

// @TODO@ Implement the trig functions.
inline f32 cos_f32(f32 rad)
{
	return static_cast<f32>(cos(rad));
}

inline f32 sin_f32(f32 rad)
{
	return static_cast<f32>(sin(rad));
}

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
			SDL_Renderer* window_renderer = SDL_CreateRenderer(window, -1, 0);

			if (window_renderer)
			{
				i32 pixels_per_meter = 100;

				constexpr vf2 BOID_VERTICES[] =
					{
						{   9.0f,   0.0f },
						{ -11.0f,  10.0f },
						{  -3.0f,   0.0f },
						{ -11.0f, -10.0f },
						{   9.0f,   0.0f }
					};

				Boid TEST_boid;
				TEST_boid.angular_acceleration = 0.0f;
				TEST_boid.angular_velocity     = 0.0f;
				TEST_boid.angle                = 0.0f;
				TEST_boid.acceleration         = 0.0f;
				TEST_boid.velocity             = 0.0f;
				TEST_boid.position             = { 3.0f, 4.0f };

				u64    performance_count = SDL_GetPerformanceCounter();
				bool32 running = true;
				while (running)
				{
					for (SDL_Event event; SDL_PollEvent(&event);)
					{
						switch (event.type)
						{
							case SDL_QUIT:
							{
								running = false;
							} break;

							#if 0
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
							#endif
						}
					}

					f32 delta_seconds;
					{
						u64 new_performance_count = SDL_GetPerformanceCounter();
						delta_seconds = static_cast<f32>(new_performance_count - performance_count) / SDL_GetPerformanceFrequency();
						performance_count = new_performance_count;
					}

					//
					// Clears.
					//
					SDL_SetRenderDrawColor(window_renderer, 0, 0, 0, 255);
					SDL_RenderClear(window_renderer);

					//
					// Grid.
					//
					SDL_SetRenderDrawColor(window_renderer, 255, 255, 255, 255);
					FOR_RANGE(i, 0, WINDOW_WIDTH / pixels_per_meter + 1)
					{
						i32 x = i * pixels_per_meter;
						SDL_RenderDrawLine(window_renderer, x, 0, x, WINDOW_HEIGHT);
					}

					FOR_RANGE(i, 0, WINDOW_HEIGHT / pixels_per_meter + 1)
					{
						i32 y = i * pixels_per_meter;
						SDL_RenderDrawLine(window_renderer, 0, y, WINDOW_WIDTH, y);
					}

					//
					// Boids.
					//

					TEST_boid.angular_acceleration  = 0.05f;
					TEST_boid.angular_velocity     += TEST_boid.angular_acceleration * delta_seconds;
					TEST_boid.angle                += TEST_boid.angular_velocity * delta_seconds;

					vf2 direction = { cos_f32(TEST_boid.angle), sin_f32(TEST_boid.angle) };

					TEST_boid.acceleration   = 0.1f;
					TEST_boid.velocity      += TEST_boid.acceleration * delta_seconds;
					TEST_boid.position      += TEST_boid.velocity * direction * delta_seconds;

					SDL_SetRenderDrawColor(window_renderer, 222, 173, 38, 255);

					vf2 offset = TEST_boid.position * static_cast<f32>(pixels_per_meter);

					SDL_Point boid_pixel_points[ARRAY_CAPACITY(BOID_VERTICES)];

					FOR_ELEMS(boid_pixel_point, boid_pixel_points)
					{
						*boid_pixel_point =
							{
								static_cast<i32>(BOID_VERTICES[boid_pixel_point_index].x * direction.x - BOID_VERTICES[boid_pixel_point_index].y * direction.y + offset.x),
								static_cast<i32>(BOID_VERTICES[boid_pixel_point_index].x * direction.y + BOID_VERTICES[boid_pixel_point_index].y * direction.x + offset.y)
							};
					}

					SDL_RenderDrawLines(window_renderer, boid_pixel_points, ARRAY_CAPACITY(boid_pixel_points));
					SDL_RenderDrawPoint(window_renderer, static_cast<i32>(offset.x), static_cast<i32>(offset.y));

					SDL_RenderPresent(window_renderer);
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
