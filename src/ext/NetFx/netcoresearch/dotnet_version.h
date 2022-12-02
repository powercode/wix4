#pragma once

// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include <cassert>
#include <verutil.h>
#include <exception>
#include <string>
#include <utility>

#include <locale>
#include <codecvt>

std::string to_string(const std::wstring& wstr)
{
    static std::wstring_convert< std::codecvt_utf8_utf16<wchar_t>, wchar_t > converter;
    return converter.to_bytes(wstr);
}

class version_exception : public std::exception
{
protected:
    HRESULT hr_ = E_FAIL;
    std::wstring version_;

    version_exception() = default;
    version_exception(HRESULT hr, std::wstring version)
        : hr_{ hr }
        , version_{std::move(version)}
    {
    }
public:
    _NODISCARD auto hr() const { return hr_; }
    _NODISCARD const auto& version() const { return version_; }
};


class version_parse_exception : public version_exception
{
public:
    version_parse_exception(HRESULT hr, const char_t* version) : version_exception(hr, version) { }
    _NODISCARD const char* what() const override { return "Version parse failed.";  }

};

class version_compare_exception : public version_exception
{
    std::wstring version2_;
public:
    version_compare_exception(HRESULT hr, const char_t* version, const char_t* version2)
        : version_exception(hr, version)
        , version2_ {version2}
    {}
    _NODISCARD const char* what() const override { return "Version compare failed."; }
    _NODISCARD const auto& version2() const { return version2_; }
};


class dotnet_version
{
    VERUTIL_VERSION* _version = nullptr;
    void InternalRelease() noexcept
    {
        VERUTIL_VERSION* v = _version;
        if (v != nullptr)
        {
            _version = nullptr;
            VerFreeVersion(v);
        }
    }
public:
    dotnet_version() noexcept = default;

    explicit dotnet_version(const char_t* stringVersion)
    {
        const auto hr = VerParseVersion(stringVersion, 0, false, &_version);
        if (FAILED(hr))
        {
            throw version_parse_exception(hr, stringVersion);
        }
    }

    dotnet_version(dotnet_version&& version) noexcept
    {
        _version = version._version;
        version._version = nullptr;
    }

    dotnet_version(const dotnet_version& version)
    {
        VerCopyVersion(version.version_, &version_);
    }

    dotnet_version& operator=(const dotnet_version& rhs) noexcept
    {
        dotnet_version tmp = rhs;
        *this = std::move(tmp);
        return *this;
    }

    dotnet_version& operator=(dotnet_version&& rhs) noexcept
    {
        version_ = rhs.version_;
        rhs.version_ = nullptr;
        return *this;
    }

    void swap(dotnet_version& other) noexcept
    {
        VERUTIL_VERSION* tmp = version_;
        version_ = other.version_;
        other.version_ = tmp;
    }

    _NODISCARD VERUTIL_VERSION** GetAddressOf() noexcept
    {
        assert(version_ == nullptr);
        return &version_;
    }

    void Attach(VERUTIL_VERSION* v) noexcept
    {
        InternalRelease();
        version_ = v;
    }

    _NODISCARD VERUTIL_VERSION* Detach() noexcept
    {
        const auto tmp = version_;
        version_ = nullptr;
        return tmp;
    }

    ~dotnet_version() noexcept
    {
        InternalRelease();
    }

    auto operator->() const noexcept
    {
        return version_;
    }

    auto operator<=>(const dotnet_version& v1) const
    {
        int res = 0;
        const auto hr = VerCompareParsedVersions(version_, v1.version_, &res);
        if (FAILED(hr))
        {
            throw version_compare_exception(hr);
        }
        return res;
    }

    explicit operator bool() const noexcept
    {
        return nullptr != version_;
    }
};

inline void swap(dotnet_version& left, dotnet_version& right) noexcept { left.swap(right); }

