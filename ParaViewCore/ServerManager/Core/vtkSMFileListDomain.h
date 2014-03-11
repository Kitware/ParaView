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
#ifndef __vtkSMFileListDomain_h
#define __vtkSMFileListDomain_h

#include "vtkSMStringListDomain.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMFileListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMFileListDomain* New();
  vtkTypeMacro(vtkSMFileListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMFileListDomain();
  ~vtkSMFileListDomain();

private:
  vtkSMFileListDomain(const vtkSMFileListDomain&); // Not implemented
  void operator=(const vtkSMFileListDomain&); // Not implemented
};

#endif
