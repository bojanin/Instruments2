# bandicoot

requirements:
- clang (ALL versions that have tsan support)
- spdlog 
  - MAC:
  - `brew install spdlog`
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

Build the captain_hook, the shared lib that hooks into the tsan runtime and communicates back to bandicoot
`ninja -C build -v captain_hook -j12`

Build Bandicoot:
`cmake --build build --target bandicoot`


Build TinyRace:
`clang++ -std=c++23 -fno-omit-frame-pointer captain_hook/tiny_race.cc -fsanitize=thread -g -O1 -o captain_hook/tiny_race`


Test your hooks are exported correctly:

MacOS:
`cmake --build build --target captain_hook && DYLD_INSERT_LIBRARIES=build/captain_hook/libcaptain_hook.dylib ./captain_hook/tiny_race`

Linux:
`cmake --build build --target captain_hook && LD_PRELOAD=build/captain_hook/libcaptain_hook.so ./captain_hook/tiny_race`

How to standalone compile captain_hook:
`clang++ -std=c++23 -g -fsanitize=thread -shared -fPIC -o libtsan_interceptor.so tsan_ipc.cc`
