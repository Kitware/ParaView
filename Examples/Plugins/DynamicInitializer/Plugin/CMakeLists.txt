set(sources
  Initializer.h
  Initializer.cxx
)

paraview_add_plugin(DynamicInitializer
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  INITIALIZERS run_me           # Function to call upon load
  EXTRA_INCLUDES Initializer.h  # Header declaring function
  SOURCES ${sources}
)
