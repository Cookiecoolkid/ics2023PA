#include <proc.h>

#define MAX_NR_PROC 4
extern uintptr_t loader(PCB *pcb, const char *filename);
extern Context *ucontext(AddrSpace *as, Area kstack, void *entry);
extern void naive_uload(PCB *pcb, const char *filename);
static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void context_kload(PCB* newPCB, void (*entry)(void*), void *arg)
{
   newPCB->cp = kcontext((Area) {newPCB->stack, newPCB + 1}, entry, arg);
}

void context_uload(PCB* newPCB, char* filename) // , char* const argv[], char* const envp[])
{
	uintptr_t* sptr = (uintptr_t*)heap.end;
    /*
    int argc = 0;
	int envc = 0;
	if(argv != NULL && envp != NULL)
	{
		while(argv[argc] != NULL) argc++;
		while(envp[envc] != NULL) envc++;

		printf("A\n");

		for(int i = argc - 1; i >= 0; i--)
		{
       		sptr -= (strlen(argv[i]) + 1);
	   		strcpy((char*)sptr, argv[i]);
		}

		printf("B\n");

		for(int i = envc - 1; i >= 0; i--)
		{
	   		sptr -= (strlen(envp[i]) + 1);
	   		strcpy((char*)sptr, envp[i]);
		}
    
		sptr -= 4; // NULL
		printf("C\n");

	    for(int i = envc - 1; i >= 0; i--)
		{
			sptr -= 4;
			*sptr = (uintptr_t)heap.end - sizeof(uintptr_t) * (i + 1);
		}

		sptr -= 4;
		printf("D\n");

		for(int i = argc - 1; i >= 0; i--)
		{
			sptr -= 4;
			*sptr = (uintptr_t)heap.end - sizeof(uintptr_t) * (envc + i + 2);
		}

    	sptr -= 4;
		*sptr = argc;
    	printf("E\n");
   }

   printf("G\n");
   */
   uintptr_t entry = loader(newPCB, filename);
   newPCB->cp = ucontext(NULL, (Area) {newPCB->stack, newPCB + 1}, (void*)entry);
   newPCB->cp->GPRx = (uintptr_t)sptr;
}

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, (void*)"@");
  //context_uload(&pcb[0], "/bin/hello");
  context_uload(&pcb[1], "/bin/pal");
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  // save the context pointer
  current->cp = prev;

  // switch between pcb[0] and pcb[1]
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  
  // then return the new context
  return current->cp;
}
