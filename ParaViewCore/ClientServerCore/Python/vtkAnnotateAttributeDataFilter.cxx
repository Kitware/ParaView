/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateAttributeDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnnotateAttributeDataFilter.h"

#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
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

vtkStandardNewMacro(vtkAnnotateAttributeDataFilter);
//----------------------------------------------------------------------------
vtkAnnotateAttributeDataFilter::vtkAnnotateAttributeDataFilter()
{
  this->ArrayName = NULL;
  this->Prefix = NULL;
  this->ElementId = 0;
  this->ProcessId = 0;
  this->SetArrayAssociation(vtkDataObject::ROW);
}

//----------------------------------------------------------------------------
vtkAnnotateAttributeDataFilter::~vtkAnnotateAttributeDataFilter()
{
  this->SetArrayName(NULL);
  this->SetPrefix(NULL);
}

//----------------------------------------------------------------------------
void vtkAnnotateAttributeDataFilter::EvaluateExpression()
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  bool evaluate_locally = (controller == NULL) ||
    (controller->GetLocalProcessId() == this->ProcessId) || (this->ProcessId == -1);

  std::ostringstream stream;
  stream
    << "def vtkAnnotateAttributeDataFilter_EvaluateExpression():" << endl
    << "    from paraview import annotation as pv_ann" << endl
    << "    from paraview.vtk.vtkPVClientServerCoreDefault import vtkAnnotateAttributeDataFilter"
    << endl
    << "    me = vtkAnnotateAttributeDataFilter('" << vtkGetReferenceAsString(this) << " ')" << endl
    << "    pv_ann.execute_on_attribute_data(me," << (evaluate_locally ? "True" : "False") << ")"
    << endl
    << "    del me" << endl
    << "vtkAnnotateAttributeDataFilter_EvaluateExpression()" << endl
    << "del vtkAnnotateAttributeDataFilter_EvaluateExpression" << endl;

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());

  if (controller && controller->GetNumberOfProcesses() > 1 && this->ProcessId != -1)
  {
    vtkMultiProcessStream stream2;
    if (this->ProcessId == controller->GetLocalProcessId())
    {
      stream2 << (this->GetComputedAnnotationValue() ? this->GetComputedAnnotationValue() : "");
    }
    controller->Broadcast(stream2, this->ProcessId);
    std::string val;
    stream2 >> val;
    this->SetComputedAnnotationValue(val.c_str());
  }
}

//----------------------------------------------------------------------------
void vtkAnnotateAttributeDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(none)") << endl;
  os << indent << "Prefix: " << (this->Prefix ? this->Prefix : "(none)") << endl;
  os << indent << "ElementId: " << this->ElementId << endl;
  os << indent << "ProcessId: " << this->ProcessId << endl;
}
