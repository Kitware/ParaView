/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataObjectDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDataObjectDisplayProxy - a simple display proxy.
// .SECTION Description
// vtkSMDataObjectDisplayProxy is a sink for display objects. 
// This is the superclass for objects that display PVParts.
// Not meant to be used directly, although this implements the simplest
// serial display with no LOD. This class has a bunch of 
// "convenience methods" (method names appended with CM). These methods
// do the equivalent of getting the property by the name and
// setting/getting its value. They are there to simplify using the property
// interface for display objects. When adding a method to the proxies
// that merely sets some property on the proxy, make sure to append the method
// name with "CM" - implying it's a convenience method. That way, one knows
// its purpose and will not be confused with a update-self property method.

#ifndef __vtkSMDataObjectDisplayProxy_h
#define __vtkSMDataObjectDisplayProxy_h

#include "vtkSMConsumerDisplayProxy.h"
class vtkSMProxy;
class vtkPVDataInformation;
class vtkPVArrayInformation;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMDataObjectDisplayProxy : public vtkSMConsumerDisplayProxy
{
public:
  static vtkSMDataObjectDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMDataObjectDisplayProxy, vtkSMConsumerDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);

  // Description:
  // Method gets called to set input when using Input property.
  // Internally leads to a call to SetInput.
  virtual void AddInput(vtkSMSourceProxy* input, const char*, int);

  // Description:
  // Connect the VTK data object to display pipeline.
  void SetInput(vtkSMProxy* input);
  
  // Description:
  // This method calls a ForceUpdate on the UpdateSuppressor
  // if the Geometry is not valid. 
  virtual void Update();
  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Get information about the geometry.
  // Some displays (like Scalar bar, 3DWidgets), may return NULL.
  virtual vtkPVGeometryInformation* GetGeometryInformation();
  
  
  // Description:
  // Set the representation for this display.
  virtual void SetRepresentation(int representation);

  // Description:
  // Set Visibility of the display.
  virtual void SetVisibility(int visible);

  // Description:
  // Flag indicating if the display supports a volume rendering 
  // representation.
  vtkGetMacro(HasVolumePipeline,    int);
  vtkGetMacro(SupportsBunykMapper,  int);
  vtkGetMacro(SupportsZSweepMapper, int);
  
  // Description:
  // Flag indicating if the display is currently rendered
  // as a volume. Typically, one would not use this flag,
  // instead check status of the property "Representation".
  // Here, only for RenderModuleProxy.
  vtkGetMacro(VolumeRenderMode, int);

  // Description:
  // Method to initlaize the Volume Transfer functions 
  // (ie. Opacity Function & Color Transfer fuction).
  void ResetTransferFunctions();
  
  // Description:
  // Convenience methods for switching between volume
  // mappers.
  void SetVolumeMapperToBunykCM();
  void SetVolumeMapperToPTCM();
  void SetVolumeMapperToZSweepCM();
  
  // Description:
  // Convenience method for determining which
  // volume mapper is in use
  virtual int GetVolumeMapperTypeCM();
  
  // Description:
  // This method will set the subproxy GeometryFilterProxy
  // as the input (using property "Input") on the argument
  // onProxy. 
  void SetInputAsGeometryFilter(vtkSMProxy* onProxy);

  //BTX
  // Interpolation types.
  enum {FLAT=0, GOURAND, PHONG};
  // Scalar mode types.
  enum {DEFAULT=0, POINT_DATA, CELL_DATA, POINT_FIELD_DATA, CELL_FIELD_DATA};
  // Representation types.
  enum {POINTS=0, WIREFRAME, SURFACE,  OUTLINE, VOLUME};
  // Volume mapper types.
  enum 
  {
    PROJECTED_TETRA_VOLUME_MAPPER =0,
    ZSWEEP_VOLUME_MAPPER,
    BUNYK_RAY_CAST_VOLUME_MAPPER,
    UNKNOWN_VOLUME_MAPPER
  };
  //ETX
 

  // Description:
  // Convienience method to set/get the material name.
  void SetMaterialCM(const char* materialname);
  // Description:
  // Convienience method to set/get the material name.
  // The material name is returned only when Shading is enabled.
  // When Shading is disabled 0 is returned.
  const char* GetMaterialCM();
  
  // Description:
  // Convenience method to get/set the Interpolation for this proxy.
  int GetInterpolationCM();
  void SetInterpolationCM(int interpolation);

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
  // Convenience method to get/set Representation.
  void SetRepresentationCM(int r);
  int GetRepresentationCM();

  // Description:
  // Convenience method to get/set ImmediateModeRendering property.
  void SetImmediateModeRenderingCM(int f);
  int GetImmediateModeRenderingCM();

