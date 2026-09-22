# CMakeLists.txt of MOONCAKE component

# Src files
file(GLOB_RECURSE MOONCAKE_SRCS
    ${MOONCAKE_ROOT_DIR}/src/*.c
    ${MOONCAKE_ROOT_DIR}/src/*.cc
    ${MOONCAKE_ROOT_DIR}/src/*.cpp
)
# Include
set(MOONCAKE_INCS
    ${MOONCAKE_ROOT_DIR}/src/
)

# Public component requirement
set(MOONCAKE_REQUIRES
)

# Private component requirement
set(MOONCAKE_PRIV_REQUIRES
)

# Register component
idf_component_register(
    SRCS ${MOONCAKE_SRCS}
    INCLUDE_DIRS ${MOONCAKE_INCS}
    REQUIRES ${MOONCAKE_REQUIRES}
    PRIV_REQUIRES ${MOONCAKE_PRIV_REQUIRES}
)
