#include <cstring>
#include <iostream>
#include "producer_consumer.h"

void parse_threads_number(char** args, int &threads_number) {
  threads_number = atoi(args[0]);
}

void parse_max_consumer_sleeping_time(char** args, int &max_consumer_sleeping_time) {
  max_consumer_sleeping_time = atoi(args[1]);
}

void parse_command_args(int argc, char** argv, int &threads_number, int& max_consumer_sleeping_time, bool& is_debug_mode) {
  switch (argc) {
    case 3:
      is_debug_mode = false;
      parse_threads_number(argv + 1, threads_number);
      parse_max_consumer_sleeping_time(argv + 1, max_consumer_sleeping_time);
      break;
    case 4:
      is_debug_mode = strcmp(argv[1], "--debug") == 0 ? true : false;
      parse_threads_number(argv + 2, threads_number);
      parse_max_consumer_sleeping_time(argv + 2, max_consumer_sleeping_time);
      break;
    default:
      std::cout
          << "usage: <./my_prog> [--debug] threads_number max_consumer_sleeping_time"
          << std::endl;
          exit(EXIT_FAILURE);
  }
}

int main(int argc, char** argv) {
  int threads_number;
  int max_consumer_sleeping_time;
  bool is_debug_mode;

  parse_command_args(argc, argv, threads_number, max_consumer_sleeping_time, is_debug_mode);

  std::cout
    << run_threads(threads_number, max_consumer_sleeping_time, is_debug_mode)
    << std::endl;

  return EXIT_SUCCESS;
}
