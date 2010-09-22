/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSequenceParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFileSequenceParser - keeps track of amount of memory consumed
// by caches in vtkPVUpateSupressor objects.
// .SECTION Description:
// vtkFileSequenceParser keeps track of the amount of memory cached
// by several vtkPVUpdateSuppressor objects.

#ifndef __vtkFileSequenceParser_h
#define __vtkFileSequenceParser_h

#include "vtkObject.h"

#include "vtkStdString.h" // For string support

namespace vtksys {
  class RegularExpression;
}


class VTK_EXPORT vtkFileSequenceParser : public vtkObject
{
public:
  static vtkFileSequenceParser* New();
  vtkTypeMacro(vtkFileSequenceParser, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  bool ParseFileSequence(vtkStdString file);

protected:
  vtkFileSequenceParser();
  ~vtkFileSequenceParser();

  vtksys::RegularExpression * reg_ex;
  vtksys::RegularExpression * reg_ex2;
  vtksys::RegularExpression * reg_ex3;
  vtksys::RegularExpression * reg_ex4;
  vtksys::RegularExpression * reg_ex5;
  vtksys::RegularExpression * reg_ex_last;

  vtkStdString SequenceName;
  int SequenceIndex;
private:
  vtkFileSequenceParser(const vtkFileSequenceParser&); // Not implemented.
  void operator=(const vtkFileSequenceParser&); // Not implemented.
};

#endif
