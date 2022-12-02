#pragma once

// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include <conutil.h>
#include <iostream>
#include <string_view>


class console
{
public:
    console(UINT codePage)
    {
        ::SetConsoleCP(codePage);
        ConsoleInitialize();
    }
    ~console() {
        flush();
        ConsoleUninitialize();
    }

    console(const console&) = delete;
    console(console&&) = delete;
    console& operator=(const console&) = delete;
    console& operator=(console&&) = delete;

    
    static auto write_error_line(std::string_view message)
    {
        ConsoleRed();
        std::wcout << message.data() << std::endl;
        ConsoleNormal();
    }

    template <class... Args>
    static auto write_error(std::string_view error)
    {
        ConsoleRed();
        std::wcout << error.data();
        ConsoleNormal();
    }

    template <class... Args>
    static auto write_error(std::wstring_view error)
    {
        ConsoleRed();
        std::wcout << error;
        ConsoleNormal();
    }

    static auto write_output(std::string_view text)    { std::wcout << text.data(); }

    static auto write_output(std::wstring_view text) { std::wcout << text; }

    static auto write_output_line(std::string_view text){ std::wcout << text.data() << std::endl; }

    static auto write_output_line(std::wstring_view text){ std::wcout << text << std::endl; }

    static auto flush() noexcept -> void { std::wcout.flush(); }
};


