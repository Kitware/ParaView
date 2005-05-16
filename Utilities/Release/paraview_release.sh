#!/bin/sh
#=============================================================================
#
# Program:   ParaView
# Module:    paraview_release.sh
# Language:  C++
# Date:      $Date$
# Version:   $Revision$
#
# Copyright (c) 2002 Kitware, Inc.  All rights reserved.
# See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even 
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
#    PURPOSE.  See the above copyright notices for more information.
#
#=============================================================================

#
# ParaView UNIX Release Script.
#
# Run with no arguments for documentation.
#

# Release version number.
TAG="ParaView-2-0-1"
VERSION="2.0.1"
PARAVIEW_VERSION="2.0"
RELEASE="2"

# Project configuration.
PROJECT="paraview"
CVS_MODULE="ParaView"
CVS_MODULE_DOCS="ParaViewReleaseDocs/${PARAVIEW_VERSION}"

# CVSROOT setting used to check out ParaView.
CVSROOT=":pserver:anonymous@www.paraview.org:/cvsroot/ParaView"
CVSROOT_GREP=":pserver:anonymous@www.paraview.org:[0-9]*/cvsroot/ParaView"
CVS_PASS="paraview"

# Default program names.
CMAKE="cmake"
MAKE="make"

# ParaView release root directory.
RELEASE_ROOT_NAME="ParaViewReleaseRoot"
RELEASE_ROOT="${HOME}/${RELEASE_ROOT_NAME}"

RELEASE_UTILITIES="ParaViewReleaseUtilities"
RELEASE_UTILITIES_CVS="ParaView/Utilities/Release"

# Installation prefix used during tarball creation.  Tarballs are
# relative to the installation prefix and do not include this in their
# paths.
PREFIX="/usr/local"

# Directory relative to PREFIX where documentation should be placed.
DOC_DIR="/doc/paraview-${PARAVIEW_VERSION}"

# No default compiler.  The config file must provide it.
CC=""
CXX=""
CFLAGS=""
CXXFLAGS=""

# Details of remote invocation.
[ -z "$REMOTE" ] && SELF="$0"

#-----------------------------------------------------------------------------
usage()
{
    cat <<EOF
ParaView Release Script Usage:
  $0 [command]

Typical usage:
  Cleanup remote host release directory:

    $0 remote <host> clean

  Create binary release tarball:

    $0 remote_binary <host>

  Create source release tarball:

    $0 remote_source <host>

  Upload tarballs:

    $0 upload

Available commands:

EOF
    cat "$0" | awk '
/^#--*$/               { doc=1; text="" }

/(^#$|^#[^-].*$)/     {
  if(doc)
    {
    if(text != "") { text = sprintf("%s  %s\n", text, $0) }
    else           { text = sprintf("  %s\n", $0) }
    }
}

/^[A-Za-z0-9_]*\(\)$/ {
  doc=0;
  if(text != "") { printf("%s:\n%s\n", $0, text) }
}
'
}

#-----------------------------------------------------------------------------
error_log()
{
    echo "An error has been logged to $1:" &&
    cat "$1" &&
    return 1
}

#-----------------------------------------------------------------------------
check_host()
{
    HOST="$1"
    if [ -z "$HOST" ]; then
        echo "Must specify host."
        return 1
    fi
}

