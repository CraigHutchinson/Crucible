# Bounded Filament UI adaptation

The ImGui vertex layout, textured premultiplied material, orthographic camera,
bottom-origin scissor conversion and primitive blend ordering derive from
Filament v1.77.3 `libs/filagui/src/ImGuiHelper.cpp` and
`libs/filagui/src/materials/uiblit.mat`, exact source
`d852e34cd5629a6f3851d613dcd17aa0ffd025a2`.
Copyright (C) 2018 The Android Open Source Project, Apache License 2.0; the
complete license is retained beside this file as `LICENSE.filament`.
The linked bundled ImGui 1.92.5 retains its upstream MIT license in the pinned
dependency source and must be included in root's packaged notices.

The concrete renderer preserves root's ImGui context, theme and ini settings;
it sets only its graphics binding/name/flags and maximum texture dimensions,
then restores those fields at teardown. It does not call ImGui NewFrame/Render,
load fonts or destroy the context. It uses one aggregate preallocated VB/IB pair
per owned slot instead of upstream's per-list allocation. Private uint16 index
rebasing includes command VtxOffset; malformed nonsequential/overlapping command
index ranges and custom callbacks are rejected before native frame acceptance.
The root's single viewport uses DisplayPos zero. Draw data remains immutable
except for the documented ImTextureData backend status/ID operations.

Dynamic RGBA textures are copied to the single bounded upload batch before each
descriptor is submitted. A compact integer ImTextureID maps only to one of four
owned records; arbitrary texture pointers/IDs are refused. Destroyed records
remain charged to residency until qualified drain. Scene/UI upload callbacks
release only their stable slot counters. No callback touches application, ImGui,
Runtime or ECS state. Engine teardown completes before descriptor/mesh/capture
storage is destroyed, including a partial startup failure.

Native transform preflight also evaluates the exact pinned instance-normal
cofactor/prescale path. It rejects tiny positive transforms with zero/subnormal
squared-normal denominators instead of clamping scale or uploading NaNs; portable
packet validation alone still accepts positive scale. The backend's received
Windows `/fp:fast` mode is not assumed to preserve subnormal arithmetic.

This is initial source authoring. Material compilation, renderer compilation,
native float-atlas support, full UI/capture/resize/teardown receiving and appearance
review have not yet passed. Source bounds do not establish allocation-free
Filament or native hardware timing. The pinned MSVC fatal Panic limitation and
unverified GPU-loss/scanout remain as recorded in the framework qualification.
