#include "imp_export.h"
#include "log.h"
#include "timer.h"

timer_handle_t t1 = TIMER_INITIALIZER;
timer_handle_t t2 = TIMER_INITIALIZER;

uint32_t counters[2] = {0};

void timer_func(timer_handle_t *h, int idx) {
  TRACE("%s expired", h->name);
  counters[idx]++;
}

void timer_func1(timer_handle_t *h) { timer_func(h, 0); }
void timer_func2(timer_handle_t *h) { timer_func(h, 1); }

const char* WASM_EXPORT(get_module_name)() {
  return "WASI test module";
}

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
  while (!tim.start_timer_async(&t1)) {}
  while (!tim.start_timer_async(&t2)) {}
}

void WASM_EXPORT(stop_timers)() {
  TRACE("stopping timers");
  auto &tim = timer_queue::instance();
  while (!tim.stop_timer_async(&t1)) {}
  while (!tim.stop_timer_async(&t2)) {}
}

void WASM_EXPORT(cleanup)() {
  TRACE("cleanup");
  timer_queue::destroy();
}

bool WASM_EXPORT(async_cleanup)() {
  return timer_queue::destroy_async();
}

void std::__libcpp_verbose_abort(char const* format, ...) {
  va_list list;
  va_start(list, format);
  TRACE_VA(format, list);
  va_end(list);

  std::exit(1);
}
