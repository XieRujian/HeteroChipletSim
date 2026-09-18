#include "util/cJSON.h"

#include "../arch.h"
#include "attention.h"

namespace core
{
    Attention::Attention(cJSON *json) : TaskImpl(json)
    {
        for (cJSON *item = json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "seq_length")
                seq_length = item->valueint;
            else if (key == "kv_size")
                kv_size = item->valueint;
            else if (key == "head_num")
                head_num = item->valueint;
            else if (key == "head_dim")
                head_dim = item->valueint;
        }
    }

    EvaluationResult Attention::accept(const Arch &arch) const { return arch->Evaluate(*this); }
}
