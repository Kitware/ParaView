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
# po_file (string): the PO file to initialize or merge
# default_po_encoding (string): default encoding to initialize PO file with
# pot_file (string): the POT file this PO file depends on
# locale (string): the locale this PO file represents (say, "fr")
# GETTEXT_MSGINIT_EXECUTABLE (string): path to the 'msginit' executable
# GETTEXT_MSGCONV_EXECUTABLE (string): path to the 'msgconv' executable 
# GETTEXT_MSGMERGE_EXECUTABLE (string): path to the 'msgmerge' executable 

IF(NOT EXISTS ${po_file})
  EXEC_PROGRAM(${GETTEXT_MSGINIT_EXECUTABLE} 
    ARGS --input=${pot_file} --output-file=${po_file} --locale=${locale})
#    OUTPUT_VARIABLE msginit_output)
  IF(NOT default_po_encoding)
    SET(default_po_encoding "utf-8")
  ENDIF(NOT default_po_encoding)
  EXEC_PROGRAM(${GETTEXT_MSGCONV_EXECUTABLE} 
    ARGS --output-file=${po_file} --to-code="${default_po_encoding}" ${po_file})
#    OUTPUT_VARIABLE msgconv_output)
ELSE(NOT EXISTS ${po_file})
  # --output-file and --update are mutually exclusive. If --update is
  # specified, the PO file will not be re-written if the result of
  # the merge produces no modification. This can be problematic if the POT
  # file is newer than the PO file, and a MO file is generated from the PO
  # file: this seems to force the MO to always be regenerated.
  EXEC_PROGRAM(${GETTEXT_MSGMERGE_EXECUTABLE} 
    ARGS --output-file=${po_file} ${po_file} ${pot_file})
#    OUTPUT_VARIABLE msgmerge_output)
ENDIF(NOT EXISTS ${po_file})
