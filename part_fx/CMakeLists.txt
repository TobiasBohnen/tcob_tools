add_executable(part_fx)

target_sources(part_fx PRIVATE
    main.cpp
    ParticleSystemEditor.cpp
    UI.cpp
)

set_target_properties(part_fx PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED TRUE
)

if(EMSCRIPTEN)
    target_compile_options(part_fx PRIVATE -fexperimental-library -pthread)
    configure_file(particlesystemeditor.zip ${CMAKE_BINARY_DIR}/particlesystemeditor.zip COPYONLY)
    target_link_options(part_fx PRIVATE -sFULL_ES3 -sASSERTIONS -sPTHREAD_POOL_SIZE=8 -sASYNCIFY -sALLOW_MEMORY_GROWTH -pthread --embed-file particlesystemeditor.zip)
else()
    configure_file(particlesystemeditor.zip ${CMAKE_BINARY_DIR}/../../x64/particlesystemeditor.zip COPYONLY)
endif()

if(NOT TCOB_BUILD_SHARED)
    target_link_libraries(part_fx PRIVATE tcob_static)
else()
    target_link_libraries(part_fx PRIVATE tcob_shared)
endif()

target_include_directories(part_fx PRIVATE ../../../tcob/include)

if(TCOB_BUILD_SHARED AND NOT TCOB_IS_CI)
    add_custom_command(TARGET part_fx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:part_fx> ${CMAKE_BINARY_DIR}/../../x64/
        COMMAND_EXPAND_LISTS
    )
endif()
