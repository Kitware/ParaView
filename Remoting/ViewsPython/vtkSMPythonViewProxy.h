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

#include "vtkRemotingViewsPythonModule.h" //needed for exports

#include "vtkNew.h" // needed for vtkNew.
#include "vtkSMViewProxy.h"

class vtkImageData;
class vtkRenderer;
class vtkSMProxy;
class vtkSMViewProxyInteractorHelper;

class VTKREMOTINGVIEWSPYTHON_EXPORT vtkSMPythonViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMPythonViewProxy* New();
  vtkTypeMacro(vtkSMPythonViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the client-side renderer.
   */
  vtkRenderer* GetRenderer();

  /**
   * Returns the client-side render window.
   */
  vtkRenderWindow* GetRenderWindow() override;

  /**
   * Returns the interactor. Note, that views may not use vtkRenderWindow at all
   * in which case they will not have any interactor and will return nullptr.
   * Default implementation returns nullptr.
   */
  vtkRenderWindowInteractor* GetInteractor() override;

  /**
   * A client process need to set the interactor to enable interactivity. Use
   * this method to set the interactor and initialize it as needed by the
   * RenderView. This include changing the interactor style as well as
   * overriding VTK rendering to use the Proxy/ViewProxy API instead.
   * Default implementation does nothing. Views that support interaction using
   * vtkRenderWindowInteractor should override this method to set the interactor
   * up.
   */
  void SetupInteractor(vtkRenderWindowInteractor* iren) override;

protected:
  vtkSMPythonViewProxy();
  ~vtkSMPythonViewProxy() override;

  /**
   * This view currently only supports rendering on client, even in tile-display
   * mode. Which means it will not show any results on the tile display.
   */
  vtkTypeUInt32 PreRender(bool interactive) override;

  /**
   * Subclasses should override this method to do the actual image capture.
   */
  vtkImageData* CaptureWindowInternal(int magX, int magY) override;

private:
  vtkSMPythonViewProxy(const vtkSMPythonViewProxy&) = delete;
  void operator=(const vtkSMPythonViewProxy&) = delete;

  vtkNew<vtkSMViewProxyInteractorHelper> InteractorHelper;
};

#endif // vtkSMPythonViewProxy_h
