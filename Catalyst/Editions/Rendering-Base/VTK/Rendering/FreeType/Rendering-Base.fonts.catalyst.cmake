set(font_files
  fonts/face_arial.cxx
  fonts/face_arial_bold.cxx
  fonts/face_arial_bold_italic.cxx
  fonts/face_arial_italic.cxx
  fonts/face_courier.cxx
  fonts/face_courier_bold.cxx
  fonts/face_courier_bold_italic.cxx
  fonts/face_courier_italic.cxx
  fonts/face_times.cxx
  fonts/face_times_bold.cxx
  fonts/face_times_bold_italic.cxx
  fonts/face_times_italic.cxx
  )

list(APPEND Module_SRCS
  ${font_files}
  )
set_source_files_properties(
  ${font_files}
  WRAP_EXCLUDE
  )
