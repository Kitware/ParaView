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
  vtkTypeRevisionMacro(vtkSMUnstructuredGridVolumeRepresentationProxy, vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns whether this representation shows selection.
  // Overridden to turn off selection visibility if no "Selection" object is
  // set.
  virtual bool GetSelectionVisibility();

  // Description:
  // Called to update the Representation. 
  // Overridden to ensure that UpdateSelectionPropVisibility() is called.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Adds to the passed in collection the props that representation has which
  // are selectable. The passed in collection object must be empty.
  virtual void GetSelectableProps(vtkCollection*);

  // Description:
  // Given a surface selection for this representation, this returns a new
  // vtkSelection for the selected cells/points in the input of this
  // representation.
  virtual void ConvertSurfaceSelectionToVolumeSelection(
   vtkSelection* input, vtkSelection* output);

  // Description:
  // Flag indicating if the display supports a volume rendering 
  // representation.
  vtkGetMacro(SupportsHAVSMapper,   int);
  vtkGetMacro(SupportsBunykMapper,  int);
  vtkGetMacro(SupportsZSweepMapper, int);

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
  virtual bool BeginCreateVTKObjects(int numObjects);

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects(int numObjects);

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

  // Description:
  // Updates selection prop visibility based on whether selection can actually
  // be shown.
  virtual void UpdateSelectionPropVisibility();

  // Called to check if the display can support volume rendering.
  // Note that will will excute the input pipeline
  // if not already up-to-date.
  virtual void DetermineVolumeSupport();

  // Description:
  // Set up the vtkUnstructuredGrid (Volume) rendering pipeline.
  virtual void SetupVolumePipeline();

  // Description:
  // Get information about extensions from the view.
  void UpdateRenderViewExtensions(vtkSMViewProxy*);

  // Unstructured volume rendering classes
  vtkSMProxy* VolumeFilter;
  vtkSMProxy* VolumePTMapper;
  vtkSMProxy* VolumeHAVSMapper;
  vtkSMProxy* VolumeBunykMapper;
  vtkSMProxy* VolumeZSweepMapper;

  // Common volume rendering classes
  vtkSMProxy* VolumeActor;
  vtkSMProxy* VolumeProperty;

  int SupportsHAVSMapper;
  int SupportsZSweepMapper;
  int SupportsBunykMapper;
  int RenderViewExtensionsTested;

  // Proxies for the selection pipeline.
  vtkSMSourceProxy* ExtractSelection;
  vtkSMSourceProxy* SelectionGeometryFilter;
  vtkSMProxy* SelectionMapper;
  vtkSMProxy* SelectionLODMapper;
  vtkSMProxy* SelectionProp3D;
  vtkSMProxy* SelectionProperty;

private:
  vtkSMUnstructuredGridVolumeRepresentationProxy(const vtkSMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif

