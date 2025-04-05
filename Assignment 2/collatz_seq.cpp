#include <iostream>
#include <string>
#include <cstdlib>
#include <hpc_helpers.hpp>

using ull = unsigned long long;

ull collatz_steps(ull n) {
    ull steps = 0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        ++steps;
    }
    return steps;
}

ull max_collatz_steps_in_range(int lower, int upper) {
    ull max_steps = 0;
    for (ull i = lower; i <= upper; ++i) {
        ull steps = collatz_steps(i);
        if (steps > max_steps) {
            max_steps = steps;
        }
    }
    return max_steps;
}

void processRanges(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t dash = arg.find('-');
        if (dash == std::string::npos) {
            std::cerr << "Err: argument must be in the form lower-upper\n";
            continue;
        }

        try {
            int lower = std::stoi(arg.substr(0, dash));
            int upper = std::stoi(arg.substr(dash + 1));

            if (lower >= upper) {
                std::cerr << "Err: For range \"" << arg << "\", the lower value (" << lower 
                          << ") is not smaller than the upper value (" << upper << "); skipping." << std::endl;
                continue;
            }

            ull max_steps = max_collatz_steps_in_range(lower, upper);
            std::cout << "Range: " << lower << "-" << upper 
                      << "; max Collatz steps = " << max_steps << std::endl;

        } catch (const std::invalid_argument&) {
            std::cerr << "Err: Invalid number in range \"" << arg << "\"; skipping." << std::endl;
        } catch (const std::out_of_range&) {
            std::cerr << "Err: Number out of range in \"" << arg << "\"; skipping." << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " lower1-upper1 [lower2-upper2] [lower3-upper3] ..." << std::endl;
        return 1;
    }

    TIMERSTART(processRanges);
    processRanges(argc, argv);
    TIMERSTOP(processRanges);

    return 0;
}


