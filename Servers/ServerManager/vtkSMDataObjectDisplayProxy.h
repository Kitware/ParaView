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
// This is the superclass for objects that display data objects.
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

class vtkSMPropertyLink;
class vtkSMProxy;
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
  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);

  // Description:
  // Obtain the proxy for the algorithm that this displays the output of.
  vtkSMProxy *GetInput(int i=0);

  // Description:
  // Set/get a texture to use in displaying the data object.
  void SetTexture(vtkSMProxy *texture);
  vtkSMProxy* GetTexture();

  // Description:
  // This method calls a ForceUpdate on the UpdateSuppressor
  // if the Geometry is not valid. 
  virtual void Update(vtkSMAbstractViewModuleProxy*);
  virtual void Update() { this->Superclass::Update(); }

  // Description:
  // Set the update time passed on to the update suppressor.
  virtual void SetUpdateTime(double time);

  // Description:
  // This method returns if the Update() or UpdateDistributedGeometry()
  // calls will actually lead to an Update. This is used by the render module
  // to decide if it can expect any pipeline updates.
  virtual int UpdateRequired();
  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Get information about the data being displayed. When representation is
  // Volume, it simply returns the DataInformation from the input,
  // otherwise, it's the data information from the GeometryFilter.
  // Some displays (like Scalar bar, 3DWidgets), may return NULL.
  virtual vtkPVGeometryInformation* GetDisplayedDataInformation();
  
  
  // Description:
  // Set the representation for this display.
  virtual void SetRepresentation(int representation);

  // Description:
  // Set Visibility of the display.
  virtual void SetVisibility(int visible);

  // Description:
  // Flag indicating if the display supports a volume rendering 
  // representation.
  // VolumePipelineType is one of INVALID (not-determined yet), 
  // NONE (no volume support), IMAGE_DATA, UNSTRUCTURED_GRID.
  vtkGetMacro(VolumePipelineType,    int);
  vtkGetMacro(SupportsHAVSMapper,   int);
  vtkGetMacro(SupportsBunykMapper,  int);
  vtkGetMacro(SupportsZSweepMapper, int);
  
  // Description:
  // Flag indicating if the display is currently rendered
  // as a volume. Typically, one would not use this flag,
  // instead check status of the property "Representation".
  // Here, only for RenderModuleProxy.
  vtkGetMacro(VolumeRenderMode, int);

/*
  // Description:
  // Convenience methods for switching between volume
  // mappers.
  void SetVolumeMapperToBunykCM();
  void SetVolumeMapperToPTCM();
  void SetVolumeMapperToHAVSCM();
  void SetVolumeMapperToZSweepCM();
  
  // Description:
  // Convenience method for determining which
  // volume mapper is in use
  virtual int GetVolumeMapperTypeCM();
*/

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
    HAVS_VOLUME_MAPPER,
    ZSWEEP_VOLUME_MAPPER,
    BUNYK_RAY_CAST_VOLUME_MAPPER,
    UNKNOWN_VOLUME_MAPPER
  };

  enum VolumePipelines
  {
    INVALID=13, /* Set when the display hasn;t yet determined if Volume 
                   is supported at all. */
    NONE = 0,   /* Display is certain that we cannot volume render */
    UNSTRUCTURED_GRID,
    IMAGE_DATA
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
  
/*
  // Description:
  // Convenience method to get/set the Interpolation for this proxy.
  int GetInterpolationCM();
  void SetInterpolationCM(int interpolation);

  // Description:
  // Convenience method to get/set Point Size.
  void SetPointSizeCM(double size);
  double GetPointSizeCM();
*/

  // Description:
  // Convenience method to get/set Line Width.
  void SetLineWidthCM(double width);
/*
  double GetLineWidthCM();

  // Description:
  // Convenience method to get/set Scalar Mode.
  void SetScalarModeCM(int  mode);
*/
  int GetScalarModeCM();

/*
  // Description:
  // Convenience method to get/set ScalarArray (Volume Mapper).
  void SetScalarArrayCM(const char* arrayname);
  const char* GetScalarArrayCM();

  // Description:
  // Convenience method to get/set Opacity.
  void SetOpacityCM(double op);
*/
  double GetOpacityCM();

/*
  // Description:
  // Convenience method to get/set ColorMode.
  void SetColorModeCM(int mode);
  int GetColorModeCM();
*/

  // Description:
  // Convenience method to get/set Actor color.
  void SetColorCM(double rgb[3]);
  void GetColorCM(double rgb[3]);
  void SetColorCM(double r, double g, double b)
    { 
    double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] =b;
    this->SetColorCM(rgb);
    }

/*
  // Description:
  // Convenience method to get/set InterpolateColorsBeforeMapping property.
  void SetInterpolateScalarsBeforeMappingCM(int flag);
  int GetInterpolateScalarsBeforeMappingCM();
*/

  // Description:
  // Convenience method to get/set ScalarVisibility property.
  void SetScalarVisibilityCM(int);
  int GetScalarVisibilityCM();

