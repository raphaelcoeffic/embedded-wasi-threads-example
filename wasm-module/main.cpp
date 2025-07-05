#include "log.h"
#include "timer.h"

#if __wasm__
#define WASM_EXPORT_AS(name) __attribute__((export_name(name)))
#define WASM_EXPORT(symbol) WASM_EXPORT_AS(#symbol) symbol
#else
#define WASM_EXPORT(symbol) symbol
#endif

timer_handle_t t1 = TIMER_INITIALIZER;
timer_handle_t t2 = TIMER_INITIALIZER;

uint32_t counters[2] = {0};

void timer_func(timer_handle_t *h, int idx) {
  TRACE("%s expired", h->name);
  counters[idx]++;
}

void timer_func1(timer_handle_t *h) { timer_func(h, 0); }
void timer_func2(timer_handle_t *h) { timer_func(h, 1); }

void WASM_EXPORT(get_counters)(uint32_t** p_counters, size_t* len) {
  TRACE("counters[0] = %u", counters[0]);
  TRACE("counters[1] = %u", counters[1]);
  *p_counters = counters;
  *len = sizeof(counters) / sizeof(uint32_t);
}

void WASM_EXPORT(create_timers)() {
  auto &tim = timer_queue::instance();
  tim.create_timer(&t1, timer_func1, "timer 1", 200, true);
  tim.create_timer(&t2, timer_func2, "timer 2", 500, true);
}

void WASM_EXPORT(start_timers)() {
  TRACE("starting timers");
  auto &tim = timer_queue::instance();
  tim.start_timer(&t1);
  tim.start_timer(&t2);
}

void WASM_EXPORT(stop_timers)() {
  TRACE("stopping timers");
  auto &tim = timer_queue::instance();
  tim.stop_timer(&t1);
  tim.stop_timer(&t2);
}

void WASM_EXPORT(cleanup)() {
  TRACE("cleanup");
  timer_queue::destroy();
}

int main() {
  using namespace std::chrono_literals;

  create_timers();
  start_timers();

  TRACE("sleep 2000ms...");
  std::this_thread::sleep_for(2000ms);

  stop_timers();
  cleanup();
}
