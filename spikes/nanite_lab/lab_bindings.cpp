// C exports of the production nanite generator for the WebAssembly lab (lab.html).
// The lab rebuilds the mesh on every change; buffers stay valid until the next build.
#include <emscripten/emscripten.h>

#include "crucible/presentation/production/nanite_model.hpp"

namespace
{
using namespace crucible::presentation::production;
NaniteMesh mesh;
std::vector<float> interleaved; // x y z nx ny nz material
}

extern "C"
{
/// Builds one unit; returns its vertex count. Read with labVertices/labIndices/labIndexCount.
EMSCRIPTEN_KEEPALIVE int labBuild(int structure, int surface, float stretch, float open, float heartTurn, unsigned variant)
{
    mesh = buildNanite({static_cast<NaniteStructure>(structure & 3), static_cast<NaniteSurface>(surface & 3)},
        {stretch, open, heartTurn}, variant);
    interleaved.clear();
    for (const auto& vertex : mesh.vertices_)
    {
        interleaved.insert(interleaved.end(), {vertex.position_[0], vertex.position_[1], vertex.position_[2],
            vertex.normal_[0], vertex.normal_[1], vertex.normal_[2], static_cast<float>(vertex.material_)});
    }
    return static_cast<int>(mesh.vertices_.size());
}
EMSCRIPTEN_KEEPALIVE const float* labVertices() { return interleaved.data(); }
EMSCRIPTEN_KEEPALIVE const std::uint32_t* labIndices() { return mesh.indices_.data(); }
EMSCRIPTEN_KEEPALIVE int labIndexCount() { return static_cast<int>(mesh.indices_.size()); }
EMSCRIPTEN_KEEPALIVE int labPresetCount() { return static_cast<int>(naniteDevicePresets().size()); }
/// Preset field: 0 hero structure, 1 hero surface, 2 swarm structure, 3 swarm surface.
EMSCRIPTEN_KEEPALIVE int labPreset(int index, int field)
{
    const auto& preset = naniteDevicePresets()[index];
    const NaniteDetail detail = field < 2 ? preset.hero_ : preset.swarm_;
    return field % 2 == 0 ? static_cast<int>(detail.structure_) : static_cast<int>(detail.surface_);
}
EMSCRIPTEN_KEEPALIVE const char* labPresetName(int index) { return naniteDevicePresets()[index].name_.data(); }
}
