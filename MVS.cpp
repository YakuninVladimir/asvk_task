#include <bits/stdc++.h>
#include <pthread.h>
#include "rapidxml.hpp"

using namespace std;
using namespace rapidxml;


struct program_connection{
    uint32_t first;
    uint32_t second;
    int32_t net_load;
};

struct success_ans{
    uint32_t num_of_iterations;
    vector<int32_t>* X;
    uint32_t min_max_load;
};

struct failure_ans{
    uint32_t num_of_iterations;
};

struct answer{
    bool is_succeded;
    union{
        success_ans success_body;
        failure_ans failure_body;
    };
};

struct system_data{
    uint32_t MAX_LOAD_OF_PROC;
    uint32_t MAX_NUM_OF_TRIES;
    uint32_t programs;
    uint32_t processors;
    uint32_t net_pairs;
    uint32_t border_net_load;
    uint32_t num_of_threads;
    uint32_t cur_thread;
    vector<uint32_t>* loads;
    vector<program_connection>* connected_prorgrams;
};

system_data & parse_xml(const string& filename ){
    xml_document<> doc;
    ifstream input_file(filename);
    string buffer((istreambuf_iterator<char>(input_file)), istreambuf_iterator<char>());
    doc.parse<0>(buffer.data());
    system_data* ans = new system_data{};
    xml_node<>* root = doc.first_node("body");
    xml_node<>* props = root->first_node("properties");
    xml_node<>* proc_loads = root->first_node("loads");
    xml_node<>* net_loads = root->first_node("netLoads");
    ans->programs = atoi(props->first_node("programms")->value());
    ans->processors = atoi(props->first_node("processors")->value());
    ans->net_pairs = atoi(props->first_node("pairs")->value());
    ans->border_net_load = atoi(props->first_node("borderLoad")->value());
    ans->num_of_threads = atoi(props->first_node("numOfThreads")->value());
    vector <uint32_t>* loads = new vector<uint32_t>(ans->programs);
    vector <program_connection>* connected_prorgrams = new vector<program_connection>(ans->net_pairs);
    xml_node<>* cur_load = proc_loads->first_node("load");
    (*loads)[0] = atoi(cur_load->value());
    for (int i = 1; i < ans->programs; i++){
        cur_load = cur_load->next_sibling();
        (*loads)[i] = atoi(cur_load->value());
    }
    xml_node<>* cur_pair = net_loads->first_node("pair");
    program_connection cur_connection{};
    cur_connection.first = atoi(cur_pair->first_node("first")->value());
    cur_connection.second = atoi(cur_pair->first_node("second")->value());
    cur_connection.net_load = atoi(cur_pair->first_node("netLoad")->value());
    (*connected_prorgrams)[0] = cur_connection;
    for (int i = 1; i < ans->net_pairs; i++){
        cur_pair = cur_pair->next_sibling();
        cur_connection.first = atoi(cur_pair->first_node("first")->value());
        cur_connection.second = atoi(cur_pair->first_node("second")->value());
        cur_connection.net_load = atoi(cur_pair->first_node("netLoad")->value());
        (*connected_prorgrams)[i] = cur_connection;
    }
    ans->loads = loads;
    ans->connected_prorgrams = connected_prorgrams;
    return *ans;
}

mutex block;
class Load_manager {
    bool update_on_iteration;

    uint32_t MAX_LOAD_OF_PROC;
    uint32_t MAX_NUM_OF_TRIES;

    uint32_t num_of_processors;
    uint32_t num_of_programs;
    uint32_t num_of_threads;
    
    uint32_t border_net_load;
    vector<int32_t> checking_solution{};
    vector<uint32_t> programm_loads{};

    vector<int32_t> min_solution;
    uint32_t min_max_load;
    uint32_t num_of_iterations;

    vector<program_connection> connected_programs;

    uint32_t find_max_load(){
        vector<uint32_t> processor_loads(num_of_processors, 0);
        for (uint32_t i = 0; i < num_of_programs; i++){
            processor_loads[checking_solution[i]]  += programm_loads[i];
        }
        uint32_t max_load{};
        for (uint32_t & i : processor_loads){
            max_load = max(i, max_load);
        }
        return max_load;
    }

    bool check_if_net_overloaded(){
        uint32_t net_load{};
        for (auto & i : connected_programs){
            if (checking_solution[i.first] != checking_solution[i.second]){
                net_load += i.net_load;
            }
        }
        return net_load  <= border_net_load;
    }

