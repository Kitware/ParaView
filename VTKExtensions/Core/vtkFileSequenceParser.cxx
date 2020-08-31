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

#include <set>
#include <string>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkFileSequenceParser);
//-----------------------------------------------------------------------------
vtkFileSequenceParser::vtkFileSequenceParser()
  : // sequence ending with numbers.
  reg_ex(new vtksys::RegularExpression("^(.*)\\.([0-9.]+)$"))
  ,
  // sequence ending with extension.
  reg_ex2(new vtksys::RegularExpression("^(.*)(\\.|_|-)([0-9.]+)\\.(.*)$"))
  ,
  // sequence ending with extension, but with no ". or _" before
  // the series number.
  reg_ex3(new vtksys::RegularExpression("^(.*)([a-zA-Z])([0-9.]+)\\.(.*)$"))
  ,
  // sequence ending with extension, and starting with series number
  // followed by ". or _".
  reg_ex4(new vtksys::RegularExpression("^([0-9.]+)(\\.|_|-)(.*)\\.(.*)$"))
  ,
  // sequence ending with extension, and starting with series number,
  // but not followed by ". or _".
  reg_ex5(new vtksys::RegularExpression("^([0-9.]+)([a-zA-Z])(.*)\\.(.*)$"))

  ,
  // fallback: any sequence with a number in the middle (taking the last number
  // if multiple exist).
  reg_ex_last(new vtksys::RegularExpression("^(.*[^0-9])([0-9]+)([^0-9]*)$"))
  , SequenceIndex(-1)
  , SequenceName(NULL)
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

  this->SetSequenceName(NULL);
}

//-----------------------------------------------------------------------------
bool vtkFileSequenceParser::ParseFileSequence(const char* file)
{
  bool match = false;
  if (this->reg_ex->find(file))
  {
    this->SetSequenceName(this->reg_ex->match(1).c_str());
    this->SequenceIndexString = this->reg_ex->match(2);
    match = true;
  }
  else if (this->reg_ex2->find(file))
  {
    this->SetSequenceName(std::string(this->reg_ex2->match(1) + this->reg_ex2->match(2) + ".." +
                            this->reg_ex2->match(4))
                            .c_str());
    this->SequenceIndexString = this->reg_ex2->match(3);
    match = true;
  }
  else if (this->reg_ex3->find(file))
  {
    this->SetSequenceName(std::string(this->reg_ex3->match(1) + this->reg_ex3->match(2) + ".." +
                            this->reg_ex3->match(4))
                            .c_str());
    this->SequenceIndexString = this->reg_ex3->match(3);
    match = true;
  }
  else if (this->reg_ex4->find(file))
  {
    this->SetSequenceName(std::string(".." + this->reg_ex4->match(2) + this->reg_ex4->match(3) +
                            "." + this->reg_ex4->match(4))
                            .c_str());
    this->SequenceIndexString = this->reg_ex4->match(1);
    match = true;
  }
  else if (this->reg_ex5->find(file))
  {
    this->SetSequenceName(std::string(".." + this->reg_ex5->match(2) + this->reg_ex5->match(3) +
                            "." + this->reg_ex5->match(4))
                            .c_str());
    this->SequenceIndexString = this->reg_ex5->match(1);
    match = true;
  }
  else
  {
    std::string fname_wo_ext = vtksys::SystemTools::GetFilenameWithoutExtension(file);
    std::string ext = vtksys::SystemTools::GetFilenameExtension(file);
    if (this->reg_ex_last->find(fname_wo_ext))
    {
      this->SetSequenceName(
        (this->reg_ex_last->match(1) + ".." + this->reg_ex_last->match(3) + ext).c_str());
      this->SequenceIndexString = this->reg_ex_last->match(2);
      match = true;
    }
  }
  if (match)
  {
    this->SequenceIndex = atoi(this->SequenceIndexString.c_str());
  }
  return match;
}

//-----------------------------------------------------------------------------
void vtkFileSequenceParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
