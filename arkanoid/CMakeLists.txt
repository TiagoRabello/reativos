################################################
# CMake App Constants
################################################
SET( MAIN_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
SET( APP_NAME        "arkanoid"                      )

################################################
# App Generation Code
################################################
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.7 )
PROJECT( ${APP_NAME} CXX C )

################################################
# App Compiler Flags
################################################
IF( WIN32 )

  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX /W4" )

ELSE( WIN32 )

  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pedantic -Wall -Werror -Wno-unknown-pragmas -Wno-missing-braces" )

ENDIF( WIN32 )

#########################################################
# FIND SDL
#########################################################
FIND_PACKAGE( SDL REQUIRED )
INCLUDE_DIRECTORIES( ${SDL_INCLUDE_DIR} )

if( WIN32 )

GET_FILENAME_COMPONENT(SDL_LIB_NAME ${SDL_LIBRARY} NAME_WE)
GET_FILENAME_COMPONENT(SDL_LIB_DIR ${SDL_LIBRARY} DIRECTORY)

SET(SDL_DLL_NAME "${SDL_LIB_NAME}.dll")
SET(SDL_DLL_PATH "${SDL_LIB_DIR}/${SDL_LIB_NAME}.dll")

ENDIF( WIN32 )

#########################################################
# App Generation Configuration
#########################################################
INCLUDE_DIRECTORIES( ${MAIN_INCLUDE_DIR} )

ADD_SUBDIRECTORY( ${MAIN_SOURCE_DIR} )
ADD_EXECUTABLE( ${APP_NAME} ${APP_SOURCES} )

TARGET_LINK_LIBRARIES( ${APP_NAME}
  ${SDL_LIBRARY}
  ${SDLMAIN_LIBRARY}
)

IF( WIN32 )

# Copy dlls to output directory.
ADD_CUSTOM_COMMAND(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL_DLL_PATH} ${SDL_DLL_NAME})

ENDIF( WIN32 )