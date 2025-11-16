#include "core/timer.h"
#include <iostream>
#include <thread>

using namespace hashbrowns;

int run_timer_tests() {
    int failures = 0;
    std::cout << "Timer Test Suite\n";
    std::cout << "================\n\n";

    // Basic start/stop with sleep (two valid cycles)
    {
        Timer timer;

        // First measurement
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto d1 = timer.stop();
        if (d1.count() <= 0) {
            std::cout << "❌ Timer duration not positive on first stop\n";
            ++failures;
        }

        // Second measurement (fresh start/stop)
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto d2 = timer.stop();
        if (d2.count() <= 0) {
            std::cout << "❌ Timer duration not positive on second stop\n";
            ++failures;
        }
    }

    if (failures == 0) {
        std::cout << "\n✅ Timer tests passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " Timer test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}
