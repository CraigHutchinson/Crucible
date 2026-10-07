#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
namespace gpu_fault {
enum class Call { create, acquire, map, copy, render, submit, wait, query, count };
enum class Kind { shader, pipeline, buffer, transfer, texture, fence, command, count };
/** Stack-scoped controller borrowed by ELF wrappers in this executable only.
 * Caller keeps the borrowed device alive through renderer destruction. Real handles
 * are observed, never freed by the controller. Wrappers record creation/consumption
 * and map/unmap pairs; Balanced requires every tracked handle consumed exactly once.
 * For failed barriers, forbid_release guards before forwarding any release to SDL.
 * One controller is active on the device coordinator thread; no production hook.
 */
class Controller {
public:
    explicit Controller(SDL_GPUDevice& device);
    ~Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    void Arm(Call call, std::size_t ordinal = 1) noexcept;
    bool Hit(Call call) noexcept;
    void Created(void* handle, Kind kind) noexcept;
    void Released(void* handle, Kind kind) noexcept;
    void Mapped(void* handle) noexcept;
    void Unmapped(void* handle) noexcept;
    [[nodiscard]] bool Owns(void* handle, Kind kind) const noexcept;
    [[nodiscard]] bool Balanced() const noexcept;
    [[nodiscard]] std::size_t Calls(Call call) const noexcept;
    [[nodiscard]] std::size_t Releases() const noexcept { return releases_; }
    [[nodiscard]] std::size_t CreatedCount() const noexcept { return created_; }
    [[nodiscard]] bool Triggered() const noexcept { return triggered_; }
    [[nodiscard]] bool Good() const noexcept { return good_; }
    [[nodiscard]] SDL_GPUDevice* Device() const noexcept { return device_; }
    [[nodiscard]] bool PhysicalWait() noexcept;
    bool hold_queries{};
    SDL_GPUFence* visible_fence{}; // Borrowed selected real fence; null withholds all.
    bool hang_wait{};
    bool forbid_release{};
    std::array<SDL_GPUFence*, 8> submitted_fences{}; // Borrowed observation only.
    std::size_t submission_count{}, canceled_count{}, successful_waits{};
private:
    struct Handle { void* pointer{}; Kind kind{}; bool mapped{}; };
    SDL_GPUDevice* device_; // Borrowed; caller keeps device alive beyond controller.
    std::array<Handle, 64> handles_{};
    std::array<std::size_t, static_cast<std::size_t>(Call::count)> calls_{};
    Call armed_{Call::count};
    std::size_t ordinal_{}, seen_{}, releases_{}, created_{};
    bool triggered_{}, good_{true};
};
Controller* Active() noexcept;
}
