ADD_SUBDIRECTORY(Cxx)

IF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)
  ADD_TEST(KWCommon-HeaderTesting ${PYTHON_EXECUTABLE}
           ${VTK_HEADER_TESTING_PY}
           "${KWCommon_SOURCE_DIR}"
           icons.h
           vtkKWEvent.h
           vtkQueue.h
           vtkVector.h
           vtkLinkedList.h
           vtkHashMap.h
           vtkArrayMap.h
           vtkAbstractList.h
           vtkAbstractMap.h
           vtkQueueIterator.h
           vtkVectorIterator.h
           vtkLinkedListIterator.h
           vtkHashMapIterator.h
           vtkArrayMapIterator.h
           vtkAbstractIterator.h
           vtkContainer.h
           )
ENDIF(PYTHON_EXECUTABLE AND VTK_HEADER_TESTING_PY)

