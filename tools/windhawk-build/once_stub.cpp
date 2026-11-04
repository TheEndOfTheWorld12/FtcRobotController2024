// Ubuntu's mingw-w64 libstdc++ declares these two thread-locals for
// std::call_once but never defines them for the win32 threading model, so a
// link here fails on them. Nothing to do with the mod: Windhawk's own
// toolchain has a runtime that defines them. Defined only to let the link
// stage run as an extra check.
namespace std {
__thread void* __once_callable = nullptr;
__thread void (*__once_call)() = nullptr;
}
