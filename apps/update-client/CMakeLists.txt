add_executable(tes3mp_update_client
        main.cpp main_downloader.cpp main_cleaner.cpp
        FileDownloader.cpp FileDownloader.hpp
        AssetEntry.cpp AssetEntry.hpp
        URI.hpp
        TimeUtils.hpp
        Helper.cpp Helper.hpp
        Downloader.cpp Downloader.hpp
        Options.cpp Options.hpp
        )

target_link_libraries(tes3mp_update_client
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${OPENSSL_LIBRARIES}
        )

if (UNIX AND NOT APPLE)
    target_link_libraries(tes3mp_update_client ${CMAKE_THREAD_LIBS_INIT})
endif()
