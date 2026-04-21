// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkProcessModuleInternals_h
#define vtkProcessModuleInternals_h

#include "vtkNew.h"
#include "vtkPVSession.h"    // for vtkPVSession
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkThreadedCallbackQueue.h"
#include "vtkWeakPointer.h" // for vtkWeakPointer

#include <map>    // for std::map
#include <vector> // for std::vector

class vtkProcessModuleInternals
{
public:
  typedef std::map<vtkIdType, vtkSmartPointer<vtkPVSession>> MapOfSessions;
  MapOfSessions Sessions;

  typedef std::vector<vtkWeakPointer<vtkPVSession>> ActiveSessionStackType;
  ActiveSessionStackType ActiveSessionStack;

  vtkNew<vtkThreadedCallbackQueue> CallbackQueue;
};

#endif

// VTK-HeaderTest-Exclude: vtkProcessModuleInternals.h
