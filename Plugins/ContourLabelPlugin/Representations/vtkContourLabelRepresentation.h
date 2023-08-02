// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkContourLabelRepresentation_h
#define vtkContourLabelRepresentation_h

#include "vtkContourLabelPluginModule.h" // export macro
#include "vtkNew.h"                      // vtkNew
#include "vtkPVDataRepresentation.h"

#include <string> // std::string

class vtkPolyData;
class vtkLabeledContourMapper;
class vtkPVLODActor;
class vtkScalarsToColors;
class vtkTextProperty;

/**
 * @class   vtkContourLabelRepresentation
 * @brief   Provide labelling capabilities for polydata datasets that represent lines.
 *
 * This representation expose the 'vtkLabeledContourMapper' in ParaView.
 * It uses the information contained in the pipeline informations vector in
 * order to set which array is displayed.
 *
 * The input of this representation needs to be a polydata with only lines inside.
 * An internal stripper filter will take care of cleaning the dataset.
 */
class VTKCONTOURLABELPLUGIN_EXPORT vtkContourLabelRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkContourLabelRepresentation* New();
  vtkTypeMacro(vtkContourLabelRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Provide vtkPVDataRepresentation specific API implementation for this representation.
   */
  int ProcessViewRequest(vtkInformationRequestKey*, vtkInformation*, vtkInformation*) override;
  void SetVisibility(bool value) override;
  vtkDataObject* GetRenderedDataObject(int port) override;
  ///@}

  /**
   * Return the rendered prop that this representation handle.
   */
  vtkPVLODActor* GetActor() { return this->Actor; }

  /**
   * Control array for coloring and labeling. If `idx == 0` the array will be used for coloring,
   * else it will be used for labeling.
   */
  using Superclass::SetInputArrayToProcess; // Force overload lookup on superclass
  void SetInputArrayToProcess(int idx, int port, int connection, int fieldAssociation,
    const char* attributeTypeorName) override;

  ///@{
  /**
   * Forward rendering parameters to the underlying mapper / actor
   */
  virtual void SetAmbientColor(double r, double g, double b);
  virtual void SetDiffuseColor(double r, double g, double b);
  virtual void SetLookupTable(vtkScalarsToColors* val);
  virtual void SetInterpolateScalarsBeforeMapping(bool val);
  virtual void SetMapScalars(int val);
  virtual void SetOpacity(float val);
  virtual void SetLineWidth(float val);
  virtual void SetRenderLinesAsTubes(bool val);
  virtual void SetLabelTextProperty(vtkTextProperty* val);
  virtual void SetSkipDistance(float val);
  ///@}

protected:
  vtkContourLabelRepresentation();
  ~vtkContourLabelRepresentation() override = default;

  ///@{
  /**
   * Provide vtkPVDataRepresentation specific API implementation for this representation.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;
  ///@}

  /**
   * Handle the selected array and update how scalars should be rendered.
   */
  void UpdateColoringParameters();

private:
  vtkContourLabelRepresentation(const vtkContourLabelRepresentation&) = delete;
  void operator=(const vtkContourLabelRepresentation&) = delete;

  double VisibleDataBounds[6]{ 0.0 };
  std::string LabelArray;
  vtkNew<vtkPolyData> Cache;
  vtkNew<vtkPVLODActor> Actor;
  vtkNew<vtkLabeledContourMapper> Mapper;
};

#endif
