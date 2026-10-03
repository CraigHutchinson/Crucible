#pragma once
#include <cstddef>
#include <vector>
struct SDL_Renderer;
struct SDL_Vertex;
namespace crucible::presentation { class Camera2D; }
namespace crucible::spike {
class WorldPacket;
/** Concrete experiment consumer: expands instances only at the SDL adapter boundary.
 * Owns startup scratch; borrows camera/packet/renderer only during drawing.
 */
class PacketPainter {
public:
    /// Checks SDL int geometry limits before allocation; throws length_error/bad_alloc.
    explicit PacketPainter(std::size_t capacity);
    ~PacketPainter();
    PacketPainter(const PacketPainter&) = delete;
    PacketPainter& operator=(const PacketPainter&) = delete;
    /** Clears/draws only the world pass; caller owns presentation/flush.
     * @return False before drawing on packet/camera/capacity mismatch; SDL errors may partially draw.
     */
    [[nodiscard]] bool TryDraw(SDL_Renderer& renderer, const WorldPacket& packet,
                               const presentation::Camera2D& camera) noexcept;
private:
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;
};
}
