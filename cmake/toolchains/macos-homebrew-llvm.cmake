# Select the documented Homebrew LLVM21 compiler AND its libc++ runtime together.
# Apple SDK libc++ on the current runner does not provide Pipeline's stop_token/jthread.
set(crucible_llvm_root "$ENV{CRUCIBLE_LLVM_ROOT}")
if(NOT EXISTS "${crucible_llvm_root}/bin/clang++" OR
   NOT EXISTS "${crucible_llvm_root}/include/c++/v1/stop_token")
    message(FATAL_ERROR "Install llvm@21 and export CRUCIBLE_LLVM_ROOT=$(brew --prefix llvm@21).")
endif()
set(CMAKE_C_COMPILER "${crucible_llvm_root}/bin/clang")
set(CMAKE_CXX_COMPILER "${crucible_llvm_root}/bin/clang++")
set(CMAKE_OBJC_COMPILER "${crucible_llvm_root}/bin/clang")
set(CMAKE_OBJCXX_COMPILER "${crucible_llvm_root}/bin/clang++")
# Vendor-documented availability selection is valid only with the bundled runtime.
set(CMAKE_CXX_FLAGS_INIT "-nostdinc++ -isystem ${crucible_llvm_root}/include/c++/v1 -D_LIBCPP_DISABLE_AVAILABILITY")
set(CMAKE_OBJCXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
set(crucible_llvm_link "-L${crucible_llvm_root}/lib/c++ -Wl,-rpath,${crucible_llvm_root}/lib/c++ -L${crucible_llvm_root}/lib/unwind -Wl,-rpath,${crucible_llvm_root}/lib/unwind -lunwind")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${crucible_llvm_link}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${crucible_llvm_link}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${crucible_llvm_link}")
