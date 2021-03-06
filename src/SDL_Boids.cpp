#include "SDL_Boids.h"

internal IndexBufferNode* allocate_index_buffer_node(Map* map)
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

internal ChunkNode** find_chunk_node(Map* map, i32 x, i32 y)
{
	// @TODO@ Better hash function.
	i32 hash = x * 7 + y * 13;
	hash  = hash < 0 ? -hash : hash;
	hash %= ARRAY_CAPACITY(map->chunk_node_hash_table);

	ChunkNode** chunk_node = &map->chunk_node_hash_table[hash];

	while (*chunk_node && ((*chunk_node)->x != x || (*chunk_node)->y != y))
	{
		chunk_node = &(*chunk_node)->next_node;
	}

	return chunk_node;
}

internal void push_index_into_map(Map* map, i32 x, i32 y, i32 index)
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

internal void remove_index_from_map(Map* map, i32 x, i32 y, i32 index)
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

internal void update_chunk_node(State* state, ChunkNode* chunk_node)
{
	IndexBufferNode* current_index_buffer_node = chunk_node->index_buffer_node;

	// @TODO@ Perhaps a better way to prefetch the surrounding `index_buffer_node`s.
	i32 chunks_wide = 2 * static_cast<i32>(ceilf(state->settings.boid_neighborhood_radius)) + 1;
	IndexBufferNode* surrounding_index_buffer_nodes[100] = {}; // @TODO@ Hack! VLA not supported :(.
	FOR_RANGE(i, 0, chunks_wide)
	{
		FOR_RANGE(j, 0, chunks_wide)
		{
			ChunkNode** surrounding_chunk_node = find_chunk_node(&state->map, chunk_node->x + i - chunks_wide / 2, chunk_node->y + j - chunks_wide / 2);

			ASSERT(IN_RANGE(i * chunks_wide + j, 0, 100));
			surrounding_index_buffer_nodes[i * chunks_wide + j] =
				*surrounding_chunk_node
					? (*surrounding_chunk_node)->index_buffer_node
					: 0;
		}
	}

	// @TODO@ Maybe cache this?
	__m256 packed_coefficients =
		_mm256_set_ps
		(
			0.0f, 0.0f,
			state->settings.cohesion_weight, state->settings.cohesion_weight,
			state->settings.alignment_weight, state->settings.alignment_weight,
			state->settings.separation_weight * state->settings.boid_neighborhood_radius, state->settings.separation_weight * state->settings.boid_neighborhood_radius
		);

	do
	{
		FOR_ELEMS(boid_index, current_index_buffer_node->index_buffer, current_index_buffer_node->index_count)
		{
			Boid*  old_boid               = &state->map.old_boids[*boid_index];
			__m256 packed_factors         = _mm256_set1_ps(0.0f);
			i32    neighboring_boid_count = 0;

			FOR_ELEMS(surrounding_index_buffer_node, surrounding_index_buffer_nodes)
			{
				for (IndexBufferNode* other_current_index_buffer_node = *surrounding_index_buffer_node; other_current_index_buffer_node; other_current_index_buffer_node = other_current_index_buffer_node->next_node)
				{
					FOR_ELEMS(other_boid_index, other_current_index_buffer_node->index_buffer, other_current_index_buffer_node->index_count)
					{
						if (*other_boid_index != *boid_index)
						{
							vf2 to_other          = state->map.old_boids[*other_boid_index].position - old_boid->position;
							f32 distance_to_other = norm(to_other);

							if (IN_RANGE(distance_to_other, state->settings.minimum_radius, state->settings.boid_neighborhood_radius))
							{
								++neighboring_boid_count;

								packed_factors =
									_mm256_add_ps
									(
										packed_factors,
										_mm256_set_ps
										(
											0.0f, 0.0f,
											to_other.y, to_other.x,
											state->map.old_boids[*other_boid_index].direction.y, state->map.old_boids[*other_boid_index].direction.x,
											-to_other.y / distance_to_other, -to_other.x / distance_to_other
										)
									);
							}
						}
					}
				}
			}

			if (neighboring_boid_count)
			{
				packed_factors = _mm256_mul_ps(_mm256_div_ps(packed_factors, _mm256_set1_ps(static_cast<f32>(neighboring_boid_count))), packed_coefficients);
			}

			// @TODO@ The extraction of floats here might be Windows specific.
			vf2 desired_movement =
				{
					packed_factors.m256_f32[0] + packed_factors.m256_f32[2] + packed_factors.m256_f32[4] + signed_unit_curve(state->settings.border_repulsion_initial_tangent, state->settings.border_repulsion_final_tangent, 1.0f - old_boid->position.x * state->settings.pixels_per_meter / WINDOW_WIDTH  * 2.0f) * state->settings.border_weight,
					packed_factors.m256_f32[1] + packed_factors.m256_f32[3] + packed_factors.m256_f32[5] + signed_unit_curve(state->settings.border_repulsion_initial_tangent, state->settings.border_repulsion_final_tangent, 1.0f - old_boid->position.y * state->settings.pixels_per_meter / WINDOW_HEIGHT * 2.0f) * state->settings.border_weight
				};

			f32 desired_movement_distance = norm(desired_movement);

			if (desired_movement_distance > state->settings.minimum_desired_movement_distance)
			{
				// @TODO@ Still need to figure out the best way to steer boids...

				vf2 desired_direction = desired_movement / desired_movement_distance;
				f32 step              = state->settings.angular_velocity * state->simulation_time_scalar * state->settings.update_frequency;
				f32 angle             = acosf(CLAMP(dot(old_boid->direction, desired_direction), -1.0f, 1.0f));

				if (angle <= step)
				{
					state->map.new_boids[*boid_index].direction = desired_direction;
				}
				else
				{
					vf2 orth = { -old_boid->direction.y, old_boid->direction.x };
					i32 side = pxd_sign(dot(desired_direction, orth));
					state->map.new_boids[*boid_index].direction = complex_mult(old_boid->direction, polar(step * (side == 0 && desired_direction == -old_boid->direction ? 1.0f : side)));
				}
			}
			else
			{
				state->map.new_boids[*boid_index].direction = old_boid->direction;
			}

			state->map.new_boids[*boid_index].position = old_boid->position + state->settings.boid_velocity * old_boid->direction * state->simulation_time_scalar * state->settings.update_frequency;
		}

		current_index_buffer_node = current_index_buffer_node->next_node;
	}
	while (current_index_buffer_node);
}

