cmake_minimum_required(VERSION 3.9)

project(docsample6)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${LTK_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LTK_LIB} ${LTKIMPINJ_LIB} ${OPENSSL_LIBS} ${LIBXML2_LIB})