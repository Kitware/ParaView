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

#include "vtkSMProxy.h"

class vtkSMDisplayerProxyInternals;
class vtkSMPart;
class vtkSMDisplayWindowProxy;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMDisplayerProxy : public vtkSMProxy
{
public:
  static vtkSMDisplayerProxy* New();
  vtkTypeRevisionMacro(vtkSMDisplayerProxy, vtkSMProxy);
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

  // Description:
  // Adds this displayer to the Display window proxy
  virtual void AddToDisplayWindow(vtkSMDisplayWindowProxy* dw);

  // Description:
  // Connects filters/sinks to an input. If the filter(s) is not
  // created, this will create it. If hasMultipleInputs is
  // true,  only one filter is created, even if the input has
  // multiple parts. All the inputs are added using the method
  // name provided. If hasMultipleInputs is not true, one filter
  // is created for each input. NOTE: The filter(s) is created
  // when SetInput is called the first and if the it wasn't already
  // created. If the filter has two inputs and one is multi-block
  // whereas the other one is not, SetInput() should be called with
  // the multi-block input first. Otherwise, it will create only
  // on filter and can not apply to the multi-block input.
  void AddInput(vtkSMSourceProxy* input, 
                const char* method,
                int hasMultipleInputs);

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

private:
  vtkSMDisplayerProxy(const vtkSMDisplayerProxy&); // Not implemented
  void operator=(const vtkSMDisplayerProxy&); // Not implemented
};

#endif