/*
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
*/

  // Description:
  // Convenience method to get/set Representation.
  void SetRepresentationCM(int r);
  int GetRepresentationCM();

/*
  // Description:
  // Convenience method to get/set ImmediateModeRendering property.
  void SetImmediateModeRenderingCM(int f);
  int GetImmediateModeRenderingCM();

  // Description:
  // Convenience method to get/set Pickability of the vtkActor.
  void SetPickableCM(int pickable);
  int GetPickableCM();
*/

protected:
  vtkSMDataObjectDisplayProxy();
  ~vtkSMDataObjectDisplayProxy();

  virtual void SetInputInternal(vtkSMSourceProxy* input, 
                                unsigned int outputPort);

  // Description:
  // Set up the PolyData rendering pipeline.
  virtual void SetupPipeline();
  virtual void SetupDefaults();

  // Description:
  // Set up the vtkUnstructuredGrid (Volume) rendering pipeline.
  virtual void SetupVolumePipeline();
  virtual void SetupVolumeDefaults();

 
  virtual void CreateVTKObjects();

  virtual void GatherDisplayedDataInformation();

//BTX
  // This is the least intrusive way of giving vtkPVComparativeVisManager
  // access to the MapperProxy. It needs this proxy to extract and cache
  // the input geometry
  friend class vtkPVComparativeVisManager;
  friend class vtkPVComparativeVis;
  friend class vtkSMComparativeVisProxy;
  friend class vtkSMRenderModuleProxy;
  friend class vtkSMSelectionManager;
  friend class pqSelectionManager;
  friend class vtkSMSelectionProxy;
  friend class vtkSMCellLabelAnnotationDisplayProxy;
  vtkGetObjectMacro(MapperProxy, vtkSMProxy);
  vtkGetObjectMacro(ActorProxy, vtkSMProxy);
  vtkGetObjectMacro(GeometryFilterProxy, vtkSMProxy);
//ETX

  vtkSMProxy *GeometryFilterProxy;
  vtkSMProxy *UpdateSuppressorProxy;
  vtkSMProxy *MapperProxy; 
  vtkSMProxy *PropertyProxy;
  vtkSMProxy *ActorProxy; 

  // Unstructured volume rendering classes
  vtkSMProxy* VolumeFilterProxy;
  vtkSMProxy* VolumePTMapperProxy;
  vtkSMProxy* VolumeHAVSMapperProxy;
  vtkSMProxy* VolumeBunykMapperProxy;
  vtkSMProxy* VolumeZSweepMapperProxy;

  // Structured grid volume rendering classes
  vtkSMProxy* VolumeFixedPointRayCastMapperProxy;

  // Common volume rendering classes
  vtkSMProxy* VolumeUpdateSuppressorProxy;
  vtkSMProxy* VolumeActorProxy;
  vtkSMProxy* VolumePropertyProxy;
  vtkSMProxy* OpacityFunctionProxy;

  // These are pointers to update suppressor proxies
  // that keep the cache for animation.
  // By default these are 
  // same as UpdateSuppressorProxy and VolumeUpdateSuppressorProxy,
  // Subclasses may change these.
  vtkSMProxy* CacherProxy;
  vtkSMProxy* VolumeCacherProxy;

  int VolumePipelineType; 

  int SupportsHAVSMapper;
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

  // Description:
  // Get information about extensions from the view module.
  void UpdateRenderModuleExtensions(vtkSMAbstractViewModuleProxy*);

  // IVars to avoid unnecessary setting of values.
  int Visibility;
  int Representation;
  
  int GeometryIsValid;
  int VolumeGeometryIsValid;
  int CanCreateProxy;

  int RenderModuleExtensionsTested;

  int DisplayedDataInformationIsValid;
  vtkPVGeometryInformation* DisplayedDataInformation;

  // Invalidate geometry. If useCache is true, do not invalidate
  // cached geometry
  virtual void InvalidateGeometryInternal(int useCache);

  // Called to check if the display can support volume rendering.
  // Note that will will excute the input pipeline
  // if not already up-to-date.
  void DetermineVolumeSupport();

  // convenience method to connect to proxies.
  bool Connect(vtkSMProxy* consumer, vtkSMProxy* producer, int outputPort);

  // Description:
  // Connect the VTK data object to display pipeline.
  void SetInput(vtkSMProxy* input, unsigned int outputPort);

  // Link used to link "ColorArray" from geometry mapper 
  // to "SelectScalarArray" on volume mappers.
  vtkSMPropertyLink* ColorArrayLink;

  // Link used to link "LookupTable" from geometry mapper
  // to "ColorTransferFunction" on  volume property.
  vtkSMPropertyLink* LookupTableLink;

  double UpdateTime;
private:
  vtkSMDataObjectDisplayProxy(const vtkSMDataObjectDisplayProxy&); // Not implemented.
  void operator=(const vtkSMDataObjectDisplayProxy&); // Not implemented.
};


#endif

