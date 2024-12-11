cmake_minimum_required(VERSION 3.10)
project(testGui)

# تنظیم استاندارد C
set(CMAKE_C_STANDARD 99)

# یافتن GTK
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

# اضافه کردن مسیرهای GTK
include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})
add_definitions(${GTK_CFLAGS_OTHER})

# افزودن فایل منبع
add_executable(testGui main.c)

# لینک کردن کتابخانه GTK
target_link_libraries(testGui ${GTK_LIBRARIES})
