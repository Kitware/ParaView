// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVFeatureEdges
 * @brief Feature Edges filter that delegates to type specific implementations
 *
 * This is a meta filter of vtkPVFeatureEdges that allows selection of
 * input vtkHyperTreeGrid or vtkPolyData
 */

#ifndef vtkPVFeatureEdges_h
#define vtkPVFeatureEdges_h

#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVFeatureEdges : public vtkPolyDataAlgorithm
{

public:
  static vtkPVFeatureEdges* New();
  vtkTypeMacro(vtkPVFeatureEdges, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off the extraction of boundary edges.
   * Default is true.
   */
  vtkSetMacro(BoundaryEdges, bool);
  vtkGetMacro(BoundaryEdges, bool);
  vtkBooleanMacro(BoundaryEdges, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the extraction of feature edges.
   * Default is true.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(FeatureEdges, bool);
  vtkGetMacro(FeatureEdges, bool);
  vtkBooleanMacro(FeatureEdges, bool);
  ///@}

  ///@{
  /**
   * Specify the feature angle for extracting feature edges.
   * Default is 30.0
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  vtkGetMacro(FeatureAngle, double);
  ///@}

  ///@{
  /**
   * Turn on/off the extraction of non-manifold edges.
   * Default is true.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(NonManifoldEdges, bool);
  vtkGetMacro(NonManifoldEdges, bool);
  vtkBooleanMacro(NonManifoldEdges, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the extraction of manifold edges. This typically
   * correspond to interior edges.
   * Default is false.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(ManifoldEdges, bool);
  vtkGetMacro(ManifoldEdges, bool);
  vtkBooleanMacro(ManifoldEdges, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the coloring of edges by type.
   * Default is false.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(Coloring, bool);
  vtkGetMacro(Coloring, bool);
  vtkBooleanMacro(Coloring, bool);
  ///@}

  ///@{
  /**
   * Turn on/off merging of coincident points using a locator.
   * Note that when merging is on, points with different point attributes
   * (e.g., normals) are merged, which may cause rendering artifacts.
   * Default is false.
   *
   * @note: Unused if input is NOT a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  ///@}

protected:
  vtkPVFeatureEdges() = default;
  ~vtkPVFeatureEdges() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkPVFeatureEdges(const vtkPVFeatureEdges&) = delete;
  void operator=(const vtkPVFeatureEdges&) = delete;

  // PolyData input options
  double FeatureAngle = 30.0;
  bool BoundaryEdges = true;
  bool FeatureEdges = true;
  bool NonManifoldEdges = true;
  bool ManifoldEdges = false;
  bool Coloring = false;

  // HTG options
  bool MergePoints = false;
};

#endif
