#pragma once
#include <stdio.h>
#include <SDL.h>
#include "unified.h"

#define kibibytes_of(N) (1024LL *             (N))
#define mebibytes_of(N) (1024LL * kibibytes_of(N))
#define gibibytes_of(N) (1024LL * mebibytes_of(N))
#define tebibytes_of(N) (1024LL * gibibytes_of(N))

constexpr memsize MEMORY_CAPACITY = mebibytes_of(1);

struct PLATFORM_Program
{
	bool32        is_running;
	bool32        is_initialized;
	f32           delta_seconds;
	SDL_Renderer* renderer;
	byteptr       memory;
	memsize       memory_capacity;
};
