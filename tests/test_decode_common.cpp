#include "decode_common.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    // Test 1: decode_argmax
    {
        std::vector<float> logits = {1.0f, 5.0f, 3.0f, 5.0f, 2.0f};
        int argmax = pk::decode_argmax(logits.data(), (int)logits.size());
        // First occurrence of max value 5.0f is index 1.
        if (argmax != 1) {
            std::fprintf(stderr, "decode_argmax failed: expected 1, got %d\n", argmax);
            return 1;
        }
    }

    // Test 2: decode_max_prob_conf uniform logits
    {
        std::vector<float> logits = {2.0f, 2.0f, 2.0f, 2.0f};
        int k = pk::decode_argmax(logits.data(), (int)logits.size());
        float conf = pk::decode_max_prob_conf(logits.data(), (int)logits.size(), k);
        // p_max = 0.25, N = 4, conf = (4 * 0.25 - 1) / 3 = 0.0
        if (std::fabs(conf - 0.0f) > 1e-6f) {
            std::fprintf(stderr, "decode_max_prob_conf uniform failed: expected 0.0, got %f\n", conf);
            return 1;
        }
    }

    // Test 3: decode_max_prob_conf dominant logit
    {
        std::vector<float> logits = {100.0f, 0.0f, 0.0f};
        int k = pk::decode_argmax(logits.data(), (int)logits.size());
        float conf = pk::decode_max_prob_conf(logits.data(), (int)logits.size(), k);
        // p_max = 1.0, N = 3, conf = (3 * 1.0 - 1) / 2 = 1.0
        if (std::fabs(conf - 1.0f) > 1e-5f) {
            std::fprintf(stderr, "decode_max_prob_conf dominant failed: expected 1.0, got %f\n", conf);
            return 1;
        }
    }

    // Test 4: decode_max_prob_conf specific calculation
    {
        std::vector<float> logits = {1.0f, 3.0f, 2.0f};
        int k = pk::decode_argmax(logits.data(), (int)logits.size()); // k = 1
        float conf = pk::decode_max_prob_conf(logits.data(), (int)logits.size(), k);

        double exp1 = std::exp(1.0 - 3.0);
        double exp3 = std::exp(3.0 - 3.0); // 1.0
        double exp2 = std::exp(2.0 - 3.0);
        double denom = exp1 + exp3 + exp2;
        double p_max = 1.0 / denom;
        double expected_conf = (3.0 * p_max - 1.0) / 2.0;

        if (std::fabs(conf - (float)expected_conf) > 1e-6f) {
            std::fprintf(stderr, "decode_max_prob_conf calculation failed: expected %f, got %f\n", expected_conf, conf);
            return 1;
        }
    }

    std::printf("test_decode_common PASS\n");
    return 0;
}
