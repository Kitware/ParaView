/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRVolumeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRVolumeRepresentation - representation for showing image
// datasets as a volume.
// .SECTION Description
// vtkAMRVolumeRepresentation is a representation for volume rendering
// vtkAMRData.

#ifndef __vtkAMRVolumeRepresentation_h
#define __vtkAMRVolumeRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for iVars.

class vtkAMRIncrementalResampleHelper;
class vtkColorTransferFunction;
class vtkOverlappingAMR;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkPVCacheKeeper;
class vtkPVLODVolume;
class vtkPVRenderView;
class vtkSmartVolumeMapper;
class vtkVolumeProperty;

class VTK_EXPORT vtkAMRVolumeRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkAMRVolumeRepresentation* New();
  vtkTypeMacro(vtkAMRVolumeRepresentation, vtkPVDataRepresentation);
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
  //Select the type of rendering approach to use.
  vtkSetMacro(RequestedRenderMode, int);
  vtkGetMacro(RequestedRenderMode, int);

  // Description:
  //Select the type of resampling techinque approach to use.
  vtkSetMacro(RequestedResamplingMode, int);
  vtkGetMacro(RequestedResamplingMode, int);

  // Description:
  // Set/Get the number of samples/cells along the i/j/k directions.
  // The default is 128x128x128
  vtkSetVector3Macro(NumberOfSamples,int);
  vtkGetVector3Macro(NumberOfSamples,int);

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
  virtual void SetVisibility(bool val);

  //***************************************************************************
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty.
  void SetInterpolationType(int val);
  void SetColor(vtkColorTransferFunction* lut);
  void SetScalarOpacity(vtkPiecewiseFunction* pwf);
  void SetScalarOpacityUnitDistance(double val);
  void SetAmbient(double);
  void SetDiffuse(double);
  void SetSpecular(double);
  void SetSpecularPower(double);
  void SetShade(bool);

  vtkSetMacro(FreezeFocalPoint,bool);
  vtkGetMacro(FreezeFocalPoint,bool);

  // Description:
  // When steaming, this is the block that's requested from the upstream.
  void SetStreamingBlockId(unsigned int val)
    {
    if (this->StreamingCapableSource)
      {
      this->StreamingBlockId = val;
      this->MarkModified();
      }
    }

  void ResetStreamingBlockId()
    { this->StreamingBlockId = 0; }
//BTX
protected:
  vtkAMRVolumeRepresentation();
  ~vtkAMRVolumeRepresentation();

  // Description:
  // Gets the metadata from upstream module and determines which blocks
  // should be loaded by this instance.
  virtual int RequestInformation(vtkInformation *rqst,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector );

  // Description:
  // Performs upstream requests to the reader
  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, 
    vtkInformationVector* );

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

  vtkOverlappingAMR* Cache;
  vtkPVCacheKeeper* CacheKeeper;
  vtkSmartVolumeMapper* VolumeMapper;
  vtkVolumeProperty* Property;
  vtkPVLODVolume* Actor;
  vtkAMRIncrementalResampleHelper* Resampler;
  vtkWeakPointer<vtkPVRenderView> RenderView;

  int ColorAttributeType;
  char* ColorArrayName;
  int RequestedRenderMode;
  int RequestedResamplingMode;
  int NumberOfSamples[3];
  bool FreezeFocalPoint;
  bool StreamingCapableSource;
  bool InitializeResampler;

  unsigned int StreamingBlockId;

  double DataBounds[6];

private:
  vtkAMRVolumeRepresentation(const vtkAMRVolumeRepresentation&); // Not implemented
  void operator=(const vtkAMRVolumeRepresentation&); // Not implemented

//ETX
};

#endif
