/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractHistogram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPExtractHistogram
 * @brief   Extract histogram for parallel dataset.
 *
 * vtkPExtractHistogram is vtkExtractHistogram subclass for parallel datasets.
 * It gathers the histogram data on the root node.
*/

#ifndef vtkPExtractHistogram_h
#define vtkPExtractHistogram_h

#include "vtkExtractHistogram.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPExtractHistogram : public vtkExtractHistogram
{
public:
  static vtkPExtractHistogram* New();
  vtkTypeMacro(vtkPExtractHistogram, vtkExtractHistogram);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPExtractHistogram();
  ~vtkPExtractHistogram() override;

  /**
   * Returns the data range for the input array to process.
   * Overridden to reduce the range in parallel.
   */
  bool GetInputArrayRange(vtkInformationVector** inputVector, double range[2]) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkMultiProcessController* Controller;

private:
  vtkPExtractHistogram(const vtkPExtractHistogram&) = delete;
  void operator=(const vtkPExtractHistogram&) = delete;
};

#endif
