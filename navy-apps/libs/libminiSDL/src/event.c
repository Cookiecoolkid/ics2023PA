#include <NDL.h>
#include <SDL.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  assert(0);
  return 0;
}

static int keycd = 0;
static int iskeydown = 0;

int SDL_PollEvent(SDL_Event *ev) {
  char* buf = (char*)malloc(sizeof(char) * 32);
  if(NDL_PollEvent(buf, sizeof(buf)))
  {
  	 if(buf[0] == 'k' && buf[1] == 'd')
 	 {
	  	ev->key.type = SDL_KEYDOWN;
		iskeydown = 1;
 	 }
  	 else if(buf[0] == 'k' && buf[1] == 'u')
 	 {	
	 	 ev->key.type = SDL_KEYUP;
		 iskeydown = 0;
		 //ev->key.keysym.sym = 0;
  	 }
	 for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++)
		 if(!strcmp(buf + 3, keyname[i])) ev->key.keysym.sym = i, keycd = i;
	 free(buf);	  
	 return 1;
  }
  free(buf);
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  char buf[64];
  // NDL_PollEvent(buf, sizeof(buf));
  while(!NDL_PollEvent(buf, sizeof(buf)));
  
  if(buf[0] == 'k' && buf[1] == 'd')
  {
	  event->key.type = SDL_KEYDOWN;
      /*	  
	  if(buf[4] == '\0') event->key.keysym.sym = (int)buf[3];
	  else if (!strcmp(buf + 3, "UP")) 
		  event->key.keysym.sym = SDLK_UP;
	  else if (!strcmp(buf + 3, "DOWN"))
		  event->key.keysym.sym = SDLK_DOWN;
	  else if (!strcmp(buf + 3, "LEFT")) 
		  event->key.keysym.sym = SDLK_LEFT;
	  else if (!strcmp(buf + 3, "RIGHT"))
		  event->key.keysym.sym = SDLK_RIGHT;*/
	  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++)
		  if(!strcmp(buf + 3, keyname[i])) event->key.keysym.sym = i;
  }
  else event->type = SDL_KEYUP, event->key.keysym.sym = 0;
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  assert(0);
  return 0;
}

uint8_t keystate[sizeof(keyname) / sizeof(keyname[0])];

uint8_t* SDL_GetKeyState(int *numkeys) {
  SDL_Event e;
  e.key.type = (iskeydown == 1 ? SDL_KEYDOWN : SDL_KEYUP);
  e.key.keysym.sym = (iskeydown == 1 ? keycd : 0);
  iskeydown = 0;
  keycd = 0;
  //int ret = SDL_PollEvent(&e);
  //printf("ret = %d\n", ret);
  if (e.key.type == SDL_KEYDOWN)
  {
	  keystate[e.key.keysym.sym] = 1;
	  printf("KeyState[%d] = 1\n", e.key.keysym.sym);
  }
  else 
  {
	  for(int i = 0; i < sizeof(keystate) / sizeof(keystate[0]); i++)
		keystate[i] = 0;
  }
  return keystate;
}
