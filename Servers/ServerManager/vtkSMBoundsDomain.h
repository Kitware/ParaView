/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoundsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoundsDomain - double range domain based on data set bounds
// .SECTION Description
// vtkSMBoundsDomain is a subclass of vtkSMDoubleRangeDomain. In its Update
// method, it determines the minimum and maximum coordinates of each dimension
// of the bounding  box of the data set with which it is associated. It
// requires a vtkSMSourceProxy to do this.
// .SECTION See Also
// vtkSMDoubleRangeDomain

#ifndef __vtkSMBoundsDomain_h
#define __vtkSMBoundsDomain_h

#include "vtkSMDoubleRangeDomain.h"

class vtkSMProxyProperty;

class VTK_EXPORT vtkSMBoundsDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMBoundsDomain* New();
  vtkTypeRevisionMacro(vtkSMBoundsDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);

  // Description:
  vtkSetClampMacro(Mode, int, 0, 1);
  vtkGetMacro(Mode, int);

//BTX
  enum Modes
  {
    NORMAL,
    MAGNITUDE
  };
//ETX

protected:
  vtkSMBoundsDomain();
  ~vtkSMBoundsDomain();

  void Update(vtkSMProxyProperty *pp);

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);
  
  int Mode;

  virtual void SaveState(const char*, ostream*, vtkIndent) {}
private:
  vtkSMBoundsDomain(const vtkSMBoundsDomain&); // Not implemented
  void operator=(const vtkSMBoundsDomain&); // Not implemented
};

#endif
