cmake_minimum_required(VERSION 3.24)
project(DjehutiResearch VERSION 0.1.0 LANGUAGES C CXX)
if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "This foundation requires Windows and Visual Studio 2022 x64.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JUCE_PATH "" CACHE PATH "Existing JUCE source checkout")
set(FRUSTLANG_ROOT "" CACHE PATH "Existing standalone FrustLang source checkout (read-only)")
set(RESEARCH_FRUST_REVISION "008afac7e5bea9569e0631795871334b2336a022" CACHE STRING "Verified FRust source revision")
find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${FRUSTLANG_ROOT}" rev-parse HEAD
    OUTPUT_VARIABLE frust_revision OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT frust_revision STREQUAL RESEARCH_FRUST_REVISION)
    message(FATAL_ERROR "FRust revision mismatch: expected ${RESEARCH_FRUST_REVISION}, found ${frust_revision}")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${FRUSTLANG_ROOT}" diff --quiet HEAD --
    projects/01_language_paradigms/02_functional projects/03_creation_dock projects/05_frate projects/09_frust_plugin_host
    RESULT_VARIABLE frust_dirty)
if(NOT frust_dirty EQUAL 0)
    message(FATAL_ERROR "Referenced FRust/Dock sources have tracked edits; reconcile the pin before building.")
endif()
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Set JUCE_PATH to an existing JUCE checkout.")
endif()
add_compile_definitions(JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 NOMINMAX WIN32_LEAN_AND_MEAN)
add_compile_options(/FS /MP1 /utf-8)
add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/deps/juce" EXCLUDE_FROM_ALL)

# Compile the actual pinned sources into Research's build tree, never into the
# reference repositories' shared bin/lib directories. LLVM remains imported.
function(research_local_outputs directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
            set_target_properties(${target} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/deps/lib/${config}"
                LIBRARY_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/deps/lib/${config}"
                RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/deps/bin/${config}"
                PDB_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/deps/pdb/${config}")
        endforeach()
    endforeach()
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        research_local_outputs("${child}")
    endforeach()
endfunction()
set(compiler_root "${FRUSTLANG_ROOT}/projects/01_language_paradigms/02_functional")
set(plugin_root "${FRUSTLANG_ROOT}/projects/09_frust_plugin_host")
set(dock_root "${FRUSTLANG_ROOT}/projects/03_creation_dock")
add_subdirectory("${compiler_root}" "${CMAKE_BINARY_DIR}/deps/frust" EXCLUDE_FROM_ALL)
add_subdirectory("${plugin_root}" "${CMAKE_BINARY_DIR}/deps/plugin_host" EXCLUDE_FROM_ALL)
add_subdirectory("${dock_root}" "${CMAKE_BINARY_DIR}/deps/dock" EXCLUDE_FROM_ALL)
research_local_outputs("${compiler_root}")
research_local_outputs("${plugin_root}")
research_local_outputs("${dock_root}")

juce_add_binary_data(ResearchProofSources HEADER_NAME ResearchProofSources.h NAMESPACE ResearchProofSources
    SOURCES Resources/frust/answer.frust Resources/frust/invalid.frust)
juce_add_gui_app(DjehutiResearch PRODUCT_NAME "Djehuti Research" COMPANY_NAME "LagDaemon Software"
    BUNDLE_ID "com.lagdaemon.djehuti.research" VERSION ${PROJECT_VERSION})
set_target_properties(DjehutiResearch PROPERTIES OUTPUT_NAME djehuti_research
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
target_sources(DjehutiResearch PRIVATE Source/Main.cpp Source/Settings.cpp Source/TrustedFrust.cpp
    Source/ResearchWorkspace.cpp Source/FoundationCheck.cpp)
target_include_directories(DjehutiResearch PRIVATE Source)
target_compile_definitions(DjehutiResearch PRIVATE RESEARCH_FRUST_REVISION="${frust_revision}")
target_link_libraries(DjehutiResearch PRIVATE ResearchProofSources CreationDock frust_plugin_host frust_runtime
    juce::juce_gui_extra)
juce_generate_juce_header(DjehutiResearch)
file(WRITE "${CMAKE_BINARY_DIR}/frust-provenance.txt"
    "FRust source/header/library revision: ${frust_revision}\nLibraries compiled in this build from read-only sources.\nLLVM: ${LLVM_DIR}\nMath foundation reference: ff2ee952abe2e697db71d9342618fb6cc359f870\n")
