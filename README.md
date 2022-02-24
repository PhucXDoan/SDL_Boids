# SDL\_Boids

An SDL implementation of a simulation of boids.

Each boid move at a constant velocity and change their direction based on three main factors:

- **Separation**, boids want to remain at a certain distance from others,
- **Alignment**, face in the same general direction, and
- **Cohesion**, to stick within the herd.

<img src="./misc/gifs/debug_SDL_Boids.gif" width="500"/>

I've implemented some fancy things such as hotloading, SIMD, basic built-in profiler, and a visualizer. Camera movement is done with standard WASD. Time can be changed using left/right arrow keys. Zooming can be done through down/up arrow keys.

<img src="./misc/gifs/production_SDL_Boids.gif" width="500"/>

The SDL2 development library, SDL2\_ttf extension, SDL2\_FontCache library are needed to compile. A `.ttf` font is probably needed too in order to render texts.
Use `W:\misc\shell.bat` to initialize the development environment and `W:\misc\build.bat` to build with MSVC.
