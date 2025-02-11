// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// Exit macros
#define DExitOnLastError(x, s, ...) ExitOnLastErrorSource(DUTIL_SOURCE_DUTIL, x, s, __VA_ARGS__)
#define DExitOnLastErrorDebugTrace(x, s, ...) ExitOnLastErrorDebugTraceSource(DUTIL_SOURCE_DUTIL, x, s, __VA_ARGS__)
#define DExitWithLastError(x, s, ...) ExitWithLastErrorSource(DUTIL_SOURCE_DUTIL, x, s, __VA_ARGS__)
#define DExitOnFailure(x, s, ...) ExitOnFailureSource(DUTIL_SOURCE_DUTIL, x, s, __VA_ARGS__)
#define DExitOnRootFailure(x, s, ...) ExitOnRootFailureSource(DUTIL_SOURCE_DUTIL, x, s, __VA_ARGS__)
#define DExitOnFailureDebugTrace(x, s, ...) ExitOnFailureDebugTraceSource(DUTIL_SOURCE_DUTIL, x, s, __VA_ARGS__)
#define DExitOnNull(p, x, e, s, ...) ExitOnNullSource(DUTIL_SOURCE_DUTIL, p, x, e, s, __VA_ARGS__)
#define DExitOnNullWithLastError(p, x, s, ...) ExitOnNullWithLastErrorSource(DUTIL_SOURCE_DUTIL, p, x, s, __VA_ARGS__)
#define DExitOnNullDebugTrace(p, x, e, s, ...)  ExitOnNullDebugTraceSource(DUTIL_SOURCE_DUTIL, p, x, e, s, __VA_ARGS__)
#define DExitOnInvalidHandleWithLastError(p, x, s, ...) ExitOnInvalidHandleWithLastErrorSource(DUTIL_SOURCE_DUTIL, p, x, s, __VA_ARGS__)
#define DExitOnWin32Error(e, x, s, ...) ExitOnWin32ErrorSource(DUTIL_SOURCE_DUTIL, e, x, s, __VA_ARGS__)
#define DExitOnGdipFailure(g, x, s, ...) ExitOnGdipFailureSource(DUTIL_SOURCE_DUTIL, g, x, s, __VA_ARGS__)

// No need for OACR to warn us about using non-unicode APIs in this file.
#pragma prefast(disable:25068)

// Asserts & Tracing

const int DUTIL_STRING_BUFFER = 1024;
static HMODULE Dutil_hAssertModule = NULL;
static DUTIL_ASSERTDISPLAYFUNCTION Dutil_pfnDisplayAssert = NULL;
static BOOL Dutil_fNoAsserts = FALSE;
static REPORT_LEVEL Dutil_rlCurrentTrace = REPORT_STANDARD;
static BOOL Dutil_fTraceFilenames = FALSE;
static DUTIL_CALLBACK_TRACEERROR vpfnTraceErrorCallback = NULL;

thread_local static DWORD vtdwSuppressTraceErrorSource = 0;


DAPI_(HRESULT) DutilInitialize(
    __in_opt DUTIL_CALLBACK_TRACEERROR pfnTraceErrorCallback
    )
{
    HRESULT hr = S_OK;

    vpfnTraceErrorCallback = pfnTraceErrorCallback;

    return hr;
}


DAPI_(void) DutilUninitialize()
{
    vpfnTraceErrorCallback = NULL;
}

DAPI_(BOOL) DutilSuppressTraceErrorSource()
{
    if (DWORD_MAX == vtdwSuppressTraceErrorSource)
    {
        return FALSE;
    }

    ++vtdwSuppressTraceErrorSource;
    return TRUE;
}

DAPI_(BOOL) DutilUnsuppressTraceErrorSource()
{
    if (0 == vtdwSuppressTraceErrorSource)
    {
        return FALSE;
    }

    --vtdwSuppressTraceErrorSource;
    return TRUE;
}

/*******************************************************************
Dutil_SetAssertModule

*******************************************************************/
extern "C" void DAPI Dutil_SetAssertModule(
    __in HMODULE hAssertModule
    )
{
    Dutil_hAssertModule = hAssertModule;
}


