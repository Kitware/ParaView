/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLookupTableProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLookupTableProxy - proxy for a VTK lookup table
// .SECTION Description
// This proxy class is an example of how vtkSMProxy can be subclassed
// to add functionality. It adds one simple method : Build().

#ifndef __vtkSMLookupTableProxy_h
#define __vtkSMLookupTableProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMLookupTableProxy : public vtkSMProxy
{
public:
  static vtkSMLookupTableProxy* New();
  vtkTypeRevisionMacro(vtkSMLookupTableProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the lookup table values.
  void Build();

  // Description:
  // push properties to VTK object.
  // Also call Build(), hence rebuilds the lookup table.
  virtual void UpdateVTKObjects();

  // Description:
  // Given the number of objects (numObjects), class name 
  // (VTKClassName) and server ids ( this->GetServerIDs()), 
  // this methods instantiates the objects on the server(s)
  // This method is overridden to change the servers.
  virtual void CreateVTKObjects(int numObjects);
  
  // Description:
  // This map is used for arrays with this name 
  // and this number of components.  In the future, they may
  // handle more than one type of array.
  // 
  // This used to be in the ScalarBarWidget. However,
  // since it is used almost everytime Lookup table is needed,
  // I put ArrayName as an Ivar in this class. 
  virtual void SetArrayName(const char* name);
  vtkGetStringMacro(ArrayName);
 
  virtual void SaveInBatchScript(ofstream* file);
protected:
  vtkSMLookupTableProxy();
  ~vtkSMLookupTableProxy();

  void LabToXYZ(double Lab[3], double xyz[3]);
  void XYZToRGB(double xyz[3], double rgb[3]);

  char* ArrayName;

private:
  vtkSMLookupTableProxy(const vtkSMLookupTableProxy&); // Not implemented
  void operator=(const vtkSMLookupTableProxy&); // Not implemented
};

#endif
