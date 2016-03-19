#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>

#include "server/messages.h"
#include "server/master.h"

#include <map>
#include <list>


#ifndef NUM_THREADS
#define NUM_THREADS    23
#endif

#ifndef MAX_REQUESTS
#define MAX_REQUESTS 30
#endif
struct worker_state{
  
  bool worker_ready;
  int requests_processing;



};

static struct Master_state {

  // The mstate struct collects all the master node state into one
  // place.  You do not need to preserve any of the fields below, they
  // exist only to implement the basic functionality of the starter
  // code.

  bool server_ready;
  int max_num_workers;
  int num_pending_client_requests;
  int next_tag;

  Worker_handle my_worker[4];
  // std::map<int, Worker_handle> my_workers;
  std::map<int, int> compareprimes_newtag_origtag;
  std::map<int, int> compareprimes_origtag_numresposnses;
  std::map<int, int> compareprimes_newtag_responses;


  std::list<Worker_handle> my_workers;
  std::map<int, Client_handle> waiting_clients;
  std::map<std::string, Response_msg> cached_responses;
  std::map<int, Request_msg> cached_requests;
  std::map<Worker_handle, worker_state> worker_states;
  int num_workers_active;
  int idle_threads;

} mstate;

// Generate a valid 'countprimes' request dictionary from integer 'n'
static void create_computeprimes_req(Request_msg& req, int n) {
  std::ostringstream oss;
  oss << n;
  req.set_arg("cmd", "countprimes");
  req.set_arg("n", oss.str());
}
Worker_handle* get_best_worker_handle(int tag, std::string client_req){

  return &mstate.my_worker[tag%mstate.num_workers_active];
}
// Implements logic required by compareprimes command via multiple
// calls to execute_work.  This function fills in the appropriate
// response.
static void execute_compareprimes(const Request_msg& req) {

    int params[4];
    // Response_msg resp(0);
    // grab the four arguments defining the two ranges
    params[0] = atoi(req.get_arg("n1").c_str());
    params[1] = atoi(req.get_arg("n2").c_str());
    params[2] = atoi(req.get_arg("n3").c_str());
    params[3] = atoi(req.get_arg("n4").c_str());
    // if (params[0]<=params[2]&& params[1]>= params[3])
    //   resp.set_response("There are more primes in first range.");
    // else if(params[2]<=params[0]&& params[3]>= params[1])
    //   resp.set_response("There are more primes in second range.");
    // else{
      int origtag = mstate.next_tag;
      mstate.compareprimes_origtag_numresposnses[origtag] =  0;
      
      for (int i=0; i<4; i++) {
        int tag_req = origtag+i+1;
        Request_msg dummy_req(tag_req);
        
        create_computeprimes_req(dummy_req, params[i]);
        // if (mstate.cached_responses.count(dummy_req.get_request_string()))
        // {
        //   dummy_resp.set_response(mstate.cached_responses[dummy_req.get_request_string()].get_response());
        //   counts[i] = atoi(dummy_resp.get_response().c_str());
        // }else{
          
          mstate.compareprimes_newtag_origtag[tag_req] = origtag;
          Worker_handle* best_worker_handle = get_best_worker_handle(tag_req, dummy_req.get_request_string());
          if (best_worker_handle!=NULL)
          {
            mstate.worker_states[*best_worker_handle].requests_processing++;
            mstate.idle_threads--;
            DLOG(INFO) << "Master routed request: [" << origtag << ":" <<dummy_req.get_tag() << ":" <<dummy_req.get_request_string()<< "]" << std::endl;
            DLOG(INFO) << "Routed request: " << dummy_req.get_request_string() << std::endl;
            send_request_to_worker(*best_worker_handle, dummy_req);
            // send_request_to_worker(mstate.my_worker[tag%mstate.max_num_workers], worker_req);

          }

        // }

      }

      
    // }
}



void master_node_init(int max_workers, int& tick_period) {

  // set up tick handler to fire every 5 seconds. (feel free to
  // configure as you please)
  tick_period = 5;

  mstate.next_tag = 0;
  mstate.max_num_workers = max_workers;
  mstate.num_pending_client_requests = 0;
  mstate.num_workers_active =0;
  // don't mark the server as ready until the server is ready to go.
  // This is actually when the first worker is up and running, not
  // when 'master_node_init' returnes
  mstate.server_ready = false;

  // fire off a request for a new worker
  DLOG(INFO) << "Master starting up [" << max_workers << "]" << std::endl;

  
  for (int i = 0; i < max_workers; ++i)
  {
    int tag = random();
    Request_msg req(tag);
    char buffer [50];
    sprintf (buffer, "name %d", i);
    req.set_arg("name", buffer);
    request_new_worker_node(req);
  }
  

}

