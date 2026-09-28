#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>

// SCS Telemetry SDK Return Codes
#define SCS_RESULT_OK 0
#define SCS_RESULT_UNSUPPORTED 1
#define SCS_RESULT_GENERIC_ERROR 2

typedef unsigned int scs_u32_t;
typedef scs_u32_t scs_result_t;

// Ford brand token in SCS 64-bit token format
constexpr uint64_t FORD_TOKEN = 0xC5A86ULL;

// Global state
static HMODULE g_hModule = NULL;
static FILE* g_logFile = NULL;

static void Log(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    OutputDebugStringA(buffer);

    char logPath[MAX_PATH];
    if (GetModuleFileNameA(g_hModule, logPath, MAX_PATH))
    {
        char* lastSlash = strrchr(logPath, '\\');
        if (lastSlash)
        {
            *(lastSlash + 1) = '\0';
            strcat_s(logPath, sizeof(logPath), "ford_fmax_plugin.log");
            FILE* f = NULL;
            if (fopen_s(&f, logPath, "a") == 0 && f)
            {
                SYSTEMTIME st;
                GetLocalTime(&st);
                fprintf(f, "[%02d:%02d:%02d.%03d] %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buffer);
                fclose(f);
            }
        }
    }
}

// Find memory pattern in a module's executable section
static uint8_t* FindPattern(uint8_t* base, size_t size, const uint8_t* pattern, size_t patternSize)
{
    if (!base || size < patternSize) return NULL;
    for (size_t i = 0; i <= size - patternSize; i++)
    {
        bool found = true;
        for (size_t j = 0; j < patternSize; j++)
        {
            if (base[i + j] != pattern[j])
            {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return NULL;
}

// Main background worker thread
static DWORD WINAPI PluginWorker(LPVOID lpParam)
{
    Log("[Ford F-MAX Plugin] Initializing Native Brand Bridge for ETS2 x64...\n");

    uint8_t* exeBase = (uint8_t*)GetModuleHandleA(NULL);
    if (!exeBase)
    {
        Log("[Ford F-MAX Plugin] ERROR: Failed to get base address of eurotrucks2.exe\n");
        return 1;
    }

    PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)exeBase;
    PIMAGE_NT_HEADERS ntHdrs = (PIMAGE_NT_HEADERS)(exeBase + dosHdr->e_lfanew);
    PIMAGE_SECTION_HEADER secHdr = IMAGE_FIRST_SECTION(ntHdrs);

    uint8_t* textStart = NULL;
    size_t textSize = 0;

    for (WORD i = 0; i < ntHdrs->FileHeader.NumberOfSections; i++)
    {
        if (memcmp(secHdr[i].Name, ".text", 5) == 0)
        {
            textStart = exeBase + secHdr[i].VirtualAddress;
            textSize = secHdr[i].Misc.VirtualSize;
            break;
        }
    }

    if (!textStart || textSize == 0)
    {
        Log("[Ford F-MAX Plugin] WARNING: Could not isolate .text, falling back to full image\n");
        textStart = exeBase;
        textSize = ntHdrs->OptionalHeader.SizeOfImage;
    }

    // Pattern for verify_dealer_brands prune loop entry:
    // 48 8B BB 40 40 00 00  - mov rdi, qword ptr [rbx + 0x4040]
    // 48 8B 8B 48 40 00 00  - mov rcx, qword ptr [rbx + 0x4048]
    const uint8_t sigPrune[] = {
        0x48, 0x8B, 0xBB, 0x40, 0x40, 0x00, 0x00,
        0x48, 0x8B, 0x8B, 0x48, 0x40, 0x00, 0x00
    };

    uint8_t* pruneEntry = FindPattern(textStart, textSize, sigPrune, sizeof(sigPrune));
    bool patchApplied = false;

    if (pruneEntry)
    {
        Log("[Ford F-MAX Plugin] Located verify_dealer_brands prune loop at %p (RVA 0x%IX)\n",
            pruneEntry, pruneEntry - exeBase);

        // Scan forward for cmp rdi, rcx (48 3B F9) followed by je (0F 84)
        for (int offset = 0; offset < 48; offset++)
        {
            if (pruneEntry[offset] == 0x48 &&
                pruneEntry[offset + 1] == 0x3B &&
                pruneEntry[offset + 2] == 0xF9 &&
                pruneEntry[offset + 3] == 0x0F &&
                pruneEntry[offset + 4] == 0x84)
            {
                uint8_t* jeInstruction = pruneEntry + offset + 3;
                int32_t origDisp = *(int32_t*)(jeInstruction + 2);

                // Target address is jeInstruction + 6 + origDisp
                // Unconditional jmp (E9 rel32) is 5 bytes. Target = jeInstruction + 5 + rel32
                // Therefore rel32 = origDisp + 1
                int32_t newDisp = origDisp + 1;

                DWORD oldProtect;
                if (VirtualProtect(jeInstruction, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
                {
                    jeInstruction[0] = 0xE9; // jmp rel32
                    *(int32_t*)(jeInstruction + 1) = newDisp;
                    jeInstruction[5] = 0x90; // nop padding

                    VirtualProtect(jeInstruction, 6, oldProtect, &oldProtect);
                    FlushInstructionCache(GetCurrentProcess(), jeInstruction, 6);

                    Log("[Ford F-MAX Plugin] SUCCESS: Patched prune check at %p -> bypassed vector::erase loop (disp: 0x%X -> 0x%X)\n",
                        jeInstruction, origDisp, newDisp);
                    patchApplied = true;
                }
                else
                {
                    Log("[Ford F-MAX Plugin] ERROR: VirtualProtect failed to set RWX at %p\n", jeInstruction);
                }
                break;
            }
        }
    }
    else
    {
        Log("[Ford F-MAX Plugin] WARNING: verify_dealer_brands pattern not found in memory.\n");
    }

    // Secondary Watchdog Loop:
    // Ensures that Ford is actively present in the brand vector at all times,
    // protecting against profile loads or reload cycles.
    // Economy/World manager global pointer offset in 1.61: 0x36AE6D8
    uintptr_t* pGlobalManager = (uintptr_t*)(exeBase + 0x36AE6D8);

    Log("[Ford F-MAX Plugin] Starting active brand guardian loop...\n");

    int consecutiveFound = 0;
    while (true)
    {
        Sleep(1000);

        __try
        {
            if (pGlobalManager && *pGlobalManager)
            {
                uint8_t* mainObj = (uint8_t*)(*pGlobalManager);
                uint64_t* brandCount = (uint64_t*)(mainObj + 0x4048);
                uint64_t** brandData = (uint64_t**)(mainObj + 0x4040);
                uint64_t* brandCapacity = (uint64_t*)(mainObj + 0x4050);

                if (brandCount && brandData && *brandData && *brandCount > 0 && *brandCount < 32)
                {
                    bool hasFord = false;
                    for (uint64_t i = 0; i < *brandCount; i++)
                    {
                        if ((*brandData)[i] == FORD_TOKEN)
                        {
                            hasFord = true;
                            break;
                        }
                    }

                    if (!hasFord)
                    {
                        if (*brandCount < *brandCapacity)
                        {
                            (*brandData)[*brandCount] = FORD_TOKEN;
                            (*brandCount)++;
                            Log("[Ford F-MAX Plugin] GUARDIAN: Injected Ford brand token (total brands: %llu)\n", *brandCount);
                        }
                        else
                        {
                            Log("[Ford F-MAX Plugin] GUARDIAN WARNING: Brand vector at capacity (%llu)\n", *brandCapacity);
                        }
                    }
                    else
                    {
                        consecutiveFound++;
                        if (consecutiveFound == 1)
                        {
                            Log("[Ford F-MAX Plugin] Verified: Ford brand token (0xC5A86) is ACTIVE in memory.\n");
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Safeguard against memory access during game shutdown or world transitions
        }
    }

    return 0;
}

// SCS Official Telemetry API Exports
extern "C"
{
    __declspec(dllexport) scs_result_t scs_telemetry_init(const scs_u32_t version, const void* const params)
    {
        Log("[Ford F-MAX Plugin] scs_telemetry_init invoked by engine (version %u)\n", version);
        return SCS_RESULT_OK;
    }

    __declspec(dllexport) void scs_telemetry_shutdown(void)
    {
        Log("[Ford F-MAX Plugin] scs_telemetry_shutdown invoked by engine\n");
    }
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PluginWorker, NULL, 0, NULL);
    }
    return TRUE;
}
