#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <thread>
#include <future>
#include <hpc_helpers.hpp>
#include <threadPool.hpp>

using ull = unsigned long long;

//collatz steps for a given number
ull collatz_steps(ull n) {
    ull steps = 0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        ++steps;
    }
    return steps;
}

//max collatz steps for a given range
ull max_collatz_steps_in_range(ull lower, ull upper) {
    ull max_steps = 0;
    for (ull i = lower; i <= upper; ++i) {
        ull steps = collatz_steps(i);
        if (steps > max_steps) {
            max_steps = steps;
        }
    }
    return max_steps;
}

void process_range_dynamic(const std::vector<std::pair<ull, ull>>& ranges, size_t num_threads, ull task_size) {
    // Initialize the thread pool with the specified number of threads
    ThreadPool TP(num_threads);
    std::vector<std::future<ull>> futures;
    std::vector<ull> range_max_results(ranges.size());

    // Enqueue tasks for each sub-range within the specified ranges
    for (size_t i = 0; i < ranges.size(); ++i) {
        ull lower = ranges[i].first;
        ull upper = ranges[i].second;

        for (ull start = lower; start <= upper; start += task_size) {
            ull end = std::min(start + task_size - 1, upper);
            futures.emplace_back(TP.enqueue([start, end]() {
                return max_collatz_steps_in_range(start, end);
            }));
        }

        // Collect and merge results for the current range
        ull max_steps = 0;
        for (size_t j = 0; j < futures.size(); ++j) {
            max_steps = std::max(max_steps, futures[j].get());
        }
        range_max_results[i] = max_steps;
        futures.clear(); // Clear futures for the next range
    }

    //print the maximum for each range
    for (size_t r = 0; r < ranges.size(); ++r) {
        std::cout << ranges[r].first << "-" << ranges[r].second << ": " << range_max_results[r] << std::endl;
    }
}

// Function for static scheduling using block-cyclic task distribution
void process_ranges_static_block_cyclic(const std::vector<std::pair<ull, ull>>& ranges, ull task_size, size_t num_threads) {
    // This vector will store the global maximum for each range
    std::vector<ull> range_max_results(ranges.size(), 0);

    // Mutex for thread-safe aggregation of results
    std::mutex result_mutex;

    // Launch asynchronous tasks for each thread using std::async
    std::vector<std::future<std::vector<ull>>> futures;
    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
        futures.emplace_back(std::async(std::launch::async, [=, &ranges]() -> std::vector<ull> {
            std::vector<ull> partial_results(ranges.size(), 0);

            for (size_t r = 0; r < ranges.size(); r++) {
                ull lower = ranges[r].first;
                ull upper   = ranges[r].second;
                ull range_length = upper - lower + 1;
                // Calculate the number of blocks needed for this range
                ull rangeBlocksNumber = (range_length + task_size - 1) / task_size;
                // Process blocks assigned to this thread (block-cyclic distribution)
                for (ull t = thread_id; t < rangeBlocksNumber; t += num_threads) {
                    ull block_start = lower + t * task_size;
                    ull block_end   = std::min(block_start + task_size - 1, upper);
                    ull res  = max_collatz_steps_in_range(block_start, block_end);
                    partial_results[r] = std::max(partial_results[r], res);
                }
            }
            return partial_results;
        }));
    }

    //merge partial results for each range
    for (auto& f : futures) {
        std::vector<ull> thread_results = f.get();
        std::lock_guard<std::mutex> lock(result_mutex);
        for (size_t r = 0; r < ranges.size(); ++r) {
            range_max_results[r] = std::max(range_max_results[r], thread_results[r]);
        }
    }

    //print the maximum for each range
    for (size_t r = 0; r < ranges.size(); ++r) {
        std::cout << ranges[r].first << "-" << ranges[r].second << ": " << range_max_results[r] << std::endl;
    }
}

