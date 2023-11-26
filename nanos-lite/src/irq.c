#include <common.h>
#define YIELD_FLAG 1

static Context* do_event(Event e, Context* c) {//The event handler functin
  switch (e.event) {
    case YIELD_FLAG:
      printf("Yield!\n");
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