internal int helper_thread_work(void* void_data)
{
	HelperThreadData* data = reinterpret_cast<HelperThreadData*>(void_data);

	while (SDL_SemWait(data->activation), !data->state->helper_threads_should_exit)
	{
		for (i32 i = data->index; i < ARRAY_CAPACITY(data->state->map.chunk_node_hash_table); i += HELPER_THREAD_COUNT + 1)
		{
			for (ChunkNode* node = data->state->map.chunk_node_hash_table[i]; node; node = node->next_node)
			{
				update_chunk_node(data->state, node);
			}
		}
		SDL_SemPost(data->state->completed_work);
	}

	return 0;
}

internal void update_simulation(State* state)
{
	state->camera_velocity_target  = +state->wasd ? state->wasd / norm(state->wasd) * state->settings.camera_speed : vf2 { 0.0f, 0.0f };
	state->camera_velocity         = inch_towards(state->camera_velocity, state->camera_velocity_target, state->settings.camera_acceleration * state->settings.update_frequency);
	state->camera_position        += state->camera_velocity * state->settings.update_frequency;

	state->camera_zoom_velocity_target  = state->arrow_keys.y * state->settings.zoom_speed * state->camera_zoom;
	state->camera_zoom_velocity         = inch_towards(state->camera_zoom_velocity, state->camera_zoom_velocity_target, state->settings.zoom_acceleration * state->settings.update_frequency);
	state->camera_zoom                 += state->camera_zoom_velocity * state->settings.update_frequency;

	if (state->camera_zoom < state->settings.zoom_minimum_scale_factor)
	{
		state->camera_zoom_velocity_target = 0.0f;
		state->camera_zoom_velocity        = 0.0f;
		state->camera_zoom                 = state->settings.zoom_minimum_scale_factor;
	}
	else if (state->camera_zoom > state->settings.zoom_maximum_scale_factor)
	{
		state->camera_zoom_velocity_target = 0.0f;
		state->camera_zoom_velocity        = 0.0f;
		state->camera_zoom                 = state->settings.zoom_maximum_scale_factor;
	}

	state->simulation_time_scalar += state->arrow_keys.x * state->settings.time_scalar_change_speed * state->settings.update_frequency;
	state->simulation_time_scalar  = CLAMP(state->simulation_time_scalar, 0.0f, state->settings.time_scalar_maximum_scale_factor);

	if (USE_HELPER_THREADS)
	{
		FOR_ELEMS(data, state->helper_thread_datas)
		{
			SDL_SemPost(data->activation);
		}

		for (i32 i = HELPER_THREAD_COUNT; i < ARRAY_CAPACITY(state->map.chunk_node_hash_table); i += HELPER_THREAD_COUNT + 1)
		{
			for (ChunkNode* node = state->map.chunk_node_hash_table[i]; node; node = node->next_node)
			{
				update_chunk_node(state, node);
			}
		}

		FOR_RANGE(i, 0, HELPER_THREAD_COUNT)
		{
			SDL_SemWait(state->completed_work);
		}
	}
	else
	{
		FOR_ELEMS(chunk_node, state->map.chunk_node_hash_table)
		{
			for (ChunkNode* node = *chunk_node; node; node = node->next_node)
			{
				update_chunk_node(state, node);
			}
		}
	}

	FOR_ELEMS(new_boid, state->map.new_boids, BOID_AMOUNT)
	{
		Boid* old_boid = &state->map.old_boids[new_boid_index];

		if (floorf(new_boid->position.x) != floorf(old_boid->position.x) || floorf(new_boid->position.y) != floorf(old_boid->position.y))
		{
			remove_index_from_map(&state->map, static_cast<i32>(floorf(old_boid->position.x)), static_cast<i32>(floorf(old_boid->position.y)), new_boid_index);
			push_index_into_map  (&state->map, static_cast<i32>(floorf(new_boid->position.x)), static_cast<i32>(floorf(new_boid->position.y)), new_boid_index);
		}
	}

	SWAP(state->map.new_boids, state->map.old_boids);
}

