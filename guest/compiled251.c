/* Compiled by MSVC as a real AMD64 Windows PE, without the C runtime.
   volatile input and noinline prevent constant folding of the calculation. */
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *text);
__declspec(dllimport) __declspec(noreturn) void __stdcall ExitProcess(unsigned code);
static volatile int values[] = {7, 11, 13, 17, 23};
__declspec(noinline) int weighted_sum(volatile int *array, int count) {
    int sum = 0;
    int i;
    for (i = 0; i < count; ++i) sum += array[i] * (i + 1);
    return sum;
}
void mainCRTStartup(void) {
    int result = weighted_sum(values, 5);
    if (result == 251) OutputDebugStringA("ForgeWin P2: compiled C program OK");
    else OutputDebugStringA("ForgeWin P2: calculation FAILED");
    ExitProcess((unsigned)result);
}
