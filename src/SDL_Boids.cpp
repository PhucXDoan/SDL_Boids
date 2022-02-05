#include "SDL_Boids.h"

// @TODO@ Implement the math functions.
inline vf2 polar(f32 rad)
{
	return { cosf(rad), sinf(rad) };
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

inline constexpr i32 pxd_floor(f32 f)
{
	i32 t = static_cast<i32>(f);
	return t + ((f < 0.0f && f - t) ? -1 : 0);
}

inline constexpr i32 pxd_ceil(f32 f)
{
	return -pxd_floor(-f);
}

inline f32 signed_unit_curve(f32 a, f32 b, f32 x)
{
	if (x < 0.0f)
	{
		return -signed_unit_curve(a, b, -x);
	}
	else
	{
		f32 y = CLAMP(x, 0.0f, 1.0f);
		f32 z = ((a + b - 2.0f) * y * y + (-2.0f * a - b + 3.0f) * y + a) * y;
		return CLAMP(z, 0.0f, 1.0f);
	}
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

// @TODO@ Make this robust.
u64 rand_u64(u64* seed)
{
	*seed += 36133;
	return *seed * *seed * 20661081381 + *seed * 53660987 - 5534096 / *seed;
}

f32 rand_range(u64* seed, f32 min, f32 max)
{
	return (rand_u64(seed) % 0xFFFFFFFF / static_cast<f32>(0xFFFFFFFF)) * (max - min);
}

vf2 calc_boid_direction(i32 boid_index, Boid* boids, Map* map)
{
	vf2 average_neighboring_boid_repulsion    = { 0.0f, 0.0f };
	vf2 average_neighboring_boid_movement     = { 0.0f, 0.0f };
	vf2 average_neighboring_boid_rel_position = { 0.0f, 0.0f };
	i32 neighboring_boid_count                = 0;

	FOR_RANGE(dy, -pxd_ceil(BOID_NEIGHBORHOOD_RADIUS), pxd_ceil(BOID_NEIGHBORHOOD_RADIUS) + 1)
	{
		FOR_RANGE(dx, -pxd_ceil(BOID_NEIGHBORHOOD_RADIUS), pxd_ceil(BOID_NEIGHBORHOOD_RADIUS) + 1)
		{
			ChunkNode** chunk_node = find_chunk_node(map, pxd_floor(boids[boid_index].position.x) + dx, pxd_floor(boids[boid_index].position.y) + dy);

			ASSERT(chunk_node);

			if (*chunk_node)
			{
				ASSERT((*chunk_node)->index_buffer_node);

				IndexBufferNode* current_index_buffer_node = (*chunk_node)->index_buffer_node;

				while (current_index_buffer_node)
				{
					FOR_ELEMS(other_boid_index, current_index_buffer_node->index_buffer, current_index_buffer_node->index_count)
					{
						if (*other_boid_index != boid_index)
						{
							vf2 to_other          = boids[*other_boid_index].position - boids[boid_index].position;
							f32 distance_to_other = norm(to_other);

							if (IN_RANGE(distance_to_other, MINIMUM_RADIUS, BOID_NEIGHBORHOOD_RADIUS))
							{
								++neighboring_boid_count;
								average_neighboring_boid_repulsion    -= to_other * BOID_NEIGHBORHOOD_RADIUS / distance_to_other;
								average_neighboring_boid_movement     += boids[*other_boid_index].direction;
								average_neighboring_boid_rel_position += to_other;
							}
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
			signed_unit_curve(BORDER_REPULSION_INITIAL_TANGENT, BORDER_REPULSION_FINAL_TANGENT, 1.0f - boids[boid_index].position.x * PIXELS_PER_METER / WINDOW_WIDTH  * 2.0f),
			signed_unit_curve(BORDER_REPULSION_INITIAL_TANGENT, BORDER_REPULSION_FINAL_TANGENT, 1.0f - boids[boid_index].position.y * PIXELS_PER_METER / WINDOW_HEIGHT * 2.0f)
		} * BORDER_WEIGHT +
		boids[boid_index].direction * DRAG_WEIGHT;

	f32 desired_movement_distance = norm(desired_movement);

	if (desired_movement_distance > MINIMUM_DESIRED_MOVEMENT_DISTANCE)
	{
		return desired_movement / desired_movement_distance;
	}
	else
	{
		return boids[boid_index].direction;
	}
}

int thread_work(void* void_data)
{
	ThreadData* data = reinterpret_cast<ThreadData*>(void_data);

	while (SDL_SemWait(data->semaphore), !data->state->threads_should_exit) // @TODO@ Comman operator (interrobang!?).
	{
		FOR_ELEMS(new_boid, data->state->new_boids + data->new_boids_offset, data->new_boids_count)
		{
			new_boid->direction = calc_boid_direction(new_boid_index + data->new_boids_offset, data->state->old_boids, &data->state->map);
		}

		SDL_SemPost(data->state->completed_work);
	}

	return 0;
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(program->memory);

	if (!program->is_initialized)
	{
		program->is_initialized = true;

		state->threads_should_exit = false;
		state->completed_work      = SDL_CreateSemaphore(0);

		FOR_ELEMS(thread, state->threads)
		{
			i32 base_boid_workload = pxd_floor(static_cast<f32>(BOID_AMOUNT) / THREAD_COUNT);

			ThreadData* data = &state->thread_datas[thread_index];
			data->semaphore        = SDL_CreateSemaphore(0);
			data->state            = state;
			data->new_boids_offset = base_boid_workload * thread_index;
			if (thread_index == ARRAY_CAPACITY(state->threads) - 1) // @TODO@ Be more confident about this workload separation.
			{
				data->new_boids_count = BOID_AMOUNT - base_boid_workload * (THREAD_COUNT - 1);
			}
			else
			{
				data->new_boids_count = base_boid_workload;
			}

			*thread = SDL_CreateThread(thread_work, "`thread_work`", reinterpret_cast<void*>(data));

			DEBUG_printf("Created thread (#%d)\n", thread_index);
		}

		state->seed = 0xBEEFFACE;

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
			old_boid->direction = polar(rand_range(&state->seed, 0.0f, TAU));
			old_boid->position  = { rand_range(&state->seed, 0.0f, static_cast<f32>(WINDOW_WIDTH) / PIXELS_PER_METER), rand_range(&state->seed, 0.0f, static_cast<f32>(WINDOW_HEIGHT) / PIXELS_PER_METER) };

			push_index_into_map(&state->map, pxd_floor(old_boid->position.x), pxd_floor(old_boid->position.y), old_boid_index);
		}

		state->new_boids = PUSH(&state->general_arena, Boid, BOID_AMOUNT);
	}

	//
	// Input.
	//

	for (SDL_Event event; SDL_PollEvent(&event);)
	{
		switch (event.type)
		{
			case SDL_QUIT:
			{
				state->threads_should_exit = true;

				FOR_ELEMS(thread, state->threads)
				{
					ThreadData* data = &state->thread_datas[thread_index];
					SDL_SemPost(data->semaphore);
					SDL_WaitThread(*thread, 0);
					SDL_DestroySemaphore(data->semaphore);
					DEBUG_printf("Freed thread (#%d)\n", thread_index);
				}

				SDL_DestroySemaphore(state->completed_work);

				program->is_running = false;
				return;
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
		for (ChunkNode* current_chunk_node = *chunk_node; current_chunk_node; current_chunk_node = current_chunk_node->next_node)
		{
			f32 redness = 0.0f;
			for (IndexBufferNode* node = current_chunk_node->index_buffer_node; node; node = node->next_node)
			{
				redness += node->index_count;
			}
			redness *= HEATMAP_SENSITIVITY;

			SDL_SetRenderDrawColor(program->renderer, static_cast<u8>(CLAMP(redness, 0, 255)), 0, 0, 255);

			SDL_Rect rect = { current_chunk_node->x * PIXELS_PER_METER, WINDOW_HEIGHT - 1 - (current_chunk_node->y + 1) * PIXELS_PER_METER, PIXELS_PER_METER, PIXELS_PER_METER };
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

	FOR_ELEMS(thread_data, state->thread_datas)
	{
		SDL_SemPost(thread_data->semaphore);
	}

	FOR_RANGE(i, 0, THREAD_COUNT)
	{
		SDL_SemWait(state->completed_work);
	}

	FOR_ELEMS(new_boid, state->new_boids, BOID_AMOUNT)
	{
		Boid* old_boid = &state->old_boids[new_boid_index];

		new_boid->position = old_boid->position + BOID_VELOCITY * old_boid->direction * program->delta_seconds;

		if (pxd_floor(new_boid->position.x) != pxd_floor(old_boid->position.x) || pxd_floor(new_boid->position.y) != pxd_floor(old_boid->position.y))
		{
			remove_index_from_map(&state->map, pxd_floor(old_boid->position.x), pxd_floor(old_boid->position.y), new_boid_index);
			push_index_into_map  (&state->map, pxd_floor(new_boid->position.x), pxd_floor(new_boid->position.y), new_boid_index);
		}

		//
		// Boid Rendering.
		//

		vf2 offset = new_boid->position * static_cast<f32>(PIXELS_PER_METER);

		SDL_Point points[ARRAY_CAPACITY(BOID_VERTICES)];
		FOR_ELEMS(point, points)
		{
			*point =
				{
					static_cast<i32>((BOID_VERTICES[point_index].x * new_boid->direction.x - BOID_VERTICES[point_index].y * new_boid->direction.y) * BOID_SCALAR + offset.x),
					WINDOW_HEIGHT - 1 - static_cast<i32>((BOID_VERTICES[point_index].x * new_boid->direction.y + BOID_VERTICES[point_index].y * new_boid->direction.x) * BOID_SCALAR + offset.y)
				};
		}

		SDL_RenderDrawLines(program->renderer, points, ARRAY_CAPACITY(points));
	}

	SDL_RenderPresent(program->renderer);

	SWAP(state->new_boids, state->old_boids);
}