// Each range is divided into exactly num_threads blocks of size (range_length / num_threads),
// threads are launched in reverse order and each thread computes its assigned block
void process_ranges_static_block(const std::vector<std::pair<ull, ull>>& ranges, size_t num_threads) {
    // Each future returns a vector<ull> where each element is the max for a range.
    std::vector<std::future<std::vector<ull>>> futures;
    
    // Launch threads in reverse order.
    for (size_t thread_id = num_threads - 1; thread_id >= 0; --thread_id) {
        futures.emplace_back(std::async(std::launch::async, [=, &ranges]() -> std::vector<ull> {
            // Vector to hold the result for each range from this thread.
            std::vector<ull> partial_results(ranges.size(), 0);
            // Process every range.
            for (size_t r = 0; r < ranges.size(); ++r) {
                ull lower = ranges[r].first;
                ull upper = ranges[r].second;
                ull range_length = upper - lower + 1;
                // Compute task_size as integer division 
                ull task_size = range_length / num_threads;
                if (task_size == 0){ //case where range_length < num_threads
                    if (thread_id >= range_length) { //excluding threads >= range_length
                        partial_results[r] = 0;
                        continue;
                    }
                    partial_results[r] = collatz_steps(lower + thread_id);
                    continue;
                }
                
                ull remainings = range_length % num_threads;
                //start and end indexes for this thread's block in the range.
                ull block_start, block_end;
                if (thread_id < remainings) { //getting task_size + 1 elements to compute
                    block_start = lower + thread_id * (task_size + 1);
                    block_end = block_start + task_size;
                } else { //getting task_size elements to compute
                    block_start = lower + remainings * (task_size + 1) + (thread_id - remainings) * task_size;
                    block_end = block_start + task_size - 1;
                }

                // Compute the maximum for the assigned block.
                partial_results[r] = max_collatz_steps_in_range(block_start, block_end);
            }
            return partial_results;
        }));
    }
    
    //merge partial results for each range
    std::vector<ull> range_max_results(ranges.size(), 0);
    for (auto& f : futures) {
        std::vector<ull> thread_results = f.get();
        for (size_t r = 0; r < thread_results.size(); ++r) {
            range_max_results[r] = std::max(range_max_results[r], thread_results[r]);
        }
    }
    
    //print the maximum for each range
    for (size_t r = 0; r < ranges.size(); ++r) {
        std::cout << ranges[r].first << "-" << ranges[r].second << ": " << range_max_results[r] << std::endl;
    }
}

void process_ranges(const std::vector<std::pair<ull, ull>>& ranges, bool dynamic, size_t num_threads, ull task_size) {
    if (dynamic) {

    }
    else{ //static
        TIMERSTART(process_ranges_static_block_cyclic);
        process_ranges_static_block_cyclic(ranges, task_size, num_threads);
        TIMERSTOP(process_ranges_static_block_cyclic);
        //process_ranges_static_block(ranges, num_threads);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-d] [-n num_threads] [-c task_size] lower1-upper1 [lower2-upper2] ..." << std::endl;
        return 1;
    }

    bool dynamic = false;
    size_t num_threads = 1;
    ull task_size = 16;
    std::vector<std::pair<ull, ull>> ranges;

    //parsing command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-d") { //dynamic or not
            dynamic = true;
        } 
        else if (arg == "-n") { //number of threads size
            if (i + 1 < argc) {
                num_threads = std::stoul(argv[++i]);
            } 
            else {
                std::cerr << "Err: -n option requires numeric argument." << std::endl;
                return 1;
            }
        } 
        else if (arg == "-c") { //task size
            if (i + 1 < argc) {
                task_size = std::stoull(argv[++i]);
            } 
            else {
                std::cerr << "Err: -c option requires numeric argument." << std::endl;
                return 1;
            }
        } 
        else { //range case
            size_t dash = arg.find('-');
            if (dash == std::string::npos) {
                std::cerr << "Err: argument must be in the form lower-upper; skipping range." << std::endl;
                continue;
            }

            try {
                ull lower = std::stoull(arg.substr(0, dash));
                ull upper = std::stoull(arg.substr(dash + 1));

                if (lower >= upper) {
                    std::cerr << "Error: For range '" << arg << "', the lower value (" << lower
                              << ") is not smaller than the upper value (" << upper << "); skipping range." << std::endl;
                    continue;
                }

                ranges.emplace_back(lower, upper);
            } catch (const std::invalid_argument&) {
                std::cerr << "Error: Invalid number in range '" << arg << "'; skipping range." << std::endl;
            } catch (const std::out_of_range&) {
                std::cerr << "Error: Number out of range in '" << arg << "'; skipping range." << std::endl;
            }
        }
    }

    process_ranges(ranges, dynamic, num_threads, task_size);

    return 0;
}