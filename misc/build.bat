@echo off

set COMMON_COMPILER_FLAGS=-DDEBUG=1 -nologo -std:c++17 -MTd -GR- -EHsc -EHa- -Od -Oi -Z7 -W4 -wd4100 -wd4201 -I W:\lib\SDL2\include\ -I W:\lib\SDL2_ttf\include\
set COMMON_LINKER_FLAGS=-DEBUG:FULL -opt:ref -incremental:no -subsystem:windows W:\lib\SDL2\lib\x64\SDL2.lib W:\lib\SDL2\lib\x64\SDL2main.lib W:\lib\SDL2_ttf\lib\x64\SDL2_ttf.lib shell32.lib

IF NOT EXIST W:\build\ mkdir W:\build\
pushd W:\build\
	del *.pdb > NUL 2> NUL
	echo "LOCK" > LOCK.tmp
	cl %COMMON_COMPILER_FLAGS% -LD W:\src\SDL_Boids.cpp                           -FmSDL_Boids.map          /link %COMMON_LINKER_FLAGS% -PDB:SDL_Boids_%RANDOM%.pdb -EXPORT:update
	cl %COMMON_COMPILER_FLAGS%     W:\src\SDL_Boids_platform.cpp -FeSDL_Boids.exe -FmSDL_Boids_platform.map /link %COMMON_LINKER_FLAGS%
	sleep 0.25
	del LOCK.tmp

	REM `-nologo`            "Suppresses the display of the copyright banner when the compiler starts up and display of informational messages during compiling."
	REM `-std:c++17`         "Enable supported C and C++ language features from the specified version of the C or C++ language standard."
	REM `-MTd`               "Defines _DEBUG and _MT. This option also causes the compiler to place the library name LIBCMTD.lib into the .obj file so that the linker will use LIBCMTD.lib to resolve external symbols."
	REM `-GR-`               "When /GR is on, the compiler defines the _CPPRTTI preprocessor macro. By default, /GR is on. /GR- disables run-time type information."
	REM `-EHsc`              "(s) Enables standard C++ stack unwinding. Catches only standard C++ exceptions when you use catch(...) syntax. Unless /EHc is also specified,
	REM                       the compiler assumes that functions declared as extern "C" may throw a C++ exception.
	REM                       (c) When used with /EHs, the compiler assumes that functions declared as extern "C" never throw a C++ exception. It has no effect when used
	REM                       with /EHa (that is, /EHca is equivalent to /EHa). /EHc is ignored if /EHs or /EHa aren't specified."
	REM `-EHa-`              "(a) Enables standard C++ stack unwinding. Catches both structured (asynchronous) and standard C++ (synchronous) exceptions when you use catch(...) syntax. /EHa overrides both /EHs and /EHc arguments.
	REM                       (-) Clears the previous option argument. For example, /EHsc- is interpreted as /EHs /EHc-, and is equivalent to /EHs."
	REM `-Od`                "Turns off all optimizations in the program and speeds compilation."
	REM `-Oi`                "Replaces some function calls with intrinsic or otherwise special forms of the function that help your application run faster."
	REM `-Z7`                "The /Z7 option produces object files that also contain full symbolic debugging information for use with the debugger.
	REM                       These object files and any libraries built from them can be substantially larger than files that have no debugging information.
	REM                       The symbolic debugging information includes the names and types of variables, functions, and line numbers. No PDB file is produced by the compiler.
	REM                       However, a PDB file can still be generated from these object files or libraries if the linker is passed the /DEBUG option."
	REM `-W4`                "/W4 displays level 1, level 2, and level 3 warnings, and all level 4 (informational) warnings that aren't off by default."
	REM `-wd4100`            "'identifier' : unreferenced formal parameter"
	REM `-wd4201`            "nonstandard extension used : nameless struct/union"
	REM `-I`                 "Adds a directory to the list of directories searched for include files."
	REM `-DEBUG:FULL`        "The /DEBUG:FULL option moves all private symbol information from individual compilation products (object files and libraries) into a single PDB, and can be the most time-consuming part of the link.
	REM                       However, the full PDB can be used to debug the executable when no other build products are available, such as when the executable is deployed."
	REM `-opt:ref`           "/OPT:REF eliminates functions and data that are never referenced."
	REM `-incremental:no`    "Controls how the linker handles incremental linking."
	REM `-subsystem:windows` "The /SUBSYSTEM option specifies the environment for the executable. Application does not require a console, probably because it creates its own windows for interaction with the user."
	REM `-Fm`                "Tells the linker to produce a mapfile containing a list of segments in the order in which they appear in the corresponding .exe file or DLL."
	REM `-LD`                "Indicates whether a multithreaded module is a DLL and specifies retail or debug versions of the run-time library. Creates a DLL."
popd
