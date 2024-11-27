// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAxisAlignedCutter
 * @brief Cut data with an Axis-Aligned cut function
 *
 * `vtkAxisAlignedCutter` is a filter producing Axis-Aligned "slices" of the input data.
 * Among slicing filters, this one allows to preserve the input type. In other words,
 * this filter reduces the dimention of the input by 1, following an implicit cutting function.
 * For example, slicing a 3D `vtkHyperTreeGrid` will produce one (or several) 2D `vtkHyperTreeGrid`.
 * For that matter, this filter is limited to Axis-Aligned functions.
 *
 * Except for `vtkOverlapingAMR`, this filter can produce multiple slices at once.
 * In such cases, the output slices are stored in a `vtkPartitionedDataSetCollection`.
 *
 * In the case of a composite input, this filter will produce a composite output with the same
 * structure, except that each leaf (HyperTreeGrid or PartitionedDataSet containting HTG(s))
 * will be replaced by a new level of nodes containing slices (one node for each slice).
 * Output will always be a PartitionedDataSetCollection, even if the input is a MultiBlock.
 *
 * Please note that a composite input is considered valid only if it only contains HTGs or
 * Partitioned DataSets of HTGs. This do not concern Overlaping AMR since they can't be stored
 * in composite datasets.
 *
 * To recap, currently supported input types are:
 * - `vtkHyperTreeGrid`, output is a `vtkPartitionedDataSetCollection` (of `vtkHyperTreeGrid`)
 * - `vtkCompositeDataSet` of `vtkHyperTreeGrid`, output is a `vtkPartitionedDataSetCollection` of
 * `vtkHyperTreeGrid` (with one additional layer)
 * - `vtkOverlappingAMR`, output is a `vtkOverlappingAMR`
 *
 * Currently supported cutting function is Axis-Aligned `vtkPVPlane`.
 *
 * @sa
 * vtkCutter vtkPVCutter vtkPlaneCutter vtkPVPlaneCutter vtkPVMetaSliceDataSet
 * vtkHyperTreeGridAxisCut vtkAMRSliceFilter
 */

#ifndef vtkAxisAlignedPlaneCutter_h
#define vtkAxisAlignedPlaneCutter_h

#include "vtkAMRSliceFilter.h" // for vtkAMRSliceFilter
#include "vtkContourValues.h"  // for vtkContourValues
#include "vtkDataObjectAlgorithm.h"
#include "vtkHyperTreeGridAxisCut.h"                // for vtkHyperTreeGridAxisCut
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" // for export macro
#include "vtkSmartPointer.h"                        // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkDataAssembly;
class vtkImplicitFunction;
class vtkHyperTreeGridAxisCut;
class vtkMultiProcessController;
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;
class vtkPlane;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkAxisAlignedCutter : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkAxisAlignedCutter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAxisAlignedCutter* New();

  ///@{
  /**
   * Specify the implicit function to perform the cutting.
   * For now, only `Axis-Aligned` planes are supported.
   */
  void SetCutFunction(vtkImplicitFunction* function);
  vtkImplicitFunction* GetCutFunction();
  ///@}

  /**
   * Get the last modified time of this filter.
   * This time also depends on the the modified
   * time of the internal CutFunction instance.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/get a slice offset value at specified index i.
   * Negative indices are clamped to 0.
   * New space is allocated if necessary.
   */
  void SetOffsetValue(int i, double value);
  double GetOffsetValue(int i);
  ///@}

  ///@{
  /**
   * Set/get the number of slice offset values.
   * Allocate new space to store them if needed.
   */
  void SetNumberOfOffsetValues(int number);
  int GetNumberOfOffsetValues();
  ///@}

  ///@{
  /**
   * Sets the level of resolution.
   * Default is 0.
   *
   * Note: Only used for cutting overlapping AMR.
   */
  vtkSetMacro(LevelOfResolution, int);
  vtkGetMacro(LevelOfResolution, int);
  ///@}

  ///@{
  /**
   * Get/Set the controller in an MPI environment. If one sets the controller to `nullptr`,
   * an instance of `vtkDummyController` is stored instead. `GetController` never returns `nullptr`.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkMultiProcessController* GetController();
  ///@}

protected:
  vtkAxisAlignedCutter();
  ~vtkAxisAlignedCutter() override = default;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation* info) override;

private:
  vtkAxisAlignedCutter(const vtkAxisAlignedCutter&) = delete;
  void operator=(const vtkAxisAlignedCutter&) = delete;

  /**
   * Helper methods that perform slicing on input HTG with given plane and fill outputSlices
   */
  bool ProcessHTG(
    vtkHyperTreeGrid* inputHTG, vtkPlane* plane, vtkPartitionedDataSetCollection* outputSlices);

  /**
   * Helper methods that perform slicing on input PDS with given plane. Add produced slices to the
   * output PDC. Slices indices are added in a new layer of nodes in the output hierarchy at given
   * node ID.
   */
  bool ProcessPDS(vtkPartitionedDataSet* inputPDS, vtkPlane* plane,
    vtkPartitionedDataSetCollection* outputPDC, vtkDataAssembly* outputHierarchy, int nodeId);

  /**
   * Cut HTG with axis-aligned plane, applying additional plane offset if needed
   */
  void CutHTGWithAAPlane(
    vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, vtkPlane* plane, double offset);

  /**
   * Cut AMR with axis-aligned plane
   */
  void CutAMRWithAAPlane(vtkOverlappingAMR* input, vtkOverlappingAMR* output, vtkPlane* plane);

  vtkSmartPointer<vtkImplicitFunction> CutFunction;
  vtkNew<vtkHyperTreeGridAxisCut> HTGCutter;
  vtkNew<vtkAMRSliceFilter> AMRCutter;
  int LevelOfResolution = 0;
  vtkNew<vtkContourValues> OffsetValues;

  vtkSmartPointer<vtkMultiProcessController> Controller;
};

VTK_ABI_NAMESPACE_END

#endif
