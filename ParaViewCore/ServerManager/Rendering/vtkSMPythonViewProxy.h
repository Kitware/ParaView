/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPythonViewProxy
 * @brief   Superclass for all view proxies
 *
 * vtkSMPythonViewProxy is a view proxy for the vtkPythonView.
*/

#ifndef vtkSMPythonViewProxy_h
#define vtkSMPythonViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMViewProxy.h"

class vtkImageData;
class vtkRenderer;
class vtkSMProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPythonViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMPythonViewProxy* New();
  vtkTypeMacro(vtkSMPythonViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Returns the client-side renderer.
   */
  vtkRenderer* GetRenderer();

  /**
   * Returns the client-side render window.
   */
  vtkRenderWindow* GetRenderWindow();

  /**
   * Returns true if the most recent render indeed employed low-res rendering.
   */
  virtual bool LastRenderWasInteractive();

  /**
   * Overridden to disable creation on an interactor. PythonView does not
   * support interactor.
   */
  bool MakeRenderWindowInteractor(bool) VTK_OVERRIDE
  { return false; }

protected:
  vtkSMPythonViewProxy();
  ~vtkSMPythonViewProxy();

  /**
   * Subclasses should override this method to do the actual image capture.
   */
  virtual vtkImageData* CaptureWindowInternal(int magnification);

private:
  vtkSMPythonViewProxy(const vtkSMPythonViewProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPythonViewProxy&) VTK_DELETE_FUNCTION;
};

#endif // vtkSMPythonViewProxy_h
