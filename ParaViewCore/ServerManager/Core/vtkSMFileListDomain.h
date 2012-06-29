/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMFileListDomain - list of filenames
// .SECTION Description
// vtkSMFileListDomain represents a domain consisting of a list of
// filenames. It only works with vtkSMStringVectorProperty. 
// Valid XML elements are:
// @verbatim
// * <String value="">
// @endverbatim
// .SECTION See Also
// vtkSMDomain vtkSMStringListDomain

#ifndef __vtkSMFileListDomain_h
#define __vtkSMFileListDomain_h

#include "vtkSMStringListDomain.h"

class VTK_EXPORT vtkSMFileListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMFileListDomain* New();
  vtkTypeMacro(vtkSMFileListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  // Returns 1 if the domain updated the property.
  virtual int SetDefaultValues(vtkSMProperty*);
protected:
  vtkSMFileListDomain();
  ~vtkSMFileListDomain();

private:
  vtkSMFileListDomain(const vtkSMFileListDomain&); // Not implemented
  void operator=(const vtkSMFileListDomain&); // Not implemented
};

#endif
