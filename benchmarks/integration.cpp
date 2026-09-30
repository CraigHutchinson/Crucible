#include <crucible/simulation.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

int main() {
    using Clock = std::chrono::steady_clock;
    for (const auto count : {100'000U, 150'000U}) {
        crucible::Simulation simulation{count};
        for (int i = 0; i < 60; ++i) simulation.tick();
        std::vector<double> samples;
        samples.reserve(600);
        for (int i = 0; i < 600; ++i) {
            const auto start = Clock::now();
            simulation.tick();
            samples.push_back(std::chrono::duration<double, std::micro>(Clock::now() - start).count());
        }
        std::ranges::sort(samples);
        std::cout << "{\"workload\":\"ecs_integration\",\"entities\":" << count
                  << ",\"ticks\":600,\"median_us\":" << (samples[299] + samples[300]) / 2
                  << ",\"p95_us\":" << samples[569]
                  << ",\"min_us\":" << samples.front() << ",\"max_us\":" << samples.back()
                  << ",\"checksum\":" << simulation.checksum() << "}\n";
    }
}
