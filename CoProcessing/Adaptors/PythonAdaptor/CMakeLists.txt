PROJECT (PythonAdaptor)

SET (PythonAdaptor_SRCS
  vtkPVPythonAdaptorHelper.cxx)

ADD_LIBRARY(PythonAdaptor ${PythonAdaptor_SRCS})
TARGET_LINK_LIBRARIES(PythonAdaptor vtkPVPythonInterpretor vtkPVServerManager)

