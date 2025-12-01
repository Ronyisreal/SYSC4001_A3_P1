# SYSC4001 Assignment 3 Part 1 - Scheduler Simulator

## Student Information
- **Students**: Rounak Mukherjee (101116888), Timur Grigoryev (101276841)
- **Course**: SYSC4001 - Operating Systems
- **Assignment**: Assignment 3 Part 1 - Scheduler Simulator

---

## Overview

This project implements three CPU scheduling algorithms:
1. **External Priorities (EP)** - No preemption
2. **Round Robin (RR)** - 100ms time quantum
3. **External Priorities + Round Robin (EP_RR)** - With preemption

Each scheduler simulates process execution with:
- Memory management (6 fixed partitions: 40MB, 25MB, 15MB, 10MB, 8MB, 2MB)
- I/O operations and wait queues
- Process state transitions (NEW → READY → RUNNING → WAITING → TERMINATED)

---

## Files Included

### Source Files:
- `interrupts_101116888_101276841_EP.cpp` - External Priorities scheduler
- `interrupts_101116888_101276841_RR.cpp` - Round Robin scheduler (PENDING)
- `interrupts_101116888_101276841_EP_RR.cpp` - Combined scheduler (PENDING)
- `interrupts_101116888_101276841.hpp` - Header file with data structures

### Build Files:
- `build.sh` - Compilation script for all schedulers
- `input_test1.txt` - Sample input file for testing

### Output:
- `execution_EP.txt` - Output from EP scheduler
- `execution_RR.txt` - Output from RR scheduler
- `execution_EP_RR.txt` - Output from EP_RR scheduler

---

## Compilation

### Using Build Script (Recommended):
```bash
chmod +x build.sh
./build.sh
```

### Manual Compilation:
```bash
# Create bin directory
mkdir -p bin

# Compile each scheduler
g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101116888_101276841_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101116888_101276841_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101116888_101276841_EP_RR.cpp
```

---

## Running the Simulators

### External Priorities (EP):
```bash
./bin/interrupts_EP input_test1.txt
```

### Round Robin (RR):
```bash
./bin/interrupts_RR input_test1.txt
```

### EP + RR Combined:
```bash
./bin/interrupts_EP_RR input_test1.txt
```

---

## Input File Format

Each line represents a process with the following format:
```
PID, Memory_Size, Arrival_Time, CPU_Time, IO_Frequency, IO_Duration
```

**Example:**
```
1, 40, 0, 500, 100, 50
2, 25, 50, 300, 75, 40
3, 15, 100, 200, 50, 30
```

**Field Descriptions:**
- **PID**: Process ID (unique identifier)
- **Memory_Size**: Amount of memory required (in MB)
- **Arrival_Time**: Time when process arrives (in ms)
- **CPU_Time**: Total CPU time needed (in ms)
- **IO_Frequency**: How often I/O occurs (every N ms of CPU time)
- **IO_Duration**: How long each I/O operation takes (in ms)

---

## Scheduler Descriptions

### 1. External Priorities (EP) - [IMPLEMENTED]

**Algorithm:**
- Processes are prioritized by size (smaller size = higher priority)
- **No preemption**: Once a process starts, it runs until completion or I/O
- Ready queue is sorted by priority before each scheduling decision

**Key Features:**
- Priority = process size (smaller processes run first)
- Processes with equal priority use FCFS ordering
- I/O operations move process to wait queue
- No interruption from higher priority arrivals

**State Transitions:**
```
NEW → READY (on arrival if memory available)
READY → RUNNING (when CPU is idle, highest priority selected)
RUNNING → WAITING (on I/O request)
WAITING → READY (when I/O completes)
RUNNING → TERMINATED (when completed)
```

**Code Structure:**
```cpp
void external_priorities(std::vector<PCB> &ready_queue) {
    // Sort by priority (size), with FCFS tiebreaker
    std::sort(ready_queue.begin(), ready_queue.end(), comparator);
}
```

**Example Execution:**
```
Process 1 (40MB) arrives at t=0, priority=40
Process 2 (25MB) arrives at t=50, priority=25
Process 3 (15MB) arrives at t=100, priority=15

Scheduling order: P3 (15), P2 (25), P1 (40)
```

---

### 2. Round Robin (RR) - [TO BE IMPLEMENTED NEXT]

**Algorithm:**
- Processes scheduled in FCFS order
- Each process runs for maximum 100ms time quantum
- After 100ms, process moves to back of ready queue
- Continues until process completes or needs I/O

**Key Features:**
- Time quantum = 100ms
- Fair CPU distribution
- Processes cycle through ready queue
- Good for time-sharing systems

---

### 3. External Priorities + Round Robin (EP_RR) - [TO BE IMPLEMENTED LAST]

**Algorithm:**
- Combines priority-based scheduling WITH preemption
- Higher priority process can preempt lower priority
- Within same priority level, use Round Robin (100ms)

