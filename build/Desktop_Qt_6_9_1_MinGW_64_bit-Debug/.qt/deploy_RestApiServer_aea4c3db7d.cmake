include("C:/Users/LENOVO/Backend_Delta/build/Desktop_Qt_6_9_1_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/RestApiServer-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase;qtwebsockets")

qt6_deploy_runtime_dependencies(
    EXECUTABLE C:/Users/LENOVO/Backend_Delta/build/Desktop_Qt_6_9_1_MinGW_64_bit-Debug/RestApiServer.exe
    GENERATE_QT_CONF
)
