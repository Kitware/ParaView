/*=========================================================================

  Program:   ParaView
  Module:    vtkCompleteArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompleteArrays
 * @brief   Filter adds arrays to empty partitions.
 *
 * This is a temporary solution for fixing a writer bug.  When partition 0
 * has no cells or points, it does not have arrays either.  The writers
 * get confused.  This filter creates empty arrays on node zero if there
 * are no cells or points in that partition.
 * This filter can also handle vtkTable by letting them pass through it
 * without modification.
*/

#ifndef vtkCompleteArrays_h
#define vtkCompleteArrays_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkCompleteArrays : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkCompleteArrays, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCompleteArrays* New();

  //@{
  /**
   * The user can set the controller used for inter-process communication.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkCompleteArrays();
  ~vtkCompleteArrays() override;

  /**
   * Set the input type of the algorithm to vtkDataSet and vtkTable.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Set the output type of the algorithm to vtkDataObject.
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Generate an output of the same type as the input.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  vtkMultiProcessController* Controller;

private:
  vtkCompleteArrays(const vtkCompleteArrays&) = delete;
  void operator=(const vtkCompleteArrays&) = delete;
};

#endif
