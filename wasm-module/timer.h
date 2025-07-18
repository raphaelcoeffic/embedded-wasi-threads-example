#ifndef TIMER_H
#define TIMER_H

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>

struct timer_handle_t;

typedef void (*timer_func_t)(timer_handle_t*);
typedef void (*timer_async_func_t)(void*, uint32_t);
typedef std::chrono::steady_clock::time_point time_point_t;

struct timer_handle_t {
  timer_func_t func;
  const char*  name;
  unsigned     period;
  bool         repeat;

  time_point_t next_trigger;
  std::atomic_bool active;
};

#define TIMER_INITIALIZER \
  { .func = nullptr, .name = nullptr }

struct timer_async_call_t {
  timer_async_func_t func;
  void *param1;
  uint32_t param2;
};

struct timer_req_t {
  enum type {
    cmd_start,
    cmd_stop,
    cmd_pend_func,
    cmd_stop_timer_queue,
  };

  type cmd;

  union {
    timer_handle_t *timer;
    timer_async_call_t func_call;
  };
};

class timer_queue {

  std::unique_ptr<std::thread> _thread;
  std::atomic<bool> _running = {false};

  std::deque<timer_req_t> _cmds;
  std::mutex _cmds_mutex;
  std::condition_variable _cmds_condition;

  std::vector<timer_handle_t*> _timers;
  std::vector<timer_async_call_t> _funcs;

  time_point_t _current_time;
  
  timer_queue();

  void update_current_time();
  void sort_timers();
  void main_loop();
  void trigger_timers();
  void async_calls();
  void process_cmds();

  void send_cmd(timer_req_t&& req);
  bool send_cmd_async(timer_req_t&& req);

  void stop();

public:
  timer_queue(timer_queue const &) = delete;
  void operator=(timer_queue const &) = delete;

  static timer_queue &instance();
  static void destroy();
  static bool destroy_async();

  static void create_timer(timer_handle_t *timer, timer_func_t func, const char *name,
                           unsigned period, bool repeat);

  void start();

  void start_timer(timer_handle_t *timer);
  void stop_timer(timer_handle_t *timer);

  void pend_function(timer_async_func_t func, void* param1, uint32_t param2);

  bool start_timer_async(timer_handle_t *timer);
  bool stop_timer_async(timer_handle_t *timer);

  bool stop_async();
};

#endif // TIMER_H
