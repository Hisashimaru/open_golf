#include "engine.h"
#include "app.h"
#include "gpu.h"
#include "resource_manager.h"
#include "renderer.h"
#include "material.h"
#include "sound.h"
#include "particle.h"
#include "common.h"
#include "game.h"

#include <crtdbg.h>

inline void* __operator_new(size_t __n) {
	return ::operator new(__n,_NORMAL_BLOCK,__FILE__,__LINE__);
}
inline void* _cdecl operator new(size_t __n,const char* __fname,int __line) {
	return ::operator new(__n,_NORMAL_BLOCK,__fname,__line);
}
inline void _cdecl operator delete(void* __p,const char*,int) {
	::operator delete(__p);
}

static void update()
{
	game_update();
}

static void draw()
{
	game_draw();
}


int main(int argc, char *argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);


	global.game_mode = GameMode::Single;
	// get launch args
	for(int i=0; i<argc; i++)
	{
		if(strcmp(argv[i], "-editmode") == 0){global.game_mode = GameMode::Edit;}
		else if(strcmp(argv[i], "-splitscreen") == 0){global.splitscreen = true;}
		else if(strcmp(argv[i], "-versus") == 0){global.game_mode = GameMode::Versus; global.splitscreen = true;}
		else if(strcmp(argv[i], "-coop") == 0){global.game_mode = GameMode::Coop;  global.splitscreen = true;}
	}

	if(global.game_mode == GameMode::Versus || global.game_mode == GameMode::Coop)
	{
		global.splitscreen = true;
	}
	init_engine();
	init_common();
	game_init();

	start_engine(update, draw);

	game_uninit();
	uninit_engine();
	return 0;
}