#pragma once
#include <stdio.h>
#include <SDL.h>
#include "unified.h"
#include "SDL_Boids_platform.h"

// @TODO@ Render boids using textures?
// @TODO@ Should resizing of window be allowed?
// @TODO@ Movable camera.
// @TODO@ Zooming.
// @TODO@ GUI.
// @TODO@ Non-Euclidan geometry.
// @TODO@ Have a random leader boid.

constexpr i32 PIXELS_PER_METER                  = 50;
constexpr f32 BOID_VELOCITY                     = 1.5f;
constexpr i32 BOID_AMOUNT                       = 4096;
constexpr i32 AVAILABLE_INDEX_BUFFER_NODE_COUNT = 512;
constexpr i32 AVAILABLE_CHUNK_NODE_COUNT        = 512;
constexpr f32 BOID_NEIGHBORHOOD_RADIUS          = 1.0f;
constexpr f32 MINIMUM_RADIUS                    = 0.005f;
constexpr f32 SEPARATION_WEIGHT                 = 8.0f;
constexpr f32 ALIGNMENT_WEIGHT                  = 4.0f;
constexpr f32 COHESION_WEIGHT                   = 8.0f;
constexpr f32 BORDER_WEIGHT                     = 16.0f;
constexpr f32 DRAG_WEIGHT                       = 32.0f;
constexpr f32 MINIMUM_DESIRED_MOVEMENT_DISTANCE = 0.01f;
constexpr f32 BORDER_REPULSION_INITIAL_TANGENT  = -8.0f;
constexpr f32 BORDER_REPULSION_FINAL_TANGENT    = 4.0f;
constexpr f32 HEATMAP_SENSITIVITY               = 8.0f;
constexpr f32 BOID_SCALAR                       = 0.75f;
constexpr i32 HELPER_THREAD_COUNT               = 4;
constexpr vf2 BOID_VERTICES[]                   =
	{
		{  5.0f,  0.0f },
		{ -5.0f,  5.0f },
		{ -1.0f,  0.0f },
		{ -5.0f, -5.0f },
		{  5.0f,  0.0f }
	};

constexpr i32 BASE_WORKLOAD_FOR_HELPER_THREADS = BOID_AMOUNT / (HELPER_THREAD_COUNT + 1);
constexpr i32 MAIN_THREAD_NEW_BOIDS_OFFSET     = HELPER_THREAD_COUNT * BASE_WORKLOAD_FOR_HELPER_THREADS;
constexpr i32 MAIN_THREAD_WORKLOAD             = BOID_AMOUNT - MAIN_THREAD_NEW_BOIDS_OFFSET;

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

struct State;
struct HelperThreadData
{
	SDL_sem*     activation;
	State*       state;
	i32          new_boids_offset;
	SDL_Thread*  helper_thread;
};

struct State
{
	bool32           helper_threads_should_exit;
	SDL_sem*         completed_work;
	HelperThreadData helper_thread_datas[HELPER_THREAD_COUNT];
	u64              seed;
	memarena         general_arena;
	Map              map;
	Boid*            old_boids;
	Boid*            new_boids;
};
