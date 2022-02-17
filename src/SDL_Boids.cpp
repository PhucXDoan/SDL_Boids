#include "SDL_Boids.h"

// @TODO@ Implement the math functions.
inline vf2 polar(f32 rad)
{
	return { cosf(rad), sinf(rad) };
}

inline f32 pxd_arccos(f32 rad)
{
	return acosf(rad);
}

inline vf2 complex_mult(vf2 a, vf2 b)
{
	return { a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x };
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

inline constexpr i32 pxd_round(f32 f)
{
	return pxd_floor(f + 0.5f);
}

inline constexpr i32 pxd_trunc(f32 f)
{
	return static_cast<i32>(f);
}

inline constexpr i32 pxd_sign(f32 f)
{
	return f < 0.0f ? -1 : f > 0.0f ? 1 : 0;
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

IndexBufferNode* allocate_index_buffer_node(Map* map)
{
	if (map->available_index_buffer_node)
	{
		IndexBufferNode* node = map->available_index_buffer_node;
		map->available_index_buffer_node = map->available_index_buffer_node->next_node;
		return node;
	}
	else
	{
		return PUSH(&map->arena, IndexBufferNode);
	}
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
			IndexBufferNode* new_head = allocate_index_buffer_node(map);
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
		if (map->available_chunk_node)
		{
			*chunk_node = map->available_chunk_node;
			map->available_chunk_node = map->available_chunk_node->next_node;
		}
		else
		{
			*chunk_node = PUSH(&map->arena, ChunkNode);
		}

		(*chunk_node)->x                                  = x;
		(*chunk_node)->y                                  = y;
		(*chunk_node)->index_buffer_node                  = allocate_index_buffer_node(map);
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
					*current_index_buffer_node = next_node;
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
		*chunk_node = next_node;
	}
}

// @TODO@ Make this robust.
inline u64 rand_u64(u64* seed)
{
	*seed += 36133;
	return *seed * *seed * 20661081381 + *seed * 53660987 - 5534096 / *seed;
}

inline f32 rand_range(u64* seed, f32 min, f32 max)
{
	return (rand_u64(seed) % 0xFFFFFFFF / static_cast<f32>(0xFFFFFFFF)) * (max - min);
}

inline f32 inch_towards(f32 start, f32 end, f32 step)
{
	f32 delta = end - start;
	if (-step <= delta && delta <= step)
	{
		return end;
	}
	else
	{
		return start + step * pxd_sign(delta);
	}
}

inline vf2 inch_towards(vf2 start, vf2 end, f32 step)
{
	return { inch_towards(start.x, end.x, step), inch_towards(start.y, end.y, step) };
}

void update_boids(State* state, i32 starting_index, i32 count)
{
	FOR_RANGE(boid_index, starting_index, starting_index + count)
	{
		Boid* old_boid = &state->map.old_boids[boid_index];

		vf2 average_neighboring_boid_repulsion    = { 0.0f, 0.0f };
		vf2 average_neighboring_boid_movement     = { 0.0f, 0.0f };
		vf2 average_neighboring_boid_rel_position = { 0.0f, 0.0f };
		i32 neighboring_boid_count                = 0;

		FOR_RANGE(dy, -pxd_ceil(BOID_NEIGHBORHOOD_RADIUS), pxd_ceil(BOID_NEIGHBORHOOD_RADIUS) + 1)
		{
			FOR_RANGE(dx, -pxd_ceil(BOID_NEIGHBORHOOD_RADIUS), pxd_ceil(BOID_NEIGHBORHOOD_RADIUS) + 1)
			{
				ChunkNode** chunk_node = find_chunk_node(&state->map, pxd_floor(old_boid->position.x) + dx, pxd_floor(old_boid->position.y) + dy);

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
								vf2 to_other          = state->map.old_boids[*other_boid_index].position - old_boid->position;
								f32 distance_to_other = norm(to_other);

								if (IN_RANGE(distance_to_other, MINIMUM_RADIUS, BOID_NEIGHBORHOOD_RADIUS))
								{
									++neighboring_boid_count;
									average_neighboring_boid_repulsion    -= to_other * BOID_NEIGHBORHOOD_RADIUS / distance_to_other;
									average_neighboring_boid_movement     += state->map.old_boids[*other_boid_index].direction;
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
				signed_unit_curve(BORDER_REPULSION_INITIAL_TANGENT, BORDER_REPULSION_FINAL_TANGENT, 1.0f - old_boid->position.x * PIXELS_PER_METER / WINDOW_WIDTH  * 2.0f),
				signed_unit_curve(BORDER_REPULSION_INITIAL_TANGENT, BORDER_REPULSION_FINAL_TANGENT, 1.0f - old_boid->position.y * PIXELS_PER_METER / WINDOW_HEIGHT * 2.0f)
			} * BORDER_WEIGHT;

		f32 desired_movement_distance = norm(desired_movement);

		if (desired_movement_distance > MINIMUM_DESIRED_MOVEMENT_DISTANCE)
		{
			// @TODO@ Still need to figure out the best way to steer boids...

			vf2 desired_direction = desired_movement / desired_movement_distance;
			f32 step              = ANGULAR_VELOCITY * state->simulation_time_scalar * UPDATE_FREQUENCY;
			f32 angle             = pxd_arccos(CLAMP(dot(old_boid->direction, desired_direction), -1.0f, 1.0f));

			if (angle <= step)
			{
				state->map.new_boids[boid_index].direction = desired_direction;
			}
			else
			{
				vf2 orth = { -old_boid->direction.y, old_boid->direction.x };
				i32 side = pxd_sign(dot(desired_direction, orth));
				state->map.new_boids[boid_index].direction = complex_mult(old_boid->direction, polar(step * (side == 0 && desired_direction == -old_boid->direction ? 1.0f : side)));
			}
		}
		else
		{
			state->map.new_boids[boid_index].direction = old_boid->direction;
		}

		state->map.new_boids[boid_index].position = old_boid->position + BOID_VELOCITY * old_boid->direction * state->simulation_time_scalar * UPDATE_FREQUENCY;
	}
}

int helper_thread_work(void* void_data)
{
	HelperThreadData* data = reinterpret_cast<HelperThreadData*>(void_data);

	while (SDL_SemWait(data->activation), !data->state->helper_threads_should_exit)
	{
		update_boids(data->state, data->new_boids_offset, BASE_WORKLOAD_FOR_HELPER_THREADS);
		SDL_SemPost(data->state->completed_work);
	}

	return 0;
}

void render_line(SDL_Renderer* renderer, vf2 start, vf2 end)
{
	SDL_RenderDrawLine(renderer, static_cast<i32>(start.x), static_cast<i32>(WINDOW_HEIGHT - 1 - start.y), static_cast<i32>(end.x), static_cast<i32>(WINDOW_HEIGHT - 1 - end.y));
}

void render_lines(SDL_Renderer* renderer, vf2* points, i32 points_capacity)
{
	FOR_ELEMS(point, points, points_capacity - 1)
	{
		render_line(renderer, *point, points[point_index + 1]);
	}
}

void render_fill_rect(SDL_Renderer* renderer, vf2 bottom_left, vf2 dimensions)
{
	SDL_Rect rect = { static_cast<i32>(bottom_left.x), static_cast<i32>(WINDOW_HEIGHT - 1 - bottom_left.y - dimensions.y), static_cast<i32>(dimensions.x), static_cast<i32>(dimensions.y) };
	SDL_RenderFillRect(renderer, &rect);
}

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	State* state = reinterpret_cast<State*>(program->memory);

	state->is_cursor_down                  = false;
	state->cursor_position                 = { 0.0f, 0.0f };
	state->seed                            = 0xBEEFFACE;
	state->map.arena.base                  = program->memory          + sizeof(State);
	state->map.arena.size                  = program->memory_capacity - sizeof(State);
	state->map.arena.used                  = 0;
	state->map.available_index_buffer_node = 0;
	state->map.available_chunk_node        = 0;

	FOR_ELEMS(chunk_node, state->map.chunk_node_hash_table)
	{
		*chunk_node = 0;
	}

	state->map.old_boids = PUSH(&state->map.arena, Boid, BOID_AMOUNT);
	FOR_ELEMS(old_boid, state->map.old_boids, BOID_AMOUNT)
	{
		old_boid->direction = polar(rand_range(&state->seed, 0.0f, TAU));
		old_boid->position  = { rand_range(&state->seed, 0.0f, static_cast<f32>(WINDOW_WIDTH) / PIXELS_PER_METER), rand_range(&state->seed, 0.0f, static_cast<f32>(WINDOW_HEIGHT) / PIXELS_PER_METER) };

		push_index_into_map(&state->map, pxd_floor(old_boid->position.x), pxd_floor(old_boid->position.y), old_boid_index);
	}

	state->map.new_boids = PUSH(&state->map.arena, Boid, BOID_AMOUNT);

	state->wasd                        = { 0.0f, 0.0f };
	state->camera_velocity_target      = { 0.0f, 0.0f };
	state->camera_velocity             = { 0.0f, 0.0f };
	state->camera_position             = vf2 ( WINDOW_WIDTH, WINDOW_HEIGHT ) / PIXELS_PER_METER / 2.0f;
	state->camera_zoom_velocity_target = 0.0f;
	state->camera_zoom_velocity        = 0.0f;
	state->camera_zoom                 = 1.0f;
	state->simulation_time_scalar      = 1.0f;
	state->real_world_counter_seconds  = 0.0f;
}

extern "C" PROTOTYPE_BOOT_UP(boot_up)
{
	State* state = reinterpret_cast<State*>(program->memory);

	state->default_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	state->grab_cursor    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND); // @TODO@ Have a handgrab cursor. This one is ugly.
	state->font = FC_CreateFont();

	if (!FC_LoadFont(state->font, program->renderer, FONT_FILE_PATH, 20, FC_MakeColor(245, 245, 245, 255), TTF_STYLE_NORMAL))
	{
		ASSERT(false);
	}

	state->helper_threads_should_exit = false;
	state->completed_work             = SDL_CreateSemaphore(0);

	FOR_ELEMS(data, state->helper_thread_datas)
	{
		data->activation       = SDL_CreateSemaphore(0);
		data->state            = state;
		data->new_boids_offset = BASE_WORKLOAD_FOR_HELPER_THREADS * data_index;
		data->helper_thread    = SDL_CreateThread(helper_thread_work, "`helper_thread_work`", reinterpret_cast<void*>(data));

		DEBUG_printf("Created helper thread (#%d)\n", data_index);
	}
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	State* state = reinterpret_cast<State*>(program->memory);

	SDL_FreeCursor(state->default_cursor);
	SDL_FreeCursor(state->grab_cursor);
	FC_FreeFont(state->font);

	state->helper_threads_should_exit = true;

	FOR_ELEMS(data, state->helper_thread_datas)
	{
		SDL_SemPost(data->activation);
		SDL_WaitThread(data->helper_thread, 0);
		SDL_DestroySemaphore(data->activation);
		DEBUG_printf("Freed helper thread (#%d)\n", data_index);
	}

	SDL_DestroySemaphore(state->completed_work);
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(program->memory);

	for (SDL_Event event; SDL_PollEvent(&event);)
	{
		switch (event.type)
		{
			case SDL_QUIT:
			{
				boot_down(program);
				program->is_running = false;
				return;
			} break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				if (!event.key.repeat)
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_a:
						{
							state->wasd += vf2 { -1.0f,  0.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_d:
						{
							state->wasd += vf2 {  1.0f,  0.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_s:
						{
							state->wasd += vf2 {  0.0f, -1.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_w:
						{
							state->wasd += vf2 {  0.0f,  1.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_LEFT:
						{
							state->arrow_keys += vf2 { -1.0f,  0.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_RIGHT:
						{
							state->arrow_keys += vf2 {  1.0f,  0.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_DOWN:
						{
							state->arrow_keys += vf2 {  0.0f, -1.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;

						case SDLK_UP:
						{
							state->arrow_keys += vf2 {  0.0f,  1.0f } * (event.key.state == SDL_PRESSED ? 1.0f : -1.0f);
						} break;
					}
				}
			} break;

			case SDL_MOUSEMOTION:
			{
				state->cursor_position = vf2 ( event.motion.x, event.motion.y );
			} break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				state->is_cursor_down = event.button.state == SDL_PRESSED;

				if (state->is_cursor_down)
				{
					SDL_SetCursor(state->grab_cursor);
				}
				else
				{
					SDL_SetCursor(state->default_cursor);
				}
			} break;
		}
	}

	state->real_world_counter_seconds += program->delta_seconds;

	if (state->real_world_counter_seconds >= UPDATE_FREQUENCY)
	{
		do
		{
			state->camera_velocity_target  = +state->wasd ? normalize(state->wasd) * CAMERA_VELOCITY : vf2 { 0.0f, 0.0f };
			state->camera_velocity         = inch_towards(state->camera_velocity, state->camera_velocity_target, CAMERA_ACCELERATION * UPDATE_FREQUENCY);
			state->camera_position        += state->camera_velocity * UPDATE_FREQUENCY;

			state->camera_zoom_velocity_target  = state->arrow_keys.y * ZOOM_VELOCITY * state->camera_zoom;
			state->camera_zoom_velocity         = inch_towards(state->camera_zoom_velocity, state->camera_zoom_velocity_target, ZOOM_ACCELERATION * UPDATE_FREQUENCY);
			state->camera_zoom                 += state->camera_zoom_velocity * UPDATE_FREQUENCY;

			if (state->camera_zoom < ZOOM_MINIMUM_SCALE_FACTOR)
			{
				state->camera_zoom_velocity_target = 0.0f;
				state->camera_zoom_velocity        = 0.0f;
				state->camera_zoom                 = ZOOM_MINIMUM_SCALE_FACTOR;
			}
			else if (state->camera_zoom > ZOOM_MAXIMUM_SCALE_FACTOR)
			{
				state->camera_zoom_velocity_target = 0.0f;
				state->camera_zoom_velocity        = 0.0f;
				state->camera_zoom                 = ZOOM_MAXIMUM_SCALE_FACTOR;
			}

			state->simulation_time_scalar += state->arrow_keys.x * TIME_SCALAR_CHANGE_SPEED * UPDATE_FREQUENCY;
			state->simulation_time_scalar  = CLAMP(state->simulation_time_scalar, 0.0f, TIME_SCALAR_MAXIMUM_SCALE_FACTOR);

			FOR_ELEMS(data, state->helper_thread_datas)
			{
				SDL_SemPost(data->activation);
			}

			update_boids(state, MAIN_THREAD_NEW_BOIDS_OFFSET, MAIN_THREAD_WORKLOAD);

			FOR_RANGE(i, 0, HELPER_THREAD_COUNT)
			{
				SDL_SemWait(state->completed_work);
			}

			FOR_ELEMS(new_boid, state->map.new_boids, BOID_AMOUNT)
			{
				Boid* old_boid = &state->map.old_boids[new_boid_index];

				if (pxd_floor(new_boid->position.x) != pxd_floor(old_boid->position.x) || pxd_floor(new_boid->position.y) != pxd_floor(old_boid->position.y))
				{
					remove_index_from_map(&state->map, pxd_floor(old_boid->position.x), pxd_floor(old_boid->position.y), new_boid_index);
					push_index_into_map  (&state->map, pxd_floor(new_boid->position.x), pxd_floor(new_boid->position.y), new_boid_index);
				}
			}

			SWAP(state->map.new_boids, state->map.old_boids);
		}
		while (state->real_world_counter_seconds -= UPDATE_FREQUENCY, state->real_world_counter_seconds >= UPDATE_FREQUENCY);

		//
		// Heat map.
		//

		SDL_SetRenderDrawColor(program->renderer, 0, 0, 0, 255);
		SDL_RenderClear(program->renderer);

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

				render_fill_rect
				(
					program->renderer,
					(vf2 ( current_chunk_node->x, current_chunk_node->y ) - state->camera_position) * PIXELS_PER_METER * state->camera_zoom + vf2 ( WINDOW_WIDTH, WINDOW_HEIGHT ) / 2.0f,
					vf2 ( 1.0f, 1.0f ) * PIXELS_PER_METER * state->camera_zoom
				);
			}
		}

		//
		// Grid.
		//

		// @TODO@ Works for now. Maybe it can be cleaned up some how?
		SDL_SetRenderDrawColor(program->renderer, 64, 64, 64, 255);
		FOR_RANGE(i, 0, pxd_ceil(static_cast<f32>(WINDOW_WIDTH) / PIXELS_PER_METER / state->camera_zoom) + 1)
		{
			f32 x = (pxd_floor(state->camera_position.x - WINDOW_WIDTH / 2.0f / PIXELS_PER_METER / state->camera_zoom + i) - state->camera_position.x) * state->camera_zoom * PIXELS_PER_METER + WINDOW_WIDTH / 2.0f;
			render_line(program->renderer, vf2 ( x, 0.0f ), vf2 ( x, WINDOW_HEIGHT ));
		}
		FOR_RANGE(i, 0, pxd_ceil(static_cast<f32>(WINDOW_HEIGHT) / PIXELS_PER_METER / state->camera_zoom) + 1)
		{
			f32 y = (pxd_floor(state->camera_position.y - WINDOW_HEIGHT / 2.0f / PIXELS_PER_METER / state->camera_zoom + i) - state->camera_position.y) * state->camera_zoom * PIXELS_PER_METER + WINDOW_HEIGHT / 2.0f;
			render_line(program->renderer, vf2 ( 0.0f, y ), vf2 ( WINDOW_WIDTH, y ));
		}

		//
		// Boid Rendering.
		//

		SDL_SetRenderDrawColor(program->renderer, 222, 173, 38, 255);

		FOR_ELEMS(old_boid, state->map.old_boids, BOID_AMOUNT)
		{
			vf2 pixel_offset = (old_boid->position - state->camera_position) * static_cast<f32>(PIXELS_PER_METER) * state->camera_zoom + vf2 ( WINDOW_WIDTH, WINDOW_HEIGHT ) / 2.0f;

			if (IN_RANGE(pixel_offset.x, 0.0f, WINDOW_WIDTH) && IN_RANGE(pixel_offset.y, 0.0f, WINDOW_HEIGHT))
			{
				vf2 points[ARRAY_CAPACITY(BOID_VERTICES)];
				FOR_ELEMS(point, points)
				{
					*point =
						vf2
						{
							BOID_VERTICES[point_index].x * old_boid->direction.x - BOID_VERTICES[point_index].y * old_boid->direction.y,
							BOID_VERTICES[point_index].x * old_boid->direction.y + BOID_VERTICES[point_index].y * old_boid->direction.x
						} * state->camera_zoom + pixel_offset;
				}

				render_lines(program->renderer, points, ARRAY_CAPACITY(points));
			}
		}

		FC_Draw(state->font, program->renderer, 5, 5, "FPS : %d\nCursor.down : %d\nCursor.x : %f\nCursor.y : %f", pxd_round(1.0f / MAXIMUM(program->delta_seconds, UPDATE_FREQUENCY)), state->is_cursor_down, state->cursor_position.x, state->cursor_position.y);

		SDL_RenderPresent(program->renderer);
	}
}
