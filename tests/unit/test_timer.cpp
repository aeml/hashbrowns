#include "core/timer.h"
#include <iostream>
#include <thread>

using namespace hashbrowns;

int run_timer_tests() {
    int failures = 0;
    std::cout << "Timer Test Suite\n";
    std::cout << "================\n\n";

    // Basic start/stop with sleep
    {
        Timer timer;
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto d1 = timer.stop();
        if (d1.count() <= 0) {
            std::cout << "❌ Timer duration not positive on first stop\n";
            ++failures;
        }

        // Second stop without a new start should not produce negative duration
        auto d2 = timer.stop();
        if (d2.count() < 0) {
            std::cout << "❌ Timer produced negative duration on second stop\n";
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
