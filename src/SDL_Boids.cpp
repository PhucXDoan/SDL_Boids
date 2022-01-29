#include "SDL_Boids.h"

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
	return y * y * y * y * y * y * y * y * y * y * y * y * y * y * y;
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

void update(PLATFORM_Program* program)
{
	State* state = reinterpret_cast<State*>(program->memory);

	if (!program->is_initialized)
	{
		program->is_initialized = true;

		state->general_arena.base = program->memory          + sizeof(State);
		state->general_arena.size = program->memory_capacity - sizeof(State);
		state->general_arena.used = 0;

		state->map.available_index_buffer_node = PUSH(&state->general_arena, IndexBufferNode, AVAILABLE_INDEX_BUFFER_NODE_COUNT);
		FOR_RANGE(i, 0, AVAILABLE_INDEX_BUFFER_NODE_COUNT - 1)
		{
			state->map.available_index_buffer_node[i].next_node = &state->map.available_index_buffer_node[i + 1];
		}
		state->map.available_index_buffer_node[AVAILABLE_INDEX_BUFFER_NODE_COUNT - 1].next_node = 0;

		state->map.available_chunk_node = PUSH(&state->general_arena, ChunkNode, AVAILABLE_CHUNK_NODE_COUNT);
		FOR_RANGE(i, 0, AVAILABLE_CHUNK_NODE_COUNT - 1)
		{
			state->map.available_chunk_node[i].next_node = &state->map.available_chunk_node[i + 1];
		}
		state->map.available_chunk_node[AVAILABLE_CHUNK_NODE_COUNT - 1].next_node = 0;

		FOR_ELEMS(chunk_node, state->map.chunk_node_hash_table)
		{
			*chunk_node = 0;
		}

		state->old_boids = PUSH(&state->general_arena, Boid, BOID_AMOUNT);
		FOR_ELEMS(old_boid, state->old_boids, BOID_AMOUNT)
		{
			f32 angle = modulo_f32(static_cast<f32>(old_boid_index), TAU);
			old_boid->direction = { cos_f32(angle), sin_f32(angle) };
			old_boid->position  = { old_boid_index % 64 / 5.0f, old_boid_index / 64 / 5.0f };

			push_index_into_map(&state->map, static_cast<i32>(old_boid->position.x), static_cast<i32>(old_boid->position.y), old_boid_index);
		}

		state->new_boids = PUSH(&state->general_arena, Boid, BOID_AMOUNT);
	}

	for (SDL_Event event; SDL_PollEvent(&event);)
	{
		switch (event.type)
		{
			case SDL_QUIT:
			{
				program->is_running = false;
			} break;
		}
	}

	SDL_SetRenderDrawColor(program->renderer, 0, 0, 0, 255);
	SDL_RenderClear(program->renderer);

	//
	// Spatial Partition Display.
	//

	FOR_ELEMS(chunk_node, state->map.chunk_node_hash_table)
	{
		if (*chunk_node)
		{
			f32 redness = 0.0f;
			for (IndexBufferNode* node = (*chunk_node)->index_buffer_node; node; node = node->next_node)
			{
				redness += node->index_count;
			}
			redness *= 2.0f;

			SDL_SetRenderDrawColor(program->renderer, static_cast<u8>(CLAMP(redness, 0, 255)), 0, 0, 255);

			SDL_Rect rect = { (*chunk_node)->x * PIXELS_PER_METER, WINDOW_HEIGHT - 1 - ((*chunk_node)->y + 1) * PIXELS_PER_METER, PIXELS_PER_METER, PIXELS_PER_METER };
			SDL_RenderFillRect(program->renderer, &rect);
		}
	}

	//
	// Grid.
	//

	SDL_SetRenderDrawColor(program->renderer, 64, 64, 64, 255);
	FOR_RANGE(i, 0, WINDOW_WIDTH / PIXELS_PER_METER + 1)
	{
		i32 x = i * PIXELS_PER_METER;
		SDL_RenderDrawLine(program->renderer, x, 0, x, WINDOW_HEIGHT);
	}

	FOR_RANGE(i, 0, WINDOW_HEIGHT / PIXELS_PER_METER + 1)
	{
		i32 y = WINDOW_HEIGHT - 1 - i * PIXELS_PER_METER;
		SDL_RenderDrawLine(program->renderer, 0, y, WINDOW_WIDTH, y);
	}

	//
	// Boids.
	//

	SDL_SetRenderDrawColor(program->renderer, 222, 173, 38, 255);

	FOR_ELEMS(new_boid, state->new_boids, BOID_AMOUNT)
	{
		Boid* old_boid = &state->old_boids[new_boid_index];

		vf2 average_neighboring_boid_repulsion    = { 0.0f, 0.0f };
		vf2 average_neighboring_boid_movement     = { 0.0f, 0.0f };
		vf2 average_neighboring_boid_rel_position = { 0.0f, 0.0f };
		i32 neighboring_boid_count                = 0;

		FOR_RANGE(dy, -ceil_f32(BOID_NEIGHBORHOOD_RADIUS), ceil_f32(BOID_NEIGHBORHOOD_RADIUS) + 1)
		{
			FOR_RANGE(dx, -ceil_f32(BOID_NEIGHBORHOOD_RADIUS), ceil_f32(BOID_NEIGHBORHOOD_RADIUS) + 1)
			{
				ChunkNode** chunk_node = find_chunk_node(&state->map, static_cast<i32>(old_boid->position.x) + dx, static_cast<i32>(old_boid->position.y) + dy);

				ASSERT(chunk_node);

				if (*chunk_node)
				{
					ASSERT((*chunk_node)->index_buffer_node);

					IndexBufferNode* current_index_buffer_node = (*chunk_node)->index_buffer_node;

					while (current_index_buffer_node)
					{
						FOR_ELEMS(other_index, current_index_buffer_node->index_buffer, current_index_buffer_node->index_count)
						{
							Boid* other_old_boid = &state->old_boids[*other_index];

							vf2 to_other          = other_old_boid->position - old_boid->position;
							f32 distance_to_other = norm(to_other);

							// @TODO@ Remove epsilon?
							if (other_old_boid != old_boid && MINIMUM_RADIUS < distance_to_other && distance_to_other < BOID_NEIGHBORHOOD_RADIUS)
							{
								++neighboring_boid_count;
								average_neighboring_boid_repulsion    -= to_other * (BOID_NEIGHBORHOOD_RADIUS / distance_to_other);
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

		if (desired_movement_distance > MINIMUM_DESIRED_MOVEMENT_DISTANCE)
		{
			new_boid->direction = desired_movement / desired_movement_distance;
		}

		new_boid->position = old_boid->position + BOID_VELOCITY * old_boid->direction * program->delta_seconds;

		if (static_cast<i32>(new_boid->position.x) != static_cast<i32>(old_boid->position.x) || static_cast<i32>(new_boid->position.y) != static_cast<i32>(old_boid->position.y))
		{
			remove_index_from_map(&state->map, static_cast<i32>(old_boid->position.x), static_cast<i32>(old_boid->position.y), new_boid_index);
			push_index_into_map  (&state->map, static_cast<i32>(new_boid->position.x), static_cast<i32>(new_boid->position.y), new_boid_index);
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

		SDL_RenderDrawLines(program->renderer, pixel_points, ARRAY_CAPACITY(pixel_points));
	}

	SDL_RenderPresent(program->renderer);

	SWAP(state->new_boids, state->old_boids);
}