protected:
  vtkSMDataObjectDisplayProxy();
  ~vtkSMDataObjectDisplayProxy();

  virtual void SetInputInternal(vtkSMSourceProxy* input);

  void ResetTransferFunctions(vtkPVDataInformation* dataInfo,
    vtkPVArrayInformation* arrayInfo);

  // Description:
  // Set up the PolyData rendering pipeline.
  virtual void SetupPipeline();
  virtual void SetupDefaults();

  // Description:
  // Set up the vtkUnstructuredGrid (Volume) rendering pipeline.
  virtual void SetupVolumePipeline();
  virtual void SetupVolumeDefaults();

 
  virtual void CreateVTKObjects(int numObjects);

  virtual void GatherGeometryInformation();

//BTX
  // This is the least intrusive way of giving vtkPVComparativeVisManager
  // access to the MapperProxy. It needs this proxy to extract and cache
  // the input geometry
  friend class vtkPVComparativeVisManager;
  friend class vtkPVComparativeVis;
  friend class vtkSMComparativeVisProxy;
  friend class vtkSMRenderModuleProxy;
  vtkGetObjectMacro(MapperProxy, vtkSMProxy);
  vtkGetObjectMacro(ActorProxy, vtkSMProxy);
  vtkGetObjectMacro(GeometryFilterProxy, vtkSMProxy);
//ETX

  vtkSMProxy *GeometryFilterProxy;
  vtkSMProxy *UpdateSuppressorProxy;
  vtkSMProxy *MapperProxy; 
  vtkSMProxy *PropertyProxy;
  vtkSMProxy *ActorProxy; 

  vtkSMProxy* VolumeFilterProxy;
  vtkSMProxy* VolumeUpdateSuppressorProxy;
  vtkSMProxy* VolumePTMapperProxy;
  vtkSMProxy* VolumeBunykMapperProxy;
  vtkSMProxy* VolumeZSweepMapperProxy;
  vtkSMProxy* VolumeActorProxy;
  vtkSMProxy* VolumePropertyProxy;
  vtkSMProxy* OpacityFunctionProxy;
  vtkSMProxy* ColorTransferFunctionProxy;

  int HasVolumePipeline; 
  int SupportsZSweepMapper;
  int SupportsBunykMapper;
  
  // Flag to avoid setting up the volume pipeline
  // if the input is not vtkUnstructuredGrid. I go as far as
  // removing all the volume subproxies as well (and not creating them at all!)
  // just to save the effort when not needed.
  int VolumeRenderMode; // Flag to tell if we are using the volume rendering.

  // Description:
  // Called when Volume rendering is turned on/off.
  virtual void VolumeRenderModeOn();
  virtual void VolumeRenderModeOff();

  // IVars to avoid unnecessary setting of values.
  int Visibility;
  int Representation;
  
  int GeometryIsValid;
  int VolumeGeometryIsValid;
  int CanCreateProxy;

  int GeometryInformationIsValid;
  vtkPVGeometryInformation* GeometryInformation;

  // Invalidate geometry. If useCache is true, do not invalidate
  // cached geometry
  virtual void InvalidateGeometryInternal(int useCache);

private:
  vtkSMDataObjectDisplayProxy(const vtkSMDataObjectDisplayProxy&); // Not implemented.
  void operator=(const vtkSMDataObjectDisplayProxy&); // Not implemented.
};


#endif

