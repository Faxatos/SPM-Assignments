#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
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

void processRanges(const std::vector<std::pair<ull, ull>>& ranges, bool dynamic, size_t num_threads, ull task_size) {

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

    TIMERSTART(processRanges);
    processRanges(ranges, dynamic, num_threads, task_size);
    TIMERSTOP(processRanges);

    return 0;
}