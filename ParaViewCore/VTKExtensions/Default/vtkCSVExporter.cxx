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

#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkAbstractArray.h"

vtkStandardNewMacro(vtkCSVExporter);
//----------------------------------------------------------------------------
vtkCSVExporter::vtkCSVExporter()
{
  this->FileStream = 0;
  this->FileName=0;
  this->FieldDelimiter =0;
  this->SetFieldDelimiter(",");
}

//----------------------------------------------------------------------------
vtkCSVExporter::~vtkCSVExporter()
{
  delete this->FileStream;
  this->FileStream = 0;
  this->SetFieldDelimiter(0);
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
bool vtkCSVExporter::Open()
{
  delete this->FileStream;
  this->FileStream = 0;
  this->FileStream = new ofstream(this->FileName);
  if (!this->FileStream || !(*this->FileStream))
    {
    vtkErrorMacro("Failed to open for writing: " << this->FileName);
    delete this->FileStream;
    this->FileStream = 0;
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkCSVExporter::WriteHeader(vtkFieldData* data)
{
  if (!this->FileStream)
    {
    vtkErrorMacro("Please call Open()");
    return;
    }
  bool first = true;
  int numArrays = data->GetNumberOfArrays();
  for (int cc=0; cc < numArrays; cc++)
    {
    vtkAbstractArray* array = data->GetAbstractArray(cc);
    int numComps = array->GetNumberOfComponents();
    for (int comp=0; comp < numComps; comp++)
      {
      if (!first)
        {
        (*this->FileStream) << this->FieldDelimiter;
        }
      (*this->FileStream) << array->GetName();
      if (numComps > 1)
        {
        (*this->FileStream) << ":" << comp;
        }
      first = false;
      }
    }
  (*this->FileStream) << "\n";
}

//----------------------------------------------------------------------------
void vtkCSVExporter::WriteData(vtkFieldData* data)
{
  if (!this->FileStream)
    {
    vtkErrorMacro("Please call Open()");
    return;
    }
  vtkIdType numTuples = data->GetNumberOfTuples();
  int numArrays = data->GetNumberOfArrays();
  for (vtkIdType tuple=0; tuple < numTuples; tuple++)
    {
    bool first = true;
    for (int cc=0; cc < numArrays; cc++)
      {
      vtkAbstractArray* array = data->GetAbstractArray(cc);
      int numComps = array->GetNumberOfComponents();
      for (int comp=0; comp < numComps; comp++)
        {
        if (!first)
          {
          (*this->FileStream) << this->FieldDelimiter;
          }
        vtkVariant value = array->GetVariantValue(tuple*numComps + comp);

        // to avoid weird characters in the output, cast char /
        // signed char / unsigned char variables to integers
        value =   ( value.IsChar() ||
                    value.IsSignedChar() ||
                    value.IsUnsignedChar()
                  )
                ? vtkVariant( value.ToInt() )
                : value;

        (*this->FileStream) << value.ToString().c_str();
        first = false;
        }
      }
    (*this->FileStream) << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkCSVExporter::Close()
{
  if (!this->FileStream)
    {
    vtkErrorMacro("Please call Open()");
    return;
    }

  this->FileStream->close();
  delete this->FileStream;
  this->FileStream = 0;
}

//----------------------------------------------------------------------------
void vtkCSVExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


