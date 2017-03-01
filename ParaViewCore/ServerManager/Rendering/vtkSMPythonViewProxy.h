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

#include "vtkNew.h" // needed for vtkNew.
#include "vtkSMViewProxy.h"

class vtkImageData;
class vtkRenderer;
class vtkSMProxy;
class vtkSMViewProxyInteractorHelper;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPythonViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMPythonViewProxy* New();
  vtkTypeMacro(vtkSMPythonViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the client-side renderer.
   */
  vtkRenderer* GetRenderer();

  /**
   * Returns the client-side render window.
   */
  vtkRenderWindow* GetRenderWindow() VTK_OVERRIDE;

  /**
   * Returns the interactor. Note, that views may not use vtkRenderWindow at all
   * in which case they will not have any interactor and will return NULL.
   * Default implementation returns NULL.
   */
  vtkRenderWindowInteractor* GetInteractor() VTK_OVERRIDE;

  /**
   * A client process need to set the interactor to enable interactivity. Use
   * this method to set the interactor and initialize it as needed by the
   * RenderView. This include changing the interactor style as well as
   * overriding VTK rendering to use the Proxy/ViewProxy API instead.
   * Default implementation does nothing. Views that support interaction using
   * vtkRenderWindowInteractor should override this method to set the interactor
   * up.
   */
  void SetupInteractor(vtkRenderWindowInteractor* iren) VTK_OVERRIDE;

protected:
  vtkSMPythonViewProxy();
  ~vtkSMPythonViewProxy();

  /**
   * Subclasses should override this method to do the actual image capture.
   */
  virtual vtkImageData* CaptureWindowInternal(int magnification) VTK_OVERRIDE;

private:
  vtkSMPythonViewProxy(const vtkSMPythonViewProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPythonViewProxy&) VTK_DELETE_FUNCTION;

  vtkNew<vtkSMViewProxyInteractorHelper> InteractorHelper;
};

#endif // vtkSMPythonViewProxy_h
