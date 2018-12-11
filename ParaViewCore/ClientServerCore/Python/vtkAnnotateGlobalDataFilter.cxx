/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateGlobalDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnnotateGlobalDataFilter.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"

#include <sstream>
#include <string>
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
static std::string vtkGetReferenceAsString(void* ref)
{
  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", ref);
  char* aplus = addrofthis;
  if ((addrofthis[0] == '0') && ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }
  return std::string(aplus);
}

vtkStandardNewMacro(vtkAnnotateGlobalDataFilter);
//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::vtkAnnotateGlobalDataFilter()
{
  this->Prefix = 0;
  this->Postfix = 0;
  this->FieldArrayName = 0;
  this->Format = 0;
  this->SetFormat("%7.5g");
  this->SetArrayAssociation(vtkDataObject::FIELD);
}

//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::~vtkAnnotateGlobalDataFilter()
{
  this->SetPrefix(0);
  this->SetPostfix(0);
  this->SetFieldArrayName(0);
}

//----------------------------------------------------------------------------
void vtkAnnotateGlobalDataFilter::EvaluateExpression()
{
  std::ostringstream stream;
  stream << "def vtkAnnotateGlobalDataFilter_EvaluateExpression():" << endl
         << "    from paraview import annotation as pv_ann" << endl
         << "    from paraview.vtk.vtkPVClientServerCoreDefault import vtkAnnotateGlobalDataFilter"
         << endl
         << "    me = vtkAnnotateGlobalDataFilter('" << vtkGetReferenceAsString(this) << " ')"
         << endl
         << "    pv_ann.execute_on_global_data(me)" << endl
         << "    del me" << endl
         << "vtkAnnotateGlobalDataFilter_EvaluateExpression()" << endl
         << "del vtkAnnotateGlobalDataFilter_EvaluateExpression" << endl;

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
}

//----------------------------------------------------------------------------
void vtkAnnotateGlobalDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldArrayName: " << (this->FieldArrayName ? this->FieldArrayName : "(none)")
     << endl;
  os << indent << "Prefix: " << (this->Prefix ? this->Prefix : "(none)") << endl;
  os << indent << "Postfix: " << (this->Postfix ? this->Postfix : "(none)") << endl;
}
