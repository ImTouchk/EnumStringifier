# EnumStringifier
A utility program that generates ToString functions for C++ enums

## Why make this?

Well, I wanted to print to the console some enum members (is that what they are called? I don't know, whatever), but C++ stores them as an integer, and there's no standard way of doing it. This tool should give you a `ToString` method that returns an `std::string` and should overload the operator `<<` for output streams, which should in turn let you print the enum member to the console.

## Building the program

The source code is contained in a single source file, without any external dependencies.
Make sure to use C++20, as the `<filesystem>` header is needed.

## Using the program

To use the program, you start it from a command line, specifying the path of the file that contains your enum. For example -
`EnumStringifier.exe D:\C++\MyEnum.hpp` in the Windows Command Prompt. The file can have any extension.
If you have more than an enum defined in your file, move the one you want to stringify to the top of the file.
