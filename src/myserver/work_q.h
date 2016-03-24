// Copyright 2013 15418 Course Staff.

#include <vector>
#include <pthread.h>

template <class T>
class WorkQueueProjIdea {
private:
  std::vector<T> storage;
  pthread_mutex_t queue_lock;
  pthread_cond_t queue_cond;

public:
  bool started;
  WorkQueueProjIdea() {
    pthread_cond_init(&queue_cond, NULL);
    pthread_mutex_init(&queue_lock, NULL);
    started = false;
  }

  T get_work() {
    pthread_mutex_lock(&queue_lock);
    while (storage.size() == 0) {
      pthread_cond_wait(&queue_cond, &queue_lock);
    }

    T item = storage.front();
    //storage.pop_front();
    storage.erase(storage.begin());

    pthread_mutex_unlock(&queue_lock);
    return item;
  }

  void put_work(const T& item) {
    pthread_mutex_lock(&queue_lock);
    started = true;
    storage.push_back(item);
    pthread_mutex_unlock(&queue_lock);
    pthread_cond_signal(&queue_cond);
  }

  int size() {
    pthread_mutex_lock(&queue_lock);
    int s = storage.size();
    pthread_mutex_unlock(&queue_lock);
    return s;
  }
};