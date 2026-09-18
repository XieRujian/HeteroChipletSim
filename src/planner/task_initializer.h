#ifndef TASK_INITIALIZER_H
#define TASK_INITIALIZER_H

#include <cstdint>
#include <string>

#include "util/common.h"

namespace planner
{
    class TaskScheduler;
    class TaskInitializer
    {
        TaskScheduler *task_scheduler_ptr_;

    public:
        TaskInitializer(TaskScheduler *task_scheduler_ptr) : task_scheduler_ptr_(task_scheduler_ptr) {}

        std::vector<TimeStamp> GetNonZeroTimeSlot() const;
        void Initialize();
        void InitializeGemm();
        void InitializeAttention();
        void InitializeConv();

        void Mesh4B1H6N4M4();
        void Mesh4B1H5N3M4();
        void Mesh4B1H5N5M3();
        void Mesh4B1H5N15M1();
        void Mesh4B3H2N4M4();

        void Mesh4M1N3K32();
        void Mesh4M1N4K2();
        void Mesh4M1N5K15();
        void Mesh4M1N15K5();
        void Mesh4M1N24K4();
        void Mesh4M1N25K3();
        void Mesh4M1N32K3();
        void Mesh4M1N32K4();
        void Mesh4M2N4K16();
        void Mesh4M2N8K6();
        void Mesh4M4N4K8();
        void Mesh4M4N8K4();
        void Mesh4M8N2K8();
        void Mesh4M8N6K2();
        void Mesh4M8N8K2();
        void Mesh4M12N2K2();
        void Mesh4M12N4K2();

        void Mesh4W8H8I1O1();
        void Mesh4W4H8I2O1();
        void Mesh4W2H4I6O1();
        void Mesh4W1H2I12O2();
    };
}

#endif // TASK_INITIALIZER_H