#include <chrono>
#include <exception>
#include <iostream>
#include <string_view>

#include "crucible/runtime/InspectorSession.hpp"
#include "crucible/runtime/RuntimeDiagnostics.hpp"

int main(int argc, char** argv) {
    using namespace crucible;
    using namespace crucible::runtime;
    try {
        InspectorSession session{64, {1, 16}, ReclamationMissionSettings{1780, 4}, true,
            InspectorSession::ExecutionPath::integrated, InspectorSession::Diagnostics::bounded};
        if (!session.GetDiagnostics()) return 2;
        session.Pause();
        static_cast<void>(session.TryShatterRelay());
        const int attempts = argc == 2 && std::string_view{argv[1]} == "exhaust" ? 10000 : 1;
        for (int i = 0; i < attempts; ++i) static_cast<void>(session.TryFuseRelay());
        session.Resume();
        static_cast<void>(session.TryPump(std::chrono::seconds{1}));
        session.Close();
        return session.GetDiagnostics()->writeJson(std::cout, std::cerr) ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