void handle_new_worker_online(Worker_handle worker_handle, int tag) {

  // 'tag' allows you to identify which worker request this response
  // corresponds to.  Since the starter code only sends off one new
  // worker request, we don't use it here.
  mstate.my_worker[mstate.num_workers_active++] = worker_handle;
  mstate.idle_threads+=NUM_THREADS;
  // Now that a worker is booted, let the system know the server is
  // ready to begin handling client requests.  The test harness will
  // now start its timers and start hitting your server with requests.
  if (mstate.num_workers_active==mstate.max_num_workers && mstate.server_ready == false) {
    server_init_complete();
    mstate.server_ready = true;
  }

}

void handle_worker_response(Worker_handle worker_handle, const Response_msg& resp) {

  // Master node has received a response from one of its workers.
  // Here we directly return this response to the client.

  DLOG(INFO) << "Master received a response from a worker: [" << resp.get_tag() << ":" << resp.get_response() << "]" << std::endl;
  mstate.worker_states[worker_handle].requests_processing--;
  mstate.num_pending_client_requests--;
  if (mstate.compareprimes_newtag_origtag.count(resp.get_tag()))
  {
    int origtag = mstate.compareprimes_newtag_origtag[resp.get_tag()];
    mstate.compareprimes_origtag_numresposnses[origtag]++;
    mstate.compareprimes_newtag_responses[resp.get_tag()] = atoi(resp.get_response().c_str());
    if (mstate.compareprimes_origtag_numresposnses[origtag]==4)
    {
      int counts[4];
      for (int i = 0; i <4 ; ++i)
      {
        counts[i]=mstate.compareprimes_newtag_responses[origtag+i+1];
        DLOG(INFO) << "counts [" << i << ":" << counts[i] << "]" << std::endl;
      }



      Response_msg new_resp(0);
      if (counts[1]-counts[0] > counts[3]-counts[2])
        new_resp.set_response("There are more primes in first range.");
      else
        new_resp.set_response("There are more primes in second range.");
      send_client_response(mstate.waiting_clients[origtag], new_resp);
    }

    return;
  }
  send_client_response(mstate.waiting_clients[resp.get_tag()], resp);
  mstate.cached_responses[mstate.cached_requests[resp.get_tag()].get_request_string()] =resp;
  
}

void handle_client_request(Client_handle client_handle, const Request_msg& client_req) {

  DLOG(INFO) << "Received request: " << client_req.get_request_string() << std::endl;


  
  
  if (client_req.get_arg("cmd") == "compareprimes")
  {
    mstate.waiting_clients[mstate.next_tag] = client_handle;
    
    execute_compareprimes(client_req);
    mstate.next_tag+=5;

    return;
  }
  
  // You can assume that traces end with this special message.  It
  // exists because it might be useful for debugging to dump
  // information about the entire run here: statistics, etc.
  if (client_req.get_arg("cmd") == "lastrequest") {
    Response_msg resp(0);
    resp.set_response("ack");
    send_client_response(client_handle, resp);
    return;
  }

  // The provided starter code cannot handle multiple pending client
  // requests.  The server returns an error message, and the checker
  // will mark the response as "incorrect"
  // if (mstate.num_pending_client_requests > 5000) {
  //   Response_msg resp(0);
  //   resp.set_response("Oh no! This server cannot handle multiple outstanding requests!");
  //   send_client_response(client_handle, resp);
  //   return;
  // }

  // Save off the handle to the client that is expecting a response.
  // The master needs to do this it can response to this client later
  // when 'handle_worker_response' is called.
  //mstate.waiting_client[tag] = client_handle;
  

  // Fire off the request to the worker.  Eventually the worker will
  // respond, and your 'handle_worker_response' event handler will be
  // called to forward the worker's response back to the server.
  

  if (mstate.cached_responses.count(client_req.get_request_string()))
  {
    Response_msg resp(0);
    resp.set_response(mstate.cached_responses[client_req.get_request_string()].get_response());
    send_client_response(client_handle, resp);

  } else {
    int tag = mstate.next_tag++;
    mstate.waiting_clients[tag] = client_handle;
    mstate.num_pending_client_requests++;
    mstate.cached_requests[tag] = client_req;
    Request_msg worker_req(tag, client_req);
    Worker_handle* best_worker_handle = get_best_worker_handle(tag, client_req.get_request_string());
    if (best_worker_handle!=NULL)
    {
      mstate.worker_states[*best_worker_handle].requests_processing++;
      mstate.idle_threads--;
      DLOG(INFO) << "Routed request: " << client_req.get_request_string() << std::endl;
      send_request_to_worker(*best_worker_handle, worker_req);
      // send_request_to_worker(mstate.my_worker[tag%mstate.max_num_workers], worker_req);

    }
    
    // We're done!  This event handler now returns, and the master
    // process calls another one of your handlers when action is
    // required.
  }
}


void handle_tick() {

  // TODO: you may wish to take action here.  This method is called at
  // fixed time intervals, according to how you set 'tick_period' in
  // 'master_node_init'.
  // for(auto const &ent1 : mstate.) {
  //   // ent1.first is the first key
  //   for(auto const &ent2 : ent1.second) {
  //     // ent2.first is the second key
  //     // ent2.second is the data
  //   }
  // }
}

