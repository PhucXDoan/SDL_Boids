#include <stdio.h>
#include <SDL.h>

// @TODO@ Render boids using textures?
// @TODO@ Should resizing of window be allowed?
// @TODO@ Make grid centered?
// @TODO@ Spatial partition.

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
	f32 angle;
	vf2 position;
};

// @TODO@ Implement the math functions.
inline f32 cos_f32(f32 rad)
{
	return static_cast<f32>(cos(rad));
}

inline f32 sin_f32(f32 rad)
{
	return static_cast<f32>(sin(rad));
}

inline f32 arccos_f32(f32 ratio)
{
	ASSERT(-1.0f <= ratio && ratio <= 1.0f);
	return static_cast<f32>(acos(ratio));
}

inline f32 norm(vf2 v)
{
	return static_cast<f32>(sqrt(v.x * v.x + v.y * v.y));
}

inline vf2 normalize(vf2 v)
{
	f32 n = norm(v);
	ASSERT(n);
	return v / n;
}

inline f32 modulo_f32(f32 n, f32 m)
{
	f32 result = n;

	while (result > m)
	{
		result -= m;
	}

	while (result < 0)
	{
		result += m;
	}

	return result;
}

inline f32 square(f32 x) { return x * x; }

inline f32 dot(vf2 u, vf2 v)
{
	return u.x * v.x + u.y * v.y;
}

void DrawCircle(SDL_Renderer* renderer, i32 centerX, i32 centerY, i32 radius)
{
	i32 diameter = radius * 2;
	i32 x        = radius - 1;
	i32 y        = 0;
	i32 tx       = 1;
	i32 ty       = 1;
	i32 error    = tx - diameter;

	while (x >= y)
	{
		SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);
		SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
		SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
		SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
		SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
		SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
		SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
		SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);

		if (error <= 0)
		{
			++y;
			error += ty;
			ty += 2;
		}

		if (error > 0)
		{
			--x;
			tx += 2;
			error += tx - diameter;
		}
	}
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
				constexpr i32 BOID_AMOUNT              = 32;
				constexpr f32 BOID_VELOCITY            = 0.5f;
				constexpr f32 BOID_NEIGHBORHOOD_RADIUS = 1.0f;

				Boid boid_swap_arrays[2][BOID_AMOUNT];
				Boid (*new_boids)[BOID_AMOUNT] = &boid_swap_arrays[0];
				Boid (*old_boids)[BOID_AMOUNT] = &boid_swap_arrays[1];

				FOR_ELEMS(new_boid, *new_boids)
				{
					new_boid->angle    = modulo_f32(static_cast<f32>(new_boid_index), TAU);
					new_boid->position = { 4.0f + new_boid_index % 8 / 3.0f, 3.0f + new_boid_index / 8 / 3.0f };
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
						Boid* old_boid      = &(*old_boids)[new_boid_index];
						vf2   old_direction = { cos_f32(old_boid->angle), sin_f32(old_boid->angle) };

						vf2 average_neighboring_boid_repulsion        = { 0.0f, 0.0f };
						vf2 average_neighboring_boid_facing_direction = { 0.0f, 0.0f };
						vf2 cohesion_direction                        = { 0.0f, 0.0f };
						i32 neighboring_boid_count                    = 0;

						FOR_ELEMS(other_old_boid, *old_boids)
						{
							vf2 to_other          = other_old_boid->position - old_boid->position;
							f32 distance_to_other = norm(to_other);

							if (other_old_boid != old_boid && 0.0f < distance_to_other && distance_to_other < BOID_NEIGHBORHOOD_RADIUS)
							{
								++neighboring_boid_count;
								average_neighboring_boid_repulsion        -= normalize(to_other) * (1.0f - distance_to_other / BOID_NEIGHBORHOOD_RADIUS);
								average_neighboring_boid_facing_direction += { cos_f32(other_old_boid->angle), sin_f32(other_old_boid->angle) };
								cohesion_direction                        += other_old_boid->position;
							}
						}

						f32 delta_angle = 0.0f;

						if (neighboring_boid_count)
						{
							average_neighboring_boid_repulsion        /= static_cast<f32>(neighboring_boid_count);
							average_neighboring_boid_facing_direction /= static_cast<f32>(neighboring_boid_count);
							cohesion_direction                        /= static_cast<f32>(neighboring_boid_count);
							cohesion_direction                        -= old_boid->position;

							if (+average_neighboring_boid_repulsion)
							{
								average_neighboring_boid_repulsion = normalize(average_neighboring_boid_repulsion);
							}

							if (+average_neighboring_boid_facing_direction)
							{
								average_neighboring_boid_facing_direction = normalize(average_neighboring_boid_facing_direction);
							}

							if (+cohesion_direction)
							{
								cohesion_direction = normalize(cohesion_direction);
							}
						}

						vf2 steering = { 0.0f, 0.0f };
						steering += cohesion_direction + average_neighboring_boid_facing_direction + average_neighboring_boid_repulsion;
						steering += vf2 { -1.0f,  0.0 } * 15.0f * square(square(square(square(square(       old_boid->position.x / (static_cast<f32>(WINDOW_WIDTH ) / PIXELS_PER_METER))))));
						steering += vf2 {  1.0f,  0.0 } * 15.0f * square(square(square(square(square(1.0f - old_boid->position.x / (static_cast<f32>(WINDOW_WIDTH ) / PIXELS_PER_METER))))));
						steering += vf2 {  0.0f, -1.0 } * 15.0f * square(square(square(square(square(       old_boid->position.y / (static_cast<f32>(WINDOW_HEIGHT) / PIXELS_PER_METER))))));
						steering += vf2 {  0.0f,  1.0 } * 15.0f * square(square(square(square(square(1.0f - old_boid->position.y / (static_cast<f32>(WINDOW_HEIGHT) / PIXELS_PER_METER))))));
						if (norm(steering))
						{
							vf2 steering_direction = normalize(steering);

							f32 dot_prod = dot(steering_direction, old_direction);

							if (-1.0f <= dot_prod && dot_prod <= 1.0f) // @TODO@ Somehow avoid this floating point error?
							{
								delta_angle = arccos_f32(dot_prod) * (dot(steering_direction, { -old_direction.y, old_direction.x }) < 0.0f ? -1.0f : 1.0f) * norm(steering);
							}
						}

						new_boid->angle = modulo_f32(old_boid->angle + delta_angle * 0.5f * delta_seconds, TAU);

						new_boid->position = old_boid->position + BOID_VELOCITY * old_direction * delta_seconds;

						//
						// Boid Rendering.
						//

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

						#if 0
						SDL_RenderDrawLine
						(
							window_renderer,
							static_cast<i32>(offset.x),
							WINDOW_HEIGHT - 1 - static_cast<i32>(offset.y),
							static_cast<i32>(offset.x) + static_cast<i32>(steering_direction.x * 50.0f),
							WINDOW_HEIGHT - 1 - (static_cast<i32>(offset.y) + static_cast<i32>(steering_direction.y * 50.0f))
						);

						DrawCircle(window_renderer, static_cast<i32>(offset.x), WINDOW_HEIGHT - 1 - static_cast<i32>(offset.y), static_cast<i32>(BOID_NEIGHBOR_RADIUS * PIXELS_PER_METER));
						#endif
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
