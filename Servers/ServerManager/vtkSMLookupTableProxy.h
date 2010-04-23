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
// to add functionality.

#ifndef __vtkSMLookupTableProxy_h
#define __vtkSMLookupTableProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMLookupTableProxy : public vtkSMProxy
{
public:
  static vtkSMLookupTableProxy* New();
  vtkTypeMacro(vtkSMLookupTableProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the lookup table values.
  void Build();

  // Description:
  // push properties to VTK object.
  // Also call Build(), hence rebuilds the lookup table.
  virtual void UpdateVTKObjects()
    { this->Superclass::UpdateVTKObjects(); }

  // Description:
  // This map is used for arrays with this name 
  // and this number of components.  In the future, they may
  // handle more than one type of array.
  // 
  // This used to be in the ScalarBarWidget. However,
  // since it is used almost everytime Lookup table is needed,
  // I put ArrayName as an Ivar in this class. 
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
 
  // Description:
  // Set whether to use a separate color for values outside the lookup table's
  // range.
  vtkSetMacro(UseLowOutOfRangeColor, int);
  vtkGetMacro(UseLowOutOfRangeColor, int);
  vtkBooleanMacro(UseLowOutOfRangeColor, int);
  vtkSetMacro(UseHighOutOfRangeColor, int);
  vtkGetMacro(UseHighOutOfRangeColor, int);
  vtkBooleanMacro(UseHighOutOfRangeColor, int);

  // Description:
  // Set the colors to use for values outside the range of the lookup table.
  vtkSetVector3Macro(LowOutOfRangeColor, double);
  vtkGetVector3Macro(LowOutOfRangeColor, double);
  vtkSetVector3Macro(HighOutOfRangeColor, double);
  vtkGetVector3Macro(HighOutOfRangeColor, double);

protected:
  vtkSMLookupTableProxy();
  ~vtkSMLookupTableProxy();

  // Description:
  // push properties to VTK object.
  // Also call Build(), hence rebuilds the lookup table.
  virtual void UpdateVTKObjects(vtkClientServerStream& stream);

  // This method is overridden to change the servers.
  virtual void CreateVTKObjects();

  char* ArrayName;

  int UseLowOutOfRangeColor;
  int UseHighOutOfRangeColor;
  double LowOutOfRangeColor[3];
  double HighOutOfRangeColor[3];

private:
  vtkSMLookupTableProxy(const vtkSMLookupTableProxy&); // Not implemented
  void operator=(const vtkSMLookupTableProxy&); // Not implemented
};

#endif
