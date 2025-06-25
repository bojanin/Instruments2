# Instruments2

Instruments2 is a GUI interface around clang sanitizers inspired by Xcode & Apple Instruments.


# Roadmap
[x] TSAN
[] ASAN
[] UBSAN
[] MEMSAN
[] LSAN
[] DFSAN
[] RTSAN


requirements:
- clang (ALL versions that have tsan support)
- spdlog 
  - MAC:
  - `brew install spdlog`
  - `brew install llvm`
  - `brew install protobuf`
    - must be > 31, if not `brew --reinstall-from-source protobuf`
  - ensure llvm in path
  - Linux:
  - `sudo apt install libspdlog-dev`

What we override that powers this.
DoReportFunc
```
tsan_rtl_report.cpp:45:1:SANITIZER_WEAK_DEFAULT_IMPL
```

Run in order:

Configure in release mode
`cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`

Build the captain_hook, the shared lib that hooks into the tsan runtime and communicates back to
instruments2
`ninja -C build -v captain_hook -j12`

Build Instruments2:
`cmake --build build --target instruments2`

Build TinyRace:
`clang++ -std=c++23 captain_hook/tiny_race.cc -fsanitize=thread -g -O2 -o captain_hook/tiny_race`
- Recommend `-O2`.Stack traces are hard to reach otherwise

Test your hooks are exported correctly:

MacOS:
`cmake --build build --target captain_hook -j12 && DYLD_INSERT_LIBRARIES=build/captain_hook/libcaptain_hook.so ./captain_hook/tiny_race`

Linux:
`cmake --build build --target captain_hook && LD_PRELOAD=build/captain_hook/libcaptain_hook.so ./captain_hook/tiny_race`

How to standalone compile captain_hook:
`clang++ -std=c++23 -g -fsanitize=thread -shared -fPIC -o libcaptain_hook.so tsan.cc`

PBTypes:
`cmake --build build --target pbtypes -j12`

