/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVExporter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCSVExporter.h"

#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariant.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <map>
#include <sstream>
#include <string>

class vtkCSVExporter::vtkInternals
{
public:
  std::string Header;
  std::string XColumnName;
  std::map<double, std::pair<std::string, int> > Lines;
  int ColumnCount;

  //@{
  // Used in STREAM_ROWS mode.
  std::map<std::string, std::string> ColumnLabels;
  //@}

  vtkInternals()
    : ColumnCount(0)
  {
  }

  void AddColumnValue(const char* delim, double xval, const std::string& value)
  {
    std::pair<std::string, int>& item = this->Lines[xval];
    std::ostringstream stream;
    stream << item.first.c_str();
    while (item.second < this->ColumnCount)
    {
      stream << delim;
      item.second++;
    }
    stream << value.c_str();
    item.first = stream.str();
  }
  void DumpLines(const char* delim, ostream& ofs)
  {
    ofs << (this->XColumnName.empty() ? "X" : this->XColumnName.c_str()) << delim
        << this->Header.c_str() << endl;
    for (std::map<double, std::pair<std::string, int> >::iterator iter = this->Lines.begin();
         iter != this->Lines.end(); ++iter)
    {
      ofs << iter->first << delim << iter->second.first.c_str();
      for (int cc = iter->second.second; cc < (this->ColumnCount - 1); cc++)
      {
        ofs << delim;
      }
      ofs << endl;
    }
  }
};

vtkStandardNewMacro(vtkCSVExporter);
//----------------------------------------------------------------------------
vtkCSVExporter::vtkCSVExporter()
{
  this->SetFieldDelimiter(",");
  this->Internals = new vtkInternals();
  this->Formatting = vtkVariant::DEFAULT_FORMATTING;
}

