/*=========================================================================

  Program:   ParaView
  Module:    vtkImageSliceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSliceRepresentation - representation for showing slices from a
// vtkImageData.
// .SECTION Description
// vtkImageSliceRepresentation is a representation for showing slices from an
// image dataset. Currently, it does not support composite datasets, however, we
// should be able to add such a support in future.

#ifndef __vtkImageSliceRepresentation_h
#define __vtkImageSliceRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkStructuredData.h" // for VTK_*_PLANE

class vtkImageData;
class vtkPVCacheKeeper;
class vtkPVImageSliceMapper;
class vtkPVLODActor;
class vtkScalarsToColors;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkImageSliceRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkImageSliceRepresentation* New();
  vtkTypeMacro(vtkImageSliceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input data arrays that this algorithm will process. Overridden to
  // pass the array selection to the mapper.
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, const char *name);
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, int fieldAttributeType)
    {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, fieldAttributeType);
    }
  virtual void SetInputArrayToProcess(int idx, vtkInformation *info)
    {
    this->Superclass::SetInputArrayToProcess(idx, info);
    }
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
                              const char* fieldAssociation,
                              const char* attributeTypeorName)
    {
    this->Superclass::SetInputArrayToProcess(idx, port, connection,
      fieldAssociation, attributeTypeorName);
    }

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // This needs to be called on all instances of vtkImageSliceRepresentation when
  // the input is modified. This is essential since the geometry filter does not
  // have any real-input on the client side which messes with the Update
  // requests.
  virtual void MarkModified();

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  // Overridden to propagate to the active representation.
  virtual void SetVisibility(bool val);

  // Description:
  // Get set the slice number to extract.
  virtual void SetSlice(unsigned int);
  vtkGetMacro(Slice, unsigned int);

  //BTX
  enum
    {
    XY_PLANE = VTK_XY_PLANE,
    YZ_PLANE = VTK_YZ_PLANE,
    XZ_PLANE = VTK_XZ_PLANE
    };
  //ETX

  // Description:
  // Get/Set the direction in which to slice a 3D input data.
  virtual void SetSliceMode(int);
  vtkGetMacro(SliceMode, int);

  //---------------------------------------------------------------------------
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //---------------------------------------------------------------------------
  // Forwarded to vtkProperty.
  void SetOpacity(double val);

  //---------------------------------------------------------------------------
  // Forwarded to vtkPVImageSliceMapper.
  void SetLookupTable(vtkScalarsToColors* val);
  void SetMapScalars(int val);
  void SetUseXYPlane(int val);

  // Description:
  // Provides access to the actor used by this representation.
  vtkPVLODActor* GetActor() { return this->Actor; }

//BTX
protected:
  vtkImageSliceRepresentation();
  ~vtkImageSliceRepresentation();

  // Description:
  // Extract the slice.
  void UpdateSliceData(vtkInformationVector**);

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Subclasses should override this to connect inputs to the internal pipeline
  // as necessary. Since most representations are "meta-filters" (i.e. filters
  // containing other filters), you should create shallow copies of your input
  // before connecting to the internal pipeline. The convenience method
  // GetInternalOutputPort will create a cached shallow copy of a specified
  // input for you. The related helper functions GetInternalAnnotationOutputPort,
  // GetInternalSelectionOutputPort should be used to obtain a selection or
  // annotation port whose selections are localized for a particular input data object.
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

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

  int SliceMode;
  unsigned int Slice;

  int WholeExtent[6];
  vtkPVCacheKeeper* CacheKeeper;

  vtkPVImageSliceMapper* SliceMapper;
  vtkPVLODActor* Actor;
  vtkImageData* SliceData;
private:
  vtkImageSliceRepresentation(const vtkImageSliceRepresentation&); // Not implemented
  void operator=(const vtkImageSliceRepresentation&); // Not implemented
//ETX
};

#endif
