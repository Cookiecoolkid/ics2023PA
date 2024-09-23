#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  char *empty[] =  {NULL };
  environ = empty;
  int argc = (int)(*args);
  char **argv = (char**)(args + 1);
  char **envp = (char**)(args + argc + 1);
  exit(main(0, NULL, NULL));
  assert(0);
}
