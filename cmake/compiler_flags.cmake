function(set_compiler_flags target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -pedantic
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wformat=2
        $<$<CXX_COMPILER_ID:GNU>:
            -Wmisleading-indentation 
            -Wduplicated-cond 
            -Wduplicated-branches 
            -Wlogical-op 
            -Wuseless-cast
            $<$<CONFIG:Release>: -Wnull-dereference>
        >
        # Already included in -Wextra for gcc
        $<$<CXX_COMPILER_ID:Clang>:
            -Wimplicit-fallthrough
        >
    )

    target_compile_options(${target}
        PUBLIC $<$<CONFIG:Debug>:
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -Og
        >
        PRIVATE $<$<CONFIG:Release>: -flto -O3>
        PRIVATE -std=c++17
    )

    target_link_options(${target}
        PUBLIC $<$<CONFIG:Debug>:
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
        >
        PRIVATE $<$<CONFIG:Release>: -flto>
    )
endfunction()
