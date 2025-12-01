/**
 * @file interrupts_101116888_101276841_EP_RR.cpp
 * @author Rounak Mukherjee (101116888), Timur Grigoryev (101276841)
 * @brief External Priorities + Round Robin Scheduler for Assignment 3 Part 1
 * 
 * This scheduler implements a combination of:
 * - External Priorities (based on size) WITH preemption
 * - Round Robin (100ms quantum) within same priority level
 * 
 * Features:
 * - Higher priority process can preempt lower priority
 * - Processes of equal priority share CPU using RR
 * - Most complex and realistic scheduler
 */

#include "interrupts_101116888_101276841.hpp"

// Time quantum for Round Robin (in milliseconds)
const unsigned int TIME_QUANTUM = 100;

/**
 * External Priorities + RR scheduling function
 * Sorts ready queue by priority first, then by arrival time for tie-breaking
 */
void ep_rr_schedule(std::vector<PCB> &ready_queue) {
    std::sort( 
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second) {
            // Primary: sort by priority (size)
            if(first.priority != second.priority) {
                return (first.priority > second.priority); // Smaller priority = higher (goes to back)
            }
            // Secondary: within same priority, use FCFS
            return (first.arrival_time > second.arrival_time);
        } 
    );
}

/**
 * Check if preemption should occur
 * Returns true if there's a higher priority process in ready queue
 */
bool should_preempt(PCB &running, std::vector<PCB> &ready_queue) {
    if(ready_queue.empty() || running.state != RUNNING) {
        return false;
    }
    
    // Find highest priority in ready queue
    unsigned int highest_ready_priority = ready_queue.back().priority;
    
    // Preempt if ready process has higher priority (lower number)
    return (highest_ready_priority < running.priority);
}

/**
 * Main simulation function for EP + RR scheduler
 * Returns tuple of execution status string (and memory status for bonus)
 */
