/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayerProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDisplayerProxy - composite proxy for mapper, actor ...
// .SECTION Description
// vtkSMDisplayerProxy is a composite proxy that manages objects related
// to rendering including geometry filter, mapper, actor and property. 
// DisplayerProxy is a "sink" and can be connected to any source.
// DisplayerProxy has to be added to a DisplayProxy in order to be
// displayed.
// .SECTION See Also
// vtkSMDisplayWindowProxy vtkSMSourceProxy

#ifndef __vtkSMDisplayerProxy_h
#define __vtkSMDisplayerProxy_h

#include "vtkSMSourceProxy.h"

class vtkSMDisplayerProxyInternals;
class vtkSMPart;

class VTK_EXPORT vtkSMDisplayerProxy : public vtkSMSourceProxy
{
public:
  static vtkSMDisplayerProxy* New();
  vtkTypeRevisionMacro(vtkSMDisplayerProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off scalar visibility as well as adjust some lighting
  // parameters: 0 specular for scalar visibility (avoid interference
  // with data), 0.1 specular, 100 specular power, 1,1,1 specular color
  // This method is public because it is accessed by the clientserver
  // stream interface. Avoid directly calling this method.
  void SetScalarVisibility(int vis);

  // Description:
  // Change the representation to one of : points, wireframe, surface.
  // Also adjust lighting as follows:
  // 1. points: ambient 1, diffuse 0
  // 2. wireframe: ambient 1, diffuse 0
  // 3. surface: ambient 0, diffuse 1
  // This method is public because it is accessed by the clientserver
  // stream interface. Avoid directly calling this method.
  void SetRepresentation(int repr);

  // Description:
  // Set the color of the render window. Also set specular color
  // to (1,1,1)
  void SetColor(double r, double g, double b);

protected:
  vtkSMDisplayerProxy();
  ~vtkSMDisplayerProxy();

  // Description:
  // Create all VTK objects including the ones for sub-proxies.
  virtual void CreateVTKObjects(int numObjects);

//BTX
  friend class vtkSMDisplayWindowProxy;
//ETX

  void DrawWireframe();
  void DrawPoints();
  void DrawSurface();

  virtual void SaveState(const char*, ostream*, vtkIndent) {}
private:
  vtkSMDisplayerProxy(const vtkSMDisplayerProxy&); // Not implemented
  void operator=(const vtkSMDisplayerProxy&); // Not implemented
};

#endif
