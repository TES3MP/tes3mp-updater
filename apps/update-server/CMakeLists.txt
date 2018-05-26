add_executable(tes3mp_update_server
        main.cpp
        Listener.hpp
        Session.hpp
        Reporter.hpp
        RequestHandler.hpp
        Options.cpp Options.hpp
        Utils.hpp Utils.cpp
        )

target_link_libraries(tes3mp_update_server
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        )

if (UNIX AND NOT APPLE)
    target_link_libraries(tes3mp_update_server ${CMAKE_THREAD_LIBS_INIT})
endif()
