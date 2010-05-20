/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUnstructuredGridVolumeRepresentationProxy - representation that can be used to
// show a unstructured grid volume in a render view.
// .SECTION Description
// vtkSMUnstructuredGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the unstructured grid volume in a vtkSMRenderViewProxy.

#ifndef __vtkSMUnstructuredGridVolumeRepresentationProxy_h
#define __vtkSMUnstructuredGridVolumeRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMUnstructuredGridVolumeRepresentationProxy : 
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMUnstructuredGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSMUnstructuredGridVolumeRepresentationProxy,
    vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. 
  // Overridden to ensure to check for OpenGL extensions,
  // and also so update the EnableLOD flag on the prop based on ViewInformation.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Flag indicating if the display supports a volume rendering 
  // representation.
  vtkGetMacro(SupportsHAVSMapper,   int);
  vtkGetMacro(SupportsBunykMapper,  int);
  vtkGetMacro(SupportsZSweepMapper, int);

  // Description:
  // Set the active volume mapper by enum index.
  void SetSelectedMapperIndex(int);
  vtkGetMacro(SelectedMapperIndex, int);

  // Description:
  // Set the active volume mapper by enum index only if
  // the mapper is supported.
  void SetSelectedMapperIndexIfSupported(int);

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

//BTX
  // Volume mapper types.
  enum 
  {
    PROJECTED_TETRA_VOLUME_MAPPER =0,
    HAVS_VOLUME_MAPPER,
    ZSWEEP_VOLUME_MAPPER,
    BUNYK_RAY_CAST_VOLUME_MAPPER,
    UNKNOWN_VOLUME_MAPPER
  };
//ETX

  // Description:
  // Set the scalar coloring mode
  void SetColorAttributeType(int type);

  // Description:
  // Set the scalar color array name. If array name is 0 or "" then scalar
  // coloring is disabled.
  void SetColorArrayName(const char* name);

  // Description:
  // Volume rendering always need ordered compositing.
  virtual bool GetOrderedCompositingNeeded()
    { return true; }

  // Description:
  // Check if this representation has the prop by checking its vtkClientServerID
  virtual bool HasVisibleProp3D(vtkProp3D* prop);

  // Description:
  // Views typically support a mechanism to create a selection in the view
  // itself, e.g. by click-and-dragging over a region in the view. The view
  // passes this selection to each of the representations and asks them to
  // convert it to a proxy for a selection which can be set on the view. 
  // Its representation does not support selection creation, it should simply
  // return NULL. This method returns a new vtkSMProxy instance which the 
  // caller must free after use.
  // This implementation converts a prop selection to a selection source.
  virtual vtkSMProxy* ConvertSelection(vtkSelection* input);

  // Description:
  // Update self property method, sets the color lookup table on the
  // VolumeProperty and  LODMapper.
  void SetLookupTable(vtkSMProxy* lut);

  // Description:
  // Called to set the view information object.
  // Don't call this directly, it is called by the View.
  // Overridden to add modification observer.
  virtual void SetViewInformation(vtkInformation*);

//BTX
protected:
  vtkSMUnstructuredGridVolumeRepresentationProxy();
  ~vtkSMUnstructuredGridVolumeRepresentationProxy();

  // Description:
  // This representation needs a surface compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

    // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Called to check if the display can support volume rendering.
  // Note that will will excute the input pipeline
  // if not already up-to-date.
  virtual void DetermineVolumeSupport();

  // Description:
  // Get information about extensions from the view.
  void UpdateRenderViewExtensions(vtkSMViewProxy*);
  
  // Unstructured volume rendering classes
  vtkSMSourceProxy* VolumeFilter;
  vtkSMProxy* VolumePTMapper;
  vtkSMProxy* VolumeHAVSMapper;
  vtkSMProxy* VolumeBunykMapper;
  vtkSMProxy* VolumeZSweepMapper;
  vtkSMProxy* VolumeDummyMapper;
  vtkSMProxy* VolumeLODMapper;

  // Common volume rendering classes
  vtkSMProxy* VolumeActor;
  vtkSMProxy* VolumeProperty;

  int SupportsHAVSMapper;
  int SupportsZSweepMapper;
  int SupportsBunykMapper;
  int RenderViewExtensionsTested;
  int SelectedMapperIndex;

private:
  vtkSMUnstructuredGridVolumeRepresentationProxy(const vtkSMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSMUnstructuredGridVolumeRepresentationProxy&); // Not implemented

  void ProcessViewInformation();
  vtkCommand* ViewInformationObserver;
//ETX
};

#endif

