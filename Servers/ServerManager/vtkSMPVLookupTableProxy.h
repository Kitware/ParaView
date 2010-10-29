/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVLookupTableProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPVLookupTableProxy - proxy for 
// vtkDiscretizableColorTransferFunction.
// .SECTION Description
// vtkSMPVLookupTableProxy ensure that Build() is called on every 
// UpdateVTKObjects();

#ifndef __vtkSMPVLookupTableProxy_h
#define __vtkSMPVLookupTableProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMPVLookupTableProxy : public vtkSMProxy
{
public:
  static vtkSMPVLookupTableProxy* New();
  vtkTypeMacro(vtkSMPVLookupTableProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overriden to call Build() on the vtkDiscretizableColorTransferFunction.
  virtual void UpdateVTKObjects()
    { this->Superclass::UpdateVTKObjects(); }

protected:
  vtkSMPVLookupTableProxy();
  ~vtkSMPVLookupTableProxy();

  virtual void CreateVTKObjects();
  virtual void UpdateVTKObjects(vtkClientServerStream& stream);
private:
  vtkSMPVLookupTableProxy(const vtkSMPVLookupTableProxy&); // Not implemented.
  void operator=(const vtkSMPVLookupTableProxy&); // Not implemented.
};

#endif

