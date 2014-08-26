/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkEnsembleReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnsembleReader.h"

#include "vtkDataObject.h"
#include "vtkDelimitedTextReader.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariant.h"

using std::string;


vtkStandardNewMacro(vtkEnsembleReader);

//----------------------------------------------------------------------------
vtkEnsembleReader::vtkEnsembleReader()
{
}

//----------------------------------------------------------------------------
vtkEnsembleReader::~vtkEnsembleReader()
{
}

//----------------------------------------------------------------------------
int vtkEnsembleReader::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  // the first pipeline request
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    this->UpdateReader();
    }
  if (this->Reader->ProcessRequest(request, inputVector, outputVector))
    {
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
      {
      this->AddCurrentTableRow(outputVector);
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkEnsembleReader::AddCurrentTableRow(vtkInformationVector* outputVector)
{
  vtkDataObject *output = vtkDataObject::GetData(outputVector);
  vtkFieldData* fieldData =
    output->GetAttributesAsFieldData(vtkDataObject::FIELD);
  // last column contains file names
  for (vtkIdType column = 0;
       column < this->Table->GetNumberOfColumns() - 1; ++column)
    {
    vtkNew<vtkDoubleArray> a;
    a->SetName (this->Table->GetColumnName(column));
    a->SetNumberOfComponents(0);
    a->SetNumberOfValues(1);
    a->SetValue(0, this->Table->GetValue(
                  this->_FileIndex, column).ToDouble());
    fieldData->AddArray(a.GetPointer());
    }
}


//----------------------------------------------------------------------------
void vtkEnsembleReader::UpdateReader()
{
  if (this->MetaFileNameMTime > this->MetaFileReadTime)
    {
    this->Table = ReadMetaFile(this->_MetaFileName);
    this->MetaFileReadTime.Modified();
    }

  if (this->FileIndexMTime >= this->FileNameMTime)
    {
    if(this->_FileIndex >= this->GetNumberOfFiles() ||
       this->_FileIndex < 0)
      {
      vtkWarningMacro("Invalid FileIndex for vtkEnsembleReader");
      this->_FileIndex = 0;
      }
    string fileName = this->GetReaderFileName(this->_MetaFileName,
                                              this->Table, this->_FileIndex);
    this->BeforeFileNameMTime = this->Reader->GetMTime();
    this->ReaderSetFileName(fileName.c_str());
    this->FileNameMTime = this->Reader->GetMTime();
    }
}


//----------------------------------------------------------------------------
void vtkEnsembleReader::PrintSelf( ostream&  os, vtkIndent indent )
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "FileName: "
       << (this->_MetaFileName != NULL ? this->_MetaFileName : "(null)")
       << endl;
    os << indent << "FileIndex: " << this->_FileIndex << endl;
}

//----------------------------------------------------------------------------
vtkIdType vtkEnsembleReader::GetNumberOfFiles() const
{
  return this->Table->GetNumberOfRows();
}

//----------------------------------------------------------------------------
vtkIdType vtkEnsembleReader::GetNumberOfParameters() const
{
  return this->Table->GetNumberOfColumns() - 1;
}


//----------------------------------------------------------------------------
string vtkEnsembleReader::GetReaderFileName(
  const char* metaFileName, vtkTable* table, vtkIdType i)
{
  vtkIdType lastColumn = table->GetNumberOfColumns() - 1;
  vtkVariant fileName = table->GetValue(i, lastColumn);
  if (! fileName.IsString())
    {
    vtkErrorMacro(<< "File name is not string: " << fileName);
    return "";
    }
  return FromRelativeToMetaFile(metaFileName, fileName.ToString().c_str());
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkTable> vtkEnsembleReader::ReadMetaFile(
  const char* metaFileName)
{
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(metaFileName);
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  reader->SetForceDouble(true);
  reader->SetTrimWhitespacePriorToNumericConversion(true);
  reader->Update();
  vtkSmartPointer<vtkTable> table = reader->GetOutput();
  this->FileIndexRange[1] = table->GetNumberOfRows() - 1;
  return table;
}
