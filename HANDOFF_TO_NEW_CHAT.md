# ForgeWin P1 handoff — 2026-09-30

Repo: txntixnz/ForgeWin. User authorized implementation, pushing and building. GitHub connector is available: use it directly. No need to ask for repo links or manual uploads.

P0 ran on the user's iPhone with iOS 16.0: sum55.exe returned RAX=55 in 33 instructions. P1 adds ModRM/SIB memory operands and bounded PE imports for three KERNEL32 prototype shims: ExitProcess, OutputDebugStringA, GetTickCount64. Timer epoch is virtual guest creation. Not full Windows semantics. No real DLL loading.

New fixture winapi71.exe sums array values 7,11,13,17,23 using indexed memory, stores/reloads stack locals, calls all three imports, emits "ForgeWin P1: memory + Windows imports OK", exits 71. Original fixture remains a regression. iOS button now launches new fixture; same bundle ID, version 0.0.2/build 2. Build generates both PE files and bundles them.

Portable tests cover old operations, memory access, sign extension, RIP-relative immediate semantics, API output/exit, unknown imports, malformed descriptors, budgets and 3000 malformed PE mutations. Check VALIDATION.txt and latest Actions status. Device verification of P1 is pending. Do not claim game compatibility, graphics, JIT, SSE/AVX, threads or full WinAPI. Next: obtain P1 phone log, expand instruction correctness, then an independently compiled minimal Windows program. User prefers brief messages and direct work.
