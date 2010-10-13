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
// .NAME vtkDataLabelRepresentation - representation for showing cell and point
// labels.
// .SECTION Description
// vtkDataLabelRepresentation is a representation for showing cell and/or point
// labels. This representation relies on all the data being cloned on all nodes
// hence beware of using this representation for large datasets.
//
// Note that vtkDataLabelRepresentation adds the label props to the
// non-composited renderer.

#ifndef __vtkDataLabelRepresentation_h
#define __vtkDataLabelRepresentation_h

#include "vtkPVDataRepresentation.h"

class vtkActor2D;
class vtkCellCenters;
class vtkCompositeDataToUnstructuredGridFilter;
class vtkLabeledDataMapper;
class vtkProp3D;
class vtkPVCacheKeeper;
class vtkTextProperty;
class vtkTransform;
class vtkUnstructuredDataDeliveryFilter;

class VTK_EXPORT vtkDataLabelRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkDataLabelRepresentation* New();
  vtkTypeMacro(vtkDataLabelRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  virtual bool GetVisibility();

  //***************************************************************************
  // Methods to change various parameters on internal objects
  void SetPointLabelVisibility(int);
  void SetPointFieldDataArrayName(const char*);
  void SetPointLabelMode(int);
  void SetPointLabelColor(double r, double g, double b);
  void SetPointLabelOpacity(double);
  void SetPointLabelFontFamily(int);
  void SetPointLabelBold(int);
  void SetPointLabelItalic(int);
  void SetPointLabelShadow(int);
  void SetPointLabelJustification(int);
  void SetPointLabelFontSize(int);

  void SetCellLabelVisibility(int);
  void SetCellFieldDataArrayName(const char*);
  void SetCellLabelMode(int);
  void SetCellLabelColor(double r, double g, double b);
  void SetCellLabelOpacity(double);
  void SetCellLabelFontFamily(int);
  void SetCellLabelBold(int);
  void SetCellLabelItalic(int);
  void SetCellLabelShadow(int);
  void SetCellLabelJustification(int);
  void SetCellLabelFontSize(int);

  // Description:
  // Used to build the internal transform.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

//BTX
protected:
  vtkDataLabelRepresentation();
  ~vtkDataLabelRepresentation();

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
  // Intialize the vtkUnstructuredDataDeliveryFilter.
  void InitializeForCommunication();

  // Description:
  // Fill input port information
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
  // Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
  virtual bool IsCached(double cache_key);

  void UpdateTransform();

  vtkCompositeDataToUnstructuredGridFilter* MergeBlocks;
  vtkPVCacheKeeper* CacheKeeper;
  vtkUnstructuredDataDeliveryFilter* DataCollector;

  vtkLabeledDataMapper* PointLabelMapper;
  vtkTextProperty* PointLabelProperty;
  vtkActor2D* PointLabelActor;

  vtkCellCenters* CellCenters;
  vtkLabeledDataMapper* CellLabelMapper;
  vtkTextProperty* CellLabelProperty;
  vtkActor2D* CellLabelActor;

  vtkProp3D* TransformHelperProp;
  vtkTransform* Transform;

  int PointLabelVisibility;
  int CellLabelVisibility;
private:
  vtkDataLabelRepresentation(const vtkDataLabelRepresentation&); // Not implemented
  void operator=(const vtkDataLabelRepresentation&); // Not implemented
//ETX
};

#endif
