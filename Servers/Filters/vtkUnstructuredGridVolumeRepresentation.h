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
// .NAME vtkUnstructuredGridVolumeRepresentation - representation for showing
// vtkUnstructuredGrid datasets as volumes.
// .SECTION Description
// vtkUnstructuredGridVolumeRepresentation is a representation for volume
// rendering vtkUnstructuredGrid datasets. It simply renders a translucent
// surface for LOD i.e. interactive rendering.

#ifndef __vtkUnstructuredGridVolumeRepresentation_h
#define __vtkUnstructuredGridVolumeRepresentation_h

#include "vtkPVDataRepresentation.h"

class vtkColorTransferFunction;
class vtkOrderedCompositeDistributor;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkProjectedTetrahedraMapper;
class vtkPVCacheKeeper;
class vtkPVGeometryFilter;
class vtkPVLODVolume;
class vtkUnstructuredDataDeliveryFilter;
class vtkUnstructuredGridVolumeMapper;
class vtkVolumeProperty;
class vtkVolumeRepresentationPreprocessor;

class VTK_EXPORT vtkUnstructuredGridVolumeRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkUnstructuredGridVolumeRepresentation* New();
  vtkTypeMacro(vtkUnstructuredGridVolumeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This is same a vtkDataObject::FieldAssociation types so you can use those
  // as well.
  enum AttributeTypes
    {
    POINT_DATA=0,
    CELL_DATA=1
    };

  // Description:
  // Methods to control scalar coloring. ColorAttributeType defines the
  // attribute type.
  vtkSetMacro(ColorAttributeType, int);
  vtkGetMacro(ColorAttributeType, int);

  // Description:
  // Pick the array to color with.
  vtkSetStringMacro(ColorArrayName);
  vtkGetStringMacro(ColorArrayName);

  // Description:
  // Register a volume mapper with the representation.
  void AddVolumeMapper(const char* name, vtkUnstructuredGridVolumeMapper*);

  // Description:
  // Set the active volume mapper to use.
  vtkSetStringMacro(ActiveVolumeMapper);
  vtkUnstructuredGridVolumeMapper* GetActiveVolumeMapper();

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // This needs to be called on all instances of vtkGeometryRepresentation when
  // the input is modified. This is essential since the geometry filter does not
  // have any real-input on the client side which messes with the Update
  // requests.
  virtual void MarkModified();

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  // Overridden to propagate to the active representation.
  virtual void SetVisibility(bool val);

  //***************************************************************************
  // Forwarded to vtkVolumeRepresentationPreprocessor
  void SetExtractedBlockIndex(unsigned int index);

  //***************************************************************************
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty and vtkProperty (when applicable).
  void SetInterpolationType(int val);
  void SetColor(vtkColorTransferFunction* lut);
  void SetScalarOpacity(vtkPiecewiseFunction* pwf);
  void SetScalarOpacityUnitDistance(double val);

//BTX
protected:
  vtkUnstructuredGridVolumeRepresentation();
  ~vtkUnstructuredGridVolumeRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  // Description:
  // Produce meta-data about this representation that the view may find useful.
  bool GenerateMetaData(vtkInformation*, vtkInformation*);

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
  virtual bool IsCached(double cache_key);

  // Description:
  // Passes on parameters to the active volume mapper
  virtual void UpdateMapperParameters();

  vtkVolumeRepresentationPreprocessor* Preprocessor;
  vtkPVCacheKeeper* CacheKeeper;
  vtkUnstructuredDataDeliveryFilter* DeliveryFilter;
  vtkOrderedCompositeDistributor* Distributor;
  vtkProjectedTetrahedraMapper* DefaultMapper;
  vtkVolumeProperty* Property;
  vtkPVLODVolume* Actor;

  vtkPVGeometryFilter* LODGeometryFilter;
  vtkUnstructuredDataDeliveryFilter* LODDeliveryFilter;
  vtkPolyDataMapper* LODMapper;

  int ColorAttributeType;
  char* ColorArrayName;
  char* ActiveVolumeMapper;

private:
  vtkUnstructuredGridVolumeRepresentation(const vtkUnstructuredGridVolumeRepresentation&); // Not implemented
  void operator=(const vtkUnstructuredGridVolumeRepresentation&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
