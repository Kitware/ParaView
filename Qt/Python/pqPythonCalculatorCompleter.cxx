// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include <QRegularExpression>

#include "pqPythonCalculatorCompleter.h"

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPythonInterpreter.h"

QStringList pqPythonCalculatorCompleter::getPythonCompletions(const QString& pythonObjectName)
{
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  // `object` contains all locals available in the `calculator.py` script
  PyObject* calc_module = PyImport_ImportModule("paraview.detail.calculator");
  if (calc_module == nullptr)
  {
    return QStringList{};
  }
  PyObject* object = PyModule_GetDict(calc_module);
  Py_INCREF(object);

  // Constant variable names, defined in calculator.py
  const QStringList hardCodedVariables{ "time_value", "t_value", "time_index", "t_index",
    "Points" };

  QStringList results;
  vtkPVDataInformation* inputInformation = this->Input->GetDataInformation(0);

  // Generate completions for "inputs[X].Y" expressions
  if (pythonObjectName.contains(QRegularExpression("^inputs\\[[0-9]+\\]$")))
  {
    results += hardCodedVariables;

    if (inputInformation->GetPointDataInformation()->GetNumberOfArrays() > 0)
      results << "PointData";
    if (inputInformation->GetCellDataInformation()->GetNumberOfArrays() > 0)
      results << "CellData";
    if (inputInformation->GetFieldDataInformation()->GetNumberOfArrays() > 0)
      results << "FieldData";
    if (inputInformation->GetVertexDataInformation()->GetNumberOfArrays() > 0)
      results << "VertexData";
    if (inputInformation->GetEdgeDataInformation()->GetNumberOfArrays() > 0)
      results << "EdgeData";
    if (inputInformation->GetRowDataInformation()->GetNumberOfArrays() > 0)
      results << "RowData";

    return results;
  }
  else
  {
    // Complete an object attribute : replace `object` containing all the locals
    // by the attributes of the object we're trying to complete.
    if (!pythonObjectName.isEmpty())
    {
      object = this->derivePyObject(pythonObjectName, object);
    }
    else
    {
      results += hardCodedVariables;
      results << "inputs";

      // Array names shortcuts are only available for the first calculator input
      this->AddArrayNamesToResults(inputInformation->GetPointDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetCellDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetFieldDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetVertexDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetEdgeDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetRowDataInformation(), results);
    }
  }

  this->appendPyObjectAttributes(object, results);
  return results;
}

//-----------------------------------------------------------------------------
void pqPythonCalculatorCompleter::AddArrayNamesToResults(
  const vtkPVDataSetAttributesInformation* attributesInformation, QStringList& results)
{
  if (!attributesInformation)
  {
    return;
  }

  for (int i = 0; i < attributesInformation->GetNumberOfArrays(); i++)
  {
    vtkPVArrayInformation* arrayInfo = attributesInformation->GetArrayInformation(i);

    for (auto& c : std::string(arrayInfo->GetName()))
    {
      if (c == ' ')
      {
        return;
      }
    }

    results << arrayInfo->GetName();
  }
}
