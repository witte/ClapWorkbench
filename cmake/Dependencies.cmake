
function(setupDependencies version os arch)
    set(deps_root_dir ${CMAKE_BINARY_DIR}/__deps)

    if (NOT EXISTS ${deps_root_dir})
        file(DOWNLOAD
            "https://github.com/witte/ClapWorkbenchSDK/releases/download/${version}/ClapWorkbenchSDK-${version}-${os}-${arch}.tar.gz"
            EXPECTED_HASH SHA256=465068243b96365513fea855bd8876e25f1e7078e2d805dc1520eccd2ba5dd6a
            ${CMAKE_BINARY_DIR}/deps.tar.gz
            SHOW_PROGRESS
        )

        file(MAKE_DIRECTORY ${deps_root_dir})
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf deps.tar.gz
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

    set(CMAKE_PREFIX_PATH "${deps_root_dir}/share;${deps_root_dir}/lib/cmake;${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
endfunction()
