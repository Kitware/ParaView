/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDisplayProxy - proxy for any entity that must be rendered.
// .SECTION Description
// vtkSMDisplayProxy is a sink for display objects. Anything that can
// be rendered has to be a vtkSMDisplayProxy, otherwise it can't be added
// be added to the vtkSMRenderModule, and hence cannot be rendered.
// This can have inputs (but not required, for displays such as 3Dwidgets/ Scalarbar).
// This is an abstract class, merely defining the interface.

#ifndef __vtkSMDisplayProxy_h
#define __vtkSMDisplayProxy_h

#include "vtkSMProxy.h"
class vtkSMRenderModuleProxy;
class vtkPVGeometryInformation;

class VTK_EXPORT vtkSMDisplayProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMDisplayProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get information about the geometry.
  // Some displays (like Scalar bar), may return an empty
  // vtkPVGeometryInformation object.
  vtkPVGeometryInformation* GetGeometryInformation();
  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*) = 0;
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*) = 0;

  // Description:
  // Called to update the Display. Default implementation does nothing.
  virtual void Update() { }
  
  // Description:
  // Convenience method to get/set the Interpolation for this proxy.
  int cmGetInterpolation();
  void cmSetInterpolation(int interpolation);
  
  //BTX
  enum {POINTS=0, WIREFRAME, SURFACE,  OUTLINE, VOLUME};
  enum {FLAT=0, GOURAND, PHONG};
  enum {DEFAULT=0, POINT_DATA, CELL_DATA, POINT_FIELD_DATA, CELL_FIELD_DATA};
  //ETX
  
  // Description:
  // Convenience method to get/set Point Size.
  void cmSetPointSize(double size);
  double cmGetPointSize();

  // Description:
  // Convenience method to get/set Line Width.
  void cmSetLineWidth(double width);
  double cmGetLineWidth();

  // Description:
  // Convenience method to get/set Scalar Mode.
  void cmSetScalarMode(int  mode);
  int cmGetScalarMode();

  // Description:
  // Convenience method to get/set ScalarArray (Volume Mapper).
  void cmSetScalarArray(const char* arrayname);
  const char* cmGetScalarArray();
  
  // Description:
  // Convenience method to get/set Opacity.
  void cmSetOpacity(double op);
  double cmGetOpacity();

  // Description:
  // Convenience method to get/set ColorMode.
  void cmSetColorMode(int mode);
  int cmGetColorMode();

  // Description:
  // Convenience method to get/set Actor color.
  void cmSetColor(double rgb[3]);
  void cmGetColor(double rgb[3]);

  // Description:
  // Convenience method to get/set InterpolateColorsBeforeMapping property.
  void cmSetInterpolateScalarsBeforeMapping(int flag);
  int cmGetInterpolateScalarsBeforeMapping();

  // Description:
  // Convenience method to get/set ScalarVisibility property.
  void cmSetScalarVisibility(int);
  int cmGetScalarVisibility();

  // Description:
  // Convenience method to get/set Position property.
  void cmSetPosition(double pos[3]);
  void cmGetPosition(double pos[3]);

  // Description:
  // Convenience method to get/set Scale property.
  void cmSetScale(double scale[3]);
  void cmGetScale(double scale[3]);
   
  // Description:
  // Convenience method to get/set Orientation property.
  void cmSetOrientation(double orientation[3]);
  void cmGetOrientation(double orientation[3]);

  // Description
  // Convenience method to get/set Origin property.
  void cmGetOrigin(double origin[3]);
  void cmSetOrigin(double origin[3]);

  // Description:
  // Convenience method to get/set Visibility property.
  void cmSetVisibility(int v);
  int cmGetVisibility();

  // Description:
  // Convenience method to get/set Representation.
  void cmSetRepresentation(int r);
  int cmGetRepresentation();
  
  
protected:
  vtkSMDisplayProxy();
  ~vtkSMDisplayProxy();

  int GeometryInformationIsValid; //This flag must be managed by the subclasses.
  virtual void GatherGeometryInformation();
  vtkPVGeometryInformation* GeometryInformation;
private:
  vtkSMDisplayProxy(const vtkSMDisplayProxy&); // Not implemented.
  void operator=(const vtkSMDisplayProxy&); // Not implemented.
};



#endif

