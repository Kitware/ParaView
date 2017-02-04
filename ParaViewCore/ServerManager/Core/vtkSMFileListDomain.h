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
/**
 * @class   vtkSMFileListDomain
 * @brief   list of filenames
 *
 * vtkSMFileListDomain represents a domain consisting of a list of
 * filenames. It only works with vtkSMStringVectorProperty.
*/

#ifndef vtkSMFileListDomain_h
#define vtkSMFileListDomain_h

#include "vtkSMStringListDomain.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMFileListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMFileListDomain* New();
  vtkTypeMacro(vtkSMFileListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSMFileListDomain();
  ~vtkSMFileListDomain();

private:
  vtkSMFileListDomain(const vtkSMFileListDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMFileListDomain&) VTK_DELETE_FUNCTION;
};

#endif