#-----------------------------------------------------------------------------
# Run a command on the specified remote host.
#
#  remote <host> [command]
#
# Only one level of remote invocation is allowed.  The <host>
# specification must be a valid ssh destination with public
# key authentication and no password.
remote()
{
    if [ ! -z "$REMOTE" ]; then
        echo "Cannot do recursive remote calls."
        return 1
    fi
    check_host "$1" || return 1
    shift
    RTASK="'$1'"; shift; for i in "$@"; do RTASK="${RTASK} '$i'"; done
    RESULT=0
    echo "------- Running remote task on $HOST. -------" &&
    (echo "REMOTE=\"1\"" &&
        (echo TASK=\"`(eval echo '${RTASK}') | (sed 's/"/\\\\"/g')`\") &&
        cat $SELF) | ssh -e none "$HOST" /bin/sh  || RESULT=1
    echo "-------- Remote task on $HOST done.  --------" &&
    return $RESULT
}

#-----------------------------------------------------------------------------
# Copy tarballs from the specified host.
#
#  remote_copy <host> [EXPR]
#
# The <host> specification must be a valid ssh destination
# with public key authentication and no password.  Only
# files matching the given expression are copied.  If
# no expression is given, "*" is used.
remote_copy()
{
    check_host "$1" || return 1
    EXPR="$2"
    [ ! -z "$EXPR" ] || EXPR="*"
    echo "------- Copying tarballs from $HOST. -------" &&
    scp "$HOST:${RELEASE_ROOT_NAME}/Tarballs/${EXPR}" . &&
    echo "---- Done copying tarballs from $HOST. -----"
}

#-----------------------------------------------------------------------------
remote_copy_source()
{
    check_host "$1" || return 1
    remote_copy "$HOST" "${PROJECT}-${VERSION}.*"
}

#-----------------------------------------------------------------------------
remote_copy_docs()
{
    check_host "$1" || return 1
    remote_copy "$HOST" "${PROJECT}-docs-${VERSION}.*"
}

#-----------------------------------------------------------------------------
remote_copy_binary()
{
    check_host "$1" || return 1
    remote_copy "$HOST" "${PROJECT}-${VERSION}-*"
}

#-----------------------------------------------------------------------------
# Create source tarballs on the specified host and copy them locally.
#
#  remote_source <host>
#
# The <host> specification must be a valid ssh destination
# with public key authentication and no password.
remote_source()
{
    check_host "$1" || return 1
    remote "$HOST" source_tarball &&
    remote_copy_source "$HOST"
}

#-----------------------------------------------------------------------------
# Create documentation tarballs on the specified host and copy them locally.
#
#  remote_docs <host>
#
# The <host> specification must be a valid ssh destination
# with public key authentication and no password.
remote_docs()
{
    check_host "$1" || return 1
    remote "$HOST" docs_tarball &&
    remote_copy_docs "$HOST"
}

#-----------------------------------------------------------------------------
# Create binary tarballs on the specified host and copy them locally.
#
#  remote_binary <host>
#
# The <host> specification must be a valid ssh destination
# with public key authentication and no password.
remote_binary()
{
    check_host "$1" || return 1
    remote "$HOST" binary_tarball &&
    remote_copy_binary "$HOST"
}

#-----------------------------------------------------------------------------
# Upload any tarballs in the current directory to the ParaView FTP site.
#
#  upload
#
# The user must be able to ssh to kitware@www.paraview.org with public
# key authentication and no password.
upload()
{
    echo "------- Copying tarballs to www.paraview.org. -------"
    files=`ls ${PROJECT}-${VERSION}*tar.* \
           ${PROJECT}-docs-${VERSION}*tar.* \
           ${PROJECT}-${VERSION}.zip \
           ${PROJECT}-docs-${VERSION}.zip`

    scp ${files} kitware@www.paraview.org:/projects/FTP/pub/paraview/v${PARAVIEW_VERSION}
    echo "---- Done copying tarballs to www.paraview.org. -----"
}

#-----------------------------------------------------------------------------
setup()
{
    [ -z "${DONE_setup}" ] || return 0 ; DONE_setup="yes"
    mkdir -p ${RELEASE_ROOT}/Logs &&
    echo "Entering ${RELEASE_ROOT}" &&
    cd ${RELEASE_ROOT}
}

#-----------------------------------------------------------------------------
# Remove the release root directory.
#
#  clean
#
clean()
{
    cd "${HOME}" &&
    echo "Cleaning up ${RELEASE_ROOT}" &&
    rm -rf "${RELEASE_ROOT_NAME}"
}

#-----------------------------------------------------------------------------
cvs_login()
{
    [ -z "${DONE_cvs_login}" ] || return 0 ; DONE_cvs_login="yes"
    setup || return 1
    (
        if [ -f "${HOME}/.cvspass" ]; then
            CVSPASS="${HOME}/.cvspass"
        else
            CVSPASS=""
        fi
        if [ -z "`grep \"$CVSROOT_GREP\" ${CVSPASS} /dev/null`" ]; then
            echo "${CVS_PASS}" | cvs -q -z3 -d $CVSROOT login
        else
            echo "Already logged in."
        fi
    ) >Logs/cvs_login.log 2>&1 || error_log Logs/cvs_login.log
}

#-----------------------------------------------------------------------------
utilities()
{
    [ -z "${DONE_utilities}" ] || return 0 ; DONE_utilities="yes"
    cvs_login || return 1
    (
        if [ -d "${RELEASE_UTILITIES}/CVS" ]; then
            cd ${RELEASE_UTILITIES} && cvs -z3 -q update -dAP -r ${TAG}
        else
            rm -rf CheckoutTemp &&
            mkdir CheckoutTemp &&
            cd CheckoutTemp &&
            cvs -q -z3 -d $CVSROOT co -r ${TAG} "${RELEASE_UTILITIES_CVS}" &&
            mv "${RELEASE_UTILITIES_CVS}" "../${RELEASE_UTILITIES}" &&
            cd .. &&
            rm -rf CheckoutTemp
        fi
    ) >Logs/utilities.log 2>&1 || error_log Logs/utilities.log
}

#-----------------------------------------------------------------------------
config()
{
    [ -z "${DONE_config}" ] || return 0 ; DONE_config="yes"
    utilities || return 1
    CONFIG_FILE="config_`uname`"
    echo "Loading ${CONFIG_FILE} ..."
    . ${RELEASE_ROOT}/${RELEASE_UTILITIES}/${CONFIG_FILE} >Logs/config.log 2>&1 || error_log Logs/config.log
    if [ -z "${CC}" ] || [ -z "${CXX}" ] || [ -z "${PLATFORM}" ]; then
        echo "${CONFIG_FILE} should specify CC, CXX, and PLATFORM." &&
        return 1
    fi
    export CC CXX CFLAGS CXXFLAGS LDFLAGS PATH LD_LIBRARY_PATH DISPLAY
}

#-----------------------------------------------------------------------------
checkout()
{
    [ -z "${DONE_checkout}" ] || return 0 ; DONE_checkout="yes"
    config || return 1
    echo "Updating ${CVS_MODULE} from cvs ..." &&
    (
        if [ -d ${PROJECT}-${VERSION}/CVS ]; then
            cd ${PROJECT}-${VERSION} &&
            cvs -q -z3 -d $CVSROOT update -dAP -r ${TAG} &&
            rm -rf Xdmf/Utilities/expat &&
            rm -rf Xdmf/Utilities/zlib
        else
            rm -rf ${PROJECT}-${VERSION} &&
            rm -rf CheckoutTemp &&
            mkdir CheckoutTemp &&
            cd CheckoutTemp &&
            cvs -q -z3 -d $CVSROOT co -r ${TAG} ${CVS_MODULE} &&
            rm -rf ${CVS_MODULE}/Xdmf/Utilities/expat &&
            rm -rf ${CVS_MODULE}/Xdmf/Utilities/zlib &&
            mv ${CVS_MODULE} ../${PROJECT}-${VERSION} &&
            cd .. &&
            rm -rf CheckoutTemp
        fi
    ) >Logs/checkout.log 2>&1 || error_log Logs/checkout.log
}

#-----------------------------------------------------------------------------
checkout_docs()
{
    [ -z "${DONE_checkout_docs}" ] || return 0 ; DONE_checkout_docs="yes"
    config || return 1
    echo "Exporting ${CVS_MODULE_DOCS} from cvs ..." &&
    (
        rm -rf ${PROJECT}-docs-${VERSION} &&
        rm -rf CheckoutTemp &&
        mkdir CheckoutTemp &&
        cd CheckoutTemp &&
        cvs -q -z3 -d $CVSROOT export -r HEAD ${CVS_MODULE_DOCS} &&
        mv ${CVS_MODULE_DOCS} ../${PROJECT}-docs-${VERSION} &&
        cd .. &&
        rm -rf CheckoutTemp
    ) >Logs/checkout_docs.log 2>&1 || error_log Logs/checkout_docs.log
}

#-----------------------------------------------------------------------------
# Create source tarballs.
#
#  source_tarball
#
source_tarball()
{
    [ -z "${DONE_source_tarball}" ] || return 0 ; DONE_source_tarball="yes"
    config || return 1
    [ -d "${PROJECT}-${VERSION}" ] || checkout || return 1
    echo "Creating source tarballs ..." &&
    (
        mkdir -p Tarballs &&
        rm -rf Tarballs/${PROJECT}-${VERSION}.tar* &&
        tar cvf Tarballs/${PROJECT}-${VERSION}.tar --exclude CVS ${PROJECT}-${VERSION} &&
        gzip -c Tarballs/${PROJECT}-${VERSION}.tar >Tarballs/${PROJECT}-${VERSION}.tar.gz &&
        compress -cf Tarballs/${PROJECT}-${VERSION}.tar >Tarballs/${PROJECT}-${VERSION}.tar.Z &&
        rm -rf Tarballs/${PROJECT}-${VERSION}.tar
    ) >Logs/source_tarball.log 2>&1 || error_log Logs/source_tarball.log
}

#-----------------------------------------------------------------------------
# Create a source zipfile.
#
#  source_zipfile
#
source_zipfile()
{
    [ -z "${DONE_source_zipfile}" ] || return 0 ; DONE_source_zipfile="yes"
    config || return 1
    [ -d "${PROJECT}-${VERSION}" ] || checkout || return 1
    echo "Creating source zipfile ..." &&
    (
        mkdir -p Tarballs &&
        rm -rf Tarballs/${PROJECT}-${VERSION}.zip &&
        rm -rf Tarballs/${PROJECT}-${VERSION} &&
        tar c --exclude CVS ${PROJECT}-${VERSION} | (cd Tarballs; tar x) &&
        cd Tarballs &&
        zip -r ${PROJECT}-${VERSION}.zip ${PROJECT}-${VERSION} &&
        rm -rf ${PROJECT}-${VERSION}
    ) >Logs/source_zipfile.log 2>&1 || error_log Logs/source_zipfile.log
}

#-----------------------------------------------------------------------------
# Create documentation tarballs.
#
#  docs_tarball
#
docs_tarball()
{
    [ -z "${DONE_docs_tarball}" ] || return 0 ; DONE_docs_tarball="yes"
    config || return 1
    [ -d "${PROJECT}-docs-${VERSION}" ] || checkout_docs || return 1
    echo "Creating documentation tarballs ..." &&
    (
        mkdir -p Tarballs &&
        rm -rf Tarballs/${PROJECT}-docs-${VERSION}.tar* &&
        tar cvf Tarballs/${PROJECT}-docs-${VERSION}.tar ${PROJECT}-docs-${VERSION} &&
        gzip -c Tarballs/${PROJECT}-docs-${VERSION}.tar >Tarballs/${PROJECT}-docs-${VERSION}.tar.gz &&
        compress -cf Tarballs/${PROJECT}-docs-${VERSION}.tar >Tarballs/${PROJECT}-docs-${VERSION}.tar.Z &&
        rm -rf Tarballs/${PROJECT}-docs-${VERSION}.tar
    ) >Logs/docs_tarball.log 2>&1 || error_log Logs/docs_tarball.log
}

#-----------------------------------------------------------------------------
# Create a documentation zipfile.
#
#  docs_zipfile
#
docs_zipfile()
{
    [ -z "${DONE_docs_zipfile}" ] || return 0 ; DONE_docs_zipfile="yes"
    config || return 1
    [ -d "${PROJECT}-docs-${VERSION}" ] || checkout_docs || return 1
    echo "Creating documentation zipfile ..." &&
    (
        mkdir -p Tarballs &&
        rm -rf Tarballs/${PROJECT}-docs-${VERSION}.zip &&
        zip -r Tarballs/${PROJECT}-docs-${VERSION}.zip ${PROJECT}-docs-${VERSION}
    ) >Logs/docs_zipfile.log 2>&1 || error_log Logs/docs_zipfile.log
}

#-----------------------------------------------------------------------------
write_standard_cache()
{
    cat > CMakeCache.txt <<EOF
BUILD_SHARED_LIBS:BOOL=ON
BUILD_TESTING:BOOL=ON
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_INSTALL_PREFIX:PATH=/usr/local
CMAKE_SKIP_RPATH:BOOL=1
CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE
VTK_USE_64BIT_IDS:BOOL=OFF
VTK_USE_HYBRID:BOOL=ON
VTK_USE_MPI:BOOL=OFF
VTK_USE_PARALLEL:BOOL=ON
VTK_USE_PATENTED:BOOL=ON
VTK_USE_RENDERING:BOOL=ON
EOF
}

#-----------------------------------------------------------------------------
write_cache()
{
    write_standard_cache
}

#-----------------------------------------------------------------------------
cache()
{
    [ -z "${DONE_cache}" ] || return 0 ; DONE_cache="yes"
    config || return 1
    echo "Writing CMakeCache.txt ..." &&
    (
        rm -rf "${PROJECT}-${VERSION}-${PLATFORM}" &&
        mkdir -p "${PROJECT}-${VERSION}-${PLATFORM}" &&
        cd "${PROJECT}-${VERSION}-${PLATFORM}" &&
        write_cache
    ) >Logs/cache.log 2>&1 || error_log Logs/cache.log
}

#-----------------------------------------------------------------------------
configure()
{
    [ -z "${DONE_configure}" ] || return 0 ; DONE_configure="yes"
    config || return 1
    [ -d "${PROJECT}-${VERSION}" ] || checkout || return 1
    cache || return 1
    echo "Running cmake ..." &&
    (
        cd "${PROJECT}-${VERSION}-${PLATFORM}" &&
        ${CMAKE} ../${PROJECT}-${VERSION} -DCMAKE_INSTALL_PREFIX:PATH=${PREFIX}
    ) >Logs/configure.log 2>&1 || error_log Logs/configure.log
}

#-----------------------------------------------------------------------------
build()
{
    [ -z "${DONE_build}" ] || return 0 ; DONE_build="yes"
    config || return 1
    if [ ! -d "${PROJECT}-${VERSION}-${PLATFORM}/Utilities" ]; then
        configure || return 1
    fi
    echo "Running make ..." &&
    (
        cd "${PROJECT}-${VERSION}-${PLATFORM}" &&
        ${MAKE}
    ) >Logs/build.log 2>&1 || error_log Logs/build.log
}

#-----------------------------------------------------------------------------
install()
{
    [ -z "${DONE_install}" ] || return 0 ; DONE_install="yes"
    config || return 1
    [ -f "${PROJECT}-${VERSION}-${PLATFORM}/bin/paraview" ] || build || return 1
    echo "Running make install ..." &&
    (
        rm -rf Install &&
        (
            cd "${PROJECT}-${VERSION}-${PLATFORM}" &&
            ${MAKE} install DESTDIR="${RELEASE_ROOT}/Install"
        ) &&
        rm -rf Install/usr/local/include Install/usr/local/lib/vtk
    ) >Logs/install.log 2>&1 || error_log Logs/install.log
}

#-----------------------------------------------------------------------------
strip()
{
    [ -z "${DONE_strip}" ] || return 0 ; DONE_strip="yes"
    config || return 1
    [ -f "Install${PREFIX}/bin/paraview" ] || install || return 1
    echo "Stripping executables ..." &&
    (
        strip Install${PREFIX}/bin/*
    ) >Logs/strip.log 2>&1 || error_log Logs/strip.log
}

#-----------------------------------------------------------------------------
manifest()
{
    [ -z "${DONE_manifest}" ] || return 0 ; DONE_manifest="yes"
    config || return 1
    [ -f "Install${PREFIX}/bin/paraview" ] || install || return 1
    echo "Writing MANIFEST ..." &&
    (
        mkdir -p Install${PREFIX}${DOC_DIR} &&
        rm -rf Install${PREFIX}${DOC_DIR}/MANIFEST &&
        touch Install${PREFIX}${DOC_DIR}/MANIFEST &&
        cd Install${PREFIX} &&
        FILES=`find . -type f |sed 's/^\.\///'` &&
        cd ${RELEASE_ROOT} &&
        (cat >> Install${PREFIX}${DOC_DIR}/MANIFEST <<EOF
${FILES}
EOF
        ) &&
        rm -rf Install/README &&
        (cat > Install/README <<EOF
ParaView $VERSION binary for $PLATFORM

Extract the file "${PROJECT}-${VERSION}-${PLATFORM}-files.tar" into your
destination directory (typically /usr/local).  The following files will
be extracted:

${FILES}

EOF
        )
    ) >Logs/manifest.log 2>&1 || error_log Logs/manifest.log
}

#-----------------------------------------------------------------------------
# Create binary tarballs.
#
#  binary_tarball
#
binary_tarball()
{
    [ -z "${DONE_binary_tarball}" ] || return 0 ; DONE_binary_tarball="yes"
    config || return 1
    strip || return 1
    manifest || return 1
    echo "Creating binary tarballs ..." &&
    (
        mkdir -p Tarballs &&
        rm -rf Install/${PROJECT}-${VERSION}-${PLATFORM}-files.tar &&
        (
            cd Install${PREFIX} &&
            tar cvf ${RELEASE_ROOT}/Install/${PROJECT}-${VERSION}-${PLATFORM}-files.tar *
        ) &&
        rm -rf Tarballs/${PROJECT}-${VERSION}-${PLATFORM}.tar* &&
        (
            cd Install &&
            tar cvf ${RELEASE_ROOT}/Tarballs/${PROJECT}-${VERSION}-${PLATFORM}.tar ${PROJECT}-${VERSION}-${PLATFORM}-files.tar README
        ) &&
        (
            cd Tarballs &&
            gzip -c ${PROJECT}-${VERSION}-${PLATFORM}.tar >${PROJECT}-${VERSION}-${PLATFORM}.tar.gz &&
            compress -cf ${PROJECT}-${VERSION}-${PLATFORM}.tar >${PROJECT}-${VERSION}-${PLATFORM}.tar.Z &&
            rm -rf ${PROJECT}-${VERSION}-${PLATFORM}.tar
        )
    ) >Logs/binary_tarball.log 2>&1 || error_log Logs/binary_tarball.log
}

#-----------------------------------------------------------------------------
run()
{
    CMD="'$1'"; shift; for i in "$@"; do CMD="${CMD} '$i'"; done
    eval "$CMD"
}

# Determine task and evaluate it.
if [ -z "$TASK" ] && [ -z "$REMOTE" ] ; then
    if [ -z "$1" ]; then
        usage
    else
        run "$@"
    fi
else
    [ -z "$TASK" ] || eval run "$TASK"
fi
