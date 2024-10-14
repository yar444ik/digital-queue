cmake_minimum_required(VERSION 3.27)
project(digital-queue)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED Sql)

add_executable(register register.cpp)
add_executable(table table.cpp)
add_executable(work work.cpp)


target_link_libraries(register
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Sql
        )

target_link_libraries(table
        Qt::Core
        Qt::Gui
        Qt::Widgets
        )

target_link_libraries(work
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Sql
        )
