#include <stdio.h>
#include <SDL.h>

// @TODO@ Render boids using textures?
// @TODO@ Should resizing of window be allowed?
// @TODO@ Make grid centered?

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
				constexpr i32 PIXELS_PER_METER = 100;
				constexpr vf2 BOID_VERTICES[] =
					{
						{   9.0f,   0.0f },
						{ -11.0f,  10.0f },
						{  -3.0f,   0.0f },
						{ -11.0f, -10.0f },
						{   9.0f,   0.0f }
					};
				constexpr i32 BOID_AMOUNT = 8;

				Boid boid_swap_arrays[2][BOID_AMOUNT];
				Boid (*new_boids)[BOID_AMOUNT] = &boid_swap_arrays[0];
				Boid (*old_boids)[BOID_AMOUNT] = &boid_swap_arrays[1];

				FOR_ELEMS(new_boid, *new_boids)
				{
					new_boid->angular_acceleration = 0.1f;
					new_boid->angular_velocity     = 0.0f;
					new_boid->angle                = 0.0f;
					new_boid->acceleration         = 0.1f;
					new_boid->velocity             = 0.0f;
					new_boid->position             = { 1.0f + new_boid_index, 4.0f };
				}

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

					SWAP(new_boids, old_boids);

					SDL_SetRenderDrawColor(window_renderer, 0, 0, 0, 255);
					SDL_RenderClear(window_renderer);

					//
					// Grid.
					//

					SDL_SetRenderDrawColor(window_renderer, 255, 255, 255, 255);
					FOR_RANGE(i, 0, WINDOW_WIDTH / PIXELS_PER_METER + 1)
					{
						i32 x = i * PIXELS_PER_METER;
						SDL_RenderDrawLine(window_renderer, x, 0, x, WINDOW_HEIGHT);
					}

					FOR_RANGE(i, 0, WINDOW_HEIGHT / PIXELS_PER_METER + 1)
					{
						i32 y = WINDOW_HEIGHT - 1 - i * PIXELS_PER_METER;
						SDL_RenderDrawLine(window_renderer, 0, y, WINDOW_WIDTH, y);
					}

					//
					// Boids.
					//

					SDL_SetRenderDrawColor(window_renderer, 222, 173, 38, 255);

					FOR_ELEMS(new_boid, *new_boids)
					{
						Boid* old_boid = &(*old_boids)[new_boid_index];

						new_boid->angular_acceleration = 0.05f;
						new_boid->angular_velocity     = old_boid->angular_velocity + old_boid->angular_acceleration * delta_seconds;
						new_boid->angle                = old_boid->angle + old_boid->angular_velocity * delta_seconds + 0.5f * old_boid->angular_acceleration * delta_seconds * delta_seconds;
						new_boid->acceleration         = 0.1f;
						new_boid->velocity             = old_boid->velocity + old_boid->acceleration * delta_seconds;
						new_boid->position             = old_boid->position + (old_boid->velocity * delta_seconds + 0.5f * old_boid->acceleration * delta_seconds * delta_seconds) * vf2 { cos_f32(old_boid->angle), sin_f32(old_boid->angle) };

						vf2 direction = { cos_f32(new_boid->angle), sin_f32(new_boid->angle) };
						vf2 offset    = new_boid->position * static_cast<f32>(PIXELS_PER_METER);

						SDL_Point pixel_points[ARRAY_CAPACITY(BOID_VERTICES)];
						FOR_ELEMS(pixel_point, pixel_points)
						{
							*pixel_point =
								{
									static_cast<i32>(BOID_VERTICES[pixel_point_index].x * direction.x - BOID_VERTICES[pixel_point_index].y * direction.y + offset.x),
									WINDOW_HEIGHT - 1 - static_cast<i32>(BOID_VERTICES[pixel_point_index].x * direction.y + BOID_VERTICES[pixel_point_index].y * direction.x + offset.y)
								};
						}

						SDL_RenderDrawLines(window_renderer, pixel_points, ARRAY_CAPACITY(pixel_points));
						SDL_RenderDrawPoint(window_renderer, static_cast<i32>(offset.x), WINDOW_HEIGHT - 1 - static_cast<i32>(offset.y));
					}

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
