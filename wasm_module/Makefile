sources = main.cpp timer.cpp
headers = timer.h log.h

wasi_sdk = /opt/wasi-sdk

CC  = $(wasi_sdk)/bin/clang
CXX = $(wasi_sdk)/bin/clang++

CXXFLAGS += --target=wasm32-wasi-threads

CXXFLAGS += -std=c++14
CXXFLAGS += -fno-exceptions -fno-rtti
CXXFLAGS += -ffunction-sections -fdata-sections

LDFLAGS += -pthread
LDFLAGS += -Wl,--import-memory -Wl,--export-memory
LDFLAGS += -Wl,--max-memory=1048576
LDFLAGS += -Wl,--gc-sections

# Release only
# CXXFLAGS += -Oz
# LDFLAGS += -Wl,--strip-all

# Debug only
CXXFLAGS += -O1 -g -fno-omit-frame-pointer
LDFLAGS += -g

module.wasm: $(sources) $(headers)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(sources)
