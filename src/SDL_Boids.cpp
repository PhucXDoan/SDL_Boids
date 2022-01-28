#include <stdio.h>
#include <SDL.h>

// @TODO@ Render boids using textures?
// @TODO@ Should resizing of window be allowed?
// @TODO@ Make grid centered?
// @TODO@ Spatial partition.

#define DEBUG 1
#include "unified.h"

#if 0
flag_struct (ArrowKey, u32)
{
	left  = 1 << 0,
	right = 1 << 1,
	down  = 1 << 2,
	up    = 1 << 3
};
#endif

struct Boid
{
	vf2 direction;
	vf2 position;
};

struct IndexBufferNode
{
	i32              index_buffer[32];
	i32              index_count;
	IndexBufferNode* next_node;
};

struct ChunkNode
{
	i32              x;
	i32              y;
	IndexBufferNode* index_buffer_node;
	ChunkNode*       next_node;
};

struct Map
{
	IndexBufferNode* available_index_buffer_node;
	ChunkNode*       available_chunk_node;
	ChunkNode*       chunk_node_hash_table[256];
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

inline constexpr i32 floor_f32(f32 f)
{
	i32 t = static_cast<i32>(f);
	return t + ((f < 0.0f && f - t) ? -1 : 0);
}

inline constexpr i32 ceil_f32(f32 f)
{
	return -floor_f32(-f);
}

inline vf2 complex_mul(vf2 u, vf2 v)
{
	return { u.x * v.x - u.y * v.y, u.x * v.y + u.y * v.x };
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

// @TODO@ Hacky! lol.
inline f32 very_strong_curve(f32 x)
{
	f32 y = CLAMP(x, -1.0f, 1.0f);
	return y * y * y * y * y * y * y * y * y;
}

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

IndexBufferNode* pop_from_available_index_buffer_node(Map* map)
{
	ASSERT(map->available_index_buffer_node);

	IndexBufferNode* node = map->available_index_buffer_node;
	map->available_index_buffer_node = map->available_index_buffer_node->next_node;

	return node;
}

ChunkNode** find_chunk_node(Map* map, i32 x, i32 y)
{
	// @TODO@ Better hash function.
	i32         hash       = (x * 7 + y * 13) % ARRAY_CAPACITY(map->chunk_node_hash_table);
	ChunkNode** chunk_node = &map->chunk_node_hash_table[hash];

	while (*chunk_node && ((*chunk_node)->x != x || (*chunk_node)->y != y))
	{
		chunk_node = &(*chunk_node)->next_node;
	}

	return chunk_node;
}

void push_index_into_map(Map* map, i32 x, i32 y, i32 index)
{
	ChunkNode** chunk_node = find_chunk_node(map, x, y);

	if (*chunk_node)
	{
		ASSERT((*chunk_node)->x == x && (*chunk_node)->y == y);
		ASSERT((*chunk_node)->index_buffer_node);

		if ((*chunk_node)->index_buffer_node->index_count >= ARRAY_CAPACITY(IndexBufferNode, index_buffer))
		{
			IndexBufferNode* new_head = pop_from_available_index_buffer_node(map);
			new_head->index_count            = 0;
			new_head->next_node              = (*chunk_node)->index_buffer_node;
			(*chunk_node)->index_buffer_node = new_head;
		}

		ASSERT(IN_RANGE((*chunk_node)->index_buffer_node->index_count, 0, ARRAY_CAPACITY(IndexBufferNode, index_buffer)));
		(*chunk_node)->index_buffer_node->index_buffer[(*chunk_node)->index_buffer_node->index_count] = index;
		++(*chunk_node)->index_buffer_node->index_count;
	}
	else
	{
		ASSERT(map->available_chunk_node);

		*chunk_node               = map->available_chunk_node;
		map->available_chunk_node = map->available_chunk_node->next_node;

		ASSERT(!(*chunk_node)->index_buffer_node);

		(*chunk_node)->x                                  = x;
		(*chunk_node)->y                                  = y;
		(*chunk_node)->index_buffer_node                  = pop_from_available_index_buffer_node(map);
		(*chunk_node)->index_buffer_node->index_buffer[0] = index;
		(*chunk_node)->index_buffer_node->index_count     = 1;
		(*chunk_node)->index_buffer_node->next_node       = 0;
		(*chunk_node)->next_node                          = 0;
	}
}

void remove_index_from_map(Map* map, i32 x, i32 y, i32 index)
{
	ChunkNode** chunk_node = find_chunk_node(map, x, y);

	ASSERT(*chunk_node);
	ASSERT((*chunk_node)->x == x && (*chunk_node)->y == y);
	ASSERT((*chunk_node)->index_buffer_node);

	IndexBufferNode** current_index_buffer_node = &(*chunk_node)->index_buffer_node;
	bool32            is_hunting                = true;

	while (is_hunting && *current_index_buffer_node)
	{
		FOR_ELEMS(other_index, (*current_index_buffer_node)->index_buffer, (*current_index_buffer_node)->index_count)
		{
			if (*other_index == index)
			{
				is_hunting = false;
				--(*current_index_buffer_node)->index_count;

				ASSERT(IN_RANGE((*current_index_buffer_node)->index_count, 0, ARRAY_CAPACITY(IndexBufferNode, index_buffer)));

				if ((*current_index_buffer_node)->index_count)
				{
					(*current_index_buffer_node)->index_buffer[other_index_index] = (*current_index_buffer_node)->index_buffer[(*current_index_buffer_node)->index_count];
				}
				else
				{
					IndexBufferNode* next_node = (*current_index_buffer_node)->next_node;
					(*current_index_buffer_node)->next_node = map->available_index_buffer_node;
					map->available_index_buffer_node        = *current_index_buffer_node;
					*current_index_buffer_node              = next_node;
				}

				break;
			}
		}

		current_index_buffer_node = &(*current_index_buffer_node)->next_node;
	}

	ASSERT(!is_hunting);

	if (!(*chunk_node)->index_buffer_node)
	{
		ChunkNode* next_node = (*chunk_node)->next_node;
		(*chunk_node)->next_node  = map->available_chunk_node;
		map->available_chunk_node = *chunk_node;
		*chunk_node               = next_node;
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
						{   5.0f,   0.0f },
						{  -5.0f,   5.0f },
						{  -1.0f,   0.0f },
						{  -5.0f,  -5.0f },
						{   5.0f,   0.0f }
					};
				constexpr i32 BOID_AMOUNT   = 2048;
				constexpr f32 BOID_VELOCITY = 1.0f;

				// @TODO@ Perhaps malloc these.
				IndexBufferNode TEMP_available_index_buffer_nodes[256];
				FOR_RANGE(i, 0, ARRAY_CAPACITY(TEMP_available_index_buffer_nodes) - 1)
				{
					TEMP_available_index_buffer_nodes[i].next_node = &TEMP_available_index_buffer_nodes[i + 1];
				}
				TEMP_available_index_buffer_nodes[ARRAY_CAPACITY(TEMP_available_index_buffer_nodes) - 1].next_node = 0;

				ChunkNode TEMP_available_chunk_nodes[ARRAY_CAPACITY(Map, chunk_node_hash_table)];
				FOR_RANGE(i, 0, ARRAY_CAPACITY(TEMP_available_chunk_nodes) - 1)
				{
					TEMP_available_chunk_nodes[i].next_node = &TEMP_available_chunk_nodes[i + 1];
				}
				TEMP_available_chunk_nodes[ARRAY_CAPACITY(TEMP_available_chunk_nodes) - 1].next_node = 0;

				Map map;
				map.available_index_buffer_node = &TEMP_available_index_buffer_nodes[0];
				map.available_chunk_node        = &TEMP_available_chunk_nodes[0];
				FOR_ELEMS(chunk_node, map.chunk_node_hash_table)
				{
					*chunk_node = 0;
				}

				Boid boid_swap_arrays[2][BOID_AMOUNT];
				Boid (*new_boids)[BOID_AMOUNT] = &boid_swap_arrays[0];
				Boid (*old_boids)[BOID_AMOUNT] = &boid_swap_arrays[1];

				FOR_ELEMS(new_boid, *new_boids)
				{
					f32 angle = modulo_f32(static_cast<f32>(new_boid_index), TAU);
					new_boid->direction = { cos_f32(angle), sin_f32(angle) };
					new_boid->position  = { new_boid_index % 64 / 5.0f, new_boid_index / 64 / 5.0f };

					push_index_into_map(&map, static_cast<i32>(new_boid->position.x), static_cast<i32>(new_boid->position.y), new_boid_index);
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
					// Spatial Partition Display.
					//

					SDL_SetRenderDrawColor(window_renderer, 128, 32, 32, 255);
					FOR_ELEMS(chunk_node, map.chunk_node_hash_table)
					{
						if (*chunk_node)
						{
							SDL_Rect rect = { (*chunk_node)->x * PIXELS_PER_METER, WINDOW_HEIGHT - 1 - ((*chunk_node)->y + 1) * PIXELS_PER_METER, PIXELS_PER_METER, PIXELS_PER_METER };
							SDL_RenderFillRect(window_renderer, &rect);
						}
					}

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

						vf2 average_neighboring_boid_repulsion    = { 0.0f, 0.0f };
						vf2 average_neighboring_boid_movement     = { 0.0f, 0.0f };
						vf2 average_neighboring_boid_rel_position = { 0.0f, 0.0f };
						i32 neighboring_boid_count                = 0;

						constexpr f32 BOID_NEIGHBORHOOD_RADIUS = 1.0f;
						FOR_RANGE(dy, -ceil_f32(BOID_NEIGHBORHOOD_RADIUS), ceil_f32(BOID_NEIGHBORHOOD_RADIUS) + 1)
						{
							FOR_RANGE(dx, -ceil_f32(BOID_NEIGHBORHOOD_RADIUS), ceil_f32(BOID_NEIGHBORHOOD_RADIUS) + 1)
							{
								ChunkNode** chunk_node = find_chunk_node(&map, static_cast<i32>(old_boid->position.x) + dx, static_cast<i32>(old_boid->position.y) + dy);

								ASSERT(chunk_node);

								if (*chunk_node)
								{
									ASSERT((*chunk_node)->index_buffer_node);

									IndexBufferNode* current_index_buffer_node = (*chunk_node)->index_buffer_node;

									while (current_index_buffer_node)
									{
										FOR_ELEMS(other_index, current_index_buffer_node->index_buffer, current_index_buffer_node->index_count)
										{
											Boid* other_old_boid = &(*old_boids)[*other_index];

											vf2 to_other          = other_old_boid->position - old_boid->position;
											f32 distance_to_other = norm(to_other);

											constexpr f32 MINIMUM_RADIUS = 0.01f;

											// @TODO@ Remove epsilon?
											if (other_old_boid != old_boid && MINIMUM_RADIUS < distance_to_other && distance_to_other < BOID_NEIGHBORHOOD_RADIUS)
											{
												++neighboring_boid_count;
												average_neighboring_boid_repulsion    -= to_other * (1.0f / distance_to_other - 1.0f / BOID_NEIGHBORHOOD_RADIUS);
												average_neighboring_boid_movement     += other_old_boid->direction;
												average_neighboring_boid_rel_position += to_other;
											}
										}

										current_index_buffer_node = current_index_buffer_node->next_node;
									}
								}
							}
						}

						if (neighboring_boid_count)
						{
							average_neighboring_boid_repulsion    /= static_cast<f32>(neighboring_boid_count);
							average_neighboring_boid_movement     /= static_cast<f32>(neighboring_boid_count);
							average_neighboring_boid_rel_position /= static_cast<f32>(neighboring_boid_count);
						}

						constexpr f32 SEPARATION_WEIGHT = 16.0f;
						constexpr f32 ALIGNMENT_WEIGHT  =  1.0f;
						constexpr f32 COHESION_WEIGHT   =  1.0f;
						constexpr f32 BORDER_WEIGHT     = 16.0f;
						constexpr f32 DRAG_WEIGHT       =  4.0f;

						vf2 desired_movement =
							average_neighboring_boid_repulsion    * SEPARATION_WEIGHT +
							average_neighboring_boid_movement     * ALIGNMENT_WEIGHT  +
							average_neighboring_boid_rel_position * COHESION_WEIGHT   +
							vf2
							{
								-very_strong_curve(old_boid->position.x * PIXELS_PER_METER / static_cast<f32>(WINDOW_WIDTH ) * 2.0f - 1.0f),
								-very_strong_curve(old_boid->position.y * PIXELS_PER_METER / static_cast<f32>(WINDOW_HEIGHT) * 2.0f - 1.0f)
							} * BORDER_WEIGHT +
							old_boid->direction * DRAG_WEIGHT;

						f32 desired_movement_distance = norm(desired_movement);

						constexpr f32 MINIMUM_DESIRED_MOVEMENT_DISTANCE = 0.05f;

						if (desired_movement_distance > MINIMUM_DESIRED_MOVEMENT_DISTANCE)
						{
							new_boid->direction = desired_movement / desired_movement_distance;
						}

						new_boid->position = old_boid->position + BOID_VELOCITY * old_boid->direction * delta_seconds;

						if (static_cast<i32>(new_boid->position.x) != static_cast<i32>(old_boid->position.x) || static_cast<i32>(new_boid->position.y) != static_cast<i32>(old_boid->position.y))
						{
							remove_index_from_map(&map, static_cast<i32>(old_boid->position.x), static_cast<i32>(old_boid->position.y), new_boid_index);
							push_index_into_map  (&map, static_cast<i32>(new_boid->position.x), static_cast<i32>(new_boid->position.y), new_boid_index);
						}

						//
						// Boid Rendering.
						//

						vf2 offset = new_boid->position * static_cast<f32>(PIXELS_PER_METER);

						SDL_Point pixel_points[ARRAY_CAPACITY(BOID_VERTICES)];
						FOR_ELEMS(pixel_point, pixel_points)
						{
							vf2 point = complex_mul(BOID_VERTICES[pixel_point_index], new_boid->direction) + offset;
							*pixel_point = { static_cast<i32>(point.x), WINDOW_HEIGHT - 1 - static_cast<i32>(point.y) };
						}

						SDL_RenderDrawLines(window_renderer, pixel_points, ARRAY_CAPACITY(pixel_points));
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
