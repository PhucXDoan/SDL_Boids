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
constexpr f32 DRAG_WEIGHT                       = 128.0f;
constexpr f32 MINIMUM_DESIRED_MOVEMENT_DISTANCE = 0.01f;
constexpr f32 BORDER_REPULSION_INITIAL_TANGENT  = -8.0f;
constexpr f32 BORDER_REPULSION_FINAL_TANGENT    = 4.0f;
constexpr f32 HEATMAP_SENSITIVITY               = 8.0f;
constexpr f32 BOID_SCALAR                       = 0.75f;
constexpr vf2 BOID_VERTICES[]                   =
	{
		vf2 {  5.0f,  0.0f },
		vf2 { -5.0f,  5.0f },
		vf2 { -1.0f,  0.0f },
		vf2 { -5.0f, -5.0f },
		vf2 {  5.0f,  0.0f }
	};

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

struct State
{
	u64      seed;
	memarena general_arena;
	Map      map;
	Boid*    old_boids;
	Boid*    new_boids;
};