internal void load_settings(Settings* settings)
{
	SDL_RWops* vars_file = SDL_RWFromFile(VARS_FILE_PATH, "r"); // @TODO@ What are the consequences of not opening in binary mode?
	if (vars_file == 0)
	{
		DEBUG_printf("\t>>>> Could not load vars file at '%s'.\n", VARS_FILE_PATH);
	}
	else
	{
		DEBUG_printf("\t>>>> Loaded vars file at '%s'.\n", VARS_FILE_PATH);

		// @TODO@ Better way to allocate memory?
		StringBuffer vars_file_text;
		vars_file_text.count    = static_cast<i32>(SDL_RWsize(vars_file));
		vars_file_text.capacity = vars_file_text.count;
		vars_file_text.buffer   = reinterpret_cast<char*>(malloc(vars_file_text.capacity));
		SDL_RWread(vars_file, vars_file_text.buffer, vars_file_text.capacity, 1);

		StringBuffer STRING_BUFFER_STACK(property_name , 256);
		StringBuffer STRING_BUFFER_STACK(property_value, 256);

		for (i32 i = 0; i < vars_file_text.capacity; ++i)
		{
			while (i < vars_file_text.capacity && vars_file_text.buffer[i] != ' ')
			{
				string_buffer_add(&property_name, vars_file_text.buffer[i]);
				++i;
			}

			while (i < vars_file_text.capacity && vars_file_text.buffer[i] == ' ')
			{
				++i;
			}

			while (i < vars_file_text.capacity && vars_file_text.buffer[i] != '\n')
			{
				string_buffer_add(&property_value, vars_file_text.buffer[i]);
				++i;
			}

			if (i > 0 && vars_file_text.buffer[i - 1] == '\r')
			{
				--property_value.count;
			}

			#define CHECK_PRIMITIVE_PROPERTY(FORMAT, PROPERTY_TYPE, PROPERTY_NAME)\
			if (string_buffer_equal(&property_name, #PROPERTY_NAME))\
			{\
				PROPERTY_TYPE result;\
				if (string_buffer_parse_##PROPERTY_TYPE##(&property_value, &result))\
				{\
					DEBUG_print_StringBuffer(&property_name);\
					DEBUG_printf(" : " FORMAT "\n", result);\
					settings->##PROPERTY_NAME = result;\
				}\
				else\
				{\
					DEBUG_printf("\n\t>>>> Parse error : ");\
					DEBUG_print_StringBuffer(&property_name);\
					DEBUG_printf(" : ");\
					DEBUG_print_StringBuffer(&property_value);\
					DEBUG_printf("\n\n");\
				}\
			}

			#define CHECK_VECTOR2_PROPERTY(PROPERTY_NAME)\
			if (string_buffer_equal(&property_name, #PROPERTY_NAME))\
			{\
				memsize x_component_length = 0;\
				FOR_ELEMS(c, property_value.buffer, property_value.count)\
				{\
					if (*c == ' ') { break; }\
					else { ++x_component_length; }\
				}\
				memsize y_component_index = x_component_length;\
				while (property_value.buffer[y_component_index] == ' ')\
				{\
					++y_component_index;\
				}\
				memsize y_component_length = 0;\
				FOR_ELEMS(c, property_value.buffer + y_component_index, property_value.count - y_component_index)\
				{\
					if (*c == ' ') { break; }\
					else { ++y_component_length; }\
				}\
				StringBuffer x_component = { x_component_length, x_component_length, property_value.buffer };\
				StringBuffer y_component = { y_component_length, y_component_length, property_value.buffer + y_component_index };\
				vf2 result;\
				if (string_buffer_parse_f32(&x_component, &result.x) && string_buffer_parse_f32(&y_component, &result.y))\
				{\
					DEBUG_print_StringBuffer(&property_name);\
					DEBUG_printf(" : %f %f\n", result.x, result.y);\
					settings->##PROPERTY_NAME = result;\
				}\
				else\
				{\
					DEBUG_printf("\n\t>>>> Parse error : ");\
					DEBUG_print_StringBuffer(&property_name);\
					DEBUG_printf(" : ");\
					DEBUG_print_StringBuffer(&property_value);\
					DEBUG_printf("\n\n");\
				}\
			}

			;    CHECK_PRIMITIVE_PROPERTY("%d", i32, pixels_per_meter)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, boid_neighborhood_radius)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, minimum_radius)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, separation_weight)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, alignment_weight)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, cohesion_weight)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, border_weight)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, angular_velocity)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, minimum_desired_movement_distance)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, border_repulsion_initial_tangent)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, border_repulsion_final_tangent)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, heatmap_sensitivity)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, camera_speed)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, camera_acceleration)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, zoom_speed)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, zoom_acceleration)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, zoom_minimum_scale_factor)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, zoom_maximum_scale_factor)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, time_scalar_change_speed)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, time_scalar_maximum_scale_factor)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, update_frequency)
			else if (string_buffer_equal(&property_name, "font_file_path"))
			{
				DEBUG_print_StringBuffer(&property_name);
				DEBUG_printf(" : ");
				DEBUG_print_StringBuffer(&property_value);
				DEBUG_printf("\n");
				settings->font_file_path = reinterpret_cast<char*>(malloc(property_value.count + 1)); // @TODO@ Leak.
				FOR_ELEMS(c, property_value.buffer, property_value.count)
				{
					settings->font_file_path[c_index] = *c;
				}
				settings->font_file_path[property_value.count] = '\0';
			}
			else CHECK_PRIMITIVE_PROPERTY("%d", i32, max_iterations_per_frame)
			else CHECK_VECTOR2_PROPERTY(testing_box_coordinates)
			else CHECK_VECTOR2_PROPERTY(testing_box_dimensions)
			else CHECK_PRIMITIVE_PROPERTY("%f", f32, boid_velocity)
			else CHECK_VECTOR2_PROPERTY(save_button_coordinates)
			else CHECK_VECTOR2_PROPERTY(save_button_dimensions)
			else CHECK_VECTOR2_PROPERTY(load_button_coordinates)
			else CHECK_VECTOR2_PROPERTY(load_button_dimensions)
			else
			{
				DEBUG_printf("\t>>>> Unknown property : ");
				DEBUG_print_StringBuffer(&property_name);
				DEBUG_printf(" : ");
				DEBUG_print_StringBuffer(&property_value);
				DEBUG_printf("\n");
			}

			#undef CHECK_PRIMITIVE_PROPERTY

			property_name.count  = 0;
			property_value.count = 0;
		}

		free(vars_file_text.buffer);
	}

	SDL_RWclose(vars_file);
}

