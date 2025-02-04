#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/eventfd.h>

int main()
{
    int efd = eventfd(0, 0);
    if (efd < 0)
    {
        return -1;
    }

    std::thread t([&]() {
        sleep(5);
        uint64_t u = 1;
        printf("Worker: Task completed, sending notification.\n");
        if (write(efd, &u, sizeof(uint64_t)) != sizeof(uint64_t)) {
            perror("write");
        } 
    });

    // 主线程等待通知
    uint64_t u;
    printf("Main: Waiting for notification...\n");
    if (read(efd, &u, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("read");
    } else {
        printf("Main: Received notification. Task completed!\n");
    }

    t.join();


    return 0;
}