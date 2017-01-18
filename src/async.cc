#include "async.h"

#include <cerrno>
#include <system_error>


namespace siren {

void
Async::EventTrigger(ThreadPool *threadPool, Loop *loop)
{
    std::vector<ThreadPoolTask *> threadPoolTasks;

    for (;;) {
        {
            std::uint64_t dummy;

            if (loop->read(threadPool->getEventFD(), &dummy, sizeof(dummy)) < 0) {
                throw std::system_error(errno, std::system_category(), "read() failed");
            }
        }

        threadPool->getCompletedTasks(&threadPoolTasks);

        for (ThreadPoolTask *threadPoolTask : threadPoolTasks) {
            auto task = static_cast<Task *>(threadPoolTask);
            task->event->trigger();
        }

        threadPoolTasks.clear();
    }
}

}
