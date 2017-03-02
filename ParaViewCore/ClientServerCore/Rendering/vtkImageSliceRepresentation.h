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
/**
 * @class   vtkImageSliceRepresentation
 * @brief   representation for showing slices from a
 * vtkImageData.
 *
 * vtkImageSliceRepresentation is a representation for showing slices from an
 * image dataset. Currently, it does not support composite datasets, however, we
 * should be able to add such a support in future.
*/

#ifndef vtkImageSliceRepresentation_h
#define vtkImageSliceRepresentation_h

#include "vtkNew.h"                               // for vtkNew.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkStructuredData.h" // for VTK_*_PLANE

class vtkImageData;
class vtkPExtentTranslator;
class vtkPVCacheKeeper;
class vtkPVImageSliceMapper;
class vtkPVLODActor;
class vtkScalarsToColors;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkImageSliceRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkImageSliceRepresentation* New();
  vtkTypeMacro(vtkImageSliceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the input data arrays that this algorithm will process. Overridden to
   * pass the array selection to the mapper.
   */
  virtual void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) VTK_OVERRIDE;
  virtual void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) VTK_OVERRIDE
  {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, fieldAttributeType);
  }
  virtual void SetInputArrayToProcess(int idx, vtkInformation* info) VTK_OVERRIDE
  {
    this->Superclass::SetInputArrayToProcess(idx, info);
  }
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    const char* fieldAssociation, const char* attributeTypeorName) VTK_OVERRIDE
  {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, attributeTypeorName);
  }
  //@}

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

  /**
   * This needs to be called on all instances of vtkImageSliceRepresentation when
   * the input is modified. This is essential since the geometry filter does not
   * have any real-input on the client side which messes with the Update
   * requests.
   */
  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   * Overridden to propagate to the active representation.
   */
  virtual void SetVisibility(bool val) VTK_OVERRIDE;

  //@{
  /**
   * Get set the slice number to extract.
   */
  virtual void SetSlice(unsigned int);
  vtkGetMacro(Slice, unsigned int);
  //@}

  enum
  {
    XY_PLANE = VTK_XY_PLANE,
    YZ_PLANE = VTK_YZ_PLANE,
    XZ_PLANE = VTK_XZ_PLANE
  };

  //@{
  /**
   * Get/Set the direction in which to slice a 3D input data.
   */
  virtual void SetSliceMode(int);
  vtkGetMacro(SliceMode, int);
  //@}

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

  /**
   * Provides access to the actor used by this representation.
   */
  vtkPVLODActor* GetActor() { return this->Actor; }

  //@{
  /**
   * Pass lighting parameters to vtkProperty.
   */
  void SetAmbient(double ambient);
  void SetDiffuse(double diffuse);
  void SetSpecular(double specular);
  void SetSpecularPower(double val);
  void SetSpecularColor(double r, double g, double b);
  //@}

protected:
  vtkImageSliceRepresentation();
  ~vtkImageSliceRepresentation();

  /**
   * Extract the slice.
   */
  void UpdateSliceData(vtkInformationVector**);

  /**
   * Fill input port information.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Subclasses should override this to connect inputs to the internal pipeline
   * as necessary. Since most representations are "meta-filters" (i.e. filters
   * containing other filters), you should create shallow copies of your input
   * before connecting to the internal pipeline. The convenience method
   * GetInternalOutputPort will create a cached shallow copy of a specified
   * input for you. The related helper functions GetInternalAnnotationOutputPort,
   * GetInternalSelectionOutputPort should be used to obtain a selection or
   * annotation port whose selections are localized for a particular input data object.
   */
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  /**
   * Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
   */
  virtual bool IsCached(double cache_key) VTK_OVERRIDE;

  int SliceMode;
  unsigned int Slice;

  vtkPVCacheKeeper* CacheKeeper;
  vtkPVImageSliceMapper* SliceMapper;
  vtkPVLODActor* Actor;
  vtkImageData* SliceData;

  // meta-data about the input image to pass on to render view for hints
  // when redistributing data.
  vtkNew<vtkPExtentTranslator> PExtentTranslator;
  int WholeExtent[6];

private:
  vtkImageSliceRepresentation(const vtkImageSliceRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageSliceRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
