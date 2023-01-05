#pragma once

#include <string_view>

constexpr auto Language() -> std::string_view;

//**ddd direct link libraries
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "shell32.lib")
