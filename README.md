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




