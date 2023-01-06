/*=========================================================================

   Program: ParaView
   Module:  vtkSMVRPythonInteractorStyleProxy.h

   Copyright (c) Kitware, Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef vtkSMVRPythonInteractorStyleProxy_h
#define vtkSMVRPythonInteractorStyleProxy_h

#include "vtkInteractionStylesModule.h" // for export macro
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

class VTKINTERACTIONSTYLES_EXPORT vtkSMVRPythonInteractorStyleProxy
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
  void HandleAnalog(const vtkVREvent& event) override;
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
