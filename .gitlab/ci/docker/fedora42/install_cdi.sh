#!/bin/sh

set -e

readonly cdi_repo="https://gitlab.dkrz.de/mpim-sw/libcdi.git"
readonly cdi_commit="cdi-2.5.2.1"

readonly cdi_root="$HOME/cdi"
readonly cdi_src="$cdi_root/src"
readonly cdi_build_root="$cdi_root/build"

git clone "$cdi_repo" "$cdi_src"
git -C "$cdi_src" checkout "$cdi_commit"

# Patch a CMake issue in cdi
# https://gitlab.dkrz.de/mpim-sw/libcdi/-/issues/22
patch -d "$cdi_src" -p1 <<'EOF'
diff --git a/CMakeLists.txt b/CMakeLists.txt
index 353ad94d..8ebe0f8e 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -63,7 +63,7 @@ endif()
 # NetCDF
 option(CDI_NETCDF "Use the netcdf library [default=ON]" ON)
 if(${CDI_NETCDF} OR netCDF_ROOT )
-  find_package(netCDF COMPONENTS C REQUIRED)
+  find_package(NetCDF COMPONENTS C REQUIRED)
   if (TARGET netCDF::netcdf)
     list(APPEND cdi_compile_defs
       HAVE_LIBNETCDF=${netCDF_FOUND}
@@ -71,6 +71,13 @@ if(${CDI_NETCDF} OR netCDF_ROOT )
       HAVE_NETCDF4=${netCDF_FOUND}
     )
     target_link_libraries(cdilib netCDF::netcdf)
+  elseif (TARGET NetCDF::NetCDF)
+    list(APPEND cdi_compile_defs
+           HAVE_LIBNETCDF=${NetCDF_FOUND}
+           HAVE_LIBNC_DAP=${NetCDF_FOUND}
+           HAVE_NETCDF4=${NetCDF_FOUND}
+    )
+    target_link_libraries(cdilib NetCDF::NetCDF)
   endif ()
 endif()

EOF

dnf install -y --setopt=install_weak_deps=False \
    libtool eccodes eccodes-devel

cdi_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$cdi_src" \
        -B "$cdi_build_root/$subdir" \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_TESTING=OFF \
        -DCDI_BUILD_APP=OFF \
        -DCDI_BUILD_UNKNOWN=OFF \
        -DCDI_ECCODES=OFF \
        -DCDI_EXTRA=OFF \
        -DCDI_IEG=OFF \
        -DCDI_LIBGRIB=OFF \
        -DCDI_LIBGRIBEX=OFF \
        -DCDI_NETCDF=ON \
        -DCDI_PTHREAD=OFF \
        -DCDI_SERVICE=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_MODULE_PATH="$cdi_src/cmake" \
        "$@"
    cmake --build "$cdi_build_root/$subdir" --target install
}

# MPI-less
cdi_build nompi /usr

rm -rf "$cdi_root"

dnf clean all
