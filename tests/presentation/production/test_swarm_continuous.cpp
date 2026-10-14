#include <array>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "crucible/presentation/production/detail/mesh_validation.hpp"
#include "crucible/presentation/production/swarm_intro_adapter.hpp"

int main(int argc, char** argv)
{
    using namespace crucible::presentation::production;
    unsigned step{1}, currentTick{};
    float currentAspect{};
    try
    {
        if (argc == 2)
        {
            const std::string_view argument{argv[1]};
            const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), step);
            if (parsed.ec != std::errc{} || parsed.ptr != argument.data() + argument.size() || step == 0 || step > 32)
                throw std::invalid_argument("Receiving step must be 1..32");
        }
        else if (argc != 1) throw std::invalid_argument("Expected at most one receiving-step argument");
        struct Viewport { float aspect_, width_; };
        constexpr std::array viewports{Viewport{1280.0F / 864.0F, 1280}, Viewport{16.0F / 9.0F, 1920}};
        for (const auto viewport : viewports)
        {
            currentAspect = viewport.aspect_;
            SwarmIntroAdapter adapter;
            if (!adapter.trySetViewport(viewport.aspect_, viewport.width_)) throw std::runtime_error("Viewport refused");
            bool finished{};
            unsigned frames{};
            for (currentTick = 0; currentTick <= 2700; currentTick += step)
            {
                if (!adapter.trySeekPresentedTicks(currentTick)) throw std::runtime_error("Forward playback refused");
                const auto frame = adapter.getFrame();
                if (!frame || !detail::isValidFrame(*frame)) throw std::runtime_error("Intermediate model packet invalid");
                const auto progress = adapter.getProgress();
                if (progress.visibleUnits_ + progress.culledUnits_ != 1536 || progress.totalPresentedTicks_ != currentTick)
                    throw std::runtime_error("Intermediate source/count receipt invalid");
                ++frames;
                if (progress.staticReady_) { finished = true; break; }
            }
            if (!finished) throw std::runtime_error("Continuous model playback did not settle");
            std::cout << "Received model frames=" << frames << " step=" << step << " aspect=" << currentAspect
                << " width=" << viewport.width_ << " settled_tick=" << currentTick << '\n';
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Continuous model receiving failed at tick=" << currentTick << " aspect=" << currentAspect
            << ": " << error.what() << '\n';
        return 1;
    }
}
