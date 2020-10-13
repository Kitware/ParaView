/*=========================================================================

  Program:   ParaView
  Module:    vtkDataLabelRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataLabelRepresentation
 * @brief   representation for showing cell and point
 * labels.
 *
 * vtkDataLabelRepresentation is a representation for showing cell and/or point
 * labels. This representation relies on all the data being cloned on all nodes
 * hence beware of using this representation for large datasets.
 * @par Caveat:
 * Note that vtkDataLabelRepresentation adds the label props to the
 * non-composited renderer.
 * @par Thanks:
 * The addition of a transformation matrix was supported by CEA/DIF
 * Commissariat a l'Energie Atomique, Centre DAM Ile-De-France, Arpajon, France.
*/

#ifndef vtkDataLabelRepresentation_h
#define vtkDataLabelRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer.

class vtkActor2D;
class vtkCellCenters;
class vtkCallbackCommand;
class vtkMergeBlocks;
class vtkLabeledDataMapper;
class vtkMaskPoints;
class vtkProp3D;
class vtkTextProperty;
class vtkTransform;

class VTKREMOTINGVIEWS_EXPORT vtkDataLabelRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkDataLabelRepresentation* New();
  vtkTypeMacro(vtkDataLabelRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool val) override;
  bool GetVisibility() override;
  //@}

  //@{
  /**
   * Get/Set the maximum number of points/cells that will be labeled.  Too many labels
   * is difficult to read so this may help with large datasets.  Default: 100.
   */
  void SetMaximumNumberOfLabels(int numLabels);
  int GetMaximumNumberOfLabels();
  //@}

  //***************************************************************************
  // Methods to change various parameters on internal objects
  void SetPointLabelVisibility(int);
  void SetPointFieldDataArrayName(const char*);
  void SetPointLabelMode(int);
  void SetPointLabelColor(double r, double g, double b);
  void SetPointLabelOpacity(double);
  void SetPointLabelFontFamily(int);
  void SetPointLabelFontFile(char*);
  void SetPointLabelBold(int);
  void SetPointLabelItalic(int);
  void SetPointLabelShadow(int);
  void SetPointLabelJustification(int);
  void SetPointLabelFontSize(int);
  void SetPointLabelFormat(const char*);

  void SetCellLabelVisibility(int);
  void SetCellFieldDataArrayName(const char*);
  void SetCellLabelMode(int);
  void SetCellLabelColor(double r, double g, double b);
  void SetCellLabelOpacity(double);
  void SetCellLabelFontFamily(int);
  void SetCellLabelFontFile(char*);
  void SetCellLabelBold(int);
  void SetCellLabelItalic(int);
  void SetCellLabelShadow(int);
  void SetCellLabelJustification(int);
  void SetCellLabelFontSize(int);
  void SetCellLabelFormat(const char*);

  //@{
  /**
   * Used to build the internal transform.
   */
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);
  void SetUserTransform(const double[16]);
  //@}

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

protected:
  vtkDataLabelRepresentation();
  ~vtkDataLabelRepresentation() override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  /**
   * Fill input port information
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

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
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void UpdateTransform();

  vtkMergeBlocks* MergeBlocks;

  vtkSmartPointer<vtkMaskPoints> PointMask;
  vtkLabeledDataMapper* PointLabelMapper;
  vtkTextProperty* PointLabelProperty;
  vtkActor2D* PointLabelActor;

  vtkCellCenters* CellCenters;
  vtkSmartPointer<vtkMaskPoints> CellMask;
  vtkLabeledDataMapper* CellLabelMapper;
  vtkTextProperty* CellLabelProperty;
  vtkActor2D* CellLabelActor;

  vtkProp3D* TransformHelperProp;
  vtkTransform* Transform;

  vtkSmartPointer<vtkDataObject> Dataset;

  // Mutes label mapper warnings
  vtkCallbackCommand* WarningObserver;
  static void OnWarningEvent(vtkObject* source, unsigned long, void* clientdata, void*);

  int PointLabelVisibility;
  int CellLabelVisibility;
  int MaximumNumberOfLabels;

private:
  vtkDataLabelRepresentation(const vtkDataLabelRepresentation&) = delete;
  void operator=(const vtkDataLabelRepresentation&) = delete;
};

#endif
