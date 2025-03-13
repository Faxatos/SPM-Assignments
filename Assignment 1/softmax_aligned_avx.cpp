#include <iostream>
#include <random>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <immintrin.h>
#include <hpc_helpers.hpp>
#include <avx_mathfun.h>

void softmax_avx(const float *input, float *output, size_t K) {
    size_t i = 0;
    // Find the maximum to stabilize the computation of the exponential
    __m256 max256 = _mm256_set1_ps(-std::numeric_limits<float>::infinity());
    for (; i + 7 < K; i += 8) {
        __m256 tmp_input256 = _mm256_load_ps(&input[i]); // aligned load
        max256 = _mm256_max_ps(max256, tmp_input256);
    }

    float max_vals[8];
    _mm256_store_ps(max_vals, max256); //aligned store
    float max_val = max_vals[0];
    for (size_t j = 1; j < 8; ++j) { //horizontal max reduction
        max_val = std::max(max_val, max_vals[j]);
    }
    for (; i < K; ++i) { //process remaining elements (if K % 8 != 0)
        max_val = std::max(max_val, input[i]);
    }

    // Compute all exponentials with the shift of max_val and compute the total sum
    __m256 sum256 = _mm256_setzero_ps();
    __m256 max_val256 = _mm256_set1_ps(max_val);
    for (i = 0; i + 7 < K; i += 8) {
        __m256 tmp_input256 = _mm256_load_ps(&input[i]); //aligned load
        tmp_input256 = _mm256_sub_ps(tmp_input256, max_val256); //subtract max_val for numerical stability
        __m256 exp_input_tmp256 = exp256_ps(tmp_input256);
        _mm256_store_ps(&output[i], exp_input_tmp256); //aligned store
        sum256 = _mm256_add_ps(sum256, exp_input_tmp256);
    }

    float sum_vals[8];
    _mm256_store_ps(sum_vals, sum256); //aligned store
    float sum_val = sum_vals[0];
    for (size_t j = 1; j < 8; ++j) { // horizontal sum reduction
        sum_val += sum_vals[j];
    }
    for (; i < K; ++i) { // process remaining elements
        float exp_val = std::exp(input[i] - max_val);
        output[i] = exp_val;
        sum_val += exp_val;
    }

    // Normalize by dividing by the total sum
    __m256 sum_val256 = _mm256_set1_ps(sum_val);
    for (i = 0; i + 7 < K; i += 8) {
        __m256 tmp_output256 = _mm256_load_ps(&output[i]); //aligned load
        tmp_output256 = _mm256_div_ps(tmp_output256, sum_val256);
        _mm256_store_ps(&output[i], tmp_output256); //aligned store
    }
    for (; i < K; ++i) { //process remaining elements
        output[i] /= sum_val;
    }
}

void fill_random(float* array, size_t K, float min = -1.0f, float max = 1.0f) {
    std::mt19937 gen(5489); // fixed seed for reproducible results
    std::uniform_real_distribution<float> dis(min, max);
    for (size_t i = 0; i < K; ++i) {
        array[i] = dis(gen);
    }
}

void printResult(const float *v, size_t K) {
    for (size_t i = 0; i < K; ++i) {
        std::fprintf(stderr, "%f\n", v[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::printf("use: %s K [print]\n", argv[0]);
        return 0;
    }
    size_t K = std::stol(argv[1]);
    bool print = (argc == 3);

    // Allocate aligned memory (32-byte alignment for AVX)
    float *input  = (float*)_mm_malloc(K * sizeof(float), 32);
    float *output = (float*)_mm_malloc(K * sizeof(float), 32);

    fill_random(input, K);

    TIMERSTART(softime_avx);
    softmax_avx(input, output, K);
    TIMERSTOP(softime_avx);

    if (print) {
        printResult(output, K);
    }

    _mm_free(input);
    _mm_free(output);
    return 0;
}
