#!/usr/bin/env python3
import pathlib, subprocess, hashlib, json
root=pathlib.Path(__file__).resolve().parents[1]
exe=root/'tests'/'compiled251.exe'
manifest=json.loads((root/'tests'/'compiled251-native.json').read_text(encoding='utf-8-sig'))
actual=hashlib.sha256(exe.read_bytes()).hexdigest()
if actual.lower()!=manifest['sha256'].lower() or manifest['native_exit_code']!=251:
    raise SystemExit('Native executable validation/identity mismatch')
result=subprocess.run([str(root/'build'/'forgewin'),str(exe)],capture_output=True,text=True)
print(result.stdout,flush=True)
(root/'build'/'compiled251-emulated.txt').write_text(result.stdout+result.stderr)
if result.returncode or 'ExitProcess code=251\n' not in result.stdout or 'Guest debug output: ForgeWin P2: compiled C program OK\n' not in result.stdout:
    raise SystemExit('Compiled C guest execution failed; inspect report and disassembly')
print('PASS: identical MSVC-built PE exited 251 on Windows and in ForgeWin; SHA-256 '+actual)
