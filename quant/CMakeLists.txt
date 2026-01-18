add_executable(quant)

target_sources(quant PRIVATE
    main.cpp
)

set_target_properties(quant PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED TRUE
)

if(TCOB_BUILD_SHARED)
    target_link_libraries(quant PRIVATE tcob_shared)
else()
    target_link_libraries(quant PRIVATE tcob_static)
endif()
