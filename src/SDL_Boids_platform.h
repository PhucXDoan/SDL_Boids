#pragma once
#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <windows.h> // @TODO@ Eventually become platform agnostic?
#include "unified.h"

#pragma warning(push, 0)
#pragma warning(disable : 4701 4702)
#include "SDL_FontCache.c"
#pragma warning(pop)

#define kibibytes_of(N) (1024LL *             (N))
#define mebibytes_of(N) (1024LL * kibibytes_of(N))
#define gibibytes_of(N) (1024LL * mebibytes_of(N))
#define tebibytes_of(N) (1024LL * gibibytes_of(N))

constexpr memsize MEMORY_CAPACITY            = mebibytes_of(1);
constexpr strlit  PROGRAM_DLL_FILE_PATH      = "W:/build/SDL_Boids.dll";
constexpr strlit  PROGRAM_DLL_TEMP_FILE_PATH = "W:/build/SDL_Boids.dll.temp";
constexpr strlit  LOCK_FILE_PATH             = "W:/build/LOCK.tmp";

#if DEBUG
constexpr i32     DEBUG_WINDOW_WIDTH         = 500;
constexpr i32     DEBUG_WINDOW_HEIGHT        = 500;
constexpr i32     DEBUG_WINDOW_COORDINATES_X = 950;
constexpr i32     DEBUG_WINDOW_COORDINATES_Y = 400;

constexpr i32     WINDOW_WIDTH               = 960;
constexpr i32     WINDOW_HEIGHT              = 540;
constexpr i32     WINDOW_COORDINATES_X       = 50;
constexpr i32     WINDOW_COORDINATES_Y       = 150;
#else
constexpr i32     WINDOW_WIDTH               = 1280;
constexpr i32     WINDOW_HEIGHT              = 720;
constexpr i32     WINDOW_COORDINATES_X       = SDL_WINDOWPOS_UNDEFINED;
constexpr i32     WINDOW_COORDINATES_Y       = SDL_WINDOWPOS_UNDEFINED;
#endif

struct Program
{
	bool32        is_running;
	bool32        is_going_to_hotload;
	f32           delta_seconds;
	SDL_Renderer* renderer;
	byte*         memory;
	memsize       memory_capacity;
	u32           window_id;

	#if DEBUG
	SDL_Renderer* debug_renderer;
	u32           debug_window_id;
	#endif
};

#define PROTOTYPE_INITIALIZE(NAME) void NAME(Program* program)
typedef PROTOTYPE_INITIALIZE(PrototypeInitialize);

#define PROTOTYPE_BOOT_DOWN(NAME) void NAME(Program* program)
typedef PROTOTYPE_BOOT_DOWN(PrototypeBootDown);

#define PROTOTYPE_BOOT_UP(NAME) void NAME(Program* program)
typedef PROTOTYPE_BOOT_UP(PrototypeBootUp);

#define PROTOTYPE_UPDATE(NAME) void NAME(Program* program)
typedef PROTOTYPE_UPDATE(PrototypeUpdate);

struct HotloadingData
{
	byte*                dll;
	FILETIME             dll_creation_time;
	PrototypeInitialize* initialize;
	PrototypeBootDown*   boot_down;
	PrototypeBootUp*     boot_up;
	PrototypeUpdate*     update;
};
