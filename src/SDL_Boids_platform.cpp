#include "SDL_Boids_platform.h"

PROTOTYPE_UPDATE(program_update_fallback) {}

FILETIME get_program_dll_creation_time(void)
{
	WIN32_FILE_ATTRIBUTE_DATA attributes;
	return
		GetFileAttributesExA(PROGRAM_DLL_FILE_PATH, GetFileExInfoStandard, &attributes)
			? attributes.ftLastWriteTime
			: FILETIME {};
}

// @STICKY@ Reference: `https://gist.github.com/ChrisDill/291c938605c200d079a88d0a7855f31a`.
void reload_program_dll(HotloadingData* hotloading_data)
{
	if (hotloading_data->dll)
	{
		SDL_UnloadObject(hotloading_data->dll);
	}

	SDL_RWops* src      = SDL_RWFromFile(PROGRAM_DLL_FILE_PATH     , "r");
	SDL_RWops* des      = SDL_RWFromFile(PROGRAM_DLL_TEMP_FILE_PATH, "w");
	i64        src_size = SDL_RWsize(src);
	byte*      buffer   = reinterpret_cast<byte*>(SDL_calloc(1, src_size));

	SDL_RWread (src, buffer, src_size, 1);
	SDL_RWwrite(des, buffer, src_size, 1);
	SDL_RWclose(src);
	SDL_RWclose(des);
	SDL_free(buffer);

	hotloading_data->dll               = reinterpret_cast<byte*>(SDL_LoadObject(PROGRAM_DLL_TEMP_FILE_PATH));
	hotloading_data->dll_creation_time = get_program_dll_creation_time();
	hotloading_data->initialize        = reinterpret_cast<PrototypeUpdate*>(SDL_LoadFunction(hotloading_data->dll, "initialize"));
	hotloading_data->boot_down         = reinterpret_cast<PrototypeUpdate*>(SDL_LoadFunction(hotloading_data->dll, "boot_down"));
	hotloading_data->boot_up           = reinterpret_cast<PrototypeUpdate*>(SDL_LoadFunction(hotloading_data->dll, "boot_up"));
	hotloading_data->update            = reinterpret_cast<PrototypeUpdate*>(SDL_LoadFunction(hotloading_data->dll, "update"));
}

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not initialize video.");
		return -1;
	}

	if (TTF_Init() == -1)
	{
		fprintf(stderr, "TTF_Error: '%s'\n", TTF_GetError());
		ASSERT(!"SDL_ttf could not initialize.");
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("SDL_Boids", WINDOW_COORDINATES_X, WINDOW_COORDINATES_Y, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	if (!window)
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not create window.");
		return -1;
	}

	SDL_Window* aux_window = SDL_CreateWindow("aux_SDL_Boids", AUX_WINDOW_COORDINATES_X, AUX_WINDOW_COORDINATES_Y, AUX_WINDOW_WIDTH, AUX_WINDOW_HEIGHT, 0);
	if (!aux_window)
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not create auxillary window.");
		return -1;
	}

	SDL_Renderer* window_renderer = SDL_CreateRenderer(window, -1, 0);
	if (!window_renderer)
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not create a renderer for the window.");
	}

	SDL_Renderer* aux_window_renderer = SDL_CreateRenderer(aux_window, -1, 0);
	if (!aux_window_renderer)
	{
		fprintf(stderr, "SDL_Error: '%s'\n", SDL_GetError());
		ASSERT(!"SDL could not create a renderer for the auxillary window.");
	}

	Program program;
	program.is_running          = true;
	program.is_going_to_hotload = false;
	program.delta_seconds       = 0.0f;
	program.renderer            = window_renderer;
	program.aux_renderer        = aux_window_renderer;
	program.memory              = reinterpret_cast<byte*>(VirtualAlloc(reinterpret_cast<LPVOID>(tebibytes_of(4)), MEMORY_CAPACITY, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	program.memory_capacity     = MEMORY_CAPACITY;

	HotloadingData hotloading_data = {};
	reload_program_dll(&hotloading_data);
	hotloading_data.initialize(&program);
	hotloading_data.boot_up(&program);

	u64 performance_count = SDL_GetPerformanceCounter();
	while (program.is_running)
	{
		u64 new_performance_count = SDL_GetPerformanceCounter();
		program.delta_seconds = static_cast<f32>(new_performance_count - performance_count) / SDL_GetPerformanceFrequency();
		performance_count     = new_performance_count;

		FILETIME current_program_dll_creation_time = get_program_dll_creation_time();
		if (CompareFileTime(&current_program_dll_creation_time, &hotloading_data.dll_creation_time))
		{
			WIN32_FILE_ATTRIBUTE_DATA attributes_;
			while (GetFileAttributesEx(LOCK_FILE_PATH, GetFileExInfoStandard, &attributes_));

			hotloading_data.boot_down(&program);
			reload_program_dll(&hotloading_data);
			hotloading_data.boot_up(&program);
		}
		else
		{
			hotloading_data.update(&program);
		}
	}

	SDL_DestroyRenderer(aux_window_renderer);
	SDL_DestroyRenderer(window_renderer);
	SDL_DestroyWindow(aux_window);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();

	return 0;
}
