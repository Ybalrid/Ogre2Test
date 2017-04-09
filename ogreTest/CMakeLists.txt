cmake_minimum_required(VERSION 2.8)

project(Ogre2Test)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-extras/)

find_package(OGRE COMPONENTS HlmsPbs HlmsUnlit REQUIRED)

include_directories(
    ${OGRE_INCLUDE_DIRS}
    ${OGRE_HlmsPbs_INCLUDE_DIRS}
    ${OGRE_HlmsUnlit_INCLUDE_DIRS}
)

add_executable(Ogre2Test sources/main.cpp)
target_link_libraries( Ogre2Test
    ${OGRE_LIBRARIES}
    ${OGRE_HlmsPbs_LIBRARIES}
    ${OGRE_HlmsUnlit_LIBRARIES}
    )



