#include <algorithm>
#include <mutex>
#include <random>

namespace util
{
    static std::mutex mtx;
    static std::mt19937 gen((std::random_device())());

    unsigned uniform_u(unsigned from, unsigned until)
    {
        mtx.lock();
        unsigned value = std::uniform_int_distribution<unsigned>(from, until)(gen);
        mtx.unlock();
        return value;
    }
    int uniform_i(int from, int until)
    {
        mtx.lock();
        int value = std::uniform_int_distribution<int>(from, until)(gen);
        mtx.unlock();
        return value;
    }
    double uniform_d(double from, double until)
    {
        mtx.lock();
        double value = std::uniform_real_distribution<double>(from, until)(gen);
        mtx.unlock();
        return value;
    }
    double exp_d(double lambda)
    {
        mtx.lock();
        double value = std::exponential_distribution<double>(lambda)(gen);
        mtx.unlock();
        return value;
    }

    bool uniform_bool() { return uniform_u(0, 1); }
    bool uniform_prob(double prob) { return uniform_d(0, 1) < prob; }

    size_t weighted_choice(const std::vector<double> &probabilities)
    {
        double sum = 0.0;
        for (const auto &p : probabilities)
            sum += p;
        double r = uniform_d(0, sum);
        double cumulative = 0.0;
        for (size_t i = 0; i < probabilities.size(); ++i)
        {
            cumulative += probabilities[i];
            if (r < cumulative)
                return i;
        }
        return probabilities.size() - 1;
    }
}