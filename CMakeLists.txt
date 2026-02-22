cmake_minimum_required(VERSION 3.15)
project(EMELinkBudget VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine platform
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
else()
    set(PLATFORM_NAME "linux")
endif()

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Find required packages
find_package(CURL REQUIRED)

# Source files
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/EMELinkBudget/src")

set(SOURCES
    ${SOURCE_DIR}/main_linkbudget_interactive.cpp
    ${SOURCE_DIR}/EMELinkBudget.cpp
    ${SOURCE_DIR}/GeometryCalculator.cpp
    ${SOURCE_DIR}/PathLossCalculator.cpp
    ${SOURCE_DIR}/PolarizationModule.cpp
    ${SOURCE_DIR}/FaradayRotation.cpp
    ${SOURCE_DIR}/NoiseCalculator.cpp
    ${SOURCE_DIR}/SNRCalculator.cpp
    ${SOURCE_DIR}/IonosphereDataProvider.cpp
    ${SOURCE_DIR}/IonospherePhysics.cpp
    ${SOURCE_DIR}/IonexReader.cpp
    ${SOURCE_DIR}/NOAAGlotecReader.cpp
    ${SOURCE_DIR}/MoonCalendarReader.cpp
    ${SOURCE_DIR}/WMMModel.cpp
    ${SOURCE_DIR}/SimpleHttpClient.cpp
    ${SOURCE_DIR}/AstronomyAPIClient.cpp
    ${SOURCE_DIR}/HaslamSkyMap.cpp
)

set(HEADERS
    ${SOURCE_DIR}/EMELinkBudget.h
    ${SOURCE_DIR}/GeometryCalculator.h
    ${SOURCE_DIR}/PathLossCalculator.h
    ${SOURCE_DIR}/PolarizationModule.h
    ${SOURCE_DIR}/FaradayRotation.h
    ${SOURCE_DIR}/NoiseCalculator.h
    ${SOURCE_DIR}/SNRCalculator.h
    ${SOURCE_DIR}/IonosphereDataProvider.h
    ${SOURCE_DIR}/IonospherePhysics.h
    ${SOURCE_DIR}/IonexReader.h
    ${SOURCE_DIR}/NOAAGlotecReader.h
    ${SOURCE_DIR}/MoonCalendarReader.h
    ${SOURCE_DIR}/WMMModel.h
    ${SOURCE_DIR}/SimpleHttpClient.h
    ${SOURCE_DIR}/AstronomyAPIClient.h
    ${SOURCE_DIR}/HaslamSkyMap.h
    ${SOURCE_DIR}/LinkBudgetTypes.h
    ${SOURCE_DIR}/Parameters.h
    ${SOURCE_DIR}/MaidenheadGrid.h
)

add_executable(EMELinkBudget ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(EMELinkBudget PRIVATE
    ${SOURCE_DIR}
)

# Link libraries
target_link_libraries(EMELinkBudget PRIVATE
    CURL::libcurl
    m  # math library
)

# Compiler flags
if(MSVC)
    target_compile_options(EMELinkBudget PRIVATE
        /Wall /WX
    )
else()
    target_compile_options(EMELinkBudget PRIVATE
        -Wall -Wextra -Wpedantic -fPIE
    )
endif()

# Install targets
install(TARGETS EMELinkBudget
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/EMELinkBudget/data/
    DESTINATION share/EMELinkBudget/data
)

# Build tests
set(TEST_SOURCES
    ${SOURCE_DIR}/test_linkbudget_quick.cpp
    ${SOURCE_DIR}/EMELinkBudget.cpp
    ${SOURCE_DIR}/GeometryCalculator.cpp
    ${SOURCE_DIR}/PathLossCalculator.cpp
    ${SOURCE_DIR}/PolarizationModule.cpp
    ${SOURCE_DIR}/FaradayRotation.cpp
    ${SOURCE_DIR}/NoiseCalculator.cpp
    ${SOURCE_DIR}/SNRCalculator.cpp
    ${SOURCE_DIR}/IonosphereDataProvider.cpp
    ${SOURCE_DIR}/IonospherePhysics.cpp
    ${SOURCE_DIR}/IonexReader.cpp
    ${SOURCE_DIR}/NOAAGlotecReader.cpp
    ${SOURCE_DIR}/MoonCalendarReader.cpp
    ${SOURCE_DIR}/WMMModel.cpp
    ${SOURCE_DIR}/SimpleHttpClient.cpp
    ${SOURCE_DIR}/AstronomyAPIClient.cpp
    ${SOURCE_DIR}/HaslamSkyMap.cpp
)

add_executable(test_linkbudget_quick ${TEST_SOURCES})
target_include_directories(test_linkbudget_quick PRIVATE ${SOURCE_DIR})
target_link_libraries(test_linkbudget_quick PRIVATE CURL::libcurl m)

if(NOT MSVC)
    target_compile_options(test_linkbudget_quick PRIVATE -Wall -Wextra -Wpedantic -fPIE)
endif()

# Enable testing
enable_testing()
add_test(NAME test_linkbudget COMMAND test_linkbudget_quick)
