#ifndef COLLECT_H
#define COLLECT_H

#include "../task.h"

namespace core
{
    class Collect : public TaskImpl
    {
    public:
        Collect(cJSON *json) : TaskImpl(json) {}
        EvaluationResult accept(const Arch &) const override { return {0, 0}; }
    };
}

#endif // COLLECT_H