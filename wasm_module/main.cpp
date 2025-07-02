#include "timer.h"
#include "log.h"

void timer_func1(timer_handle_t* h)
{
  TRACE("%s expired", h->name);
}

int main()
{
  using namespace std::chrono_literals;

  auto& tim = timer_queue::instance();

  timer_handle_t t1 = {0};
  tim.create_timer(&t1, timer_func1, "timer 1", 200, true);

  timer_handle_t t2 = {0};
  tim.create_timer(&t2, timer_func1, "timer 2", 500, true);

  TRACE("starting timers");
  tim.start_timer(&t1);
  tim.start_timer(&t2);

  TRACE("sleep 2000ms...");
  std::this_thread::sleep_for(2000ms);
    
  TRACE("...done: stopping timers");
  tim.stop_timer(&t1);
  tim.stop_timer(&t2);

  TRACE("cleanup");
  timer_queue::destroy();
}