/*******************************************************************
Dutil_SetAssertDisplayFunction

*******************************************************************/
extern "C" void DAPI Dutil_SetAssertDisplayFunction(
    __in DUTIL_ASSERTDISPLAYFUNCTION pfn
    )
{
    Dutil_pfnDisplayAssert = pfn;
}


/*******************************************************************
Dutil_AssertMsg

*******************************************************************/
extern "C" void DAPI Dutil_AssertMsg(
    __in_z LPCSTR szMessage
    )
{
    static BOOL fInAssert = FALSE; // TODO: make this thread safe (this is a cheap hack to prevent re-entrant Asserts)

    HRESULT hr = S_OK;
    DWORD er = ERROR_SUCCESS;

    int id = IDRETRY;
    HKEY hkDebug = NULL;
    HANDLE hAssertFile = INVALID_HANDLE_VALUE;
    char szPath[MAX_PATH] = { };
    DWORD cch = 0;

    if (fInAssert)
    {
        return;
    }
    fInAssert = TRUE;

    char szMsg[DUTIL_STRING_BUFFER];
    hr = ::StringCchCopyA(szMsg, countof(szMsg), szMessage);
    DExitOnFailure(hr, "failed to copy message while building assert message");

    if (Dutil_pfnDisplayAssert)
    {
        // call custom function to display the assert string
        if (!Dutil_pfnDisplayAssert(szMsg))
        {
            ExitFunction();
        }
    }
    else
    {
        OutputDebugStringA(szMsg);
    }

    if (!Dutil_fNoAsserts)
    {
        er = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Delivery\\Debug", 0, KEY_QUERY_VALUE, &hkDebug);
        if (ERROR_SUCCESS == er)
        {
            cch = countof(szPath);
            er = ::RegQueryValueExA(hkDebug, "DeliveryAssertsLog", NULL, NULL, reinterpret_cast<BYTE*>(szPath), &cch);
            szPath[countof(szPath) - 1] = '\0'; // ensure string is null terminated since registry won't guarantee that.
            if (ERROR_SUCCESS == er)
            {
                hAssertFile = ::CreateFileA(szPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (INVALID_HANDLE_VALUE != hAssertFile)
                {
                    if (INVALID_SET_FILE_POINTER != ::SetFilePointer(hAssertFile, 0, 0, FILE_END))
                    {
                        if (SUCCEEDED(::StringCchCatA(szMsg, countof(szMsg), "\r\n")))
                        {
                            ::WriteFile(hAssertFile, szMsg, lstrlenA(szMsg), &cch, NULL);
                        }
                    }
                }
            }
        }

        // if anything went wrong while fooling around with the registry, just show the usual assert dialog box
        if (ERROR_SUCCESS != er)
        {
            hr = ::StringCchCatA(szMsg, countof(szMsg), "\nAbort=Debug, Retry=Skip, Ignore=Skip all");
            DExitOnFailure(hr, "failed to concat string while building assert message");

            id = ::MessageBoxA(0, szMsg, "Debug Assert Message",
                MB_SERVICE_NOTIFICATION | MB_TOPMOST | 
                MB_DEFBUTTON2 | MB_ABORTRETRYIGNORE);
        }
    }

    if (id == IDABORT)
    {
        if (Dutil_hAssertModule)
        {
            ::GetModuleFileNameA(Dutil_hAssertModule, szPath, countof(szPath));

            hr = ::StringCchPrintfA(szMsg, countof(szMsg), "Module is running from: %s\nIf you are not using pdb-stamping, place your PDB near the module and attach to process id: %d (0x%x)", szPath, ::GetCurrentProcessId(), ::GetCurrentProcessId());
            if (SUCCEEDED(hr))
            {
                ::MessageBoxA(0, szMsg, "Debug Assert Message", MB_SERVICE_NOTIFICATION | MB_TOPMOST | MB_OK);
            }
        }

        ::DebugBreak();
    }
    else if (id == IDIGNORE)
    {
        Dutil_fNoAsserts = TRUE;
    }

LExit:
    ReleaseFileHandle(hAssertFile);
    ReleaseRegKey(hkDebug);
    fInAssert = FALSE;
}


/*******************************************************************
Dutil_Assert

*******************************************************************/
extern "C" void DAPI Dutil_Assert(
    __in_z LPCSTR szFile, 
    __in int iLine
    )
{
    HRESULT hr = S_OK;
    char szMessage[DUTIL_STRING_BUFFER] = { };
    hr = ::StringCchPrintfA(szMessage, countof(szMessage), "Assertion failed in %s, %i", szFile, iLine);
    if (SUCCEEDED(hr))
    {
        Dutil_AssertMsg(szMessage);
    }
    else
    {
        Dutil_AssertMsg("Assert failed to build string");
    }
}


/*******************************************************************
Dutil_AssertSz

*******************************************************************/
extern "C" void DAPI Dutil_AssertSz(
    __in_z LPCSTR szFile, 
    __in int iLine, 
    __in_z __format_string LPCSTR szMsg
    )
{
    HRESULT hr = S_OK;
    char szMessage[DUTIL_STRING_BUFFER] = { };

    hr = ::StringCchPrintfA(szMessage, countof(szMessage), "Assertion failed in %s, %i\n%s", szFile, iLine, szMsg);
    if (SUCCEEDED(hr))
    {
        Dutil_AssertMsg(szMessage);
    }
    else
    {
        Dutil_AssertMsg("Assert failed to build string");
    }
}


/*******************************************************************
Dutil_TraceSetLevel

*******************************************************************/
extern "C" void DAPI Dutil_TraceSetLevel(
    __in REPORT_LEVEL rl,
    __in BOOL fTraceFilenames
    )
{
    Dutil_rlCurrentTrace = rl;
    Dutil_fTraceFilenames = fTraceFilenames;
}


/*******************************************************************
Dutil_TraceGetLevel

*******************************************************************/
extern "C" REPORT_LEVEL DAPI Dutil_TraceGetLevel()
{
    return Dutil_rlCurrentTrace;
}


/*******************************************************************
Dutil_Trace

*******************************************************************/
extern "C" void DAPIV Dutil_Trace(
    __in_z LPCSTR szFile, 
    __in int iLine, 
    __in REPORT_LEVEL rl, 
    __in_z __format_string LPCSTR szFormat, 
    ...
    )
{
    AssertSz(REPORT_NONE != rl, "REPORT_NONE is not a valid tracing level");

    HRESULT hr = S_OK;
    char szOutput[DUTIL_STRING_BUFFER] = { };
    char szMsg[DUTIL_STRING_BUFFER] = { };

    if (Dutil_rlCurrentTrace < rl)
    {
        return;
    }

    va_list args;
    va_start(args, szFormat);
    hr = ::StringCchVPrintfA(szOutput, countof(szOutput), szFormat, args);
    va_end(args);

    if (SUCCEEDED(hr))
    {
        LPCSTR szPrefix = "Trace/u";
        switch (rl)
        {
        case REPORT_STANDARD:
            szPrefix = "Trace/s";
            break;
        case REPORT_VERBOSE:
            szPrefix = "Trace/v";
            break;
        case REPORT_DEBUG:
            szPrefix = "Trace/d";
            break;
        }

        if (Dutil_fTraceFilenames)
        {
            hr = ::StringCchPrintfA(szMsg, countof(szMsg), "%s [%s,%d]: %s\r\n", szPrefix, szFile, iLine, szOutput);
        }
        else
        {
            hr = ::StringCchPrintfA(szMsg, countof(szMsg), "%s: %s\r\n", szPrefix, szOutput);
        }

        if (SUCCEEDED(hr))
        {
            OutputDebugStringA(szMsg);
        }
        // else fall through to the case below
    }

    if (FAILED(hr))
    {
        if (Dutil_fTraceFilenames)
        {
            ::StringCchPrintfA(szMsg, countof(szMsg), "Trace [%s,%d]: message too long, skipping\r\n", szFile, iLine);
        }
        else
        {
            ::StringCchPrintfA(szMsg, countof(szMsg), "Trace: message too long, skipping\r\n");
        }

        szMsg[countof(szMsg)-1] = '\0';
        OutputDebugStringA(szMsg);
    }
}


/*******************************************************************
Dutil_TraceError

*******************************************************************/
extern "C" void DAPIV Dutil_TraceError(
    __in_z LPCSTR szFile, 
    __in int iLine, 
    __in REPORT_LEVEL rl, 
    __in HRESULT hrError, 
    __in_z __format_string LPCSTR szFormat, 
    ...
    )
{
    HRESULT hr = S_OK;
    char szOutput[DUTIL_STRING_BUFFER] = { };
    char szMsg[DUTIL_STRING_BUFFER] = { };

    // if this is NOT an error report and we're not logging at this level, bail
    if (REPORT_ERROR != rl && Dutil_rlCurrentTrace < rl)
    {
        return;
    }

    va_list args;
    va_start(args, szFormat);
    hr = ::StringCchVPrintfA(szOutput, countof(szOutput), szFormat, args);
    va_end(args);

    if (SUCCEEDED(hr))
    {
        if (Dutil_fTraceFilenames)
        {
            if (FAILED(hrError))
            {
                hr = ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError 0x%x [%s,%d]: %s\r\n", hrError, szFile, iLine, szOutput);
            }
            else
            {
                hr = ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError [%s,%d]: %s\r\n", szFile, iLine, szOutput);
            }
        }
        else
        {
            if (FAILED(hrError))
            {
                hr = ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError 0x%x: %s\r\n", hrError, szOutput);
            }
            else
            {
                hr = ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError: %s\r\n", szOutput);
            }
        }

        if (SUCCEEDED(hr))
        {
            OutputDebugStringA(szMsg);
        }
        // else fall through to the failure case below
    }

    if (FAILED(hr))
    {
        if (Dutil_fTraceFilenames)
        {
            if (FAILED(hrError))
            {
                ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError 0x%x [%s,%d]: message too long, skipping\r\n", hrError, szFile, iLine);
            }
            else
            {
                ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError [%s,%d]: message too long, skipping\r\n", szFile, iLine);
            }
        }
        else
        {
            if (FAILED(hrError))
            {
                ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError 0x%x: message too long, skipping\r\n", hrError);
            }
            else
            {
                ::StringCchPrintfA(szMsg, countof(szMsg), "TraceError: message too long, skipping\r\n");
            }
        }

        szMsg[countof(szMsg)-1] = '\0';
        OutputDebugStringA(szMsg);
    }
}


DAPIV_(void) Dutil_TraceErrorSource(
    __in_z LPCSTR szFile,
    __in int iLine,
    __in REPORT_LEVEL rl,
    __in UINT source,
    __in HRESULT hr,
    __in_z __format_string LPCSTR szFormat,
    ...
    )
{
    // if this callback is currently suppressed, or
    // if this is NOT an error report and we're not logging at this level, bail
    if (vtdwSuppressTraceErrorSource || REPORT_ERROR != rl && Dutil_rlCurrentTrace < rl)
    {
        return;
    }

    if (DUTIL_SOURCE_UNKNOWN != source && vpfnTraceErrorCallback)
    {
        va_list args;
        va_start(args, szFormat);
        vpfnTraceErrorCallback(szFile, iLine, rl, source, hr, szFormat, args);
        va_end(args);
    }
}


/*******************************************************************
Dutil_RootFailure

*******************************************************************/
extern "C" void DAPI Dutil_RootFailure(
    __in_z LPCSTR szFile,
    __in int iLine,
    __in HRESULT hrError
    )
{
#ifndef DEBUG
    UNREFERENCED_PARAMETER(szFile);
    UNREFERENCED_PARAMETER(iLine);
    UNREFERENCED_PARAMETER(hrError);
#endif // DEBUG

    TraceError(hrError, "Root failure at %s:%d", szFile, iLine);
}
