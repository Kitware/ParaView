########################################################################################################
#
# Makefile to build Tcl on Mac OS X packaged as a Framework
#	uses standard unix build system in tcl/unix
#
# RCS: @(#) Id
#
########################################################################################################

#-------------------------------------------------------------------------------------------------------
# customizable settings

DESTDIR			?=
INSTALL_ROOT		?= ${DESTDIR}

BUILD_DIR		?= ${CURDIR}/../../build
SYMROOT			?= ${BUILD_DIR}/${PROJECT}
OBJROOT			?= ${SYMROOT}

EXTRA_CONFIGURE_ARGS 	?= 
EXTRA_MAKE_ARGS		?= 

INSTALL_PATH		?= /Library/Frameworks
PREFIX			?= /usr
BINDIR			?= ${PREFIX}/bin
MANDIR			?= ${PREFIX}/man

# set to non-empty value to install manpages in addition to html help:
INSTALL_MANPAGES 	?= 

TCL_PACKAGE_PATH	?= "~/Library/Tcl /Library/Tcl /Network/Library/Tcl /System/Library/Tcl \
			    ~/Library/Frameworks /Library/Frameworks /Network/Library/Frameworks \
			    /System/Library/Frameworks"

#-------------------------------------------------------------------------------------------------------
# meta targets

meta 			:= all install embedded install-embedded clean distclean test

styles			:= develop deploy

all			:= ${styles}
all			: ${all}

install			:= ${styles:%=install-%}
install			: ${install}
install-%:		action := install-

embedded		:= ${styles:%=embedded-%}
embedded		: embedded-deploy
install-embedded	:= $(embedded:%=install-%)
install-embedded	: install-embedded-deploy

clean			:= ${styles:%=clean-%}
clean			: ${clean}
clean-%:		action := clean-
distclean		:= ${styles:%=distclean-%}
distclean		: ${distclean}
distclean-%:		action := distclean-

test			:= ${styles:%=test-%}
test			: ${test}
test-%:			action := test-

targets			:= $(foreach v,${meta},${$v})

#-------------------------------------------------------------------------------------------------------
# build styles

develop_make_args	:= BUILD_STYLE=Development CONFIGURE_ARGS=--enable-symbols
deploy_make_args	:= BUILD_STYLE=Deployment \
			   MAKE_ARGS=INSTALL_PROGRAM="'$$\$${INSTALL} $$\$${INSTALL_STRIP_PROGRAM}'" \
			   MAKE_ARGS+=INSTALL_LIBRARY="'$$\$${INSTALL} $$\$${INSTALL_STRIP_LIBRARY}'" \
			   MAKE_ARGS+=MEM_DEBUG_FLAGS="-DNDEBUG"
embedded_make_args	:= EMBEDDED_BUILD=1
install_make_args	:= INSTALL_BUILD=1

$(targets): 
	${MAKE} ${action}${PROJECT} \
	$(foreach s,${styles} embedded install,$(if $(findstring $s,$@),${${s}_make_args}))

#-------------------------------------------------------------------------------------------------------
# project specific settings

PROJECT			:= tcl
PRODUCT_NAME		:= Tcl

UNIX_DIR		:= ${CURDIR}/../unix
GENERIC_DIR		:= ${CURDIR}/../generic

PRODUCT_VERSION		:= $(shell eval $$(grep '^TCL_VERSION=' ${UNIX_DIR}/configure.in); \
				echo "$${TCL_VERSION}")
PRODUCT_LONGVERSION	:= $(shell eval $$(grep '^TCL_PATCH_LEVEL=' ${UNIX_DIR}/configure.in); \
				echo "${PRODUCT_VERSION}$${TCL_PATCH_LEVEL}")

PRIVATE_HEADERS		:= tclInt.h tclIntDecls.h tclIntPlatDecls.h tclMath.h
TARGETS			:= tclsh tcltest
TCLSH			:= tclsh${PRODUCT_VERSION}
TCL_EXE			?= ${SYMROOT}/${TCLSH}

DYLIB_INSTALL_PATH	?= ${INSTALL_PATH}