//----------------------------------------------------------------------------
vtkCSVExporter::~vtkCSVExporter()
{
  if (this->OutputStream)
  {
    this->Close();
  }
  delete this->Internals;
  this->Internals = nullptr;
  delete this->OutputStream;
  this->OutputStream = nullptr;
  this->SetFieldDelimiter(nullptr);
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkCSVExporter::SetColumnLabel(const char* name, const char* label)
{
  if (name != nullptr)
  {
    auto& internals = *this->Internals;
    internals.ColumnLabels[name] = label ? label : "";
  }
}

//----------------------------------------------------------------------------
void vtkCSVExporter::ClearColumnLabels()
{
  auto& internals = *this->Internals;
  internals.ColumnLabels.clear();
}

//----------------------------------------------------------------------------
const char* vtkCSVExporter::GetColumnLabel(const char* name)
{
  if (name != nullptr)
  {
    const auto& internals = *this->Internals;
    auto iter = internals.ColumnLabels.find(name);
    if (iter != internals.ColumnLabels.end())
    {
      return !iter->second.empty() ? iter->second.c_str() : nullptr;
    }
  }
  return name;
}

//----------------------------------------------------------------------------
bool vtkCSVExporter::Open(vtkCSVExporter::ExporterModes mode)
{
  delete this->OutputStream;
  this->OutputString.clear();

  if (this->WriteToOutputString)
  {
    this->OutputStream = new std::ostringstream();
  }
  else
  {
    this->OutputStream = new vtksys::ofstream(this->FileName);
  }

  if (!this->OutputStream || !(*this->OutputStream))
  {
    vtkErrorMacro("Failed to open for writing: " << this->FileName);
    delete this->OutputStream;
    this->OutputStream = nullptr;
    return false;
  }
  this->Mode = mode;
  return true;
}

//----------------------------------------------------------------------------
std::string vtkCSVExporter::GetOutputString()
{
  return this->OutputString;
}

//----------------------------------------------------------------------------
void vtkCSVExporter::AddColumn(
  vtkAbstractArray* yarray, const char* yarrayname /*=nullptr*/, vtkDataArray* xarray /*=nullptr*/)
{
  if (this->Mode != STREAM_COLUMNS)
  {
    vtkErrorMacro("Incorrect exporter mode. 'OpenFile' must be called with "
                  "'STREAM_COLUMNS' to use 'AddColumn' API.");
    return;
  }

  yarrayname = yarrayname ? yarrayname : yarray->GetName();
  this->Internals->Header += (this->Internals->ColumnCount > 0) ? "," : "";
  this->Internals->Header += "\"" + std::string(yarrayname) + "\"";

  assert(xarray == nullptr || (xarray->GetNumberOfTuples() == yarray->GetNumberOfTuples()));
  if (xarray && xarray->GetName() && this->Internals->XColumnName.empty())
  {
    this->Internals->XColumnName = "\"" + std::string(xarray->GetName()) + "\"";
  }
  for (vtkIdType cc = 0, max = yarray->GetNumberOfTuples(); cc < max; ++cc)
  {
    this->Internals->AddColumnValue(this->FieldDelimiter,
      xarray ? xarray->GetTuple1(cc) : static_cast<double>(cc),
      yarray->GetVariantValue(cc).ToString(this->Formatting, this->Precision));
  }
  this->Internals->ColumnCount++;
}

//----------------------------------------------------------------------------
void vtkCSVExporter::WriteHeader(vtkFieldData* data)
{
  if (!this->OutputStream)
  {
    vtkErrorMacro("Please call Open(STREAM_ROWS)");
    return;
  }
  if (this->Mode != STREAM_ROWS)
  {
    vtkErrorMacro("Incorrect exporter mode. 'OpenFile' must be called with "
                  "'STREAM_ROWS' to use 'WriteHeader' API.");
    return;
  }
  bool first = true;
  const int numArrays = data->GetNumberOfArrays();
  for (int cc = 0; cc < numArrays; cc++)
  {
    vtkAbstractArray* array = data->GetAbstractArray(cc);
    auto name = array->GetName();
    auto label = this->GetColumnLabel(name);
    if (label == nullptr)
    {
      // skip columns with no names.
      continue;
    }
    int numComps = array->GetNumberOfComponents();
    for (int comp = 0; comp < numComps; comp++)
    {
      if (!first)
      {
        (*this->OutputStream) << this->FieldDelimiter;
      }
      (*this->OutputStream) << label;
      if (numComps > 1)
      {
        (*this->OutputStream) << ":" << comp;
      }
      first = false;
    }
  }
  (*this->OutputStream) << "\n";
}

//----------------------------------------------------------------------------
void vtkCSVExporter::WriteData(vtkFieldData* data)
{
  if (!this->OutputStream)
  {
    vtkErrorMacro("Please call Open()");
    return;
  }
  if (this->Mode != STREAM_ROWS)
  {
    vtkErrorMacro("Incorrect exporter mode. 'OpenFile' must be called with "
                  "'STREAM_ROWS' to use 'WriteData' API.");
    return;
  }

  vtkIdType numTuples = data->GetNumberOfTuples();
  int numArrays = data->GetNumberOfArrays();
  for (vtkIdType tuple = 0; tuple < numTuples; tuple++)
  {
    bool first = true;
    for (int cc = 0; cc < numArrays; cc++)
    {
      vtkAbstractArray* array = data->GetAbstractArray(cc);
      auto name = array->GetName();
      auto label = this->GetColumnLabel(name);
      if (label == nullptr)
      {
        // skip columns with no names.
        continue;
      }

      auto validMask = vtkUnsignedCharArray::SafeDownCast(
        data->GetArray(("__vtkValidMask__" + std::string(name)).c_str()));
      int numComps = array->GetNumberOfComponents();
      for (int comp = 0; comp < numComps; comp++)
      {
        if (!first)
        {
          (*this->OutputStream) << this->FieldDelimiter;
        }

        if (validMask == nullptr || validMask->GetValue(tuple) == 1)
        {
          vtkVariant value = array->GetVariantValue(tuple * numComps + comp);

          // to avoid weird characters in the output, cast char /
          // signed char / unsigned char variables to integers
          value = (value.IsChar() || value.IsSignedChar() || value.IsUnsignedChar())
            ? vtkVariant(value.ToInt())
            : value;

          (*this->OutputStream) << value.ToString(this->Formatting, this->Precision).c_str();
        }
        first = false;
      }
    }
    (*this->OutputStream) << "\n";
  }
}

//----------------------------------------------------------------------------
void vtkCSVExporter::Close()
{
  if (!this->OutputStream)
  {
    return;
  }
  if (this->Mode == STREAM_COLUMNS)
  {
    this->Internals->DumpLines(this->FieldDelimiter, *this->OutputStream);
  }

  auto fileStream = dynamic_cast<vtksys::ofstream*>(this->OutputStream);
  if (fileStream == nullptr && this->WriteToOutputString)
  {
    auto ss = dynamic_cast<std::ostringstream*>(OutputStream);
    this->OutputString = ss->str();
  }

  delete this->OutputStream;
  this->OutputStream = nullptr;
}

//----------------------------------------------------------------------------
void vtkCSVExporter::Abort()
{
  if (this->OutputStream)
  {
    this->Close();
    vtksys::SystemTools::RemoveFile(this->FileName);
  }
}

//----------------------------------------------------------------------------
void vtkCSVExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "FieldDelimiter: " << (this->FieldDelimiter ? this->FieldDelimiter : "(none)")
     << endl;
}
