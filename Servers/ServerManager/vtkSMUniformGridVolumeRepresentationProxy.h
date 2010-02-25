/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUniformGridVolumeRepresentationProxy - representation that can be used to
// show a uniform grid volume in a render view.
// .SECTION Description
// vtkSMUniformGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the uniform grid volume in a vtkSMRenderViewProxy.
// It supports rendering the uniform grid volume data.

#ifndef __vtkSMUniformGridVolumeRepresentationProxy_h
#define __vtkSMUniformGridVolumeRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMUniformGridVolumeRepresentationProxy :
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMUniformGridVolumeRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMUniformGridVolumeRepresentationProxy,
    vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Set the active volume mapper by enum index.
  void SetSelectedMapperIndex(int);
  vtkGetMacro(SelectedMapperIndex, int);

  // Description:
  // Convenience methods for switching between volume
  // mappers.
  void SetVolumeMapperToFixedPoint();
  void SetVolumeMapperToXYZ();

  // Note: Do we need to have the name like this?
  // GetVolumeMapperTypeCM()? What is CM stands for.
  virtual int GetVolumeMapperType();

//BTX
protected:

  // Volume mapper types.
  enum
    {
    FIXED_POINT_MAPPER,
    GPU_MAPPER,
    UNKNOWN_VOLUME_MAPPER
    };


  vtkSMUniformGridVolumeRepresentationProxy();
  ~vtkSMUniformGridVolumeRepresentationProxy();

  // Description:
  // This representation needs a uniform grid volume compositing strategy.
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

  // Structured grid volume rendering classes
  vtkSMProxy* VolumeFixedPointRayCastMapper;
  vtkSMProxy* VolumeGPURayCastMapper;

  // Common volume rendering classes
  vtkSMProxy* VolumeActor;
  vtkSMProxy* VolumeProperty;
  vtkSMProxy* ClientMapper;

  // @Note: Should we have check if one or more
  // Mapper type is supported?.
  int SelectedMapperIndex;


private:
  vtkSMUniformGridVolumeRepresentationProxy(const vtkSMUniformGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSMUniformGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif

