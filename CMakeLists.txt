cmake_minimum_required(VERSION 3.31)
project(wxFDIconTheme VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(wxWidgets_CONFIGURATION gtku)
find_package(wxWidgets REQUIRED COMPONENTS base core)
include(${wxWidgets_USE_FILE})

add_library(wxFDIconTheme
        src/fdicontheme.cpp
        src/fdicontheme.h
)

target_link_libraries(wxFDIconTheme PRIVATE ${wxWidgets_LIBRARIES})


add_executable(fdit_viewer src/main.cpp
        src/dvcard.cpp
        src/dvcard.h)
target_link_libraries(fdit_viewer PRIVATE wxFDIconTheme ${wxWidgets_LIBRARIES})