**Key Features:**
- Preemption enabled
- Priority-based with fair time-sharing
- Most complex of the three schedulers

---

## Output Format

The execution output shows state transitions:

```
+-----------------------------------------------+
| Time of Transition | PID | Old State | New State |
+-----------------------------------------------+
|                  0 |   1 |       NEW |     READY |
|                  0 |   1 |     READY |   RUNNING |
|                100 |   1 |   RUNNING |   WAITING |
|                150 |   1 |   WAITING |     READY |
|                500 |   1 |   RUNNING | TERMINATED |
+-----------------------------------------------+
```

**Columns:**
- **Time of Transition**: Simulation time in milliseconds
- **PID**: Process ID
- **Old State**: State before transition
- **New State**: State after transition

---

## Memory Management

### Memory Partitions:
```
Partition 1: 40 MB
Partition 2: 25 MB
Partition 3: 15 MB
Partition 4: 10 MB
Partition 5:  8 MB
Partition 6:  2 MB
Total: 100 MB
```

### Allocation Strategy:
- Best-fit algorithm (smallest partition that fits)
- Process cannot start if no suitable partition available
- Partition freed when process terminates

---

## Testing

### Test Case 1: Basic Functionality
```bash
./bin/interrupts_EP input_test1.txt
```
**Expected**: Processes scheduled by size (smallest first)

### Test Case 2: Multiple I/O Operations
Create input file with high I/O frequency to test wait queue management

### Test Case 3: Memory Constraints
Create processes larger than available partitions to test blocking

---

## Key Implementation Details

### EP Scheduler Logic:

1. **Process Arrival**:
   - Check if memory available
   - Assign partition using best-fit
   - Add to ready queue
   - Record NEW → READY transition

2. **Scheduling Decision** (CPU idle):
   - Sort ready queue by priority
   - Select highest priority (smallest size)
   - Record READY → RUNNING transition
   - Set start time if first run

3. **Process Execution**:
   - Decrement remaining time each ms
   - Check for I/O at specified frequency
   - Check for completion

4. **I/O Handling**:
   - Move to wait queue
   - Record RUNNING → WAITING
   - Track I/O start time
   - Return to ready when I/O complete

5. **Completion**:
   - Free memory partition
   - Record RUNNING → TERMINATED
   - Mark process as terminated

---

## Data Structures

### PCB (Process Control Block):
```cpp
struct PCB {
    int PID;                    // Process ID
    unsigned int size;          // Memory requirement
    unsigned int arrival_time;  // When process arrives
    int start_time;             // When first scheduled (-1 if not started)
    unsigned int processing_time; // Total CPU time needed
    unsigned int remaining_time;  // Time left to execute
    int partition_number;       // Assigned memory partition
    enum states state;          // Current state
    unsigned int io_freq;       // I/O frequency
    unsigned int io_duration;   // I/O duration
    unsigned int priority;      // Priority value
    unsigned int time_in_cpu;   // Time spent in CPU (for RR)
};
```

### States:
```cpp
enum states {
    NEW,          // Just arrived
    READY,        // Ready to run
    RUNNING,      // Currently executing
    WAITING,      // Waiting for I/O
    TERMINATED,   // Completed
    NOT_ASSIGNED  // CPU idle state
};
```

---

## Troubleshooting

### Compilation Errors:
```bash
# Make sure you're in the correct directory
ls interrupts_101116888_101276841_EP.cpp

# Check g++ is installed
g++ --version

# Try manual compilation with verbose output
g++ -g -O0 -I . -v -o bin/interrupts_EP interrupts_101116888_101276841_EP.cpp
```

### Runtime Errors:
- **"Unable to open file"**: Check input file path
- **"Simulation timeout"**: Increase timeout in code or check for infinite loops
- **Segmentation fault**: Check array bounds and null pointers

### Output Issues:
- Check `execution_EP.txt` is created in current directory
- Verify input file format is correct
- Ensure all processes can fit in memory

---

## Next Steps

1.  **EP Scheduler** - COMPLETE
2.  **RR Scheduler** - TO IMPLEMENT
3.  **EP_RR Scheduler** - TO IMPLEMENT
4.  **Generate 20+ test scenarios**
5.  **Run simulations and collect metrics**
6.  **Write analysis report (2+ pages)**

---

## Grading Alignment

### Scheduler Implementation (1.5 marks):
- [x] EP: 0.5 marks - COMPLETE
- [ ] RR: 0.5 marks - PENDING
- [ ] EP_RR: 0.5 marks - PENDING

### Simulation Execution (1 mark):
- [ ] Run 20+ scenarios
- [ ] Compute metrics (throughput, wait time, turnaround, response)
- [ ] Write 2+ page report analyzing results

### Bonus (1 mark):
- [ ] Memory usage tracking
- [ ] Memory analysis in report

---

SYSC4001 - Operating Systems
Assignment 3 Part 1
