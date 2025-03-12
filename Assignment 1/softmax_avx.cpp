#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>      
#include <hpc_helpers.hpp>
#include <avx_mathfun.h>

void softmax_avx(const float *input, float *output, size_t K) {
	size_t i;
	// Find the maximum to stabilize the computation of the exponential
	__m256 max256 = _mm256_set1_ps(-std::numeric_limits<float>::infinity());
    for (i = 0; i + 7 < K; i += 8) {
        __m256 tmp_input256 = _mm256_loadu_ps(&input[i]);
        max256 = _mm256_max_ps(max256, tmp_input256);
    }

	float max_vals[8];
    _mm256_storeu_ps(max_vals, max256);
    float max_val = max_vals[0];
    for (size_t j = 1; j < 8; ++j) { //horizontal max reduction (reducing the 8 max_vals to a single max_val)
        max_val = std::max(max_val, max_vals[j]);
    }
	for (; i < K; ++i) { //processing remaining elements [enters loop if (k % 8 != 0)]
        max_val = std::max(max_val, input[i]);
    }

	// computes all exponentials with the shift of max_val and the total sum
	__m256 sum256 = _mm256_setzero_ps();
	__m256 max_val256 = _mm256_set1_ps(max_val);
    for (i = 0; i + 7 < K; i += 8) {
        __m256 tmp_input256 = _mm256_loadu_ps(&input[i]);
        tmp_input256 = _mm256_sub_ps(tmp_input256, max_val256); //subtract max_val for numerical stability (see report for more details)
        __m256 exp_input_tmp256 = exp256_ps(tmp_input256);
        _mm256_storeu_ps(&output[i], exp_input_tmp256);
        sum256 = _mm256_add_ps(sum256, exp_input_tmp256);
    }

	float sum_vals[8];
    _mm256_storeu_ps(sum_vals, sum256);
    float sum_val = sum_vals[0];
    for (size_t j = 1; j < 8; ++j) { //horizontal sum reduction (reducing the 8 sum_vals to a single sum_val)
        sum_val += sum_vals[j];
    }
	for (; i < K; ++i) { //processing remaining elements [enters loop if (k % 8 != 0)]
        float exp_val = std::exp(input[i] - max_val);
        output[i] = exp_val;
        sum_val += exp_val;
    }

	// normalize by dividing for the total sum
	__m256 sum_val256 = _mm256_set1_ps(sum_val);
	for (i = 0; i + 7 < K; i += 8) {
		__m256 tmp_output256 = _mm256_loadu_ps(&output[i]);
		tmp_output256 = _mm256_div_ps(tmp_output256, sum_val256);
		_mm256_storeu_ps(&output[i], tmp_output256);
    }
	for (; i < K; ++i) { //processing remaining elements [enters loop if (k % 8 != 0)]
        output[i] /= sum_val;
    }
}

std::vector<float> generate_random_input(size_t K, float min = -1.0f, float max = 1.0f) {
    std::vector<float> input(K);
    //std::random_device rd;
    //std::mt19937 gen(rd());
	std::mt19937 gen(5489); // fixed seed for reproducible results
    std::uniform_real_distribution<float> dis(min, max);
    for (size_t i = 0; i < K; ++i) {
        input[i] = dis(gen);
    }
    return input;
}

void printResult(std::vector<float> &v, size_t K) {
	for(size_t i=0; i<K; ++i) {
		std::fprintf(stderr, "%f\n",v[i]);
	}
}


int main(int argc, char *argv[]) {
	if (argc == 1) {
		std::printf("use: %s K [1]\n", argv[0]);
		return 0;		
	}
	size_t K=0;
	if (argc >= 2) {
		K = std::stol(argv[1]);
	}
	bool print=false;
	if (argc == 3) {
		print=true;
	}	
	std::vector<float> input=generate_random_input(K);
	std::vector<float> output(K);

	TIMERSTART(softime_avx);
	softmax_avx(input.data(), output.data(), K);
	TIMERSTOP(softime_avx);
	
	// print the results on the standard output
	if (print) {
		printResult(output, K);
	}
}

