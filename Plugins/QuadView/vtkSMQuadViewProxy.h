/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMQuadViewProxy
// .SECTION Description
// Custom proxy used to managed the QuadView initialization and customed
// default representation as well as the ImageCapture code.

#ifndef __vtkSMQuadViewProxy_h
#define __vtkSMQuadViewProxy_h

#include "vtkSMRenderViewProxy.h"

class vtkSMProxyLink;

class vtkSMQuadViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMQuadViewProxy* New();
  vtkTypeMacro(vtkSMQuadViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the
  // error message otherwise 0.
  virtual const char* IsSelectVisiblePointsAvailable();

//BTX
protected:
  vtkSMQuadViewProxy();
  ~vtkSMQuadViewProxy();

  virtual void CreateVTKObjects();
  virtual vtkImageData* CaptureWindowInternal(int magnification);
  virtual int CreateSubProxiesAndProperties(vtkSMSessionProxyManager* pm,
    vtkPVXMLElement *element);

  // Override to gather information when interaction is done from the server side to properly
  // get fields name and probed data
  virtual void PostRender(bool interactive);

  void UpdateInternalViewExtent(vtkImageData * image, int columnIndex, int rowIndex);

  vtkSMProxyLink* WidgetLinker;

private:
  vtkSMQuadViewProxy(const vtkSMQuadViewProxy&); // Not implemented
  void operator=(const vtkSMQuadViewProxy&); // Not implemented
//ETX
};

#endif
