/**
 * @file interrupts_101116888_101276841_EP.cpp
 * @author Rounak Mukherjee (101116888), Timur Grigoryev (101276841)
 * @brief External Priorities Scheduler (No Preemption) for Assignment 3 Part 1
 * 
 * This scheduler implements External Priorities scheduling where:
 * - Priority is based on process size (smaller size = higher priority)
 * - No preemption: once a process starts, it runs until completion or I/O
 * - Processes are scheduled in priority order from the ready queue
 */

#include "interrupts_101116888_101276841.hpp"

/**
 * External Priorities scheduling function
 * Sorts ready queue by priority (size), with smallest size having highest priority
 */
void external_priorities(std::vector<PCB> &ready_queue) {
    std::sort( 
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second) {
            // If priorities are equal, use FCFS (arrival time)
            if(first.priority == second.priority) {
                return (first.arrival_time > second.arrival_time);
            }
            // Otherwise, smaller priority value = higher priority (goes to back of vector)
            return (first.priority > second.priority); 
        } 
    );
}

/**
 * Main simulation function for External Priorities scheduler
 * Returns tuple of execution status string (and memory status for bonus)
 */
std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   // Processes ready to run
    std::vector<PCB> wait_queue;    // Processes waiting for I/O completion
    std::vector<PCB> job_list;      // All processes for tracking

    unsigned int current_time = 0;
    unsigned int io_start_time = 0; // Track when I/O started
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
                    ready_queue.push_back(process);
                    job_list.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                } else {
                    // No memory available - process must wait
                    // In real system, would stay in NEW state until memory available
                    // For this simulation, we'll keep checking each cycle
                }
            }
        }

        //============================================================================
        // STEP 2: MANAGE WAIT QUEUE - Check for I/O completion
        //============================================================================
        std::vector<PCB> still_waiting;
        for(auto &process : wait_queue) {
            // Calculate how long process has been waiting
            unsigned int wait_time = current_time - io_start_time;
            
            if(wait_time >= process.io_duration) {
                // I/O complete - move back to ready queue
                process.state = READY;
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
            
            // Check if process needs I/O
            if(running.io_freq > 0 && running.remaining_time > 0) {
                // Calculate if I/O should trigger
                unsigned int time_since_start = running.processing_time - running.remaining_time;
                
                if(time_since_start > 0 && time_since_start % running.io_freq == 0) {
                    // Time for I/O - move to wait queue
                    running.state = WAITING;
                    io_start_time = current_time;
                    wait_queue.push_back(running);
                    sync_queue(job_list, running);
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                    
                    // CPU becomes idle
                    idle_CPU(running);
                }
            }
            
            // Check if process completed
            if(running.remaining_time == 0) {
                // Process finished
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                idle_CPU(running);
            }
        }

        //============================================================================
        // STEP 4: SCHEDULE NEW PROCESS (if CPU is idle)
        //============================================================================
        if(running.state == NOT_ASSIGNED && !ready_queue.empty()) {
            // CPU is idle and we have processes ready - schedule one
            external_priorities(ready_queue); // Sort by priority
            
            // Get highest priority process (at back of vector after sorting)
            running = ready_queue.back();
            ready_queue.pop_back();
            
            // Set start time if first time running
            if(running.start_time == -1) {
                running.start_time = current_time;
            }
            
            running.state = RUNNING;
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        }

        //============================================================================
        // ADVANCE TIME
        //============================================================================
        current_time++;
        
        // Safety check - prevent infinite loop (adjust as needed for your test cases)
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
        std::cout << "To run the program, do: ./interrupts_EP <your_input_file.txt>" << std::endl;
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

    std::cout << "External Priorities Scheduler (No Preemption)" << std::endl;
    std::cout << "Students: Rounak Mukherjee (101116888), Timur Grigoryev (101276841)" << std::endl;
    std::cout << "Processing " << list_process.size() << " processes..." << std::endl;

    // Run the simulation
    auto [exec] = run_simulation(list_process);

    // Write output to file
    write_output(exec, "execution_EP.txt");

    return 0;
}
