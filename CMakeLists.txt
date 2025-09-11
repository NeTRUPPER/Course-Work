cmake_minimum_required(VERSION 3.16)
project(TouristRentalApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql PrintSupport)


# Set up Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
 

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/database.cpp
    src/rentalmanager.cpp
    src/customer.cpp
    src/equipment.cpp
    src/rental.cpp
    src/security.cpp
    src/customerdialog.cpp
    src/equipmentdialog.cpp
    src/rentaldialog.cpp
)

# Header files
set(HEADERS
    include/mainwindow.h
    include/database.h
    include/rentalmanager.h
    include/customer.h
    include/equipment.h
    include/rental.h
    include/security.h
    include/customerdialog.h
    include/equipmentdialog.h
    include/rentaldialog.h
)

# UI files
set(UI_FILES
    ui/mainwindow.ui
    ui/customerform.ui
    ui/equipmentform.ui
    ui/rentalform.ui
)

# Resource files
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Sql
    Qt6::PrintSupport
)

# Set compiler flags

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin) 