/*=========================================================================

  program:   paraview
  module:    vtkpvthreshold.h

  copyright (c) kitware, inc.
  all rights reserved.
  see copyright.txt or http://www.paraview.org/html/copyright.html for details.

     this software is distributed without any warranty; without even
     the implied warranty of merchantability or fitness for a particular
     purpose.  see the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkpvthreshold
 * @brief   threshold filter
 *
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
  vtkTypeMacro(vtkPVThreshold, vtkThreshold);

  static vtkPVThreshold* New();

  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPVThreshold() = default;
  virtual ~vtkPVThreshold() override = default;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkPVThreshold(const vtkPVThreshold&) = delete;
  void operator=(const vtkPVThreshold&) = delete;
};

#endif
