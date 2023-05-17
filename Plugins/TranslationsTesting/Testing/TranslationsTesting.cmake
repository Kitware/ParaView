file(GLOB ts_files "${PARAVIEW_TRANSLATIONS_DIRECTORY}/*.ts")
file(COPY ${ts_files} DESTINATION ${TS_TEMPORARY_DIR})
file(GLOB ts_files "${TS_TEMPORARY_DIR}/*.ts")

# Runs a Python utility to fill translation files with "_TranslationsTesting"
execute_process(COMMAND "${PYTHON3_INTERPRETER}" "${PYTHON3_SCRIPT}" ${ts_files})
file(MAKE_DIRECTORY "${TS_TEMPORARY_DIR}")
# Testing local translation loading system using fa_IR (Iranian persian)
# instead of en to test the locale loading system.
set(DESTINATION_QM "${TS_TEMPORARY_DIR}/paraview_fa_IR.qm")
# Runs lconvert to transform ts files to a translation binary (.qm)
execute_process(COMMAND "${LCONVERT_EXECUTABLE}" ${ts_files} -o "${DESTINATION_QM}")
