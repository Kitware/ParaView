/*=========================================================================

  Program:   ParaView
  Module:    vtkSIPythonSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSIPythonSourceProxy_h
#define vtkSIPythonSourceProxy_h

#include "vtkNew.h"                     // needed for vtkNew.
#include "vtkPVPythonAlgorithmModule.h" // for exports
#include "vtkSISourceProxy.h"

#include <memory> // for unique_ptr

class vtkPythonInterpreter;

/**
 * @class vtkSIPythonSourceProxy
 * @brief vtkSISourceProxy for all `VTKPythonAlgorithmBase`-based algorithms.
 *
 * vtkSIPythonSourceProxy makes is possible to work with a
 * VTKPythonAlgorithmBase-based Python class in ParaView. By handling the
 * push/pull APIs to forward the calls to the Python object, we can make such
 * Python algorithms behave like regular C++-based algorithm subclasses.
 */
class VTKPVPYTHONALGORITHM_EXPORT vtkSIPythonSourceProxy : public vtkSISourceProxy
{
public:
  static vtkSIPythonSourceProxy* New();
  vtkTypeMacro(vtkSIPythonSourceProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Push(vtkSMMessage*) override;
  void Pull(vtkSMMessage*) override;
  void RecreateVTKObjects() override;
  void Initialize(vtkPVSessionCore* session) override;

protected:
  vtkSIPythonSourceProxy();
  ~vtkSIPythonSourceProxy() override;

  /**
   * Overridden to use the Python interpreter to instantiate Python classes.
   */
  vtkObjectBase* NewVTKObject(const char* className) override;

  /**
   * Overridden to release any Python objects created.
   */
  void DeleteVTKObjects() override;

private:
  vtkSIPythonSourceProxy(const vtkSIPythonSourceProxy&) = delete;
  void operator=(const vtkSIPythonSourceProxy&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  bool ReimportModules;

  // to get notification of ExitEvent.
  vtkNew<vtkPythonInterpreter> Dummy;
  void OnPyFinalize();
};

#endif