    void generate_random_vector(){
        vector<int32_t>res(num_of_programs);
        for (auto & i : res){
            i = rand() % num_of_processors;
        }
        checking_solution = res;
    }

    void alg_iteration(){
        generate_random_vector();
        if (check_if_net_overloaded()){
            uint32_t cur_load = find_max_load();
            if (cur_load >= min_max_load){
                return;
            }
            min_max_load = cur_load;
            min_solution = checking_solution;
            update_on_iteration = true;
        }
    }

    bool iterate(uint32_t num_of_tries){
        while(num_of_tries--){
            alg_iteration();
            if (update_on_iteration){
                update_on_iteration = false;
                num_of_iterations++;
                return true;
            }
        }
        return false;
    }

public:
    Load_manager(system_data & input_data){
        
        num_of_programs = input_data.programs;
        num_of_processors = input_data.processors;
        border_net_load = input_data.border_net_load;
        num_of_threads = input_data.num_of_threads;
        connected_programs = *input_data.connected_prorgrams;
        programm_loads = *input_data.loads;
        num_of_iterations = 0;
        update_on_iteration = false;
        min_max_load = input_data.MAX_LOAD_OF_PROC;
        min_solution.assign(num_of_programs, -1);
    }

    answer* generate_solution(uint32_t max_load, uint32_t num_of_tries){
        answer * ans = new answer();
        MAX_LOAD_OF_PROC = max_load;
        MAX_NUM_OF_TRIES = num_of_tries;
        while (iterate(MAX_NUM_OF_TRIES));
        block.lock();
        if (min_max_load == MAX_LOAD_OF_PROC){
            ans->is_succeded = false;
            ans->failure_body.num_of_iterations = num_of_iterations;
        }
        else{
            ans->is_succeded = true;
            ans->success_body.num_of_iterations = num_of_iterations;
            ans->success_body.min_max_load = min_max_load;
            ans->success_body.X =  new vector<int32_t> (min_solution);
        }
        block.unlock();
        return ans;
    }
};

vector<answer*> min_values;

void handle_thread(system_data data){
    srand(time(0));
    answer * ans = Load_manager(data).generate_solution(data.MAX_LOAD_OF_PROC, data.MAX_NUM_OF_TRIES);
    min_values[data.cur_thread] = ans;
}


// using namespace std::chrono;
template <class DT = std::chrono::milliseconds,
          class ClockT = std::chrono::steady_clock>
class Timer
{
    using timep_t = typename ClockT::time_point;
    timep_t _start = ClockT::now(), _end = {};

public:
    void tick() { 
        _end = timep_t{}; 
        _start = ClockT::now(); 
    }
    
    void tock() { _end = ClockT::now(); }
    
    template <class T = DT> 
    auto duration() const { 
        return std::chrono::duration_cast<T>(_end - _start); 
    }
};

int32_t main(int argc, char** argv){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);
    srand(time(0));
    
    string filename = "";

    for (int i = 1; i < argc; i++){
        if (!strcmp(argv[i], "-f")){
            filename = argv[i + 1];
        }
    }

    system_data input_data = parse_xml(filename);
    Timer clock;

    clock.tick();
    input_data.MAX_NUM_OF_TRIES = 5000 / input_data.num_of_threads;
    int num_of_iterations{};

    min_values.resize(input_data.num_of_threads);
    uint32_t pref_min_load = 100;
    vector <int32_t> pref_solution(input_data.programs, -1);
    bool update = true;
    while(update){
        update = false;
        num_of_iterations++;
        vector<thread> threads (input_data.num_of_threads);
        for (int thread_num = 0; thread_num < input_data.num_of_threads; thread_num++){
            input_data.MAX_LOAD_OF_PROC = pref_min_load;
            input_data.cur_thread = thread_num;
            threads[thread_num] = thread(&handle_thread, input_data);
        }

        for (int i = 0; i < input_data.num_of_threads; i++){
            threads[i].join();
        }

        for (auto & i : min_values){
            if (i->is_succeded && i->success_body.min_max_load < pref_min_load){
                pref_min_load = i->success_body.min_max_load;
                pref_solution = *(i->success_body.X);
                update = true;
            }
        }
    }
    clock.tock();
    cout << "Run time = " << clock.duration().count() << " ms" << endl;
    
    if (pref_min_load == 100){
        cout << "failure" << endl;
        cout << num_of_iterations << endl;
    }
    else{
        cout << "success" << endl;
        cout << num_of_iterations << endl;
        for (auto & i : pref_solution){
            cout << i << ' ';
        }
        cout << endl;
        cout << pref_min_load << endl;
    }
}

