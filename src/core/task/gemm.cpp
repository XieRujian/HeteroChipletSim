#include "util/cJSON.h"

#include "../arch.h"
#include "gemm.h"

namespace core
{
    Gemm::Gemm(cJSON *json) : TaskImpl(json)
    {
        for (cJSON *item = json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "m")
                m = item->valueint;
            else if (key == "n")
                n = item->valueint;
            else if (key == "k")
                k = item->valueint;
        }
    }

    EvaluationResult Gemm::accept(const Arch &arch) const { return arch->Evaluate(*this); }
}