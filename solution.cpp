/*You are a software developer for a major e-commerce platform that handles millions of transactions per day. Your platform runs on a distributed system 
consisting of multiple servers, and you are responsible for designing a process scheduling algorithm that can handle these transactions efficiently.
  • The system consists of multiple servers, each running a different service. Each service handles a specific type of transaction. 
    For example, one service handles payment processing, another handles order processing, and so on.
  • Each service consists of a pool of worker threads, which are responsible for processing incoming requests. Each worker thread has a priority level 
    assigned to it, and a certain amount of resources assigned to it. The priority level determines the order in which the threads are scheduled to
    process requests. The resources assigned to each thread determine the maximum number of requests it can handle simultaneously.
  • Incoming requests are queued up and assigned to the appropriate service based on the type of transaction. Once a request is assigned to a service, 
    it is further queued up and assigned to a worker thread based on its priority level.
  • Your task is to design a process scheduling algorithm that can efficiently allocate resources to worker threads to process incoming requests. 
    The algorithm should take into account the following factors:
    ➢ The priority level of each worker thread
    ➢ The number of available resources assigned to each worker thread
    ➢ The type of transaction associated with each request
  • Your algorithm should prioritize worker threads with higher priority levels and assign them resources accordingly. If a worker thread with a higher 
    priority level is not available, the algorithm should assign resources to the next available worker thread with a lower priority level.
  • The algorithm should also take into account the type of transaction associated with each request.  Some types of transactions may require more 
    resources than others. For example, payment processing may require more resources than order processing.
  • Your program should read the input from the standard input and write the output to the standard output. The input will contain the following 
    information:
      ➢ The number of services n in the system
      ➢ The number of worker threads m for each service
      ➢ The priority level and resources assigned to each worker thread. Each worker thread should be on a separate line and its information should be 
        separated by spaces in the following format: priority_level resources 
      ➢ The type of transaction associated with each request, and the number of resources required for that transaction. Each request should be on a 
        separate line and its information should be separated by spaces in the following format: transaction_type resources_required 
  • Your program should output the following information:
      ➢ The order in which requests were processed
      ➢ The average waiting time for requests
      ➢ The average turnaround time for requests
      ➢ The number of requests that were rejected due to lack of resources
  • Additionally, your program should output the following information during periods of high traffic:
      ➢ The number of requests that were forced to wait due to lack of available resources 
      ➢ The number of requests that were blocked due to the absence of any available worker threads
-----------------------------------------------------------------------------------------------------------------------------------------------------------*/      
#include<bits/stdc++.h>
#include <thread>
using namespace std;

int numberOfServices, numberOfWorkerThreads, numberOfRequests;
int waiting = 0, blocked = 0;
vector<int> ExecutedRequest;
mutex msg;
mutex waiting_lock, blocked_lock;
#define threshold 2

struct workerThread{
        int tid;
        int priority_level;
        int resources;
        bool available = 1;
        pthread_mutex_t check_availability;
    };

struct Request {
    int request_id;
    int transaction_type;
    int resources_required;
    chrono::high_resolution_clock::time_point arrival_time;
    chrono::high_resolution_clock::time_point start_time;
    chrono::high_resolution_clock::time_point completion_time;
    chrono::milliseconds waiting_time;
    chrono::milliseconds turnaround_time;
    bool isCompleted = 0;
};

vector<Request*> requests;

class Service{
   public : 
    int transaction_type;
    int numberOfWorkerThreads;
    vector<workerThread*> wth;
    void setupServiceThreads(int t_type, int nWT){
        transaction_type = t_type;
        numberOfWorkerThreads = nWT;
        cout << "\n\n---------Enter priority level and resources assigned to each worker thread of [ Sever id " << transaction_type << " ] separated by spaces--------------\n\n";
        cout << "Worker Thread [id] : [priority_level]  [resources]\n";
            int priority_level, resources;
            for(int i = 0; i < numberOfWorkerThreads; i++){
                cout << "Worker Thread " << i << ": ";
                cin >> priority_level >> resources;
                workerThread *worker = new workerThread;
                worker->tid = i;
                worker->priority_level = priority_level;
                worker->resources = resources;
                wth.push_back(worker);
            }
        sort(wth.begin(),wth.end(),[](workerThread* a, workerThread* b){
            return a->priority_level < b->priority_level;
        });
    }
};

Service service[100];

void setup_services(){

    cout << "Enter number of services (n) : " ; cin >> numberOfServices;
    cout << "Enter number of worker threads for each service (m) : " ; cin >> numberOfWorkerThreads;

    for(int i = 0; i < numberOfServices; i++){
            service[i].setupServiceThreads(i,numberOfWorkerThreads);
    }
    
}

