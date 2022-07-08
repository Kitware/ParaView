/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlaneCutter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVPlaneCutter
 * @brief   Slice with plane filter
 *
 * This is a subclass of vtkPlaneCutter that allows selection of input vtkHyperTreeGrid,
 * vtkOverlappingAMR.
 */

#ifndef vtkPVPlaneCutter_h
#define vtkPVPlaneCutter_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkPlaneCutter.h"

class vtkAMRCutPlane;
class vtkAMRSliceFilter;
class vtkHyperTreeGridAxisCut;
class vtkHyperTreeGridPlaneCutter;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVPlaneCutter : public vtkPlaneCutter
{
public:
  vtkTypeMacro(vtkPVPlaneCutter, vtkPlaneCutter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVPlaneCutter* New();

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@
  /**
   * If set to true, the dual grid is used for cutting.
   *
   * Default is false.
   *
   * Note: Only used for cutting hyper tree grids.
   */
  vtkGetMacro(Dual, bool);
  vtkSetMacro(Dual, bool);
  //@}

  ///@{
  /**
   * Sets the level of resolution
   *
   * Default is 0.
   *
   * Note: Only used for cutting overlapping AMR.
   */
  vtkSetMacro(LevelOfResolution, int);
  vtkGetMacro(LevelOfResolution, int);
  ///@}

  ///@{
  /**
   * Sets if plane cutter is used instead of the specialized AMR cutter.
   *
   * Default is true.
   *
   * Note: Only used for cutting overlapping AMR.
   */
  vtkSetMacro(UseNativeCutter, bool);
  vtkGetMacro(UseNativeCutter, bool);
  vtkBooleanMacro(UseNativeCutter, bool);
  ///@}

protected:
  vtkPVPlaneCutter();
  ~vtkPVPlaneCutter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation* info) override;
  int FillOutputPortInformation(int, vtkInformation* info) override;

  bool Dual = false;
  int LevelOfResolution = 0;
  bool UseNativeCutter = true;

  vtkNew<vtkAMRCutPlane> AMRPlaneCutter;
  vtkNew<vtkAMRSliceFilter> AMRAxisAlignedPlaneCutter;
  vtkNew<vtkHyperTreeGridPlaneCutter> HTGPlaneCutter;
  vtkNew<vtkHyperTreeGridAxisCut> HTGAxisAlignedPlaneCutter;

private:
  vtkPVPlaneCutter(const vtkPVPlaneCutter&) = delete;
  void operator=(const vtkPVPlaneCutter&) = delete;
};

#endif
