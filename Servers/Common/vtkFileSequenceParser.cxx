/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSequenceParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFileSequenceParser.h"

#include "vtkObjectFactory.h"

#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(vtkFileSequenceParser);
//-----------------------------------------------------------------------------
vtkFileSequenceParser::vtkFileSequenceParser() :
  // sequence ending with numbers.
  reg_ex( new vtksys::RegularExpression("^(.*)\\.([0-9.]+)$")),
  // sequence ending with extension.
  reg_ex2( new vtksys::RegularExpression("^(.*)(\\.|_|-)([0-9.]+)\\.(.*)$")),
  // sequence ending with extension, but with no ". or _" before
  // the series number.
  reg_ex3( new vtksys::RegularExpression("^(.*)([a-zA-Z])([0-9.]+)\\.(.*)$")),
  // sequence ending with extension, and starting with series number
  // followed by ". or _".
  reg_ex4( new vtksys::RegularExpression("^([0-9.]+)(\\.|_|-)(.*)\\.(.*)$")),
  // sequence ending with extension, and starting with series number,
  // but not followed by ". or _".
  reg_ex5( new vtksys::RegularExpression("^([0-9.]+)([a-zA-Z])(.*)\\.(.*)$")),
  // fallback: any sequence with a number in the middle (taking the last number
  // if multiple exist).
  reg_ex_last( new vtksys::RegularExpression("^(.*[^0-9])([0-9]+)([^0-9]+)$"))
{
}

//-----------------------------------------------------------------------------
vtkFileSequenceParser::~vtkFileSequenceParser()
{
  delete this->reg_ex;
  delete this->reg_ex2;
  delete this->reg_ex3;
  delete this->reg_ex4;
  delete this->reg_ex5;
  delete this->reg_ex_last;
}


//-----------------------------------------------------------------------------
bool vtkFileSequenceParser::ParseFileSequence(vtkStdString file)
{
  bool match = false;
  return match;
}

//-----------------------------------------------------------------------------
void vtkFileSequenceParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