TCL_LIBRARY		:= @TCL_IN_FRAMEWORK@
LIBDIR			:= ${INSTALL_PATH}/${PRODUCT_NAME}.framework/Versions/${PRODUCT_VERSION}
DYLIB_INSTALL_DIR	:= ${DYLIB_INSTALL_PATH}/${PRODUCT_NAME}.framework/Versions/${PRODUCT_VERSION}
INCLUDEDIR		:= ${LIBDIR}/Headers
PRIVATEINCLUDEDIR	:= ${LIBDIR}/PrivateHeaders
SCRIPTDIR		:= ${LIBDIR}/Resources/Scripts
DOCDIR			:= ${LIBDIR}/Resources/English.lproj/Documentation/Reference
INFOPLIST		:= ${LIBDIR}/Resources/Info.plist

BUILD_STYLE		=
OBJ_DIR			= ${OBJROOT}/${BUILD_STYLE}

${PROJECT}:		override INSTALL_ROOT = ${OBJ_DIR}/

MAKE_VARS		:= INSTALL_ROOT TCL_PACKAGE_PATH TCL_LIBRARY DYLIB_INSTALL_DIR
MAKE_ARGS_V		= $(foreach v,${MAKE_VARS},$v=${$v})
export CPPROG		:= cp -p

#-------------------------------------------------------------------------------------------------------
# build rules

${PROJECT}: install-${PROJECT}

${OBJ_DIR}/Makefile: ${UNIX_DIR}/Makefile.in ${UNIX_DIR}/configure
	mkdir -p ${OBJ_DIR} && cd ${OBJ_DIR} && ${UNIX_DIR}/configure \
	--prefix=${PREFIX} --bindir=${BINDIR} --libdir=${LIBDIR} \
	--includedir=${INCLUDEDIR} --mandir=${MANDIR} --enable-threads \
	--enable-framework ${CONFIGURE_ARGS} ${EXTRA_CONFIGURE_ARGS}
	cd ${OBJ_DIR} && mkdir -p ${PRODUCT_NAME}.framework && \
	ln -fs ../${PRODUCT_NAME} ${PRODUCT_NAME}.framework/${PRODUCT_NAME}

build-${PROJECT}: ${OBJ_DIR}/Makefile
	${MAKE} -C ${OBJ_DIR} ${TARGETS} ${MAKE_ARGS_V} ${MAKE_ARGS} ${EXTRA_MAKE_ARGS}
# symolic link hackery to trick
# 'make install INSTALL_ROOT=${OBJ_DIR}'
# into building Tcl.framework and tclsh in ${SYMROOT}
	cd ${OBJ_DIR}; mkdir -p $(dir ./${INSTALL_PATH}) $(dir ./${BINDIR}) ${SYMROOT}; \
	rm -f ./${INSTALL_PATH}; ln -fs ${SYMROOT} ./${INSTALL_PATH}; \
	rm -f ./${BINDIR}; ln -fs ${SYMROOT} ./${BINDIR}; \
	ln -fs ${OBJ_DIR}/tcltest ${SYMROOT}

clean-${PROJECT}:
	${MAKE} -C ${OBJ_DIR} clean ${EXTRA_MAKE_ARGS}

distclean-${PROJECT}:
	${MAKE} -C ${OBJ_DIR} distclean ${EXTRA_MAKE_ARGS}
	rm -rf ${OBJ_DIR} ${PRODUCT_NAME}.framework tclsh${PRODUCT_VERSION} tcltest
	
test-${PROJECT}: build-${PROJECT}
	${MAKE} -C ${OBJ_DIR} test ${EXTRA_MAKE_ARGS}

install-${PROJECT}: build-${PROJECT}
# install to ${INSTALL_ROOT} with optional stripping
	${MAKE} -C ${OBJ_DIR} install-binaries install-libraries \
	SCRIPT_INSTALL_DIR=${INSTALL_ROOT}${SCRIPTDIR} ${MAKE_ARGS_V} ${MAKE_ARGS} ${EXTRA_MAKE_ARGS}
	mkdir -p ${INSTALL_ROOT}${PRIVATEINCLUDEDIR} && \
	cd ${GENERIC_DIR} && ${CPPROG} ${PRIVATE_HEADERS} ${INSTALL_ROOT}${PRIVATEINCLUDEDIR}
ifeq (${BUILD_STYLE},Development)
# keep copy of debug library around, so that
# Deployment build can be installed on top
# of Development build without overwriting
# the debug library
	cd ${INSTALL_ROOT}${LIBDIR} && ln -f "${PRODUCT_NAME}" "${PRODUCT_NAME}_debug"
