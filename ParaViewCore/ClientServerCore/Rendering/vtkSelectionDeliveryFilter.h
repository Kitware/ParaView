/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionDeliveryFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionDeliveryFilter
// .SECTION Description
// vtkSelectionDeliveryFilter is a filter that can deliver vtkSelection from
// data-server nodes to the client. This should not be instantiated on the
// pure-render-server nodes to avoid odd side effects (We can fix this later if
// the need arises).

#ifndef __vtkSelectionDeliveryFilter_h
#define __vtkSelectionDeliveryFilter_h

#include "vtkSelectionAlgorithm.h"

class vtkClientServerMoveData;
class vtkReductionFilter;

class VTK_EXPORT vtkSelectionDeliveryFilter : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionDeliveryFilter* New();
  vtkTypeMacro(vtkSelectionDeliveryFilter, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSelectionDeliveryFilter();
  ~vtkSelectionDeliveryFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;

private:
  vtkSelectionDeliveryFilter(const vtkSelectionDeliveryFilter&); // Not implemented
  void operator=(const vtkSelectionDeliveryFilter&); // Not implemented
//ETX
};

#endif
