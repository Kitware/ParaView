/*=========================================================================

  program: ParaView
  module:  vtkPVGhostCellsGenerator.h

  copyright (c) kitware, inc.
  all rights reserved.
  see copyright.txt or http://www.paraview.org/html/copyright.html for details.

     this software is distributed without any warranty; without even
     the implied warranty of merchantability or fitness for a particular
     purpose.  see the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVGhostCellsGenerator
 * @brief Ghost Cells Generator that adds support for vtkHyperTreeGrid.
 *
 * Meta class that switches between ghost cells generator filters to
 * select the right one depending on the input type.
 */

#ifndef vtkPVGhostCellsGenerator_h
#define vtkPVGhostCellsGenerator_h

#include "vtkPVDataSetAlgorithmSelectorFilter.h"
#include "vtkPVVTKExtensionsFiltersParallelDIY2Module.h" //needed for exports

#include <memory> // For unique_ptr

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSFILTERSPARALLELDIY2_EXPORT vtkPVGhostCellsGenerator
  : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  static vtkPVGhostCellsGenerator* New();
  vtkTypeMacro(vtkPVGhostCellsGenerator, vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   * Does not apply to Hyper Tree Grid.
   */
  void SetController(vtkMultiProcessController* controller);

  /**
   * Specify if the filter must generate the ghost cells only if required by
   * the pipeline.
   * If false, ghost cells are computed even if they are not required.
   * Does not apply to Hyper Tree Grid.
   * Default is TRUE.
   */
  void SetBuildIfRequired(bool enable);

  /**
   * When BuildIfRequired is `false`, this can be used to set the number
   * of ghost layers to generate. Note, if the downstream pipeline requests more
   * ghost levels than the number specified here, then the filter will generate
   * those extra ghost levels as needed. Accepted values are in the interval
   * [1, VTK_INT_MAX].
   * Does not apply to Hyper Tree Grid.
   * Default is 1.
   */
  void SetNumberOfGhostLayers(int nbGhostLayers);

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPVGhostCellsGenerator();
  ~vtkPVGhostCellsGenerator() override;

private:
  vtkPVGhostCellsGenerator(const vtkPVGhostCellsGenerator&) = delete;
  void operator=(const vtkPVGhostCellsGenerator&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
