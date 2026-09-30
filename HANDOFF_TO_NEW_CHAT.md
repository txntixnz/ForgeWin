# ForgeWin P0 handoff — 2026-09-30

User: wants a from-scratch Windows emulator for their jailbroken iPhone 13 Pro Max on iOS 16.0, eventually targeting Fallout 4, Cyberpunk 2077 and RDR2. Explicitly said to forget UTM. User prefers concise updates and bundled ZIPs.

## Truthful current state

Portable C++17 subset x64 interpreter + restricted PE32+ loader implemented from scratch. A generated import-free Windows PE calculates sum(1..10)=55 in 33 guest instructions. Tests passed on Linux with address/undefined behavior sanitizers (leak detection disabled due to host /proc restrictions).

Objective-C++ UIKit app source targets iOS 16 ARM64, uses core/forge.hpp, runs bundled fixture/imported file in background, and exports logs. tools/build-ios.sh directly compiles and packages an ad-hoc signed IPA on macOS with Xcode. GitHub Actions workflow supplied. Neither Apple compilation nor physical-device execution has occurred. No IPA exists in the delivered package.

## Do not overclaim

This is not Windows, not game compatible, not a complete CPU emulator. No WinAPI, imported DLLs, general ModRM memory operations, SSE/AVX, graphics, audio, JIT, or Windows kernel. The UI states these limits. The fixture is real executable code, but no external Windows-built program has been validated. Returning from the entry point is a test harness convention.

## Files

- core/forge.hpp: memory, PE loader, interpreter and report generator.
- core/main.cpp: portable diagnostic CLI.
- tests/tests.cpp: arithmetic, calls, register widths/extensions, bounds, permissions, budget, malformed-file tests.
- tools/make_demo.py: reproducible AMD64 PE fixture generator.
- ios/main.mm + ios/Info.plist: app source.
- tools/build-ios.sh: macOS IPA build.
- .github/workflows/build-ios.yml: manually triggered/push build.
- VALIDATION.txt: actual Linux test output.

## Next action

Obtain a successful macOS build and iPhone log. Address compile/install errors concretely. Then expand instruction decoding with correctness tests and add a minimal Windows import layer. Do not claim that A15 tuning can overcome missing APIs or graphics support. Do not silently replace the user's from-scratch goal with UTM or streaming.
