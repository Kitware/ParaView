/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBlockDeliveryStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBlockDeliveryStrategy
// .SECTION Description
// vtkSMBlockDeliveryStrategy is a vtkSMSimpleStrategy subclass which adjust the
// strategy to be more suitable for vtkSMBlockDeliveryRepresentationProxy.
// It merely changes the servers flag on the UpdateSuppressor proxies to
// DATA_SERVER only, since block delivery representation does not deliver the
// data to the client using this strategy.

#ifndef __vtkSMBlockDeliveryStrategy_h
#define __vtkSMBlockDeliveryStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMBlockDeliveryStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMBlockDeliveryStrategy* New();
  vtkTypeMacro(vtkSMBlockDeliveryStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMBlockDeliveryStrategy();
  ~vtkSMBlockDeliveryStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void BeginCreateVTKObjects();

private:
  vtkSMBlockDeliveryStrategy(const vtkSMBlockDeliveryStrategy&); // Not implemented
  void operator=(const vtkSMBlockDeliveryStrategy&); // Not implemented
//ETX
};

#endif

