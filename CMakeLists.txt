cmake_minimum_required(VERSION 3.19)

project(RestApiServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core HttpServer Sql)

qt_standard_project_setup()

set(SOURCES
    main.cpp
    api.cpp
    auth_routes.cpp
    dashboard_routes.cpp
    database_manager.cpp
    calendar_routes.cpp
    problems_routes.cpp
    ranking_routes.cpp
    forum_routes.cpp
    visualizations_routes.cpp
    exams_routes.cpp
    notebooks_routes.cpp
    response_utils.cpp
    jwt_helper.cpp
)

set(HEADERS
    api.h
    auth_routes.h
    dashboard_routes.h
    database_manager.h
    calendar_routes.h
    problems_routes.h
    ranking_routes.h
    forum_routes.h
    visualizations_routes.h
    exams_routes.h
    notebooks_routes.h
    response_utils.h
    jwt_helper.h
)

qt_add_executable(RestApiServer
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(RestApiServer
    PRIVATE
        Qt::Core
        Qt::HttpServer
        Qt::Sql
)

include(GNUInstallDirs)
install(TARGETS RestApiServer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET RestApiServer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
