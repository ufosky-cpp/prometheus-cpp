//
// Created by Qihe Bian on 22/03/2018.
//

#include <iostream>
#include <atomic>
#include <thread>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include <prometheus/json_serializer.h>

int32_t thread_count = 100;
int32_t key_count = 100;
int32_t duration = 30;

prometheus::Exposer exposer{"127.0.0.1:8080"};
auto registry = std::make_shared<prometheus::Registry>();

bool should_exit = false;
//std::atomic<int64_t> work_func_count{};
//std::unordered_map<std::string, std::unique_ptr<std::atomic<int64_t>>> funcs_count{};

void work_func(std::string func_name, int num) {

  auto& counter_family =
      prometheus::BuildCounter().Name(func_name).Register(*registry);
  counter_family.Add({{"name", "counter1"}});
  counter_family.Add({{"name", "counter2"}});
}

void thread_func() {
  while (true && !should_exit) {
    std::vector<std::function<void(int)>> funcs;

    for (int i = 0; i < key_count && !should_exit; ++i) {
      std::stringstream ss;
      ss << "func" << std::setw(4) << std::setfill('0') << i;
      auto func = std::bind(work_func, ss.str(), std::placeholders::_1);
      funcs.emplace_back(func);
    }
    for (auto& func: funcs) {
      if (should_exit) {
        return;
      }
      int r = rand() % 1000;
      func(r);
    }
  }
}

int main(int argc, char *argv[]) {
  exposer.RegisterCollectable(registry);

  std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
//  for (int i = 0; i < key_count; ++i) {
//    std::stringstream ss;
//    ss << "func" << std::setw(4) << std::setfill('0') << i;
//    funcs_count.emplace(std::make_pair(ss.str(), std::unique_ptr<std::atomic<int64_t>>(new std::atomic<int64_t>{})));
//  }
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i) {
    threads.emplace_back(std::thread(thread_func));
  }
  while (true && !should_exit) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
    if (seconds >= duration) {
      should_exit = true;
    }
    usleep(1000);
  }
  for (auto& thread: threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  auto collected = registry->Collect();
  auto serializer = std::unique_ptr<prometheus::JsonSerializer>{new prometheus::JsonSerializer};
  auto body = serializer->Serialize(collected);
  std::cout << body << std::endl;
//  std::cout << "work_func: " << work_func_count.load() << std::endl;
//  for (auto &p: funcs_count) {
//    std::cout << p.first << ": " << p.second->load() << std::endl;
//  }
  return 0;
}
