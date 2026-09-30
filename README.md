# ForgeWin P1 — memory and Windows API milestone

Targets iPhone 13 Pro Max / iOS 16.0. From-scratch interpreter; no UTM.

## Install/test this version

Download the latest successful **ForgeWin_P1_iOS16_IPA** artifact from this repository's Actions tab. Extract it, update the existing app through TrollStore, then tap **Run memory + Windows API test**. Share its diagnostic log.

Expected: `ExitProcess code=71` and `ForgeWin P1: memory + Windows imports OK`.

## Added in P1

- 64-bit ModRM/SIB addressing with displacement, index scale, REX base/index extensions and RIP-relative references for supported 32/64-bit operations.
- Memory forms of MOV, ADD, SUB, XOR, CMP; LEA; C7 immediate moves; FF indirect call/jump and push.
- Bounded PE import descriptor/thunk/name parsing and IAT binding.
- Three KERNEL32.dll **prototype shims**: ExitProcess stops the guest and records its code; OutputDebugStringA captures bounded text in the report; GetTickCount64 reports monotonic milliseconds since this guest execution was created (virtual boot). This is not host Windows uptime or a full Windows service implementation.
- API entry validates x64 stack alignment. No real Windows DLL is loaded.
- New generated executable uses an indexed array sum, stack locals, RIP-relative addressing and all three imports. Its expected exit code is 71. Original sum55 test remains in the regression suite.

P0 was successfully compiled on GitHub and its sum55 fixture passed on the user's iPhone running iOS 16.0. P1 requires a new physical-device test. See Actions for its current build status.

Still absent: most Windows APIs, real DLL loading, TLS, exceptions, threads, most x64 instruction families (including SSE/AVX), graphics, DirectX/Vulkan translation, audio and JIT. **Fallout 4, Cyberpunk and RDR2 cannot run.** These remain research goals, not promised compatibility.

Run `bash tools/test.sh` for portable sanitizer-backed tests. In restricted Linux hosts use `ASAN_OPTIONS=detect_leaks=0 bash tools/test.sh`. Build the IPA on a Mac with Xcode using `bash tools/build-ios.sh`; GitHub Actions runs both steps automatically on pushes to main/master. Test executables are generated from source during every build.

Source references: [PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [x64 ABI](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention), [GetTickCount64](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64).

