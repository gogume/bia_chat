# bia_chat

A cross-platform C++ socket chat and game server library.

## How to Build

### 1. Building for Mac / Linux (Native)
When building for Mac or Linux, the code automatically uses standard POSIX sockets.

**Compile the library object file:**
```bash
g++ -std=c++17 -c bia_chat.cpp -o bia_chat.o
```

---

### 2. Building for Windows (Cross-compiling from Mac)
To generate `.dll` and `.exe` files for Windows from your Mac, you must use the `MinGW-w64` cross-compiler.

**Compile the Windows DLL:**
We must link the `ws2_32` library to enable Winsock2 networking on Windows.
```bash
g++ -std=c++17 -shared ./lib/bia_chat.cpp -o bia_chat.dll -DBIA_CHAT_EXPORTS -lws2_32
```