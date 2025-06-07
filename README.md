# bandicoot

What we override that powers this.
DoReportFunc
```
tsan_rtl_report.cpp:45:1:SANITIZER_WEAK_DEFAULT_IMPL
```


Run in order:

Configure in release mode
`cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`

Build the librt, the shared lib that hooks into the tsan runtime and communicates back to bandicoot
`ninja -C build -v librt -j12`


Build Bandicoot:


Build TinyRace:
`clang++ librt/tiny_race.cc -fsanitize=thread -g -O1 -o librt/tiny_race`


Test your hooks are exported correctly:

MacOS:
`cmake --build build --target librt && DYLD_INSERT_LIBRARIES=build/librt/liblibrt.dylib DYLD_FORCE_FLAT_NAMESPACE=y ./librt/tiny_race`
Linux:
`cmake --build build --target librt && LD_PRELOAD=build/librt/liblibrt.so ./librt/tiny_race`




