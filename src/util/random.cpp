#include <random.h>
#include <random>

namespace util
{
    static std::random_device rd = std::random_device();
    static Generator global(rd());

    unsigned Generator::uniform_u(unsigned from, unsigned until)
    {
        mtx_.lock();
        unsigned value = std::uniform_int_distribution<unsigned>(from, until)(gen_);
        mtx_.unlock();
        return value;
    }
    int Generator::uniform_i(int from, int until)
    {
        mtx_.lock();
        unsigned value = std::uniform_int_distribution<int>(from, until)(gen_);
        mtx_.unlock();
        return value;
    }
    double Generator::uniform_d(double from, double until)
    {
        mtx_.lock();
        double value = std::uniform_real_distribution<double>(from, until)(gen_);
        mtx_.unlock();
        return value;
    }
    double Generator::exp_d(double lambda)
    {
        mtx_.lock();
        double value = std::exponential_distribution<double>(lambda)(gen_);
        mtx_.unlock();
        return value;
    }

    unsigned uniform_u(unsigned from, unsigned until) { return global.uniform_u(from, until); }
    int uniform_i(int from, int until) { return global.uniform_i(from, until); }
    double uniform_d(double from, double until) { return global.uniform_d(from, until); }
    double exp_d(double lambda) { return global.exp_d(lambda); }
    bool uniform_bool() { return global.uniform_u(0, 1); }
    bool uniform_prob(double prob) { return global.uniform_d(0, 1) < prob; }

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