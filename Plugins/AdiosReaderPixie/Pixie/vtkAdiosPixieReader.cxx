/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdiosPixieReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdiosPixieReader.h"
#include "vtkAdiosInternals.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkExtentTranslator.h>
#include <vtkImageData.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtksys/SystemTools.hxx>

#include <assert.h>
#include <map>
#include <sstream>
#include <string>

using std::ostringstream;

//*****************************************************************************
class vtkAdiosPixieReader::Internals
{
public:
  Internals(vtkAdiosPixieReader* owner)
  {
    this->Owner = owner;
    this->MeshFile = NULL;

    // Manage piece information
    vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
    this->ExtentTranslator->SetPiece(ctrl->GetLocalProcessId());
    this->ExtentTranslator->SetNumberOfPieces(ctrl->GetNumberOfProcesses());
    this->ExtentTranslator->SetGhostLevel(0); // FIXME ???
  }
  // --------------------------------------------------------------------------
  virtual ~Internals()
  {
    if (this->MeshFile)
    {
      delete this->MeshFile;
      this->MeshFile = NULL;
    }
  }
  // --------------------------------------------------------------------------
  void SetMethod(ADIOS_READ_METHOD method) { this->Method = method; }
  // --------------------------------------------------------------------------
  bool NextStep()
  {
    return this->MeshFile->NextStep();
  } // --------------------------------------------------------------------------
  bool Reset()
  {
    this->MeshFile->Close();
    return this->MeshFile->Open();
  }
  // --------------------------------------------------------------------------
  void FillOutput(vtkDataObject* output)
  {
    vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);
    if (multiBlock)
    {
      vtkImageData* grid =
        AdiosPixie::NewPixieImageData(this->ExtentTranslator.GetPointer(), this->MeshFile, true);
      if (grid)
      {
        multiBlock->SetBlock(0, grid);
        grid->FastDelete();
      }
    }
  }

  // --------------------------------------------------------------------------
  void UpdateFileName(const char* currentFileName)
  {
    if (!currentFileName)
      return;

    if (!this->MeshFile)
    {
      this->MeshFile = new AdiosStream(currentFileName, this->Method, this->Owner->GetParameters());

      // Make sure that file is open and metadata loaded
      this->MeshFile->Open();
    }
    else // Check if the filename has changed
    {
      if (strcmp(currentFileName, this->MeshFile->GetFileName()) != 0)
      {
        delete this->MeshFile; // not NULL because we are in the else
        this->MeshFile =
          new AdiosStream(currentFileName, this->Method, this->Owner->GetParameters());

        // Make sure that file is open and metadata loaded
        this->MeshFile->Open();
      }
    }
  }
  // --------------------------------------------------------------------------
  bool Open() { return this->MeshFile->Open(); }

private:
  AdiosStream* MeshFile;
  ADIOS_READ_METHOD Method;
  vtkAdiosPixieReader* Owner;
  vtkNew<vtkExtentTranslator> ExtentTranslator;
};
//*****************************************************************************

vtkStandardNewMacro(vtkAdiosPixieReader);
//----------------------------------------------------------------------------
vtkAdiosPixieReader::vtkAdiosPixieReader()
{
  this->FileName = NULL;
  this->Parameters = NULL;
  this->SetParameters("");
  this->Internal = new Internals(this);
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkAdiosPixieReader::~vtkAdiosPixieReader()
{
  this->SetFileName(NULL);
  this->SetParameters(NULL);
  if (this->Internal)
  {
    delete this->Internal;
    this->Internal = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkAdiosPixieReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Parameters: " << (this->Parameters ? this->Parameters : "(none)") << "\n";
}

//----------------------------------------------------------------------------
int vtkAdiosPixieReader::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(name, &fs) != 0)
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAdiosPixieReader::RequestData(vtkInformation*,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

  this->Internal->UpdateFileName(this->GetFileName());
  this->Internal->FillOutput(output);

  return 1;
}

//----------------------------------------------------------------------------
void vtkAdiosPixieReader::SetReadMethod(int methodEnum)
{
  this->Internal->SetMethod((ADIOS_READ_METHOD)methodEnum);
}

//----------------------------------------------------------------------------
void vtkAdiosPixieReader::NextStep()
{
  if (this->Internal->NextStep())
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAdiosPixieReader::Reset()
{
  if (this->Internal->Reset())
  {
    this->Modified();
  }
}
