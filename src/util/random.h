#ifndef RANDOM_H
#define RANDOM_H

#include <random>
#include <mutex>

namespace util
{
    unsigned uniform_u(unsigned from, unsigned until);
    int uniform_i(int from, int until);
    double uniform_d(double from, double until);
    double exp_d(double lambda);
    bool uniform_bool();
    bool uniform_prob(double prob);
    size_t weighted_choice(const std::vector<double> &probabilities);
    template <typename T>
    void shuffle(std::vector<T> &vec)
    {
        for (uint32_t i = 1; i < vec.size(); ++i)
            std::swap(vec[i], vec[uniform_u(0, i)]);
    }
}

#endif /* RANDOM_H */
