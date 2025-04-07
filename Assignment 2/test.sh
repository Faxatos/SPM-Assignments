#!/bin/bash

#check input arguments
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <number of tests>"
    exit 1
fi

iter_num=$1

#test set values
test_vals=("1-1000" "50000000-100000000" "1000000000-1100000000")
n_vals=(2 4 8 16 24 32)
c_vals=(32 64 256 1024 4096)
custom_ranges=("${test_vals[@]}")

#clean and build
make cleanall
make

#run tests
for n in "${n_vals[@]}"; do
    for c in "${c_vals[@]}"; do
        echo -e "\n--- Running with num_threads=$n, task_size=$c ---"
        for ((i = 1; i <= iter_num; i++)); do
            echo -e "\t- Iteration $i:"
            
            #run without -d flag
            result_static=$(./collatz_par -n $n -c $c ${custom_ranges[@]})
            echo -e "\t  -> Static Scheduling Result: $result_static"
            
            #run with -d flag
            result_dynamic=$(./collatz_par -n $n -c $c -d ${custom_ranges[@]})
            echo -e "\t  -> Dynamic Scheduling Result: $result_dynamic"
        done
    done
done