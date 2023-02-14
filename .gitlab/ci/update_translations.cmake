find_program(LUPDATE_COMMAND NAMES lupdate lupdate-qt5 lupdate-qt6)

file(GLOB_RECURSE new_translations "${NEW_TRANSLATIONS_DIR}/*.ts")
set(new_translation_names)
foreach(new_translation IN LISTS new_translations)
  get_filename_component(new_translation_name ${new_translation} NAME)
  message(STATUS "Updating translations: ${new_translation_name}")
  execute_process(COMMAND "${LUPDATE_COMMAND}" "${NEW_TRANSLATIONS_DIR}/${new_translation_name}" -ts "${TRANSLATIONS_DIR}/${new_translation_name}")
endforeach()
