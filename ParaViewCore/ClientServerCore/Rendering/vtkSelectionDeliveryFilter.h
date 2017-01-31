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
/**
 * @class   vtkSelectionDeliveryFilter
 *
 * vtkSelectionDeliveryFilter is a filter that can deliver vtkSelection from
 * data-server nodes to the client. This should not be instantiated on the
 * pure-render-server nodes to avoid odd side effects (We can fix this later if
 * the need arises).
*/

#ifndef vtkSelectionDeliveryFilter_h
#define vtkSelectionDeliveryFilter_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSelectionAlgorithm.h"

class vtkClientServerMoveData;
class vtkReductionFilter;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkSelectionDeliveryFilter
  : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionDeliveryFilter* New();
  vtkTypeMacro(vtkSelectionDeliveryFilter, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSelectionDeliveryFilter();
  ~vtkSelectionDeliveryFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;

private:
  vtkSelectionDeliveryFilter(const vtkSelectionDeliveryFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSelectionDeliveryFilter&) VTK_DELETE_FUNCTION;
};

#endif
