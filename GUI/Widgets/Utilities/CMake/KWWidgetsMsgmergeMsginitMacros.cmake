# This file is a helper file for the gettext translation macros found in
# KWWidgetsInternationalizationMacros.cmake.
# If is used as a custom command in the KWWidgets_CREATE_PO_TARGETS macro.
# This macro refreshes a PO file when its dependency, the POT template file,
# has changed. The problem is that if no PO file has ever been created yet,
# the 'msginit' executable should be used to initialize the PO file from
# the POT file. If it has been created already, the 'msgmerge' executable 
# should be used to merge the PO file with the current POT file.
# Since ADD_CUSTOM_COMMAND does not support such logic, it will use
# this file instead, and execute it using CMake with the relevant parameters.
# Given the parameters, this file will either use 'msginit' or 'msgmerge'.
#
# po_file (string): the original PO file in the source tree, if any
# po_build_file (string): the build PO file to initialize or merge
# default_po_encoding (string): default encoding to initialize PO file with
# pot_build_file (string): the POT file this PO file depends on
# locale (string): the locale this PO file represents (say, "fr")
# GETTEXT_MSGINIT_EXECUTABLE (string): path to the 'msginit' executable
# GETTEXT_MSGCONV_EXECUTABLE (string): path to the 'msgconv' executable 
# GETTEXT_MSGMERGE_EXECUTABLE (string): path to the 'msgmerge' executable 

IF(NOT EXISTS "${po_build_file}")
  IF(EXISTS "${po_file}")
    MESSAGE("Initializing PO file ${po_build_file} as a copy of ${po_file}")
    CONFIGURE_FILE("${po_file}" "${po_build_file}" COPYONLY)
  ELSE(EXISTS "${po_file}")
    IF(NOT "${GETTEXT_MSGINIT_EXECUTABLE}" STREQUAL "")
      MESSAGE("Initializing PO file ${po_build_file}")
      EXEC_PROGRAM(${GETTEXT_MSGINIT_EXECUTABLE} 
        ARGS --input=${pot_build_file} --output-file=${po_build_file} --locale=${locale}
        OUTPUT_VARIABLE msginit_output)
      IF(NOT "${GETTEXT_MSGCONV_EXECUTABLE}" STREQUAL "")
        IF(NOT default_po_encoding)
          SET(default_po_encoding "utf-8")
        ENDIF(NOT default_po_encoding)
        EXEC_PROGRAM(${GETTEXT_MSGCONV_EXECUTABLE} 
          ARGS --output-file=${po_build_file} --to-code="${default_po_encoding}" ${po_build_file})
        #    OUTPUT_VARIABLE msgconv_output)
      ENDIF(NOT "${GETTEXT_MSGCONV_EXECUTABLE}" STREQUAL "")
    ENDIF(NOT "${GETTEXT_MSGINIT_EXECUTABLE}" STREQUAL "")
  ENDIF(EXISTS "${po_file}")
ELSE(NOT EXISTS "${po_build_file}")
#  MESSAGE("Merging PO file ${po_build_file} with POT file ${pot_build_file}")
  # --output-file and --update are mutually exclusive. If --update is
  # specified, the PO file will not be re-written if the result of
  # the merge produces no modification. This can be problematic if the POT
  # file is newer than the PO file, and a MO file is generated from the PO
  # file: this seems to force the MO to always be regenerated.
  IF(NOT "${GETTEXT_MSGMERGE_EXECUTABLE}" STREQUAL "")
    EXEC_PROGRAM(${GETTEXT_MSGMERGE_EXECUTABLE} 
      ARGS --output-file=${po_build_file} ${po_build_file} ${pot_build_file})
    #    OUTPUT_VARIABLE msgmerge_output)
  ENDIF(NOT "${GETTEXT_MSGMERGE_EXECUTABLE}" STREQUAL "")
  # msggrep.exe -T -e "#-#-#" would have done the trick but not in 0.13.1
#   IF(NOT "${GETTEXT_MSGCAT_EXECUTABLE}" STREQUAL "" AND EXISTS "${po_file}")
#     EXEC_PROGRAM(${GETTEXT_MSGCAT_EXECUTABLE} 
#       ARGS ${po_build_file} ${po_file}
#       OUTPUT_VARIABLE msgcat_output)
#     STRING(REGEX MATCH "^\"#-#.*$" matched "${msgcat_output}")
#     MESSAGE("match: ${matched}")
# #    STRING(REGEX REPLACE "^(Python )([0-9]\\.[0-9])(.*)$" "\\2" 
#  #     major_minor "${version}")

#   ENDIF(NOT "${GETTEXT_MSGCAT_EXECUTABLE}" STREQUAL "" AND EXISTS "${po_file}")
ENDIF(NOT EXISTS "${po_build_file}")
