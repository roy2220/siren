#include "thread_pool.h"

#include <cerrno>
#include <system_error>

#include <unistd.h>


namespace siren {

void
ThreadPool::initialize()
{
    if (pipe(fds_) < 0) {
        throw std::system_error(errno, std::system_category(), "pipe() failed");
    }
}


void
ThreadPool::finalize()
{
    if (close(fds_[0]) < 0 && errno != EINTR) {
        throw std::system_error(errno, std::system_category(), "close() failed");
    }

    if (close(fds_[1]) < 0 && errno != EINTR) {
        throw std::system_error(errno, std::system_category(), "close() failed");
    }
}


void
ThreadPool::worker()
{
    for (;;) {
        ThreadPoolTask *task;

        {
            std::unique_lock<std::mutex> uniqueLock(mutex_);

            while (taskList_.isEmpty()) {
                conditionVariable_.wait(uniqueLock);
            }

            task = static_cast<ThreadPoolTask *>(taskList_.getHead());

            if (task == &noTask_) {
                return;
            } else {
                task->remove();
            }
        }

        try {
            task->procedure_();
        } catch (...) {
            task->exception_ = std::current_exception();
        }

        task->procedure_ = nullptr;

        for (;;) {
            ssize_t numberOfBytes = write(fds_[1], &task, sizeof task);

            if (numberOfBytes < 0) {
                if (errno != EINTR) {
                    throw std::system_error(errno, std::system_category(), "write() failed");
                }

                continue;
            } else {
                break;
            }
        }
    }
}

}
