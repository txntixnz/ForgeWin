@echo off
setlocal
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%i"
if not defined VS_INSTALL exit /b 1
call "%VS_INSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist build\guest mkdir build\guest
cl /nologo /TC /Od /GS- /W4 /c guest\compiled251.c /Fobuild\guest\compiled251.obj /FAs /Fabuild\guest\compiled251.asm
if errorlevel 1 exit /b 1
link /nologo /machine:x64 /entry:mainCRTStartup /subsystem:console /nodefaultlib /incremental:no /out:build\guest\compiled251.exe build\guest\compiled251.obj kernel32.lib
if errorlevel 1 exit /b 1
dumpbin /headers /imports /disasm build\guest\compiled251.exe > build\guest\compiled251-disassembly.txt
exit /b %errorlevel%
