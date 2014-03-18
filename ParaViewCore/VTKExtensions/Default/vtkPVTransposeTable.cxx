#include "vtkPVTransposeTable.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

#include <string>
#include <vector>

//----------------------------------------------------------------------------
// Internal class that holds selected columns
class PVTransposeTableInternal
{
public:
  bool Clear()
  {
    if (this->Columns.empty())
      {
      return false;
      }
    this->Columns.clear();
    return true;
  }

  bool Has(const std::string& v)
  {
    std::size_t len = this->Columns.size();
    for (std::size_t i = 0; i < len; i++)
      {
      if (this->Columns[i] == v)
        {
        return true;
        }
      }
    return false;
  }

  bool Set(const std::string& v)
  {
  if (this->Has(v))
    {
    return false;
    }
  this->Columns.push_back(v);
  return true;
  }

  std::vector<std::string> Columns;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTransposeTable);

//----------------------------------------------------------------------------
vtkPVTransposeTable::vtkPVTransposeTable()
{
  this->Internal = new PVTransposeTableInternal();
  this->DoNotTranspose = false;
}

//----------------------------------------------------------------------------
vtkPVTransposeTable::~vtkPVTransposeTable()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVTransposeTable::EnableAttributeArray(const char* arrName)
{
  if (arrName)
    {
    if (this->Internal->Set(arrName))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVTransposeTable::ClearAttributeArrays()
{
  if (this->Internal->Clear())
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVTransposeTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVTransposeTable::RequestData(vtkInformation*,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  vtkTable* inTable = vtkTable::GetData(inputVector[0]);
  vtkTable* outTable = vtkTable::GetData(outputVector, 0);

  if (inTable->GetNumberOfColumns() == 0)
    {
    return 1;
    }

  // Construct a table that holds only the selected columns
  vtkNew<vtkTable> subTable;
  std::vector<std::string>::iterator iter = this->Internal->Columns.begin();
  if (this->GetUseIdColumn())
    {
    if (vtkAbstractArray* arr = inTable->GetColumnByName(this->GetIdColumnName()))
      {
      subTable->AddColumn(arr);
      }
    }
  for (; iter!=this->Internal->Columns.end(); ++iter)
    {
    if (!strcmp(iter->c_str(), this->GetIdColumnName()))
      {
      continue;
      }
    if (vtkAbstractArray* arr = inTable->GetColumnByName(iter->c_str()))
      {
      subTable->AddColumn(arr);
      }
    }

  // Now transpose the subtable
  if (subTable->GetNumberOfColumns() > 0 && !this->DoNotTranspose)
    {
    vtkNew<vtkTransposeTable> transpose;
    transpose->SetInputData(subTable.GetPointer());
    transpose->SetAddIdColumn(this->GetAddIdColumn());
    transpose->SetIdColumnName(this->GetIdColumnName());
    transpose->SetUseIdColumn(this->GetUseIdColumn());
    transpose->Update();

    outTable->ShallowCopy(transpose->GetOutput());
    }
  if (this->DoNotTranspose)
    {
    outTable->ShallowCopy(subTable.GetPointer());
    }

  return 1;
}
