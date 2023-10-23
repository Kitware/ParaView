// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRPythonInteractorStyleProxy_h
#define vtkSMVRPythonInteractorStyleProxy_h

#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

#include <string>

class vtkPVXMLElement;
class vtkSmartPyObject;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;
class vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRPythonInteractorStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRPythonInteractorStyleProxy* New();
  vtkTypeMacro(vtkSMVRPythonInteractorStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the filename of the python file containing the custom
   * python interactor style class.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Reload the python file
   */
  void ReloadPythonFile();
  ///@}

  ///@{
  /**
   * Overridden to support refreshing the python from the UI.
   */
  void RecreateVTKObjects() override;
  ///@}

  /**
   * Specify the Python object that implements the handler methods this
   * object will forward to.  This will also invoke Initialize() on the
   * Python object, providing an opportunity to perform tasks commonly
   * done in the constructor of C++ vtkSMVRInteractorStyleProxy subclasses.
   *
   * A reference will be taken on the object.
   */
  void SetPythonObject(void* obj);
  ///@}

  ///@{
  /**
   * Overridden to support retrieving the user-selected FileName and
   * using it to read the python file.
   */
  void UpdateVTKObjects() override;
  ///@}

protected:
  vtkSMVRPythonInteractorStyleProxy();
  ~vtkSMVRPythonInteractorStyleProxy() override;
  void HandleTracker(const vtkVREvent& event) override;
  void HandleValuator(const vtkVREvent& event) override;
  void HandleButton(const vtkVREvent& event) override;
  void InvokeHandler(const char* mname, const vtkVREvent& event);
  vtkPVXMLElement* SaveConfiguration() override;
  bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator) override;

  // Read the python file located on the filesystem at "path" into "contents"
  bool ReadPythonFile(const char* path, std::string& contents);

  char* FileName;

private:
  vtkSMVRPythonInteractorStyleProxy(
    const vtkSMVRPythonInteractorStyleProxy&) = delete;              // Not implemented.
  void operator=(const vtkSMVRPythonInteractorStyleProxy&) = delete; // Not implemented.

  class Internal;
  Internal* Internals;
};

#endif // vtkSMVRPythonInteractorStyleProxy_h
