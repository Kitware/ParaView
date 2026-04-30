// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVClipDataSet
 * @brief   Clip filter
 *
 *
 * This is a subclass of vtkTableBasedClipDataSet that allows selection of input
 * scalars.
 */

#ifndef vtkPVClipDataSet_h
#define vtkPVClipDataSet_h

#include "vtkNew.h"                                 // for vtkNew
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkTableBasedClipDataSet.h"

#include <memory> // for unique_ptr

class vtkEdgesCacheInternal;
class vtkDataObjectMeshCache;
class vtkDataObjectTree;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVClipDataSet : public vtkTableBasedClipDataSet
{
public:
  vtkTypeMacro(vtkPVClipDataSet, vtkTableBasedClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVClipDataSet* New();

  ///@{
  /**
   * This filter uses vtkAMRDualClip for clipping AMR datasets. To disable that
   * behavior, turn this flag off.
   */
  vtkSetMacro(UseAMRDualClipForAMR, bool);
  vtkGetMacro(UseAMRDualClipForAMR, bool);
  vtkBooleanMacro(UseAMRDualClipForAMR, bool);
  ///@}

  ///@{
  /**
   * For a vtkPVBlox implicit function we can do an exact clip of the exterior portion of the box.
   */
  vtkSetMacro(ExactBoxClip, bool);
  vtkGetMacro(ExactBoxClip, bool);
  vtkBooleanMacro(ExactBoxClip, bool);
  ///@}

protected:
  vtkPVClipDataSet(vtkImplicitFunction* cf = nullptr);
  ~vtkPVClipDataSet() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation* info) override;
  int FillOutputPortInformation(int, vtkInformation* info) override;

  /**
   * Here we instantiate a new filter instead of simply use polymorphism and call the parent
   * RequestData to benefit from the override mechanism (like Viskores variant).
   *
   * @see vtkObjectFactory and  vtkObjectFactoryNewMacro.
   */
  int ClipUsingSuperclass(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * Delegate process to vtkPVThreshold.
   */
  int ClipUsingThreshold(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  bool UseAMRDualClipForAMR;
  bool ExactBoxClip;

private:
  vtkPVClipDataSet(const vtkPVClipDataSet&) = delete;
  void operator=(const vtkPVClipDataSet&) = delete;

  bool ClipDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  bool ClipLeaf(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  void InitializeOutput(vtkDataObjectTree* input, vtkDataObjectTree* output);

  std::unique_ptr<vtkEdgesCacheInternal> EdgesCache;
  vtkNew<vtkDataObjectMeshCache> MeshCache;
};

#endif
