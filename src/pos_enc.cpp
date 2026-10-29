#include "pos_enc.hpp"
#include <cmath>
#include <cassert>

namespace pk {

// INF_VAL from NeMo multi_head_attention.py (used as the sinusoid base).
static constexpr double kInfVal = 10000.0;

void rel_pos_encoding(int T, int d_model, std::vector<float>& out) {
    assert(T > 0 && d_model > 0 && (d_model % 2) == 0);
    const int P = 2 * T - 1;             // number of relative positions
    const int half = d_model / 2;        // number of (sin,cos) pairs

    // div_term[i] = exp(2i * -(log(INF_VAL)/d_model)) for i in [0, half).
    // Compute in double for parity, store the precomputed factors.
    std::vector<double> div_term(half);
    const double factor = -(std::log(kInfVal) / (double)d_model);
    for (int i = 0; i < half; ++i) {
        div_term[i] = std::exp((double)(2 * i) * factor);
    }

    out.assign((size_t)P * d_model, 0.0f);
    // Bolt optimization: positions run symmetrically from +(T-1) down to -(T-1).
    // Center pos = 0 is at index center = T - 1.
    // Leverage trig symmetry: sin(-x) = -sin(x), cos(-x) = cos(x), sin(0) = 0, cos(0) = 1.
    // This halves std::sin/std::cos calls and eliminates trig calls for pos = 0.
    const int center = T - 1;
    float* row_zero = out.data() + (size_t)center * d_model;
    for (int i = 0; i < half; ++i) {
        row_zero[2 * i]     = 0.0f;
        row_zero[2 * i + 1] = 1.0f;
    }

    for (int k = 1; k < T; ++k) {
        float* row_pos = out.data() + (size_t)(center - k) * d_model;
        float* row_neg = out.data() + (size_t)(center + k) * d_model;
        const double dk = (double)k;
        for (int i = 0; i < half; ++i) {
            const double arg = dk * div_term[i];
            const float s = (float)std::sin(arg);
            const float c = (float)std::cos(arg);
            row_pos[2 * i]     = s;
            row_pos[2 * i + 1] = c;
            row_neg[2 * i]     = -s;
            row_neg[2 * i + 1] = c;
        }
    }
}

void local_rel_pos_encoding(int att_left, int att_right, int d_model,
                            std::vector<float>& out) {
    assert(att_left >= 0 && att_right >= 0 && d_model > 0 && (d_model % 2) == 0);
    const int P = att_left + att_right + 1;   // local relative positions
    const int half = d_model / 2;

    std::vector<double> div_term(half);
    const double factor = -(std::log(kInfVal) / (double)d_model);
    for (int i = 0; i < half; ++i) {
        div_term[i] = std::exp((double)(2 * i) * factor);
    }

    out.assign((size_t)P * d_model, 0.0f);
    // Bolt optimization: positions run from +att_left down to -att_right.
    // Center pos = 0 is at index att_left.
    // Leverage trig symmetry: sin(-x) = -sin(x), cos(-x) = cos(x), sin(0) = 0, cos(0) = 1.
    const int center = att_left;
    float* row_zero = out.data() + (size_t)center * d_model;
    for (int i = 0; i < half; ++i) {
        row_zero[2 * i]     = 0.0f;
        row_zero[2 * i + 1] = 1.0f;
    }

    const int max_k = std::max(att_left, att_right);
    for (int k = 1; k <= max_k; ++k) {
        const bool has_pos = (k <= att_left);
        const bool has_neg = (k <= att_right);
        float* row_pos = has_pos ? (out.data() + (size_t)(center - k) * d_model) : nullptr;
        float* row_neg = has_neg ? (out.data() + (size_t)(center + k) * d_model) : nullptr;
        const double dk = (double)k;
        for (int i = 0; i < half; ++i) {
            const double arg = dk * div_term[i];
            const float s = (float)std::sin(arg);
            const float c = (float)std::cos(arg);
            if (has_pos) {
                row_pos[2 * i]     = s;
                row_pos[2 * i + 1] = c;
            }
            if (has_neg) {
                row_neg[2 * i]     = -s;
                row_neg[2 * i + 1] = c;
            }
        }
    }
}

} // namespace pk
