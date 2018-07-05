# Toolchain file with HazelHen specific settings

SET(CMAKE_SYSTEM_NAME      Hazelhen)
SET(CMAKE_C_COMPILER       cc)
SET(CMAKE_CXX_COMPILER     CC)
SET(CMAKE_Fortran_COMPILER ftn)
SET(LIBXML2_LIBRARIES      /usr/lib64/libxml2.so)

# Also set
# export CRAYPE_LINK_TYPE=dynamic
# in your environment

