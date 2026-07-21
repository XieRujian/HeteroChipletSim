#ifndef RANDOM_H
#define RANDOM_H

#include <random>
#include <mutex>

namespace util
{
    class Generator
    {
        std::mutex mtx_;
        std::mt19937 gen_;

    public:
        Generator(unsigned seed) : gen_(seed) {}
        unsigned uniform_u(unsigned from, unsigned until);
        int uniform_i(int from, int until);
        double uniform_d(double from, double until);
        double exp_d(double lambda);
    };

    unsigned uniform_u(unsigned from, unsigned until);
    int uniform_i(int from, int until);
    double uniform_d(double from, double until);
    double exp_d(double lambda);
    bool uniform_bool();
    bool uniform_prob(double prob);
    size_t weighted_choice(const std::vector<double> &probabilities);
}

#endif /* RANDOM_H */
