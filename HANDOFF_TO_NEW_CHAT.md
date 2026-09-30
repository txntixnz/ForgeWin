# ForgeWin P2 handoff

Repo txntixnz/ForgeWin, authorized direct pushes and Actions builds. Use GitHub connector, do not request user to upload files manually. User prefers brief replies.

P0 phone validation: 55/33 instructions. P1 phone validation: exit 71, memory + Windows imports OK, 29 steps. Current next test P2: MSVC-compiled C program, expected exit251 and "ForgeWin P2: compiled C program OK". Native Windows execution plus SHA-identical emulation gates IPA creation. Check current Actions run for build success and P2 device log for hardware status.

Windows guest source guest/compiled251.c; tools/build-guest.cmd uses installed Visual Studio; workflow has windows-guest then macOS build job. Upload includes compiler assembly and disassembly. No CRT, only KERNEL32 ExitProcess/OutputDebugStringA imports. Core still very incomplete: no graphics/JIT/SSE/AVX/real DLLs/general WinAPI. Do not overclaim gaming readiness.

Version 0.0.3/build3, same bundle ID. tools/check-compiled.py validates SHA and native/emulated exit+debug message before packaging. New instruction support: signed IMUL, MOVSXD REX.W, TEST, INC/DEC (CF preserved), accumulator immediate forms, multi-byte NOP. Prior tests retained.
