// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPython.h" // must be the first thing that's included

#include "vtkPythonSelector.h"
#include "vtkPythonUtil.h"

#include "vtkCompositeDataSet.h"
#include "vtkFieldData.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPyObject.h"

#include <cassert>
#include <map>
#include <sstream>

class vtkPythonSelector::vtkInternals
{
  std::map<void*, vtkSmartPointer<vtkDataArray>> Map;

public:
  static constexpr const char* MASK_ARRAYNAME = "__vtkPythonSelector__internal__array__";
  void Reset() { this->Map.clear(); }
  void RegisterMaskArray(vtkDataObject* input, vtkDataObject* output, int association)
  {
    auto inDatasets = vtkCompositeDataSet::GetDataSets<vtkDataObject>(input);
    auto outDatasets = vtkCompositeDataSet::GetDataSets<vtkDataObject>(output);
    assert(inDatasets.size() == outDatasets.size());
    for (size_t cc = 0; cc < inDatasets.size(); ++cc)
    {
      auto in = inDatasets[cc];
      auto out = outDatasets[cc];
      if (in && out)
      {
        auto dsa = out->GetAttributesAsFieldData(association);
        if (auto array = dsa ? dsa->GetArray(MASK_ARRAYNAME) : nullptr)
        {
          this->Map[in] = array;
          dsa->RemoveArray(MASK_ARRAYNAME);
        }
      }
    }
  }

  vtkDataArray* GetMaskArray(vtkDataObject* input) const
  {
    auto iter = this->Map.find(input);
    return iter != this->Map.end() ? iter->second.GetPointer() : nullptr;
  }
};

vtkStandardNewMacro(vtkPythonSelector);
//----------------------------------------------------------------------------
vtkPythonSelector::vtkPythonSelector()
  : Internals(new vtkPythonSelector::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPythonSelector::~vtkPythonSelector()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkPythonSelector::Execute(vtkDataObject* input, vtkDataObject* output)
{
  assert(input != nullptr);
  assert(output != nullptr);
  assert(this->Node != nullptr);

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject psModule;
  psModule.TakeReference(PyImport_ImportModule("paraview.detail.python_selector"));
  if (!psModule)
  {
    vtkWarningMacro("Failed to import 'paraview.python_selector'");
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return;
    }
  }

  vtkSmartPyObject functionName(PyUnicode_FromString("execute"));
  vtkSmartPyObject inputObj(vtkPythonUtil::GetObjectFromPointer(input));
  vtkSmartPyObject nodeObj(vtkPythonUtil::GetObjectFromPointer(this->Node));
  vtkSmartPyObject arrayNameObj(PyUnicode_FromString(vtkInternals::MASK_ARRAYNAME));
  vtkSmartPyObject outputObj(vtkPythonUtil::GetObjectFromPointer(output));
  vtkSmartPyObject retVal(
    PyObject_CallMethodObjArgs(psModule, functionName.GetPointer(), inputObj.GetPointer(),
      nodeObj.GetPointer(), arrayNameObj.GetPointer(), outputObj.GetPointer(), nullptr));
  if (!retVal)
  {
    vtkWarningMacro("Could not invoke 'python_selector.execute()'");
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
  }
  else
  {
    auto& internals = (*this->Internals);
    const int association =
      vtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());
    internals.RegisterMaskArray(input, output, association);
    this->Superclass::Execute(input, output);
    internals.Reset();
  }
}

//----------------------------------------------------------------------------
bool vtkPythonSelector::ComputeSelectedElements(
  vtkDataObject* inputDO, vtkSignedCharArray* insidednessArray)
{
  assert(vtkCompositeDataSet::SafeDownCast(inputDO) == nullptr);
  assert(insidednessArray != nullptr);
  auto& internals = (*this->Internals);
  if (auto array = internals.GetMaskArray(inputDO))
  {
    insidednessArray->ShallowCopy(array);
    // restore name.
    insidednessArray->SetName(this->InsidednessArrayName.c_str());
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkPythonSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