void* scheduler(void* id){
    int req_id = *((int*) id);
    Request* request = requests[req_id];
    int server_id = request->transaction_type;
    int resources_required = request->resources_required;
    bool executed = false;
    bool excecutable = false;

    for(int i = 0; i<service[server_id].numberOfWorkerThreads; i++){
        workerThread* curr_worker = service[server_id].wth[i];
        if(curr_worker->resources >= resources_required){
                excecutable = 1;
                break;
        }
    }
    if(!excecutable){
        msg.lock(); 
        cout << "\nRequest [" << req_id << "]  : [REJECTED]" << endl;
        msg.unlock();
        blocked_lock.lock();
        blocked++;
        blocked_lock.unlock();
        return NULL;
    }
    
    waiting_lock.lock();
    waiting++;
    if(waiting >= threshold){
        msg.lock(); cout << "\n" << "Waiting Queue Count : " << waiting <<" [High Traffic]" << endl; msg.unlock();
    }
    waiting_lock.unlock();

    while(!executed){
        for(int i = 0; i<service[server_id].numberOfWorkerThreads; i++){
            workerThread* curr_worker = service[server_id].wth[i];
            if(curr_worker->resources < resources_required) continue;

            pthread_mutex_lock(&curr_worker->check_availability);
            if(curr_worker->available){
                curr_worker->available = 0;
                pthread_mutex_unlock(&curr_worker->check_availability);
                //schedule
                request->start_time = chrono::high_resolution_clock::now();
                usleep(5000000);
                msg.lock();
                cout << "\nRequest [" << req_id << "] : [RUNNING] [Server " << server_id << "] [Thread " << i << "]"<< endl;
                msg.unlock();
                executed = 1;
                request->completion_time = chrono::high_resolution_clock::now();
                request->waiting_time = chrono::duration_cast<chrono::milliseconds>(request->start_time - request->arrival_time);
                request->turnaround_time = chrono::duration_cast<chrono::milliseconds>(request->completion_time - request->arrival_time);
                pthread_mutex_lock(&curr_worker->check_availability);
                curr_worker->available = 1;
                pthread_mutex_unlock(&curr_worker->check_availability);
                break;
            }
            else{
                pthread_mutex_unlock(&curr_worker->check_availability);
            }
            
        }

        if(executed){  
            msg.lock(); 
            cout << "\nRequest [" << req_id << "] : [SUCCESS]" << endl;
            ExecutedRequest.push_back(req_id);
            msg.unlock();
            request->isCompleted = 1;
            waiting_lock.lock();
            waiting--;
            waiting_lock.unlock();
        }
    }
    return NULL;
}

void input_requests(){
    cout << "\n\nEnter number of requests : "; cin >> numberOfRequests;
    cout << "\n\n--------------Enter the type of transaction(service) associated with each request and the number of resources required for that transaction(service) seperated by space-------------\n\n";
    cout << "Request [id] : [transaction_type]  [resources_required]\n";
    
    int transaction_type, resources_required;
    pthread_t requestThread[numberOfRequests];
    for(int i = 0; i < numberOfRequests; i++){
        msg.lock(); cout << "Request [" << i << "] : "; 
        cin >> transaction_type >> resources_required; msg.unlock();
        if (transaction_type < 0 || transaction_type >= numberOfServices) {
            msg.lock(); cout << "\nInvalid transaction type " << transaction_type << " Re-enter last request" << endl; msg.unlock();
            i--;
            continue;
        }
        Request *req = new Request;
        req->request_id = i;
        req->transaction_type = transaction_type;
        req->resources_required = resources_required;
        req->arrival_time = chrono::high_resolution_clock::now();
        requests.push_back(req);
        // cout<<"hii"<<endl;
    }

    for(int i = 0; i < numberOfRequests; i++){
        requests[i]->arrival_time = chrono::high_resolution_clock::now();
        pthread_create(&requestThread[i], NULL, scheduler, (void*) &requests[i]->request_id);

    }

    for (int i = 0; i < numberOfRequests; ++i) {
        pthread_join(requestThread[i], NULL);
    }

    cout << "\n\n";

    cout << left << setw(12) << "Server ID" << setw(12) << "WorkerID" << setw(12) << "Priority" << setw(12) << "Resources" << endl;
    
    for(int i = 0; i < numberOfServices ; i++) {
        for (int j = 0; j < numberOfWorkerThreads; j++) {
            cout << left << setw(12) << i << setw(12) << j << setw(12) << service[i].wth[j]->priority_level << setw(12) << service[i].wth[j]->resources << endl;
        }
    }
    cout << endl << endl;

    cout << left << setw(12) << "Request ID" << setw(12) << "Server ID" << setw(15) << "Resources_Req" << setw(15) << "ArrivalTime" << setw(15) << "StartTime" << setw(18) << "CompletionTime" << setw(15) << "WaitingTime" << setw(18) << "TurnaroundTime" << endl;
    
    for(int i = 0; i<numberOfRequests; i++){
        if(!requests[i]->isCompleted){
            cout << left << setw(12) << i << setw(12) << requests[i]->transaction_type << setw(15) << requests[i]->resources_required << setw(15) << "-" << setw(15) << "-" << setw(18) << "-" << setw(15) << "-" << setw(18) << "-" << setw(18) << ": REJECTED" << endl;
            continue;
        }
        cout << left << setw(12) << i << setw(12) << requests[i]->transaction_type << setw(15) << requests[i]->resources_required << setw(15) << chrono::duration_cast<chrono::milliseconds>(requests[i]->arrival_time.time_since_epoch()).count() << setw(15) << chrono::duration_cast<chrono::milliseconds>(requests[i]->start_time.time_since_epoch()).count() << setw(18) << chrono::duration_cast<chrono::milliseconds>(requests[i]->completion_time.time_since_epoch()).count() << setw(15) << requests[i]->waiting_time.count() << setw(18) << requests[i]->turnaround_time.count() << endl;
    }

    chrono::milliseconds average_waiting_time(0);
    chrono::milliseconds average_turnaround_time(0);
    int count = 0;
    for(Request *req: requests){
        if(req->isCompleted){
            average_waiting_time += req->waiting_time;
            average_turnaround_time += req->turnaround_time;
            count += 1;
        }
    }
    average_waiting_time /= count;
    average_turnaround_time /= count;
    cout << "\n\nSequence of request executed : ";
    for(auto req : ExecutedRequest) cout << req << " ";
    cout << "\nRequests rejected : " << blocked << endl;
    cout << "Average waiting time : " << average_waiting_time.count() << " ms" << endl;
    cout << "Average turnaround time :" << average_turnaround_time.count() << " ms" <<  endl;

}

int main(){
    setup_services();
    input_requests();
    return 0;
}