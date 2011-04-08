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
// .NAME vtkSMViewProxy - Superclass for all view proxies.
// .SECTION Description
// vtkSMViewProxy is a superclass for all view proxies. A view proxy
// abstracts the logic to take one or more representation proxies and show then
// in some viewport such as vtkRenderWindow.
// This class may directly be used as the view proxy for views that do all the
// rendering work at the GUI level. The VTKObject corresponding to this class
// has to be a vtkView subclass.
// .SECTION Events
// \li vtkCommand::StartEvent(callData: int:0) -- start of StillRender().
// \li vtkCommand::EndEvent(callData: int:0) -- end of StillRender().
// \li vtkCommand::StartEvent(callData: int:1) -- start of InteractiveRender().
// \li vtkCommand::EndEvent(callData: int:1) -- end of InteractiveRender().

#ifndef __vtkSMViewProxy_h
#define __vtkSMViewProxy_h

#include "vtkSMProxy.h"

class vtkImageData;
class vtkSMRepresentationProxy;
class vtkView;

class VTK_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Renders the view using full resolution.
  virtual void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  virtual void InteractiveRender();

  // Description:
  // Called vtkPVView::Update on the server-side.
  virtual void Update();

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int);

  // Description:
  // Captures a image from this view. Default implementation returns NULL.
  // Subclasses should override CaptureWindowInternal() to do the actual image
  // capture.
  vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Returns the client-side vtkView, if any.
  vtkView* GetClientSideView();

  // Description:
  // Saves a screenshot of the view to disk. The writerName argument specifies
  // the vtkImageWriter subclass to use.
  int WriteImage(const char* filename, const char* writerName, int magnification);

//BTX
protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy();

  // Description:
  // Subclasses should override this method to do the actual image capture.
  virtual vtkImageData* CaptureWindowInternal(int vtkNotUsed(magnification))
    { return NULL; }

  virtual void PostRender(bool vtkNotUsed(interactive)) {}

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void CreateVTKObjects();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  vtkSetStringMacro(DefaultRepresentationName);
  char* DefaultRepresentationName;

private:
  vtkSMViewProxy(const vtkSMViewProxy&); // Not implemented
  void operator=(const vtkSMViewProxy&); // Not implemented

  // When view's time changes, there's no way for the client-side proxies to
  // know that they may re-execute and their data info is invalid. So mark those
  // dirty explicitly.
  void ViewTimeChanged();
//ETX
};

#endif
