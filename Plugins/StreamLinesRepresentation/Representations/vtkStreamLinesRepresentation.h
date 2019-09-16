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
/**
 * @class   vtkStreamLinesRepresentation
 * @brief   Representation for showing data sets using live streamlines.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Bastien Jacquet, Kitware 2017
 * This work was supported by Total SA.
 */

#ifndef vtkStreamLinesRepresentation_h
#define vtkStreamLinesRepresentation_h

#include "vtkNew.h" // needed for vtkNew.
#include "vtkPVDataRepresentation.h"
#include "vtkSmartPointer.h"      //  for vtkSmartPointer
#include "vtkStreamLinesModule.h" // for export macro

class vtkColorTransferFunction;
class vtkExtentTranslator;
class vtkImageData;
class vtkInformation;
class vtkInformationRequestKey;
class vtkPExtentTranslator;
class vtkPolyDataMapper;
class vtkProperty;
class vtkPVLODActor;
class vtkScalarsToColors;
class vtkStreamLinesMapper;

class VTKSTREAMLINES_EXPORT vtkStreamLinesRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkStreamLinesRepresentation* New();
  vtkTypeMacro(vtkStreamLinesRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey*, vtkInformation*, vtkInformation*) override;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool val) override;

  //***************************************************************************
  // Forwarded to vtkProperty.
  virtual void SetAmbientColor(double r, double g, double b);
  virtual void SetColor(double r, double g, double b);
  virtual void SetDiffuseColor(double r, double g, double b);
  virtual void SetEdgeColor(double r, double g, double b);
  virtual void SetInterpolation(int val);
  virtual void SetLineWidth(double val);
  virtual void SetOpacity(double val);
  virtual void SetPointSize(double val);
  virtual void SetSpecularColor(double r, double g, double b);
  virtual void SetSpecularPower(double val);

  //***************************************************************************
  // Forwarded to Actor.
  virtual void SetOrientation(double, double, double);
  virtual void SetOrigin(double, double, double);
  virtual void SetPickable(int val);
  virtual void SetPosition(double, double, double);
  virtual void SetScale(double, double, double);
  virtual void SetUserTransform(const double[16]);

  //***************************************************************************
  // Forwarded to vtkStreamLinesMapper
  virtual void SetAnimate(bool val);
  virtual void SetAlpha(double val);
  virtual void SetStepLength(double val);
  virtual void SetNumberOfParticles(int val);
  virtual void SetMaxTimeToLive(int val);
  virtual void SetNumberOfAnimationSteps(int val);

  virtual void SetInputVectors(int, int, int, int attributeMode, const char* name);

  //***************************************************************************
  // Forwarded to Mapper and LODMapper.
  virtual void SetInterpolateScalarsBeforeMapping(int val);
  virtual void SetLookupTable(vtkScalarsToColors* val);

  //@{
  /**
   * Sets if scalars are mapped through a color-map or are used
   * directly as colors.
   * 0 maps to VTK_COLOR_MODE_DIRECT_SCALARS
   * 1 maps to VTK_COLOR_MODE_MAP_SCALARS
   * @see vtkScalarsToColors::MapScalars
   */
  void SetMapScalars(int val);

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Provides access to the actor used by this representation.
   */
  vtkPVLODActor* GetActor() { return this->Actor; }

  /**
   * Convenience method to get the array name used to scalar color with.
   */
  const char* GetColorArrayName();

  // Description:
  // Set the input data arrays that this algorithm will process. Overridden to
  // pass the array selection to the mapper.
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) override
  {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, fieldAttributeType);
  }
  void SetInputArrayToProcess(int idx, vtkInformation* info) override
  {
    this->Superclass::SetInputArrayToProcess(idx, info);
  }
  void SetInputArrayToProcess(int idx, int port, int connection, const char* fieldAssociation,
    const char* attributeTypeorName) override
  {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, attributeTypeorName);
  }

protected:
  vtkStreamLinesRepresentation();
  ~vtkStreamLinesRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

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
   * Passes on parameters to the active mapper
   */
  void UpdateMapperParameters();

  /**
   * Used in ConvertSelection to locate the rendered prop.
   */
  virtual vtkPVLODActor* GetRenderedProp() { return this->Actor; };

  vtkSmartPointer<vtkDataObject> Cache;
  vtkAlgorithm* MBMerger;
  vtkStreamLinesMapper* StreamLinesMapper;
  vtkProperty* Property;
  vtkPVLODActor* Actor;

  unsigned long DataSize;
  double DataBounds[6];

  // meta-data about the input image to pass on to render view for hints
  // when redistributing data.
  vtkNew<vtkPExtentTranslator> PExtentTranslator;
  double Origin[3];
  double Spacing[3];
  int WholeExtent[6];

private:
  vtkStreamLinesRepresentation(const vtkStreamLinesRepresentation&) = delete;
  void operator=(const vtkStreamLinesRepresentation&) = delete;
};

#endif