internal void save_settings(Settings* settings)
{
	SDL_RWops* vars_file = SDL_RWFromFile(VARS_FILE_PATH, "w"); // @TODO@ What are the consequences of not opening in binary mode?

	StringBuffer STRING_BUFFER_STACK(line, 64);

	#define ADD_PRIMITIVE_PROPERTY(PROPERTY_NAME)\
	do\
	{\
		string_buffer_add(&line, #PROPERTY_NAME);\
		string_buffer_add(&line, ' ');\
		string_buffer_add(&line, settings->##PROPERTY_NAME);\
		string_buffer_add(&line, '\n');\
		SDL_RWwrite(vars_file, line.buffer, sizeof(char), line.count);\
		line.count = 0;\
	}\
	while (false)

	#define ADD_VECTOR2_PROPERTY(PROPERTY_NAME)\
	do\
	{\
		string_buffer_add(&line, #PROPERTY_NAME);\
		string_buffer_add(&line, ' ');\
		string_buffer_add(&line, settings->##PROPERTY_NAME##.x);\
		string_buffer_add(&line, ' ');\
		string_buffer_add(&line, settings->##PROPERTY_NAME##.y);\
		string_buffer_add(&line, '\n');\
		SDL_RWwrite(vars_file, line.buffer, sizeof(char), line.count);\
		line.count = 0;\
	}\
	while (false)

	ADD_PRIMITIVE_PROPERTY(pixels_per_meter);
	ADD_PRIMITIVE_PROPERTY(boid_neighborhood_radius);
	ADD_PRIMITIVE_PROPERTY(minimum_radius);
	ADD_PRIMITIVE_PROPERTY(separation_weight);
	ADD_PRIMITIVE_PROPERTY(alignment_weight);
	ADD_PRIMITIVE_PROPERTY(cohesion_weight);
	ADD_PRIMITIVE_PROPERTY(border_weight);
	ADD_PRIMITIVE_PROPERTY(angular_velocity);
	ADD_PRIMITIVE_PROPERTY(minimum_desired_movement_distance);
	ADD_PRIMITIVE_PROPERTY(border_repulsion_initial_tangent);
	ADD_PRIMITIVE_PROPERTY(border_repulsion_final_tangent);
	ADD_PRIMITIVE_PROPERTY(heatmap_sensitivity);
	ADD_PRIMITIVE_PROPERTY(camera_speed);
	ADD_PRIMITIVE_PROPERTY(camera_acceleration);
	ADD_PRIMITIVE_PROPERTY(zoom_speed);
	ADD_PRIMITIVE_PROPERTY(zoom_acceleration);
	ADD_PRIMITIVE_PROPERTY(zoom_minimum_scale_factor);
	ADD_PRIMITIVE_PROPERTY(zoom_maximum_scale_factor);
	ADD_PRIMITIVE_PROPERTY(time_scalar_change_speed);
	ADD_PRIMITIVE_PROPERTY(time_scalar_maximum_scale_factor);
	ADD_PRIMITIVE_PROPERTY(update_frequency);
	ADD_PRIMITIVE_PROPERTY(font_file_path);
	ADD_PRIMITIVE_PROPERTY(max_iterations_per_frame);
	ADD_VECTOR2_PROPERTY(testing_box_coordinates);
	ADD_VECTOR2_PROPERTY(testing_box_dimensions);
	ADD_PRIMITIVE_PROPERTY(boid_velocity);
	ADD_VECTOR2_PROPERTY(save_button_coordinates);
	ADD_VECTOR2_PROPERTY(save_button_dimensions);
	ADD_VECTOR2_PROPERTY(load_button_coordinates);
	ADD_VECTOR2_PROPERTY(load_button_dimensions);

	SDL_RWwrite(vars_file, line.buffer, sizeof(char), line.count);

	SDL_RWclose(vars_file);
}

extern "C" PROTOTYPE_INITIALIZE(initialize)
{
	State* state = reinterpret_cast<State*>(program->memory);

	state->settings                         = {};
	load_settings(&state->settings);

	state->is_debug_cursor_down             = false;
	state->debug_cursor_position            = { 0.0f, 0.0f };
	state->last_debug_cursor_click_position = { 0.0f, 0.0f };
	state->seed                             = 0xBEEFFACE;
	state->map.arena.base                   = program->memory          + sizeof(State);
	state->map.arena.size                   = program->memory_capacity - sizeof(State);
	state->map.arena.used                   = 0;
	state->map.available_index_buffer_node  = 0;
	state->map.available_chunk_node         = 0;

	FOR_ELEMS(chunk_node, state->map.chunk_node_hash_table)
	{
		*chunk_node = 0;
	}

	state->map.old_boids = PUSH(&state->map.arena, Boid, BOID_AMOUNT);
	FOR_ELEMS(old_boid, state->map.old_boids, BOID_AMOUNT)
	{
		old_boid->direction = polar(rand_range(&state->seed, 0.0f, TAU));
		old_boid->position  = { rand_range(&state->seed, 0.0f, static_cast<f32>(WINDOW_WIDTH) / state->settings.pixels_per_meter), rand_range(&state->seed, 0.0f, static_cast<f32>(WINDOW_HEIGHT) / state->settings.pixels_per_meter) };

		push_index_into_map(&state->map, static_cast<i32>(floorf(old_boid->position.x)), static_cast<i32>(floorf(old_boid->position.y)), old_boid_index);
	}

	state->map.new_boids = PUSH(&state->map.arena, Boid, BOID_AMOUNT);

	state->wasd                        = { 0.0f, 0.0f };
	state->camera_velocity_target      = { 0.0f, 0.0f };
	state->camera_velocity             = { 0.0f, 0.0f };
	state->camera_position             = vf2 ( WINDOW_WIDTH, WINDOW_HEIGHT ) / static_cast<f32>(state->settings.pixels_per_meter) / 2.0f;
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
	state->debug_font     = FC_CreateFont();

	#if DEBUG
	if (!FC_LoadFont(state->debug_font, program->debug_renderer, state->settings.font_file_path, 20, FC_MakeColor(245, 245, 245, 255), TTF_STYLE_NORMAL))
	{
		ASSERT(false);
	}
	#endif

	if (USE_HELPER_THREADS)
	{
		state->helper_threads_should_exit = false;
		state->completed_work             = SDL_CreateSemaphore(0);

		FOR_ELEMS(data, state->helper_thread_datas)
		{
			data->index            = data_index;
			data->activation       = SDL_CreateSemaphore(0);
			data->state            = state;
			data->helper_thread    = SDL_CreateThread(helper_thread_work, "`helper_thread_work`", reinterpret_cast<void*>(data));

			DEBUG_printf("Created helper thread (#%d)\n", data_index);
		}
	}
}

extern "C" PROTOTYPE_BOOT_DOWN(boot_down)
{
	State* state = reinterpret_cast<State*>(program->memory);

	SDL_FreeCursor(state->default_cursor);
	SDL_FreeCursor(state->grab_cursor);
	FC_FreeFont(state->debug_font);

	if (USE_HELPER_THREADS)
	{
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
}

extern "C" PROTOTYPE_UPDATE(update)
{
	State* state = reinterpret_cast<State*>(program->memory);

	for (SDL_Event event; SDL_PollEvent(&event);)
	{
		switch (event.type)
		{
			case SDL_WINDOWEVENT:
			{
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
				{
					boot_down(program);
					program->is_running = false;
					return;
				}
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
				#if DEBUG
				if (event.motion.windowID == program->debug_window_id)
				{
					state->debug_cursor_position = vf2 ( event.motion.x, event.motion.y );

					if
					(
						state->is_debug_cursor_down &&
						IN_RANGE(state->last_debug_cursor_click_position.x - state->settings.testing_box_coordinates.x, 0.0f, state->settings.testing_box_dimensions.x) &&
						IN_RANGE(state->last_debug_cursor_click_position.y - state->settings.testing_box_coordinates.y, 0.0f, state->settings.testing_box_dimensions.y)
					)
					{
						state->settings.boid_velocity = state->held_boid_velocity + (state->debug_cursor_position.x - state->last_debug_cursor_click_position.x) / 100.0f;
						state->settings.boid_velocity = CLAMP(state->settings.boid_velocity, 0.0f, 4.0f);
					}
				}
				#endif
			} break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				#if DEBUG
				if (event.button.windowID == program->debug_window_id || event.button.state == SDL_RELEASED)
				{
					state->is_debug_cursor_down = event.button.state == SDL_PRESSED;

					if (state->is_debug_cursor_down)
					{
						SDL_SetCursor(state->grab_cursor);
						state->last_debug_cursor_click_position = vf2 ( event.button.x, event.button.y );

						if
						(
							IN_RANGE(state->last_debug_cursor_click_position.x - state->settings.testing_box_coordinates.x, 0.0f, state->settings.testing_box_dimensions.x) &&
							IN_RANGE(state->last_debug_cursor_click_position.y - state->settings.testing_box_coordinates.y, 0.0f, state->settings.testing_box_dimensions.y)
						)
						{
							state->held_boid_velocity = state->settings.boid_velocity;
						}
						else if
						(
							IN_RANGE(state->last_debug_cursor_click_position.x - state->settings.save_button_coordinates.x, 0.0f, state->settings.save_button_dimensions.x) &&
							IN_RANGE(state->last_debug_cursor_click_position.y - state->settings.save_button_coordinates.y, 0.0f, state->settings.save_button_dimensions.y)
						)
						{
							save_settings(&state->settings);
						}
						else if
						(
							IN_RANGE(state->last_debug_cursor_click_position.x - state->settings.load_button_coordinates.x, 0.0f, state->settings.load_button_dimensions.x) &&
							IN_RANGE(state->last_debug_cursor_click_position.y - state->settings.load_button_coordinates.y, 0.0f, state->settings.load_button_dimensions.y)
						)
						{
							load_settings(&state->settings);
						}
					}
					else
					{
						SDL_SetCursor(state->default_cursor);
					}
				}
				#endif
			} break;
		}
	}

	#if DEBUG
	if (PROFILING_ITERATION_COUNT != 0)
	{
		LARGE_INTEGER LARGE_INTEGER_TEMP;

		QueryPerformanceCounter(&LARGE_INTEGER_TEMP);
		u64 START = LARGE_INTEGER_TEMP.QuadPart;

		FOR_RANGE(i_, 0, PROFILING_ITERATION_COUNT)
		{
			update_simulation(state);
		}

		QueryPerformanceCounter(&LARGE_INTEGER_TEMP);
		u64 END = LARGE_INTEGER_TEMP.QuadPart;

		QueryPerformanceFrequency(&LARGE_INTEGER_TEMP);
		u64 FREQUENCY = LARGE_INTEGER_TEMP.QuadPart;

		DEBUG_printf
		(
			"\n\n\n--------------------------------\nIterations                     : %d\nBoids                          : %d\nMultithreading                 : %d\nElapsed Time                   : %f\n--------------------------------\n\n\n",
			PROFILING_ITERATION_COUNT,
			BOID_AMOUNT,
			USE_HELPER_THREADS,
			static_cast<f64>(END - START) / FREQUENCY
		);

		boot_down(program);
		program->is_running = false;
		return;
	}
	#endif

	state->real_world_counter_seconds += program->delta_seconds;

	if (state->real_world_counter_seconds >= state->settings.update_frequency)
	{
		FOR_RANGE(i_, 0, state->settings.max_iterations_per_frame)
		{
			update_simulation(state);

			state->real_world_counter_seconds -= state->settings.update_frequency;
			if (state->real_world_counter_seconds <= state->settings.update_frequency)
			{
				break;
			}
		}

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
				redness *= state->settings.heatmap_sensitivity;

				SDL_SetRenderDrawColor(program->renderer, static_cast<u8>(CLAMP(redness, 0, 255)), 0, 0, 255);

				render_fill_rect
				(
					program->renderer,
					(vf2 ( current_chunk_node->x, current_chunk_node->y ) - state->camera_position) * static_cast<f32>(state->settings.pixels_per_meter) * state->camera_zoom + vf2 ( WINDOW_WIDTH, WINDOW_HEIGHT ) / 2.0f,
					vf2 ( 1.0f, 1.0f ) * static_cast<f32>(state->settings.pixels_per_meter) * state->camera_zoom
				);
			}
		}

		//
		// Grid.
		//

		// @TODO@ Works for now. Maybe it can be cleaned up some how?
		SDL_SetRenderDrawColor(program->renderer, 64, 64, 64, 255);
		FOR_RANGE(i, 0, ceilf(static_cast<f32>(WINDOW_WIDTH) / state->settings.pixels_per_meter / state->camera_zoom) + 1)
		{
			f32 x = (floorf(state->camera_position.x - WINDOW_WIDTH / 2.0f / state->settings.pixels_per_meter / state->camera_zoom + i) - state->camera_position.x) * state->camera_zoom * state->settings.pixels_per_meter + WINDOW_WIDTH / 2.0f;
			render_line(program->renderer, vf2 ( x, 0.0f ), vf2 ( x, WINDOW_HEIGHT ));
		}
		FOR_RANGE(i, 0, ceilf(static_cast<f32>(WINDOW_HEIGHT) / state->settings.pixels_per_meter / state->camera_zoom) + 1)
		{
			f32 y = (floorf(state->camera_position.y - WINDOW_HEIGHT / 2.0f / state->settings.pixels_per_meter / state->camera_zoom + i) - state->camera_position.y) * state->camera_zoom * state->settings.pixels_per_meter + WINDOW_HEIGHT / 2.0f;
			render_line(program->renderer, vf2 ( 0.0f, y ), vf2 ( WINDOW_WIDTH, y ));
		}

		//
		// Boid Rendering.
		//

		SDL_SetRenderDrawColor(program->renderer, 222, 173, 38, 255);

		FOR_ELEMS(old_boid, state->map.old_boids, BOID_AMOUNT)
		{
			vf2 pixel_offset = (old_boid->position - state->camera_position) * static_cast<f32>(state->settings.pixels_per_meter) * state->camera_zoom + vf2 ( WINDOW_WIDTH, WINDOW_HEIGHT ) / 2.0f;

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

		SDL_RenderPresent(program->renderer);

		//
		// Debug Window.
		//

		#if DEBUG
		SDL_SetRenderDrawColor(program->debug_renderer, 0, 0, 0, 255);
		SDL_RenderClear(program->debug_renderer);

		constexpr vf2 MEMORY_ARENA_COORDINATES = { 325.0f, 375.0f };
		constexpr vf2 MEMORY_ARENA_DIMENSIONS  = {  15.0f, 150.0f };

		f32 usage = static_cast<f32>(state->map.arena.used) / state->map.arena.size;

		SDL_SetRenderDrawColor(program->debug_renderer, 64, 64, 64, 255);
		render_fill_rect
		(
			program->debug_renderer,
			MEMORY_ARENA_COORDINATES,
			{ MEMORY_ARENA_DIMENSIONS.x, MEMORY_ARENA_DIMENSIONS.y * usage }
		);

		SDL_SetRenderDrawColor(program->debug_renderer, 128, 128, 128, 255);
		render_fill_rect
		(
			program->debug_renderer,
			{ MEMORY_ARENA_COORDINATES.x, MEMORY_ARENA_COORDINATES.y + MEMORY_ARENA_DIMENSIONS.y * usage } ,
			{ MEMORY_ARENA_DIMENSIONS.x, MEMORY_ARENA_DIMENSIONS.y * (1.0f - usage) }
		);

		SDL_SetRenderDrawColor(program->debug_renderer, 128, 128, 0, 255);
		SDL_Rect rect = { static_cast<i32>(state->settings.testing_box_coordinates.x), static_cast<i32>(state->settings.testing_box_coordinates.y), static_cast<i32>(state->settings.testing_box_dimensions.x), static_cast<i32>(state->settings.testing_box_dimensions.y) };
		SDL_RenderFillRect(program->debug_renderer, &rect);

		rect = { static_cast<i32>(state->settings.save_button_coordinates.x), static_cast<i32>(state->settings.save_button_coordinates.y), static_cast<i32>(state->settings.save_button_dimensions.x), static_cast<i32>(state->settings.save_button_dimensions.y) };
		SDL_RenderFillRect(program->debug_renderer, &rect);

		rect = { static_cast<i32>(state->settings.load_button_coordinates.x), static_cast<i32>(state->settings.load_button_coordinates.y), static_cast<i32>(state->settings.load_button_dimensions.x), static_cast<i32>(state->settings.load_button_dimensions.y) };
		SDL_RenderFillRect(program->debug_renderer, &rect);

		// @TODO@ Accurate way to get FPS.
		FC_Draw
		(
			state->debug_font,
			program->debug_renderer,
			5,
			5,
			"FPS : %d\nBoid Velocity : %f\nTime Scalar : %f\nUsed : %llub\nCapacity : %llub\nPercent : %f%%\nSave\nLoad",
			static_cast<i32>(roundf(1.0f / MAXIMUM(program->delta_seconds, state->settings.update_frequency))),
			state->settings.boid_velocity,
			state->simulation_time_scalar,
			state->map.arena.used,
			state->map.arena.size,
			usage
		);

		FOR_ELEMS(chunk_node, state->map.chunk_node_hash_table)
		{
			constexpr i32 BUCKETS_PER_ROW  = 32;
			constexpr f32 BUCKET_DIMENSION = 10.0f;
			constexpr f32 BUCKET_PADDING   = 5.0f;
			constexpr f32 LAYER_PADDING    = 40.0f;
			constexpr f32 STARTING_Y       = 350.0f;

			if (*chunk_node)
			{
				ChunkNode* current_chunk_node = *chunk_node;
				i32        current_node_index = 0;
				do
				{
					u8 gray = 64;

					for (IndexBufferNode* current_index_buffer_node = current_chunk_node->index_buffer_node; current_index_buffer_node; current_index_buffer_node = current_index_buffer_node->next_node)
					{
						gray += 64;
					}

					SDL_SetRenderDrawColor(program->debug_renderer, gray, gray, gray, 255);

					render_fill_rect
					(
						program->debug_renderer,
						{ chunk_node_index % BUCKETS_PER_ROW * (BUCKET_DIMENSION + BUCKET_PADDING) + BUCKET_PADDING, STARTING_Y - chunk_node_index / BUCKETS_PER_ROW * LAYER_PADDING - BUCKET_DIMENSION * current_node_index },
						{ BUCKET_DIMENSION, BUCKET_DIMENSION }
					);

					current_chunk_node = current_chunk_node->next_node;
					++current_node_index;
				}
				while (current_chunk_node);
			}
			else
			{
				SDL_SetRenderDrawColor(program->debug_renderer, 32, 32, 32, 255);
				render_fill_rect
				(
					program->debug_renderer,
					{ chunk_node_index % BUCKETS_PER_ROW * (BUCKET_DIMENSION + BUCKET_PADDING) + BUCKET_PADDING, STARTING_Y - chunk_node_index / BUCKETS_PER_ROW * LAYER_PADDING },
					{ BUCKET_DIMENSION, BUCKET_DIMENSION }
				);
			}
		}

		SDL_RenderPresent(program->debug_renderer);
		#endif
	}
}