endif
# fixup Framework structure
	cd ${INSTALL_ROOT}${LIBDIR}/.. && \
	rm -f Current && ln -fs ${PRODUCT_VERSION} Current && \
	cd .. && ln -fs Versions/Current/* . && \
	ln -fs Versions/${PRODUCT_VERSION}/lib*stub* .
ifeq (${INSTALL_BUILD},1)
ifeq (${EMBEDDED_BUILD},1)
# if we are embedding frameworks, don't install tclsh
	rm -f "${INSTALL_ROOT}${BINDIR}/${TCLSH}"
	-rmdir -p "${INSTALL_ROOT}${BINDIR}" 2>&-
else
# redo prebinding
	cd ${INSTALL_ROOT}/; \
	if [ ! -d usr/lib ]; then mkdir -p usr; ln -fs /usr/lib usr/; RM_USRLIB=1; fi; \
	if [ ! -d System ]; then ln -fs /System .; RM_SYSTEM=1; fi; \
	redo_prebinding -r . "./${BINDIR}/${TCLSH}"; \
	if [ -n "$${RM_USRLIB:-}" ]; then rm -f usr/lib; rmdir -p usr 2>&-; fi; \
	if [ -n "$${RM_SYSTEM:-}" ]; then rm -f System; fi
# install tclsh symbolic link
	ln -fs ${TCLSH} ${INSTALL_ROOT}${BINDIR}/tclsh
ifeq (${BUILD_STYLE},Deployment)
ifneq (${INSTALL_MANPAGES},)
# install manpages
	${MAKE} -C ${OBJ_DIR} install-doc ${MAKE_ARGS_V} ${MAKE_ARGS} ${EXTRA_MAKE_ARGS}
endif
# build html documentation
	export DYLD_FRAMEWORK_PATH=${SYMROOT} && \
	${MAKE} -C ${OBJ_DIR} html-tcl ${MAKE_ARGS_V} ${MAKE_ARGS} ${EXTRA_MAKE_ARGS} \
	DISTDIR=${INSTALL_ROOT}${DOCDIR} TCL_EXE=${TCL_EXE} && \
	cd ${INSTALL_ROOT}${DOCDIR} && ln -fs contents.htm html/${PRODUCT_NAME}TOC.html && \
	rm -fr "${PRODUCT_NAME}" && mv -f html "${PRODUCT_NAME}"
endif
endif
endif
# write Info.plist file
	@printf > ${INSTALL_ROOT}${INFOPLIST} '\
	<?xml version="1.0" encoding="UTF-8"?>\n\
	<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN"\
	"http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n\
	<plist version="1.0">\n\
	<dict>\n\
		<key>CFBundleDevelopmentRegion</key>\n\
		<string>English</string>\n\
		<key>CFBundleExecutable</key>\n\
		<string>Tcl</string>\n\
		<key>CFBundleGetInfoString</key>\n\
		<string>Tcl Library ${PRODUCT_VERSION}, Copyright © 2003 Tcl Core Team.\n\
	MacOS X Port by Jim Ingham &lt;jingham@apple.com&gt; &amp; Ian Reid, Copyright\
	© 2001-2002, Apple Computer, Inc.</string>\n\
		<key>CFBundleIdentifier</key>\n\
		<string>com.tcltk.tcllibrary</string>\n\
		<key>CFBundleInfoDictionaryVersion</key>\n\
		<string>6.0</string>\n\
		<key>CFBundleName</key>\n\
		<string>Tcl Library ${PRODUCT_VERSION}</string>\n\
		<key>CFBundlePackageType</key>\n\
		<string>FMWK</string>\n\
		<key>CFBundleShortVersionString</key>\n\
		<string>${PRODUCT_LONGVERSION}</string>\n\
		<key>CFBundleSignature</key>\n\
		<string>Tcl </string>\n\
		<key>CFBundleVersion</key>\n\
		<string>${PRODUCT_LONGVERSION}</string>\n\
	</dict>\n\
	</plist>\n'

#-------------------------------------------------------------------------------------------------------

.PHONY: ${meta} ${targets} ${PROJECT} build-${PROJECT} install-${PROJECT} \
	clean-${PROJECT} distclean-${PROJECT}

#-------------------------------------------------------------------------------------------------------