std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   // Processes ready to run
    std::vector<PCB> wait_queue;    // Processes waiting for I/O completion
    std::vector<PCB> job_list;      // All processes for tracking

    unsigned int current_time = 0;
    unsigned int quantum_remaining = TIME_QUANTUM; // Time left in current quantum
    PCB running;

    // Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    // Create output table header
    execution_status = print_exec_header();

    // Main simulation loop - continues until all processes terminate
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //============================================================================
        // STEP 1: POPULATE READY QUEUE - New processes arriving at current time
        //============================================================================
        bool new_arrival = false;
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {
                // Try to assign memory partition
                bool memory_assigned = assign_memory(process);
                
                if(memory_assigned) {
                    // Memory available - add to ready queue
                    process.state = READY;
                    process.time_in_cpu = 0; // Initialize CPU time counter
                    ready_queue.push_back(process);
                    job_list.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                    new_arrival = true;
                }
            }
        }

        //============================================================================
        // STEP 2: MANAGE WAIT QUEUE - Check for I/O completion
        //============================================================================
        std::vector<PCB> still_waiting;
        for(auto &process : wait_queue) {
            // Each process tracks its own wait start time in time_in_cpu temporarily
            unsigned int wait_time = current_time - process.time_in_cpu;
            
            if(wait_time >= process.io_duration) {
                // I/O complete - move back to ready queue
                process.state = READY;
                process.time_in_cpu = 0; // Reset for CPU time tracking
                ready_queue.push_back(process);
                sync_queue(job_list, process);
                execution_status += print_exec_status(current_time, process.PID, WAITING, READY);
            } else {
                // Still waiting for I/O
                still_waiting.push_back(process);
            }
        }
        wait_queue = still_waiting;

        //============================================================================
        // STEP 2.5: CHECK FOR PREEMPTION (if new arrival or I/O completion)
        //============================================================================
        if(new_arrival || still_waiting.size() != wait_queue.size()) {
            // Sort ready queue to find highest priority
            ep_rr_schedule(ready_queue);
            
            // Check if we should preempt current running process
            if(should_preempt(running, ready_queue)) {
                // Preempt current process
                running.state = READY;
                running.time_in_cpu = 0; // Reset for next burst
                ready_queue.insert(ready_queue.begin(), running);
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                
                // CPU becomes idle, reset quantum
                idle_CPU(running);
                quantum_remaining = TIME_QUANTUM;
            }
        }

        //============================================================================
        // STEP 3: HANDLE RUNNING PROCESS
        //============================================================================
        if(running.state == RUNNING) {
            // Process is currently running - execute for 1ms
            running.remaining_time--;
            running.time_in_cpu++; // Track time in this CPU burst
            quantum_remaining--;
            
            // Check if process needs I/O
            if(running.io_freq > 0 && running.remaining_time > 0) {
                unsigned int total_cpu_time = running.processing_time - running.remaining_time;
                
                if(total_cpu_time > 0 && total_cpu_time % running.io_freq == 0) {
                    // Time for I/O - move to wait queue
                    running.state = WAITING;
                    running.time_in_cpu = current_time; // Store I/O start time
                    wait_queue.push_back(running);
                    sync_queue(job_list, running);
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                    
                    // CPU becomes idle, reset quantum
                    idle_CPU(running);
                    quantum_remaining = TIME_QUANTUM;
                }
            }
            
            // Check if process completed
            if(running.remaining_time == 0 && running.state == RUNNING) {
                // Process finished
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                idle_CPU(running);
                quantum_remaining = TIME_QUANTUM;
            }
            
            // Check if quantum expired (and process still running, same priority processes exist)
            if(quantum_remaining == 0 && running.state == RUNNING && running.remaining_time > 0) {
                // Check if there are other processes with same priority
                bool same_priority_exists = false;
                for(const auto &proc : ready_queue) {
                    if(proc.priority == running.priority) {
                        same_priority_exists = true;
                        break;
                    }
                }
                
                if(same_priority_exists) {
                    // Time quantum expired with same-priority processes waiting
                    // Preempt and move to back of ready queue
                    running.state = READY;
                    running.time_in_cpu = 0; // Reset for next burst
                    ready_queue.insert(ready_queue.begin(), running);
                    sync_queue(job_list, running);
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                    
                    // CPU becomes idle, reset quantum
                    idle_CPU(running);
                    quantum_remaining = TIME_QUANTUM;
                } else {
                    // No same-priority processes, just reset quantum and continue
                    quantum_remaining = TIME_QUANTUM;
                }
            }
        }

        //============================================================================
        // STEP 4: SCHEDULE NEW PROCESS (if CPU is idle)
        //============================================================================
        if(running.state == NOT_ASSIGNED && !ready_queue.empty()) {
            // CPU is idle and we have processes ready - schedule one
            ep_rr_schedule(ready_queue); // Sort by priority, then FCFS
            
            // Get highest priority process (at back of vector after sorting)
            running = ready_queue.back();
            ready_queue.pop_back();
            
            // Set start time if first time running
            if(running.start_time == -1) {
                running.start_time = current_time;
            }
            
            running.state = RUNNING;
            quantum_remaining = TIME_QUANTUM; // Reset quantum for new process
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        }

        //============================================================================
        // ADVANCE TIME
        //============================================================================
        current_time++;
        
        // Safety check - prevent infinite loop
        if(current_time > 100000) {
            std::cerr << "Simulation timeout at 100000ms" << std::endl;
            break;
        }
    }
    
    // Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    // Validate command line arguments
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts_EP_RR <your_input_file.txt>" << std::endl;
        return -1;
    }

    // Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    // Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    // Parse the entire input file and populate a vector of PCBs
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    std::cout << "External Priorities + Round Robin Scheduler (100ms quantum, with preemption)" << std::endl;
    std::cout << "Students: Rounak Mukherjee (101116888), Timur Grigoryev (101276841)" << std::endl;
    std::cout << "Processing " << list_process.size() << " processes..." << std::endl;

    // Run the simulation
    auto [exec] = run_simulation(list_process);

    // Write output to file
    write_output(exec, "execution_EP_RR.txt");

    return 0;
}
