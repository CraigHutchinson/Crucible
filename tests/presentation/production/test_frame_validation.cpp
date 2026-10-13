#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "crucible/presentation/production/detail/frame_validation.hpp"

namespace
{
using namespace crucible::presentation::production;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void receivePacketBounds()
{
    std::vector<NanitePose> poses(CinematicFrame::maxInstances + 1);
    CinematicFrame frame;
    require(detail::isValidFrame(frame), "Empty decorative scene retains a valid camera");
    frame.poses_ = std::span{poses}.first(CinematicFrame::maxInstances);
    require(detail::isValidFrame(frame), "Exact maximum complete packet is accepted");
    frame.poses_ = poses;
    require(!detail::isValidFrame(frame), "One extra instance rejects the whole packet");
    frame.poses_ = std::span{poses}.first(1);
    poses[0].position_ = {-512, 512, 512};
    poses[0].scale_ = 16;
    poses[0].coreIntensity_ = 4;
    frame.keyIntensity_ = 8;
    frame.glowIntensity_ = 4;
    require(detail::isValidFrame(frame), "Inclusive position, scale and light maxima are accepted");
    poses[0].position_[0] = std::nextafter(-512.0F, -std::numeric_limits<float>::infinity());
    require(!detail::isValidFrame(frame), "Position just outside the world bound is refused");
    poses[0] = {};
    for (float scale : {0.0F, -1.0F, 17.0F, std::numeric_limits<float>::quiet_NaN()})
    {
        poses[0].scale_ = scale;
        require(!detail::isValidFrame(frame), "Invalid scale never reaches a native transform");
    }
    poses[0] = {};
    poses[0].orientation_ = {0, 0, 0, 0};
    require(!detail::isValidFrame(frame), "Zero quaternion is refused");
    poses[0].orientation_ = {0, 0, 0, -1};
    require(detail::isValidFrame(frame), "Quaternion sign-equivalent orientation is accepted");
    poses[0].orientation_[3] = 1.01F;
    require(!detail::isValidFrame(frame), "Nonunit quaternion is refused without renormalizing caller data");
    poses[0] = {};
    poses[0].coreIntensity_ = -1;
    require(!detail::isValidFrame(frame), "Negative per-instance intensity is refused");
    poses[0] = {};
    frame.glowIntensity_ = std::numeric_limits<float>::infinity();
    require(!detail::isValidFrame(frame), "Infinite global glow is refused");
}

void receiveCameraDegeneracy()
{
    CinematicFrame frame;
    frame.camera_.eye_ = frame.camera_.target_;
    require(!detail::isValidFrame(frame), "Coincident camera eye and target are refused");
    frame.camera_.eye_ = {0, 2, 0};
    require(!detail::isValidFrame(frame), "Vertical-only camera cannot build a lookAt basis");
    frame.camera_.eye_ = {0.001F, 2, 0};
    require(!detail::isValidFrame(frame), "Near-vertical camera below the normalized cross bound is refused");
    frame.camera_.eye_ = {0.1F, 2, 0};
    require(detail::isValidFrame(frame), "Separated nonvertical camera is accepted");
    frame.camera_.eye_ = {0.01F, 0, 0};
    require(!detail::isValidFrame(frame), "Too-short sight line is refused");
    frame.camera_.eye_ = {0.2F, 0, 0};
    for (float fov : {0.1F, 2.8F, std::numeric_limits<float>::quiet_NaN()})
    {
        frame.camera_.verticalFovRadians_ = fov;
        require(!detail::isValidFrame(frame), "Open FOV bounds and nonfinite values are refused");
    }
    frame.camera_.verticalFovRadians_ = 0.75F;
    frame.camera_.target_[2] = 513;
    require(!detail::isValidFrame(frame), "Out-of-bound camera target is refused");
}
}

int main()
{
    try
    {
        receivePacketBounds();
        receiveCameraDegeneracy();
        std::cout << "Complete scene packet and camera boundary receiving PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
