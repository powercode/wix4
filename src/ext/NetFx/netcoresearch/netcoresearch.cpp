// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"

#include <filesystem>
#include <string_view>
#include <sstream>

#include "dotnet_version.h"
#include "console.h"
#include "dynamic_module.h"

struct NETCORESEARCH_STATE
{
    std::wstring_view TargetName;
    DWORD MajorVersion;
    dotnet_version Version;
};

auto tryParse(std::wstring_view input, __out DWORD& result ) -> bool
{
    std::wstringstream ss;
    ss << input;
    ss >> result;
    return !ss.fail();
}

auto get_process_path() -> std::filesystem::path;



static HRESULT GetDotnetEnvironmentInfo(
    __in DWORD majorVersion,
    __in std::wstring_view targetName,
    __inout dotnet_version& version
    );

static void HOSTFXR_CALLTYPE GetDotnetEnvironmentInfoResult(
    __in const hostfxr_dotnet_environment_info* pInfo,
    __in LPVOID pvContext
    );

int __cdecl wmain(int argc, LPWSTR argv[])
{
    if (argc != 3)
    {
        return E_INVALIDARG;
    }

    const std::wstring_view majVerStr{ argv[1] };
    const std::wstring_view targetName{ argv[2] };
    console console{ CP_UTF8 };

    try
    {
        DWORD major_version;
        if (!tryParse(majVerStr, __out major_version))
        {
            console::write_error(std::format(L"'{}' is not a valid major version.", majVerStr));
            return E_INVALIDARG;
        }

        dotnet_version version;

        const HRESULT hr = GetDotnetEnvironmentInfo(major_version, targetName, OUT version);
        if (FAILED(hr))
        {
            console::write_error_line("Failed to search for .NET Core." );
            return hr;
        }
        if (version)
        {
            console::write_output_line(version->sczVersion);
            return 0;
        }
    }
    catch (version_exception& e)
    {
        console::write_error(std::format( "{}\n" , e.what()));
        return e.hr();
    }
    catch(std::exception& e)
    {
        console::write_error(e.what());
    }
    return E_FAIL;
}

auto get_process_path() -> std::filesystem::path
{
    std::wstring path;

    DWORD copied;
    do {
        path.resize(MAX_PATH);
        copied = GetModuleFileName(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (copied == 0)
        {
            throw std::runtime_error("Failed to get process path");
        }

    } while (copied >= path.size());

    path.resize(copied);

    return { path };
}


static HRESULT GetDotnetEnvironmentInfo(
    NETCORESEARCH_STATE state = { targetName, majorVersion };
    const auto hostfxrDll = get_process_path().remove_filename() /= L"hostfxr.dll";
    dynamic_module hostfxr{ hostfxrDll };

    const auto hostfxrGetDotnetEnvironmentInfo = hostfxr.get_function<hostfxr_get_dotnet_environment_info_fn>("hostfxr_get_dotnet_environment_info");

    const HRESULT hr = hostfxrGetDotnetEnvironmentInfo(nullptr, nullptr, GetDotnetEnvironmentInfoResult, &state);
    if (FAILED(hr))
    {
        ConsoleWriteError(hr, CONSOLE_COLOR_RED, "Failed to get .NET Core environment info.");
        return hr;
    }

    if (state.Version)
    {
        version = std::move(state.Version);
    }

    return S_OK;
}


static void HOSTFXR_CALLTYPE GetDotnetEnvironmentInfoResult(
    __in const hostfxr_dotnet_environment_info* pInfo,
    __in LPVOID pvContext
    )
{
    const auto pState = static_cast<NETCORESEARCH_STATE*>(pvContext);
    dotnet_version resultVersion;

    const bool isSdk = pState->TargetName.compare(L"sdk") == 0;
    try
    {
        for (size_t i = 0; i < pInfo->sdk_count; ++i)
        {
            const hostfxr_dotnet_environment_sdk_info* pSdkInfo = pInfo->sdks + i;
            ReleaseVerutilVersion(pDotnetVersion);

            hr = VerParseVersion(pSdkInfo->version, 0, FALSE, &pDotnetVersion);
            ConsoleExitOnFailure(hr, CONSOLE_COLOR_RED, "Failed to parse sdk version: %ls", pSdkInfo->version);

            if (pDotnetVersion->dwMajor != pState->dwMajorVersion)
            {
                continue;
            }

                if (sdkVersion->dwMajor != pState->MajorVersion)
                {
                    continue;
                }

                if (nCompare > -1)
                {
                    continue;
                }
            }

            ReleaseVerutilVersion(pState->pVersion);
            pState->pVersion = pDotnetVersion;
            pDotnetVersion = nullptr;
        }
        else // framework, not Sdk
        {
            const hostfxr_dotnet_environment_framework_info* pFrameworkInfo = pInfo->frameworks + i;
            ReleaseVerutilVersion(pDotnetVersion);

            if (CSTR_EQUAL != ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, pState->wzTargetName, -1, pFrameworkInfo->name, -1))
            {
                continue;
            }

                if (CSTR_EQUAL != ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, pState->TargetName.data(), -1, pFrameworkInfo->name, -1))
                {
                    continue;
                }

            if (pDotnetVersion->dwMajor != pState->dwMajorVersion)
            {
                continue;
            }

                if (frameworkVersion->dwMajor != pState->MajorVersion)
                {
                    continue;
                }

                if (nCompare > -1)
                {
                    continue;
                }
            }

            ReleaseVerutilVersion(pState->pVersion);
            pState->pVersion = pDotnetVersion;
            pDotnetVersion = nullptr;
	    }
    }

LExit:
    ReleaseVerutilVersion(pDotnetVersion);
}
