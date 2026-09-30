# Verified bootstrap; keep this version consistent across sub0 consumers.
include_guard(GLOBAL)
set(CPM_DOWNLOAD_VERSION 0.42.1)
set(CPM_DOWNLOAD_SHA256 f3a6dcc6a04ce9e7f51a127307fa4f699fb2bade357a8eb4c5b45df76e1dc6a5)
set(_cpm_file "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS "${_cpm_file}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${_cpm_file}" EXPECTED_HASH SHA256=${CPM_DOWNLOAD_SHA256}
        TLS_VERIFY ON STATUS _cpm_status)
    list(GET _cpm_status 0 _cpm_code)
    if(NOT _cpm_code EQUAL 0)
        file(REMOVE "${_cpm_file}")
        message(FATAL_ERROR "CPM download failed: ${_cpm_status}")
    endif()
endif()
file(SHA256 "${_cpm_file}" _cpm_hash)
if(NOT _cpm_hash STREQUAL CPM_DOWNLOAD_SHA256)
    message(FATAL_ERROR "CPM checksum mismatch: remove ${_cpm_file} and configure again.")
endif()
include("${_cpm_file}")
