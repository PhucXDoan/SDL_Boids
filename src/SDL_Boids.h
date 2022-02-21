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


constexpr i32    PIXELS_PER_METER                  = 50;
constexpr i32    BOID_AMOUNT                       = 4096;
constexpr f32    BOID_NEIGHBORHOOD_RADIUS          = 1.0f;
constexpr f32    MINIMUM_RADIUS                    = 0.005f;
constexpr f32    SEPARATION_WEIGHT                 = 16.0f;
constexpr f32    ALIGNMENT_WEIGHT                  = 4.0f;
constexpr f32    COHESION_WEIGHT                   = 8.0f;
constexpr f32    BORDER_WEIGHT                     = 32.0f;
constexpr f32    ANGULAR_VELOCITY                  = 1.5f;
constexpr f32    MINIMUM_DESIRED_MOVEMENT_DISTANCE = 0.01f;
constexpr f32    BORDER_REPULSION_INITIAL_TANGENT  = -8.0f;
constexpr f32    BORDER_REPULSION_FINAL_TANGENT    = 4.0f;
constexpr f32    HEATMAP_SENSITIVITY               = 8.0f;
constexpr bool32 USE_HELPER_THREADS                = false; // @TODO@ This is necessary as it's not possible to set `HELPER_THREADS_COUNT` to 0 as of now.
constexpr i32    HELPER_THREAD_COUNT               = 4;
constexpr f32    CAMERA_VELOCITY                   = 8.0f;
constexpr f32    CAMERA_ACCELERATION               = 64.0f;
constexpr f32    ZOOM_VELOCITY                     = 0.75f;
constexpr f32    ZOOM_ACCELERATION                 = 8.0f;
constexpr f32    ZOOM_MINIMUM_SCALE_FACTOR         = 0.5f;
constexpr f32    ZOOM_MAXIMUM_SCALE_FACTOR         = 4.00f;
constexpr f32    TIME_SCALAR_CHANGE_SPEED          = 1.0f;
constexpr f32    TIME_SCALAR_MAXIMUM_SCALE_FACTOR  = 2.0f;
constexpr f32    UPDATE_FREQUENCY                  = 1.0f / 60.0f;
constexpr strlit FONT_FILE_PATH                    = "C:/code/misc/fonts/consola.ttf";
constexpr bool32 SHOULD_CATCH_UP                   = true;
constexpr vf2    TESTING_BOX_COORDINATES           = { 180.0f, 124.0f };
constexpr vf2    TESTING_BOX_DIMENSIONS            = { 90.0f, 20.0f };
constexpr vf2    BOID_VERTICES[]                   =
	{
		{  4.5f,  0.0f },
		{ -4.5f,  3.0f },
		{ -0.5f,  0.0f },
		{ -4.5f, -3.0f },
		{  4.5f,  0.0f }
	};

const __m256 PACKED_COEFFICIENTS =
	_mm256_set_ps
	(
		0.0f, 0.0f,
		COHESION_WEIGHT, COHESION_WEIGHT,
		ALIGNMENT_WEIGHT, ALIGNMENT_WEIGHT,
		SEPARATION_WEIGHT * BOID_NEIGHBORHOOD_RADIUS, SEPARATION_WEIGHT * BOID_NEIGHBORHOOD_RADIUS
	);

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
	SDL_Cursor*      default_cursor;
	SDL_Cursor*      grab_cursor;
	bool32           is_cursor_down;
	vf2              cursor_position;
	vf2              last_cursor_click_position;
	FC_Font*         font;
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
