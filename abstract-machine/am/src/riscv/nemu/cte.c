#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
	  case -1: ev.event = EVENT_YIELD; break;
	  case 0: 
	  case 1: 
	  case 2:
	  case 3:
	  case 4:
	  case 5: 
	  case 6:
	  case 7:
	  case 8:
	  case 9:
	  case 10:
	  case 11:
	  case 12:
	  case 13:
	  case 14:
	  case 15:
	  case 16:
	  case 17:
	  case 18:
	  case 19:
	  case 20: ev.event = EVENT_SYSCALL; break; // From U mode enter S mode
      default: printf("Can't tell this mcause %d\n", c->mcause);ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
//	printf("mcause: %d, mstatus: %d, mepc: %d\n", c->mcause, c->mstatus, c->mepc);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *newContext = (Context*)(kstack.end - sizeof(Context));
  newContext->mepc = (uintptr_t)entry;

  newContext->gpr[10] = (uintptr_t)arg;
  return newContext;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
