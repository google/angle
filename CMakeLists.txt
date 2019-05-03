cmake_minimum_required(VERSION 3.3)
project(ANGLE)

# Copies of WEBKIT_MACROS that are being used

macro(WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS)
    set(_file ${CMAKE_CURRENT_SOURCE_DIR}/Platform${PORT}.cmake)
    if (EXISTS ${_file})
        message(STATUS "Using platform-specific CMakeLists: ${_file}")
        include(${_file})
    else ()
        message(STATUS "Platform-specific CMakeLists not found: ${_file}")
    endif ()
endmacro()

function(WEBKIT_COPY_FILES target_name)
    set(options FLATTENED)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs FILES)
    cmake_parse_arguments(opt "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(files ${opt_FILES})
    set(dst_files)
    foreach (file IN LISTS files)
        if (IS_ABSOLUTE ${file})
            set(src_file ${file})
        else ()
            set(src_file ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endif ()
        if (opt_FLATTENED)
            get_filename_component(filename ${file} NAME)
            set(dst_file ${opt_DESTINATION}/${filename})
        else ()
            get_filename_component(file_dir ${file} DIRECTORY)
            file(MAKE_DIRECTORY ${opt_DESTINATION}/${file_dir})
            set(dst_file ${opt_DESTINATION}/${file})
        endif ()
        add_custom_command(OUTPUT ${dst_file}
            COMMAND ${CMAKE_COMMAND} -E copy ${src_file} ${dst_file}
            MAIN_DEPENDENCY ${file}
            VERBATIM
        )
        list(APPEND dst_files ${dst_file})
    endforeach ()
    add_custom_target(${target_name} DEPENDS ${dst_files})
endfunction()

# The file commit.h references the Git hash being used. When committing this should
# reference the git commit that was branched off of

file(COPY src/commit.h DESTINATION ${CMAKE_BINARY_DIR}/id)
include_directories(${CMAKE_BINARY_DIR})

set(USE_ANGLE_EGL ON)

################################################################################
#
# Remove above when committing to WebKit repository
#
################################################################################

include(Compiler.cmake)
include(GLESv2.cmake)

if (WIN32)
    # FIXME: DX11 support will not compile if this preprocessor definition is set
    # DirectX Documentation is setting that version to 0x700 but there is no
    # corresponding value in sdkddkver.h
    remove_definitions(-D_WIN32_WINNT=0x601)
endif ()

# ANGLE makes a number of small static libraries that are then joined into a
# bigger library that is built shared. Rather than making the small libraries
# there will be a libANGLE whose sources are dependent on whether the library
# is being used as a compiler or as a GLES implementation.
#
# The corresponding gn targets are described below
#
# libANGLE (Compiler only)
# + angle_common
# + preprocessor
# + translator
#
# libANGLE (GLES)
# + libANGLE (Compiler only)
# + xxhash
# + angle_image_util
# + angle_system_utils (OS specific)
# + angle_(renderer) (Backend and OS specific)

set(ANGLE_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/third_party/base"
)

set(ANGLE_DEFINITIONS
    ANGLE_ENABLE_ESSL
)

set(ANGLE_SOURCES
    ${libangle_common_sources}
    ${angle_preprocessor_sources}
    ${angle_translator_essl_sources}
    ${angle_translator_glsl_sources}
    ${angle_translator_sources}
)

set(ANGLEGLESv2_LIBRARIES
    libANGLE
)

set(ANGLEEGL_SOURCES ${})

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

add_library(ANGLE INTERFACE)

if (USE_ANGLE_EGL)
    add_library(libANGLE STATIC
        ${ANGLE_SOURCES}
        ${libangle_sources}
        ${libangle_image_util_sources}
        ${xxhash_sources}
    )
    target_include_directories(libANGLE PRIVATE ${ANGLE_PRIVATE_INCLUDE_DIRECTORIES})
    target_compile_definitions(libANGLE PRIVATE LIBANGLE_IMPLEMENTATION)
    target_compile_definitions(libANGLE PUBLIC ${ANGLE_DEFINITIONS})

    add_library(libGLESv2 SHARED ${libglesv2_sources})
    target_include_directories(libGLESv2 PRIVATE ${ANGLE_PRIVATE_INCLUDE_DIRECTORIES})
    target_compile_definitions(libGLESv2 PRIVATE LIBGLESV2_IMPLEMENTATION)
    target_link_libraries(libGLESv2 ${ANGLEGLESv2_LIBRARIES})

    add_library(libEGL SHARED ${libegl_sources})
    target_include_directories(libEGL PRIVATE ${ANGLE_PRIVATE_INCLUDE_DIRECTORIES})
    target_compile_definitions(libEGL PRIVATE
        GL_GLES_PROTOTYPES=1
        EGL_EGL_PROTOTYPES=1
        GL_GLEXT_PROTOTYPES
        EGL_EGLEXT_PROTOTYPES
        EGLAPI=
    )
    target_link_libraries(libEGL libGLESv2)

    if (WIN32)
        target_sources(libGLESv2 PRIVATE src/libGLESv2/libGLESv2_autogen.def)
        target_sources(libEGL PRIVATE src/libEGL/libEGL.def)
    endif ()

    target_link_libraries(ANGLE INTERFACE libEGL libGLESv2)
    target_compile_definitions(ANGLE INTERFACE
        GL_GLES_PROTOTYPES=1
        EGL_EGL_PROTOTYPES=1
        GL_GLEXT_PROTOTYPES
        EGL_EGLEXT_PROTOTYPES
    )
else ()
    add_library(libANGLE SHARED ${ANGLE_SOURCES})
    target_include_directories(libANGLE PRIVATE ${angle_PRIVATE_INCLUDE_DIRECTORIES})
    target_compile_definitions(libANGLE PRIVATE
        ${ANGLE_DEFINITIONS}
        LIBANGLE_IMPLEMENTATION
    )
endif ()

target_link_libraries(ANGLE INTERFACE libANGLE)
