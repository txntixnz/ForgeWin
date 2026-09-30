# ForgeWin P2 — compiled Windows C program

A from-scratch experimental x64 Windows execution core for iPhone / iOS 16.0. No UTM. **Not game compatible.**

## Install

Open Actions, choose the latest successful **ForgeWin P2 iOS 16 IPA** run, download **ForgeWin_P2_iOS16_IPA**, extract and update through TrollStore. Tap **Run compiled Windows C test** and share the diagnostic log.

Expected: `ExitProcess code=251` and `ForgeWin P2: compiled C program OK`.

## What this milestone checks

`guest/compiled251.c` is compiled by Microsoft Visual C on a Windows GitHub runner into an actual AMD64 Windows PE. The C source uses volatile array input, a non-inlined function, a loop, signed multiplication, a branch, debug output and process exit. No machine-code generator creates this test. The earlier Python-generated fixtures remain as regression tests.

The CI workflow runs this EXE natively on Windows, requires exit code 251, records its SHA-256, then downloads the exact file to the macOS job. ForgeWin must execute those same bytes and match both exit code and debug output before packaging the EXE inside the IPA. Source, compiler assembly listing and disassembly are in the `compiled-windows-guest` artifact; the execution report is in `P2-emulator-report`.

P2 adds signed IMUL (32/64-bit register/memory and immediate forms), MOVSXD with REX.W, INC/DEC with carry preservation, TEST, accumulator ADD/SUB/CMP and multi-byte NOP decoding. Tests check multiplication overflow and earlier P0/P1 functionality. Only these supported widths/forms are implemented.

## Status and limits

P0 and P1 passed on the user's iPhone running iOS 16.0. P2 needs its own device test. See Actions for current build evidence.

This is a narrow, no-CRT C test. It does not demonstrate general Windows application compatibility. No real Windows DLLs are loaded: only three KERNEL32 prototype shims exist (ExitProcess, OutputDebugStringA, GetTickCount64). The clock reports virtual guest elapsed milliseconds. No graphics, audio, DirectX, Vulkan, JIT, SSE/AVX, TLS, Windows threads, exceptions or C runtime. Fallout 4, Cyberpunk 2077 and RDR2 remain unsupported.

## Build

GitHub Actions handles both Windows and macOS stages. For local testing, run `bash tools/test.sh` (or `ASAN_OPTIONS=detect_leaks=0 bash tools/test.sh` in hosts with restricted leak inspection). To build locally on a Mac, first place the native-verified `compiled251.exe` and `compiled251-native.json` from the Windows artifact into `tests/`, run portable tests, then `bash tools/build-ios.sh`.

References: [Microsoft x64 ABI](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention), [Microsoft PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
