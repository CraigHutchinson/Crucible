#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "crucible/presentation/production/detail/mesh_validation.hpp"

namespace
{
using namespace crucible::presentation::production;
void require(bool valid, const char* message)
{
    if (!valid) throw std::runtime_error(message);
}
void checkMesh()
{
    std::array<NaniteMeshVertex, 3> triangle{{
        {{0, 0, 0}, {0, -1, 0}}, {{1, 0, 0}, {0, -1, 0}}, {{0, 0, 1}, {0, -1, 0}}}};
    NaniteMeshFrame frame{triangle};
    const auto original = triangle;
    require(detail::isValidFrame(frame), "Outward triangle rejected");
    for (std::uint32_t role = 0; role <= 3; ++role)
    {
        for (auto& vertex : triangle) { vertex.role_ = static_cast<NaniteMaterialRole>(role); vertex.intensity_ = 4; }
        require(detail::isValidFrame(frame), "Valid role/intensity rejected");
    }
    triangle = original;
    triangle[1].role_ = NaniteMaterialRole::core;
    require(!detail::isValidFrame(frame), "Mixed triangle role accepted");
    for (auto& vertex : triangle) vertex.role_ = static_cast<NaniteMaterialRole>(4);
    require(!detail::isValidFrame(frame), "Unknown role accepted");
    triangle = original;
    triangle[1].intensity_ = 0.5F;
    require(!detail::isValidFrame(frame), "Mixed triangle intensity accepted");
    for (float invalid : {-1.0F, 4.1F, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
    {
        triangle = original;
        for (auto& vertex : triangle) vertex.intensity_ = invalid;
        require(!detail::isValidFrame(frame), "Invalid intensity accepted");
    }
    triangle = original;
    triangle[0].position_[0] = 512.01F;
    require(!detail::isValidFrame(frame), "Unbounded coordinate accepted");
    triangle = original;
    triangle[0].normal_ = {0, 1, 0};
    require(!detail::isValidFrame(frame), "Inward normal accepted");
    triangle = original;
    triangle[0].normal_ = {0, -0.9F, 0};
    require(!detail::isValidFrame(frame), "Nonunit normal accepted");
    triangle[0].normal_ = {std::numeric_limits<float>::quiet_NaN(), -1, 0};
    require(!detail::isValidFrame(frame), "Nonfinite normal accepted");
    triangle = original;
    triangle[1].position_[0] = 1e-5F;
    triangle[2].position_[2] = 1e-5F;
    require(!detail::isValidFrame(frame), "Near-degenerate triangle accepted");
    triangle = original;
    triangle[2].position_ = triangle[1].position_;
    require(!detail::isValidFrame(frame), "Degenerate triangle accepted");
    triangle = original;
    frame.vertices_ = std::span<const NaniteMeshVertex>{triangle.data(), 2};
    require(!detail::isValidFrame(frame), "Incomplete triangle accepted");
    std::vector<NaniteMeshVertex> maximum(NaniteMeshFrame::maxVertices);
    for (std::size_t index = 0; index < maximum.size(); ++index) maximum[index] = original[index % 3];
    frame.vertices_ = maximum;
    require(detail::isValidFrame(frame), "Maximum bounded mesh rejected");
    maximum.insert(maximum.end(), original.begin(), original.end());
    frame.vertices_ = maximum;
    require(!detail::isValidFrame(frame), "Oversized mesh accepted");
    frame.vertices_ = {};
    frame.camera_.orthographic_ = CinematicCamera::OrthographicExtent{1024, 32768};
    require(detail::isValidFrame(frame), "Maximum orthographic extent rejected");
    frame.camera_.orthographic_->halfHeight_ = 2852;
    require(detail::isValidFrame(frame), "Narrow aspect orthographic extent rejected");
    frame.camera_.orthographic_->halfWidth_ = 0.01F;
    require(!detail::isValidFrame(frame), "Tiny orthographic extent accepted");
    frame.camera_.orthographic_->halfWidth_ = std::numeric_limits<float>::infinity();
    require(!detail::isValidFrame(frame), "Nonfinite orthographic extent accepted");
}
}
int main()
{
    try { checkMesh(); std::cout << "Mesh packet bounds, roles, normals and winding PASS\n"; return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
