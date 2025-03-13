#!/bin/bash

#check if exactly two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <number of tests> <value to test>\nUse test_vals1 or test_vals2 as <value to test> for predefined test sets"
    exit 1
fi

iter_num=$1
arg=$2

#define test sets
test_vals1=(101 1001 10001 100001) #vals that are powers of 2
test_vals2=(128 1024 8192 131072) #vals that are not powers of 2

#clean and build
make cleanall
make

#determine values to test
if [ "$arg" == "test_vals1" ]; then
    values=("${test_vals1[@]}")
elif [ "$arg" == "test_vals2" ]; then
    values=("${test_vals2[@]}")
else
    values=($arg)
fi

#run tests on the provided values
for val in "${values[@]}"; do
    echo -e "\n>>> Running tests for K=$val <<<"
    for ((i = 1; i <= iter_num; i++)); do
        echo -e "\t- Iteration $i:"
        echo -e "\t  -> Testing softmax_plain..."
        ./softmax_plain $val
        echo -e "\t  -> Testing softmax_auto..."
        ./softmax_auto $val
        echo -e "\t  -> Testing softmax_avx..."
        ./softmax_avx $val
    done
done