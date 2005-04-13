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
  int GetInterpolationCM();
  void SetInterpolationCM(int interpolation);
  
  //BTX
  enum {POINTS=0, WIREFRAME, SURFACE,  OUTLINE, VOLUME};
  enum {FLAT=0, GOURAND, PHONG};
  enum {DEFAULT=0, POINT_DATA, CELL_DATA, POINT_FIELD_DATA, CELL_FIELD_DATA};
  //ETX
  
  // Description:
  // Convenience method to get/set Point Size.
  void SetPointSizeCM(double size);
  double GetPointSizeCM();

  // Description:
  // Convenience method to get/set Line Width.
  void SetLineWidthCM(double width);
  double GetLineWidthCM();

  // Description:
  // Convenience method to get/set Scalar Mode.
  void SetScalarModeCM(int  mode);
  int GetScalarModeCM();

  // Description:
  // Convenience method to get/set ScalarArray (Volume Mapper).
  void SetScalarArrayCM(const char* arrayname);
  const char* GetScalarArrayCM();
  
  // Description:
  // Convenience method to get/set Opacity.
  void SetOpacityCM(double op);
  double GetOpacityCM();

  // Description:
  // Convenience method to get/set ColorMode.
  void SetColorModeCM(int mode);
  int GetColorModeCM();

  // Description:
  // Convenience method to get/set Actor color.
  void SetColorCM(double rgb[3]);
  void GetColorCM(double rgb[3]);
  void SetColorCM(double r, double g, double b)
    { 
    double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] =b;
    this->SetColorCM(rgb);
    }
    
    
  // Description:
  // Convenience method to get/set InterpolateColorsBeforeMapping property.
  void SetInterpolateScalarsBeforeMappingCM(int flag);
  int GetInterpolateScalarsBeforeMappingCM();

  // Description:
  // Convenience method to get/set ScalarVisibility property.
  void SetScalarVisibilityCM(int);
  int GetScalarVisibilityCM();

  // Description:
  // Convenience method to get/set Position property.
  void SetPositionCM(double pos[3]);
  void GetPositionCM(double pos[3]);
  void SetPositionCM(double r, double g, double b)
    { 
    double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] =b;
    this->SetPositionCM(rgb);
    }

  // Description:
  // Convenience method to get/set Scale property.
  void SetScaleCM(double scale[3]);
  void GetScaleCM(double scale[3]);
  void SetScaleCM(double r, double g, double b)
    { 
    double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] =b;
    this->SetScaleCM(rgb);
    } 
   
  // Description:
  // Convenience method to get/set Orientation property.
  void SetOrientationCM(double orientation[3]);
  void GetOrientationCM(double orientation[3]);
  void SetOrientationCM(double r, double g, double b)
    { 
    double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] =b;
    this->SetOrientationCM(rgb);
    }

  // Description
  // Convenience method to get/set Origin property.
  void GetOriginCM(double origin[3]);
  void SetOriginCM(double origin[3]);
  void SetOriginCM(double r, double g, double b)
    { 
    double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] =b;
    this->SetOriginCM(rgb);
    }

  // Description:
  // Convenience method to get/set Visibility property.
  void SetVisibilityCM(int v);
  int GetVisibilityCM();

  // Description:
  // Convenience method to get/set Representation.
  void SetRepresentationCM(int r);
  int GetRepresentationCM();

  // Description:
  // Convenience method to get/set ImmediateModeRendering property.
  void SetImmediateModeRenderingCM(int f);
  int GetImmediateModeRenderingCM();
 
  // Description:
  // Save the display in batch script. This will eventually get 
  // removed as we will generate batch script from ServerManager
  // state. However, until then.
  virtual void SaveInBatchScript(ofstream* file);
  
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

