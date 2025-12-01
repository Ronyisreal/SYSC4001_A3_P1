/**
 * @file interrupts_101116888_101276841_RR.cpp
 * @author Rounak Mukherjee (101116888), Timur Grigoryev (101276841)
 * @brief Round Robin Scheduler with 100ms time quantum for Assignment 3 Part 1
 * 
 * This scheduler implements Round Robin scheduling where:
 * - Time quantum = 100ms
 * - Processes scheduled in FCFS order
 * - After 100ms, process moves to back of ready queue
 * - Fair CPU time distribution among all processes
 */

#include "interrupts_101116888_101276841.hpp"

// Time quantum for Round Robin (in milliseconds)
const unsigned int TIME_QUANTUM = 100;

/**
 * FCFS scheduling function for Round Robin
 * Sorts ready queue by arrival time (FCFS order)
 */
void fcfs_rr(std::vector<PCB> &ready_queue) {
    std::sort( 
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second) {
            return (first.arrival_time > second.arrival_time); 
        } 
    );
}

/**
 * Main simulation function for Round Robin scheduler
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
            
            // Check if quantum expired (and process still running)
            if(quantum_remaining == 0 && running.state == RUNNING && running.remaining_time > 0) {
                // Time quantum expired - preempt and move to back of ready queue
                running.state = READY;
                running.time_in_cpu = 0; // Reset for next burst
                ready_queue.insert(ready_queue.begin(), running); // Add to front (will go to back after sort)
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                
                // CPU becomes idle, reset quantum
                idle_CPU(running);
                quantum_remaining = TIME_QUANTUM;
            }
        }

        //============================================================================
        // STEP 4: SCHEDULE NEW PROCESS (if CPU is idle)
        //============================================================================
        if(running.state == NOT_ASSIGNED && !ready_queue.empty()) {
            // CPU is idle and we have processes ready - schedule one
            fcfs_rr(ready_queue); // Sort by arrival time (FCFS)
            
            // Get next process (at back of vector after sorting)
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
        std::cout << "To run the program, do: ./interrupts_RR <your_input_file.txt>" << std::endl;
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

    std::cout << "Round Robin Scheduler (100ms quantum)" << std::endl;
    std::cout << "Students: Rounak Mukherjee (101116888), Timur Grigoryev (101276841)" << std::endl;
    std::cout << "Processing " << list_process.size() << " processes..." << std::endl;

    // Run the simulation
    auto [exec] = run_simulation(list_process);

    // Write output to file
    write_output(exec, "execution_RR.txt");

    return 0;
}
