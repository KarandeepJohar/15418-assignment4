
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <glog/logging.h>

#include "server/messages.h"
#include "server/worker.h"
#include "tools/cycle_timer.h"
#include "tools/work_queue.h"
#include <pthread.h>
#ifndef NUMTHREADS
#define NUM_THREADS     35
#endif
WorkQueue <Request_msg> wq;
WorkQueue <Request_msg> tellmenow_q;
WorkQueue <Request_msg> projectidea_q;

int stick_this_thread_to_core(int start_core_id=1, int end_core_id=sysconf(_SC_NPROCESSORS_ONLN)) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   DLOG(INFO) << "num_cores: [" << num_cores << "]\n";
   if (start_core_id < 0 || start_core_id >= num_cores || end_core_id < 0 || end_core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   for (int i = start_core_id; i < end_core_id; ++i)
   {
     CPU_SET(i, &cpuset);
   }
   

   pthread_t current_thread = pthread_self();    
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}


void* handle_requests(void* threadid) {
  stick_this_thread_to_core();
  while(1) {
      Request_msg req= wq.get_work();
      // Make the tag of the reponse match the tag of the request.  This
      // is a way for your master to match worker responses to requests.
      Response_msg resp(req.get_tag());

      // Output debugging help to the logs (in a single worker node
      // configuration, this would be in the log logs/worker.INFO)
      DLOG(INFO) << "Worker got request: [" << req.get_tag() << ":" << req.get_request_string() << "]\n";

      double startTime = CycleTimer::currentSeconds();

      

      // actually perform the work.  The response string is filled in by
      // 'execute_work'


      //add work to queue  req
      execute_work(req, resp);

      

      double dt = CycleTimer::currentSeconds() - startTime;
      DLOG(INFO) << "Worker completed work in " << (1000.f * dt) << " ms (" << req.get_tag()  << ")\n";

      // send a response string to the master
      worker_send_response(resp);
  }
  pthread_exit(NULL);
}
void* handle_tellmenow_requests(void* threadid) {
  stick_this_thread_to_core();
  while(1) {
      Request_msg req= tellmenow_q.get_work();
      // Make the tag of the reponse match the tag of the request.  This
      // is a way for your master to match worker responses to requests.
      Response_msg resp(req.get_tag());

      // Output debugging help to the logs (in a single worker node
      // configuration, this would be in the log logs/worker.INFO)
      DLOG(INFO) << "Worker got request: [" << req.get_tag() << ":" << req.get_request_string() << "]\n";

      double startTime = CycleTimer::currentSeconds();

      

      // actually perform the work.  The response string is filled in by
      // 'execute_work'


      //add work to queue  req
      execute_work(req, resp);

      

      double dt = CycleTimer::currentSeconds() - startTime;
      DLOG(INFO) << "Worker completed work in " << (1000.f * dt) << " ms (" << req.get_tag()  << ")\n";

      // send a response string to the master
      worker_send_response(resp);
  }
  pthread_exit(NULL);
}
void* handle_projectidea_requests(void* threadid) {

  stick_this_thread_to_core(0,1);
  while(1) {
      Request_msg req= projectidea_q.get_work();
      // Make the tag of the reponse match the tag of the request.  This
      // is a way for your master to match worker responses to requests.
      Response_msg resp(req.get_tag());

      // Output debugging help to the logs (in a single worker node
      // configuration, this would be in the log logs/worker.INFO)
      DLOG(INFO) << "Worker got request: [" << req.get_tag() << ":" << req.get_request_string() << "]\n";

      double startTime = CycleTimer::currentSeconds();

      

      // actually perform the work.  The response string is filled in by
      // 'execute_work'


      //add work to queue  req
      execute_work(req, resp);

      

      double dt = CycleTimer::currentSeconds() - startTime;
      DLOG(INFO) << "Worker completed work in " << (1000.f * dt) << " ms (" << req.get_tag()  << ")\n";

      // send a response string to the master
      worker_send_response(resp);
  }
  pthread_exit(NULL);
}
void worker_node_init(const Request_msg& params) {

  // This is your chance to initialize your worker.  For example, you
  // might initialize a few data structures, or maybe even spawn a few
  // pthreads here.  Remember, when running on Amazon servers, worker
  // processes will run on an instance with a dual-core CPU.
  // WorkQueue<Request_msg> wq = WorkQueue();
  
  int i;
  pthread_t threads[NUM_THREADS];
  for( i=0; i < NUM_THREADS; i++ ){
        pthread_create(&threads[i], NULL, handle_requests, NULL);

     }
  pthread_t t1,t2;
  pthread_create(&t1, NULL, handle_tellmenow_requests, NULL);
  pthread_create(&t2, NULL, handle_projectidea_requests, NULL);

  // initialize work queue
  // launch threads with function to wait for requests
  DLOG(INFO) << "**** Initializing worker: " << params.get_arg("name") << " ****\n";

}

void worker_handle_request(const Request_msg& req) {
  DLOG(INFO) << "Worker handle_requests: [" << req.get_tag() << ":" << req.get_request_string() << "]\n";
  if (req.get_arg("cmd")=="tellmenow")
  {
    tellmenow_q.put_work(req);
  }else if (req.get_arg("cmd")=="projectidea"){
    projectidea_q.put_work(req);
  }else{
    wq.put_work(req);
  }
}
