project (mpisamples)
#add_subdirectory(src)

# Make sure the compiler can find include files from our Hello library.
#include_directories (include /usr/include/libxml2 /usr/local/include/libxml2)

# Make sure the linker can find the Hello library once it is built.
link_directories (bin)

FIND_LIBRARY(MPI_LIBRARY NAMES mpi
  PATHS  /usr/lib /usr/local/lib)
IF (MPI_LIBRARY)
ELSE (MPI_LIBRARY)
        message(SEND_ERROR "You must install MPI before")
ENDIF (MPI_LIBRARY)

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)

add_executable (bin/lastovetsky src/lastovetsky.c)
