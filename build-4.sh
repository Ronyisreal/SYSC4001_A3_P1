#!/bin/bash
# Build script for SYSC4001 Assignment 3 Part 1
# Students: Rounak Mukherjee (101116888), Timur Grigoryev (101276841)

echo "Building schedulers..."

# Create bin directory if it doesn't exist
if [ ! -d "bin" ]; then
    mkdir bin
else
    rm -f bin/*
fi

# Compile External Priorities scheduler
echo "Compiling External Priorities (EP)..."
g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101116888_101276841_EP.cpp
if [ $? -eq 0 ]; then
    echo "✓ EP compiled successfully"
else
    echo "✗ EP compilation failed"
    exit 1
fi

# Compile Round Robin scheduler
echo "Compiling Round Robin (RR)..."
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101116888_101276841_RR.cpp
if [ $? -eq 0 ]; then
    echo "✓ RR compiled successfully"
else
    echo "✗ RR compilation failed"
    exit 1
fi

# Compile External Priorities + Round Robin scheduler
echo "Compiling EP + RR (EP_RR)..."
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101116888_101276841_EP_RR.cpp
if [ $? -eq 0 ]; then
    echo "✓ EP_RR compiled successfully"
else
    echo "✗ EP_RR compilation failed"
    exit 1
fi

echo ""
echo "All schedulers compiled successfully!"
echo "Run with: ./bin/interrupts_EP <input_file>"
echo "          ./bin/interrupts_RR <input_file>"
echo "          ./bin/interrupts_EP_RR <input_file>"
