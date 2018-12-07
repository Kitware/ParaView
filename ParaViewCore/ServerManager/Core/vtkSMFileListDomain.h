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
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMFileListDomain();
  ~vtkSMFileListDomain() override;

private:
  vtkSMFileListDomain(const vtkSMFileListDomain&) = delete;
  void operator=(const vtkSMFileListDomain&) = delete;
};

#endif
