# This file is a helper file for the gettext translation macros found in
# KWWidgetsInternationalizationMacros.cmake.
# If is used as a custom command in the KWWidgets_CREATE_POT_TARGETS macro.
# This macro extracts translatable strings out of source files into a POT
# file (template translation fiel). The problem is that even if no changes
# occurred as far as the translation strings are concerned, xgettext will
# always create a new file with a different POT-Creation-Date field. This
# forces all the depending targets to be updated when they do not really have
# to. Fix that by comparing the next POT file to the old one without taking
# the POT-Creation-Date into account.
#
# 'pot_build_file' (string): the POT file the strings should be extracted to
# 'pot_uptodate_file' (string): the dummy file which will be up to date
# 'options' (string): options
# 'keywords' (string): keywords
# 'copyright_holder': optional copyright holder of the template file
# 'files_from': 
# GETTEXT_XGETTEXT_EXECUTABLE (string): path to the 'xgettext' executable

IF(NOT "${GETTEXT_XGETTEXT_EXECUTABLE}" STREQUAL "")
  
  # Extract the strings, store the result in a variable instead of a POT file

  EXEC_PROGRAM(${GETTEXT_XGETTEXT_EXECUTABLE} 
    RETURN_VALUE xgettext_return
    OUTPUT_VARIABLE xgettext_output
    ARGS --output="-" ${options} ${keywords} --copyright-holder="${copyright_holder}" --files-from=${files_from})
  IF(xgettext_return)
    MESSAGE("${xgettext_output}")
  ELSE(xgettext_return)

    SET(xgettext_output "${xgettext_output}\n")

    # Check if the new POT file would be different than the old one
    # without taking into account the POT-Creation-Date.

    SET(update_pot_file 0)
    IF(EXISTS ${pot_build_file})
      STRING(REGEX REPLACE "\"POT-Creation-Date:[^\"]*\"" "" 
        xgettext_output_nodate "${xgettext_output}")
      FILE(READ "${pot_build_file}" xgettext_old)
      STRING(REGEX REPLACE "\"POT-Creation-Date:[^\"]*\"" "" 
        xgettext_old_nodate "${xgettext_old}")
      IF(NOT "${xgettext_output_nodate}" STREQUAL "${xgettext_old_nodate}")
        SET(update_pot_file 1)
      ENDIF(NOT "${xgettext_output_nodate}" STREQUAL "${xgettext_old_nodate}")
    ELSE(EXISTS ${pot_build_file})
      SET(update_pot_file 1)
    ENDIF(EXISTS ${pot_build_file})

    # Create the POT file if it is really needed

    IF(update_pot_file)
      MESSAGE("Updating ${pot_build_file}")
      FILE(WRITE "${pot_build_file}" "${xgettext_output}")
    ENDIF(update_pot_file)

    # Update the dummy file to say: this POT target is up to date as
    # far as its dependencies are concerned. This will prevent the POT
    # target to be triggered again and again because the sources are older
    # than the POT, but the POT does not really need to be changed, etc.

    FILE(WRITE "${pot_uptodate_file}" 
      "${pot_build_file} is *really* up-to-date.")

  ENDIF(xgettext_return)
ENDIF(NOT "${GETTEXT_XGETTEXT_EXECUTABLE}" STREQUAL "")
