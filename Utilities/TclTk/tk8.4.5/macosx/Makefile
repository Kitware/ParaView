################################################################################
#
# Makefile to build AquaTk on Mac OS X packaged as a Framework
#	uses Project Builder command line tool 'pbxbuild'
#
# RCS: @(#) Id
#
################################################################################

#-------------------------------------------------------------------------------
# customizable settings

DESTDIR			?=
INSTALL_ROOT		?= ${DESTDIR}

BUILD_DIR		?= ${CURDIR}/../../build
SYMROOT			?= ${BUILD_DIR}/${PROJECT}
OBJROOT			?= ${SYMROOT}

EXTRA_MAKE_ARGS		?= 

INSTALL_PATH		?= /Library/Frameworks
APPLICATION_INSTALL_PATH ?= /Applications/Utilities

PREFIX			?= /usr
BINDIR			?= ${PREFIX}/bin

TCL_FRAMEWORK_DIR	?= ${BUILD_DIR}/tcl
TCLSH_DIR		?= ${TCL_FRAMEWORK_DIR}

#-------------------------------------------------------------------------------

PROJECT			= tk
TARGET			= Wish

DEVBUILDSTYLE		= Development
DEPBUILDSTYLE		= Deployment

PBXBUILD		= /usr/bin/pbxbuild

MAKE_VARS		:= SYMROOT OBJROOT BINDIR APPLICATION_INSTALL_PATH \
				TCL_FRAMEWORK_DIR TCLSH_DIR
MAKE_ARGS_V		= $(foreach v,${MAKE_VARS},$v=${$v})

BUILD			= ${PBXBUILD} -target "${TARGET}" ${MAKE_ARGS_V} \
				DYLIB_INSTALL_PATH="${INSTALL_PATH}" \
				${EXTRA_MAKE_ARGS} ${MAKEOVERRIDES}

DEVBUILD		= ${BUILD} -buildstyle "${DEVBUILDSTYLE}"
DEPBUILD		= ${BUILD} -buildstyle "${DEPBUILDSTYLE}"

INSTALLOPTS		= INSTALL_ROOT="${INSTALL_ROOT}"

EMBEDDEDOPTS		= EMBEDDED_BUILD=1

################################################################################

all: develop deploy

install: install-develop install-deploy

embedded: embedded-deploy

install-embedded: install-embedded-deploy cleanup-embedded

clean: clean-develop clean-deploy

################################################################################

develop:
	${DEVBUILD}

deploy:
	${DEPBUILD}

install-develop: 
	${DEVBUILD} install ${INSTALLOPTS}

install-deploy:
	${DEPBUILD} install ${INSTALLOPTS}

embedded-develop:
	${DEVBUILD} ${EMBEDDEDOPTS}

embedded-deploy:
	${DEPBUILD} ${EMBEDDEDOPTS}

install-embedded-develop:
	${DEVBUILD} install ${INSTALLOPTS} ${EMBEDDEDOPTS}

install-embedded-deploy:
	${DEPBUILD} install ${INSTALLOPTS} ${EMBEDDEDOPTS}

clean-develop:
	${DEVBUILD} clean

clean-deploy:
	${DEPBUILD} clean

################################################################################

cleanup-embedded:
	@-cd ${INSTALL_ROOT}/; \
	chmod -RH u+w Library/Frameworks/Tcl.framework; \
	rm -rf Library/Frameworks/Tcl.framework; \
	chmod -RH u+w Library/Frameworks/Tk.framework; \
	rm -rf Library/Frameworks/Tk.framework; \
	rmdir -p Library/Frameworks 2>&-;

################################################################################

.PHONY: all install embedded clean develop deploy install-develop install-deploy \
embedded-develop embedded-deploy install-embedded-develop install-embedded-deploy \
clean-develop clean-deploy cleanup-embedded

################################################################################
