#include "TimerWheel.hh"
#include <thread>
#include <unistd.h>

std::unique_ptr<TimerWheel> timerWheel = std::make_unique<TimerWheel>();

int main()
{
    TimerTask::Task t = []()
    {
        std::cout << "Task 1" << std::endl;
    };

    std::thread timer([]()
                      {
        int i = 0;
        while(true){
            sleep(1);
            std::cout << "TimerWheel is running..." << std::endl;
            i++;
            if(i == 3) {
                timerWheel->resetTimer(99);
            } else if(i == 5) {
                timerWheel->cancelTimer(99);
                std::cout << "99 is be cancelled" << std::endl;
            }
            timerWheel->run();
        } });

    timerWheel->addTimer(99, 5, t);
    std::cout << "99 is be set" << std::endl;

    timer.join();
    return 0;
}