```
mkdir b
cd b
cmake -DLIBHERMIT_ROOT_DIR=libhermit_dir -DWAMR_ROOT_DIR=wasm-micro-runtime_dir -DWAMR_BUILD_PLATFORM=libhermit -DWAMR_BUILD_INTERP=1 ..
make
```
