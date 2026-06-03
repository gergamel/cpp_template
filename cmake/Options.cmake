# Project-wide compiler options and warning sets
set(CPP_TEMPLATE_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    -Wformat=2
    -Werror
)
if(MSVC)
    list(APPEND CPP_TEMPLATE_WARNINGS /W4 /permissive- /EHsc)
endif()
