# ForgeWin P0 — from-scratch Windows execution prototype

Target: iPhone 13 Pro Max, ARM64, iOS 16.0 or newer.

**This is source code for an early interpreter prototype, not a full Windows emulator or a game-ready IPA. Fallout 4, Cyberpunk 2077 and RDR2 do not run in P0.** No UTM, QEMU, Wine, or other emulator engine is included. The core and loader were written for this project.

## What actually works

The portable C++17 core loads a restricted AMD64 PE32+ executable, maps its sections at its preferred guest address, creates a guest stack and interprets a small subset of x86-64 instructions. The bundled import-free Windows executable sums 10 through 1, returning 55 after 33 instructions. This result is calculated by executing guest instructions; it is not hardcoded in the emulator.

The loader checks file and image bounds, section overlap, executable entry points and some unsupported startup features. The memory implementation distinguishes writable and executable regions. Unsupported opcodes and addressing modes stop with a register dump and the last 32 instruction addresses. A one-million-instruction budget prevents indefinite execution.

The iOS source includes buttons to run the bundled executable, import another file, and share diagnostics. Execution runs off the UI thread. This interpreter does not request JIT permissions or require a jailbreak to execute guest instructions. Installing its ad-hoc signed IPA requires a compatible installation method such as the user's existing TrollStore setup; this build has not been device-tested.

## Current boundaries

- Register arithmetic/moves: supported subset of MOV, XOR, ADD, SUB and CMP in 32/64-bit widths with REX extensions.
- Stack/control flow: PUSH/POP registers, relative CALL/JMP, conditional jumps, RET, NOP.
- Flags: CF, PF, ZF, SF and OF for implemented arithmetic. AF is not modeled.
- General ModRM memory addressing, floating point, SSE/AVX and most instructions: absent.
- Windows DLL imports, delay imports, TLS and managed executables: explicitly rejected.
- Windows services, kernel, desktop, processes/threads, filesystem APIs and exception handling: absent.
- DirectX/Vulkan-to-Metal translation, graphics rendering, audio and controller emulation: absent.
- No dynamic recompilation or A15-specific performance optimization yet.
- Maximum input file: 64 MiB; maximum mapped PE image: 32 MiB; guest stack: 1 MiB. These are prototype guards, not intended game limits.
- Preferred image base only; no relocation needed for the isolated fixture. Relocation processing is not implemented.
- A RET to the initial zero sentinel ends the test harness; this is not Windows process startup/exit behavior.

## Build an IPA using GitHub Actions

1. Create a new GitHub repository, for example `ForgeWin-iOS16`.
2. Extract this ZIP. Put the **contents of `ForgeWin_P0` at the repository root**: `core`, `ios`, `tests`, `tools`, and `.github` must be at the top level. Include the hidden `.github` directory. Do not upload only the ZIP.
3. Open **Actions → ForgeWin P0 iOS 16 IPA → Run workflow**. Pushing to `main` or `master` also triggers the workflow.
4. When the run succeeds, download its **ForgeWin_P0_iOS16_IPA** artifact and extract it.
5. Install `ForgeWin_P0_iOS16.ipa` using your existing TrollStore installation.
6. Open ForgeWin P0 and tap **Run built-in x64 test**. Expected output: `Guest entry returned RAX=55`, `instructions=33`.
7. Use **Share diagnostic log** to export the result.

The workflow uses a macOS runner with Apple's iPhoneOS SDK, builds the ARM64 app with an iOS 16.0 deployment target, and applies an ad-hoc signature. It does not require an Apple developer account to compile. No actual IPA was built in the current Linux environment, and the workflow and iOS interface have not been run on macOS/device yet. A successful GitHub build still needs installation and launch validation.

On a Mac with Xcode installed, run `bash tools/build-ios.sh` instead. The output will be `build/ForgeWin_P0_iOS16.ipa`.

## Portable core checks

From the project root, run:

```sh
bash tools/test.sh
```

Linux environments that prohibit LeakSanitizer's process inspection can use:

```sh
ASAN_OPTIONS=detect_leaks=0 bash tools/test.sh
```

AddressSanitizer and UndefinedBehaviorSanitizer remain enabled with that setting. It disables only leak detection. The CLI is then available as:

```sh
./build/forgewin tests/sum55.exe
```

## Practical next milestones

1. Compile and launch this app on the iPhone; confirm the same guest result.
2. Add tested x64 memory addressing and a broader instruction subset.
3. Add an import resolver and a minimal Windows ABI layer with small independently compiled programs as tests.
4. Establish a graphics test before adding a rendering translation layer.
5. Profile the interpreter and design ARM64 dynamic translation only after correctness tests exist.
6. Evaluate older/smaller games before attempting Fallout 4. Cyberpunk and RDR2 remain speculative long-term targets; no performance or compatibility is promised.

Writing a complete Windows environment and modern graphics stack from scratch is still a very large project. This package provides an executable starting point and honest diagnostics, not a completion claim.

## Format and platform references

- Microsoft PE format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
- Apple document picker: https://developer.apple.com/documentation/uikit/uidocumentpickerviewcontroller
- Intel architecture manuals (instruction behavior reference): https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html

No Windows binaries or commercial game files are included. `tests/sum55.exe` is a generated test fixture whose complete machine code and PE construction are in `tools/make_demo.py`.
