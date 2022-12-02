#pragma once

// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include <windows.h>
#include <filesystem>


class dynamic_module
{
    HMODULE module_ = nullptr;
public:
    
    dynamic_module(const std::filesystem::path& modulePath)
    {
        module_ = ::LoadLibraryExW(modulePath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (nullptr == module_)
        {
            throw std::runtime_error("Failed to load module");
        }
    }

    ~dynamic_module() noexcept
    {
        if (nullptr != module_)
        {
            ::FreeLibrary(module_);
        }
    }

    dynamic_module(dynamic_module&& rhs) noexcept
    {
        module_ = rhs.module_;
        rhs.module_ = nullptr;
    }
    
    dynamic_module& operator=(dynamic_module&& rhs) noexcept
    {
        module_ = rhs.module_;
        rhs.module_ = nullptr;
        return *this;
    }

    // disable copy operations
    dynamic_module(const dynamic_module&) = delete;
    dynamic_module& operator=(const dynamic_module&) = delete;


    constexpr operator bool() const noexcept { return nullptr != module_; }

    constexpr operator HMODULE() const noexcept { return module_; }

    void swap(dynamic_module& other) noexcept
    {
        const HMODULE tmp = module_;
        module_ = other.module_;
        other.module_ = tmp;
    }

    template<class T>
    T get_function(std::string_view functionName)
    {
        auto proc = ::GetProcAddress(module_, functionName.data());
        if (proc == nullptr)
        {
            throw std::runtime_error("Fatal error: Process address not found.");
        }
        return reinterpret_cast<T>(proc);
    }
};

inline void swap(dynamic_module& left, dynamic_module& right) noexcept { left.swap(right); }
