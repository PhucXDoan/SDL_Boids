#pragma once
#include <stdio.h>
#include <SDL.h>
#include "unified.h"
#include "SDL_Boids_platform.h"

// @TODO@ Render boids using textures?
// @TODO@ Should resizing of window be allowed?
// @TODO@ GUI.
// @TODO@ Non-Euclidan geometry.
// @TODO@ Have a random leader boid.

#if DEBUG
global constexpr i32    PROFILING_ITERATION_COUNT = 0; // @NOTE@ `0` to not profile.
#endif

global constexpr i32    BOID_AMOUNT         = 512;
global constexpr bool32 USE_HELPER_THREADS  = false; // @TODO@ This is necessary as it's not possible to set `HELPER_THREADS_COUNT` to 0 as of now.
global constexpr i32    HELPER_THREAD_COUNT = 4;
global constexpr vf2    BOID_VERTICES[]     =
	{
		{  3.0f,  0.0f },
		{ -3.0f,  2.0f },
		{ -0.0f,  0.0f },
		{ -3.0f, -2.0f },
		{  3.0f,  0.0f }
	};

struct Settings
{
	i32    pixels_per_meter                  = 38;
	f32    boid_neighborhood_radius          = 1.0f;
	f32    minimum_radius                    = 0.005f;
	f32    separation_weight                 = 16.0f;
	f32    alignment_weight                  = 4.0f;
	f32    cohesion_weight                   = 8.0f;
	f32    border_weight                     = 32.0f;
	f32    angular_velocity                  = 1.5f;
	f32    minimum_desired_movement_distance = 0.01f;
	f32    border_repulsion_initial_tangent  = -8.0f;
	f32    border_repulsion_final_tangent    = 4.0f;
	f32    heatmap_sensitivity               = 8.0f;
	f32    camera_velocity                   = 8.0f;
	f32    camera_acceleration               = 64.0f;
	f32    zoom_velocity                     = 0.75f;
	f32    zoom_acceleration                 = 8.0f;
	f32    zoom_minimum_scale_factor         = 0.5f;
	f32    zoom_maximum_scale_factor         = 4.00f;
	f32    time_scalar_change_speed          = 1.0f;
	f32    time_scalar_maximum_scale_factor  = 2.0f;
	f32    update_frequency                  = 1.0f / 60.0f;
	strlit font_file_path                    = "C:/code/misc/fonts/consola.ttf";
	i32    max_iterations_per_frame          = 8;
	vf2    testing_box_coordinates           = { 180.0f, 24.0f };
	vf2    testing_box_dimensions            = {  90.0f, 20.0f };
};

struct Boid
{
	vf2 direction;
	vf2 position;
};

struct IndexBufferNode
{
	i32              index_buffer[256];
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
	memarena         arena;
	IndexBufferNode* available_index_buffer_node;
	ChunkNode*       available_chunk_node;
	ChunkNode*       chunk_node_hash_table[256];
	Boid*            old_boids;
	Boid*            new_boids;
};

struct State;
struct HelperThreadData
{
	i32          index;
	SDL_sem*     activation;
	State*       state;
	SDL_Thread*  helper_thread;
};

struct State
{
	Settings         settings;
	SDL_Cursor*      default_cursor;
	SDL_Cursor*      grab_cursor;
	bool32           is_debug_cursor_down;
	vf2              debug_cursor_position;
	vf2              last_debug_cursor_click_position;
	FC_Font*         debug_font;
	bool32           helper_threads_should_exit;
	SDL_sem*         completed_work;
	HelperThreadData helper_thread_datas[HELPER_THREAD_COUNT];
	u64              seed;
	Map              map;
	vf2              mouse_position;
	vf2              wasd;
	vf2              arrow_keys;
	vf2              camera_velocity_target;
	vf2              camera_velocity;
	vf2              camera_position;
	f32              camera_zoom_velocity_target;
	f32              camera_zoom_velocity;
	f32              camera_zoom;
	f32              simulation_time_scalar;
	f32              real_world_counter_seconds;
	f32              boid_velocity;
	f32              held_boid_velocity;
};

#include "SDL_Boids_auxiliary.cpp"
