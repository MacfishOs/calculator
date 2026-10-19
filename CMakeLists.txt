cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cutefish-calculator)
project(${PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT Core Gui Quick QuickControls2 LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS ${QT})

if (MSVC)
    add_compile_options("/utf-8")
endif ()

set(SRCS
    main.cpp
    calcengine.cpp
    engine/constants.cpp
    engine/evaluator.cpp
    engine/functions.cpp
    engine/hmath.cpp
    engine/number.c
)

set(RESOURCES
    qml.qrc
)

add_executable(${PROJECT_NAME} ${SRCS} ${RESOURCES})
target_link_libraries(${PROJECT_NAME}
        Qt6::Core
        Qt6::Quick
        Qt6::QuickControls2
)

file(GLOB TS_FILES translations/*.ts)
if (TS_FILES)
    qt6_add_translation(QM_FILES ${TS_FILES})
    add_custom_target(translations DEPENDS ${QM_FILES} SOURCES ${TS_FILES})
    add_dependencies(${PROJECT_NAME} translations)
endif ()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION /usr/bin)

install(FILES
    cutefish-calculator.desktop
    DESTINATION /usr/share/applications/
    COMPONENT Runtime
)

if (QM_FILES)
    install(FILES ${QM_FILES} DESTINATION /usr/share/${PROJECT_NAME}/translations)
endif ()
