#include <crucible/simulation.hpp>
#include <sub0pipeline/sub0pipeline.hpp>
#include <sub0pub/sub0pub.hpp>
#include <sub0log/log.hpp>
#include <sub0log/reader.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>

namespace {
struct TickCommand {};
struct Publisher : sub0::Publish<TickCommand> {
    void send() { sub0::publish(this, TickCommand{}); }
};
// This test is single-threaded. Delivery is synchronous; defer ECS mutation to the DAG.
struct Inbox : sub0::Subscribe<TickCommand> {
    bool pending{};
    void receive(const TickCommand&) noexcept override { pending = true; }
};
}

int main() {
    const auto directory = std::filesystem::current_path() / "stack-telemetry";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    bool valid = false;
    {
        auto logger = sub0log::Logger::create({.directory_ = directory.string(), .stem_ = "stack"});
        if (!logger.valid()) {
            std::cerr << "Logger creation failed\n";
            return 1;
        }
        sub0log::Logger::ScopedBind bind{logger};
        crucible::Simulation simulation{1000};
        Inbox inbox;
        Publisher publisher;
        publisher.send();
        if (!inbox.pending || simulation.checksum() != 0.0) return 1;

        sub0pipeline::Pipeline pipeline;
        auto integrate = pipeline.emplace([&] {
            if (inbox.pending) {
                simulation.tick();
                inbox.pending = false;
            }
        }).name("integrate");
        pipeline.emplace([&] {
            sub0log_info(sub0log::SubsystemId{1}, "tick checksum={}", simulation.checksum());
        }).name("telemetry").succeed(integrate);
        auto executor = sub0pipeline::makeSequentialExecutor();
        if (!pipeline.run(*executor) || inbox.pending) {
            std::cerr << "Pipeline did not complete the admitted tick\n";
            return 1;
        }

    }
    // Release the writer mapping before opening an independent file reader;
    // mapped writes and file reads need not be coherent on mounted filesystems.

    for (const auto& entry : std::filesystem::directory_iterator{directory}) {
        if (entry.path().extension() != ".s0l") continue;
        std::ifstream input{entry.path(), std::ios::binary};
        if (!input) {
            std::cerr << "Telemetry file could not be read\n";
            return 1;
        }
        std::vector<char> bytes{std::istreambuf_iterator<char>{input}, {}};
        if (bytes.empty() || input.bad()) {
            std::cerr << "Telemetry file is empty or incomplete\n";
            return 1;
        }
        const auto* first = reinterpret_cast<const std::byte*>(bytes.data());
        std::vector<std::byte> image{first, first + bytes.size()};
        auto reader = sub0log::SegmentReader::open(image);
        sub0log::Decoder decoder;
        if (!reader.valid()) {
            std::cerr << "Telemetry segment could not be opened\n";
            return 1;
        }
        const auto records = decoder.decodeAll(reader);
        valid = records.size() == 1 && reader.unreadableBytes() == 0
            && decoder.undecodableRecords() == 0
            && sub0log::Decoder::format(records.front()).find("25") != std::string::npos;
    }
    std::filesystem::remove_all(directory);
    return valid ? 0 : 1;
}
