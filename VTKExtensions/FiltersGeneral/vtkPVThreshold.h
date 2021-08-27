/*=========================================================================

  program: ParaView
  module:  vtkPVThreshold.h

  copyright (c) kitware, inc.
  all rights reserved.
  see copyright.txt or http://www.paraview.org/html/copyright.html for details.

     this software is distributed without any warranty; without even
     the implied warranty of merchantability or fitness for a particular
     purpose.  see the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVThreshold
 * @brief threshold filter to add support for vtkHyperTreeGrid.
 *
 * This is a subclass of vtkThreshold that allows to apply threshold filters
 * to either vtkDataSet or vtkHyperTreeGrid.
 */

#ifndef vtkPVThreshold_h
#define vtkPVThreshold_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkThreshold.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVThreshold : public vtkThreshold
{
public:
  static vtkPVThreshold* New();
  vtkTypeMacro(vtkPVThreshold, vtkThreshold);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPVThreshold() = default;
  ~vtkPVThreshold() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  vtkPVThreshold(const vtkPVThreshold&) = delete;
  void operator=(const vtkPVThreshold&) = delete;
};

#endif
