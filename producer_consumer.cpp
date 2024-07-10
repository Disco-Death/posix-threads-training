#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <pthread.h>
#include <random>
#include <sstream>
#include <unistd.h>
#include <vector>

typedef struct thread_parametrs {
  int consumers_number;
  int endpoint_consimers_number = 0;
  int id = 0;
  bool is_debug_mode = false;
  bool is_reading_end = false;
  int max_consumer_sleeping_time;
  int startpoint_cosumers_number = 0;
  bool is_ready_number = false;
} thread_params;

pthread_cond_t consumer_cond = PTHREAD_COND_INITIALIZER;
pthread_key_t cur_tid;
pthread_mutex_t abs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t *thread_ids;
pthread_cond_t value_cond = PTHREAD_COND_INITIALIZER;

int gen_random_int_num_with_limits(int low_limit, int high_limit) {  // random function
  std::random_device rd;  // Only used once to initialise (seed) engine
  
  std::mt19937 gen(
      rd());  // Random-number engine used (Mersenne-Twister in this case)
  std::uniform_int_distribution<int> dis(low_limit, high_limit);
  
  return dis(gen);
}

int get_tid() {  // function creates custom threads ids
  // 1 to 3+N thread ID
  static std::atomic<int> static_thread_counter = {1};

  int *tid = static_cast<int *>(pthread_getspecific(cur_tid));
  if (tid == NULL) {
    int *new_tid = new int(static_thread_counter.fetch_add(1));
    pthread_setspecific(cur_tid, new_tid);
    return *new_tid;
  }
  return *(std::atomic<int> *)tid;
}

void tid_destructor(void *ptr) {  // destructor function for custom thread ids
  int *tid = (int *)ptr;
  delete tid;
}

void *producer_routine(void *arg) {
    // Wait for consumer to start
    // Read data, loop through each value and update the value,
    // notify consumer, wait for consumer to process

    struct thread_params* p = (struct thread_params*)arg;

    pthread_mutex_lock(&my_mutex);
    if (!p->consumers_are_running) pthread_cond_wait(&my_cond_consumers_is_ready, my_mutex);

    // read numbers from stdin
    std::string input;
    getline(std::cin, input);

    std::istringstream string_stream(input);
    for (long num; string_stream >> num;) {
        p->my_num = num;
        p->empty_flag = false;

        while (!p->empty_flag) {
            pthread_cond_signal(&my_cond_full);
            pthread_cond_wait(&my_cond_empty, &my_mutex);
        }
    }
    p->done_flag = true;
    // wakeup consumer threads, so they can return calculations
    pthread_cond_broadcast(&my_cond_full);
    pthread_mutex_unlock(&my_mutex);

    pthread_cancel(my_pthreads_ids[1]);
    return nullptr;
}

void *consumer_routine(void *arg) {
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  thread_params *params = (thread_params *)arg;
  int *l_sum = new int(0);
  // for every update issued by producer, read the value and add to sum
  // return pointer to result (for particular consumer)
  while (true) {
    pthread_mutex_lock(&abs_mutex);
    params->startpoint_cosumers_number++;
    pthread_cond_broadcast(&consumer_cond);

    do {
      pthread_cond_wait(&value_cond, &abs_mutex);
    } while (!params->is_ready_number);

    if (!params->is_reading_end) {
      *l_sum += params->id;
      if (params->is_debug_mode)
        std::cout << "tid " << get_tid() << ": psum " << *l_sum << std::endl;

      params->startpoint_cosumers_number--;
      params->is_ready_number = false;

      pthread_cond_broadcast(&consumer_cond);

      pthread_mutex_unlock(&abs_mutex);
    } else {
      params->endpoint_consimers_number++;

      pthread_cond_broadcast(&consumer_cond);

      pthread_mutex_unlock(&abs_mutex);

      return l_sum;
    }
    usleep(gen_random_int_num_with_limits(0, params->max_consumer_sleeping_time) * 1000);
  }
}

void *consumer_interruptor_routine(void *arg) {
  thread_params *par = (thread_params *)arg;
  // interrupt random consumer while producer is running
  while (!par->is_reading_end) {
    pthread_t cancel_id = thread_ids[gen_random_int_num_with_limits(2, par->consumers_number + 1)];
    pthread_cancel(cancel_id);
  }
  return nullptr;
}

// the declaration of run threads can be changed as you like
int run_threads(int threads_number, int max_consumer_sleeping_time, bool is_debug_mode) {
  // start N threads and wait until they're done
  // return aggregated sum of values

  // pthread_key_create(&cur_tid, tid_destructor);
  pthread_key_create(&cur_tid, tid_destructor);
  thread_ids = new pthread_t[2 + threads_number];

  thread_params params = {
    1,
    true,
    false,
    is_debug_mode,
    threads_number,
    max_consumer_sleeping_time,
    0,
    0
  };

  pthread_create(&thread_ids[0], NULL, producer_routine, &params);

  for (auto i = 0; i < threads_number; ++i) {
    pthread_create(&thread_ids[2 + i], NULL, consumer_routine, &params);
  }

  pthread_create(&thread_ids[1], NULL, consumer_interruptor_routine, &params);

  auto sum = std::make_unique<int>(0);
  for (int i = 0; i < threads_number; ++i) {
    void *result_ptr;
    pthread_join(thread_ids[2 + i], &result_ptr);
    *sum += *(int *)result_ptr;
    delete (int *)result_ptr;
  }

  pthread_join(thread_ids[0], nullptr);
  pthread_join(thread_ids[1], nullptr);

  delete[] thread_ids;

  return *sum;
}
