/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonSelector.h"

#include "vtkCompositeDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkSelectionNode.h"

#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkPythonSelector);

//----------------------------------------------------------------------------
vtkPythonSelector::vtkPythonSelector()
{
}

//----------------------------------------------------------------------------
vtkPythonSelector::~vtkPythonSelector()
{
}

//----------------------------------------------------------------------------
bool vtkPythonSelector::ComputeSelectedElements(vtkDataObject* input, vtkDataObject* output)
{
  assert(input != nullptr);
  assert(output != nullptr);
  assert(this->Node != nullptr);

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();

  char addrOfInputDO[1024], addrOfNode[1024], addrOfOutputDO[1024];
  sprintf(addrOfInputDO, "%p", input);
  sprintf(addrOfNode, "%p", this->Node);
  sprintf(addrOfOutputDO, "%p", output);

  std::ostringstream stream;
  stream << "def vtkPythonSelector_ComputeSelectedElements():" << endl
         << "    from paraview import python_selector as ps" << endl
         << "    from paraview import vtk" << endl
         << "    inputDO = vtk.vtkDataObject('" << addrOfInputDO << "')" << endl
         << "    outputDO = vtk.vtkDataObject('" << addrOfOutputDO << "')" << endl
         << "    node = vtk.vtkSelectionNode('" << addrOfNode << "')" << endl
         << "    ps.execute(inputDO, node, '" << this->InsidednessArrayName << "', outputDO)"
         << endl
         << "    del outputDO" << endl
         << "    del node" << endl
         << "    del inputDO" << endl
         << "    del ps" << endl
         << "vtkPythonSelector_ComputeSelectedElements()" << endl
         << "del vtkPythonSelector_ComputeSelectedElements" << endl;
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());

  return true;
}

//----------------------------------------------------------------------------
void vtkPythonSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
