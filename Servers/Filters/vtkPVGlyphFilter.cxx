/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyphFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGlyphFilter.h"

#include "vtkGarbageCollector.h"
#include "vtkMaskPoints.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPVGlyphFilter, "1.12");
vtkStandardNewMacro(vtkPVGlyphFilter);

//-----------------------------------------------------------------------------
vtkPVGlyphFilter::vtkPVGlyphFilter()
{
  this->SetColorModeToColorByScalar();
  this->SetScaleModeToScaleByVector();
  this->MaskPoints = vtkMaskPoints::New();
  this->MaximumNumberOfPoints = 5000;
  this->NumberOfProcesses = vtkMultiProcessController::GetGlobalController() ?
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() : 1;
  this->UseMaskPoints = 1;
}

//-----------------------------------------------------------------------------
vtkPVGlyphFilter::~vtkPVGlyphFilter()
{
  if(this->MaskPoints)
    {
    this->MaskPoints->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::SetInput(vtkDataSet *input)
{
  this->MaskPoints->SetInput(input);
  this->Superclass::SetInput(this->MaskPoints->GetOutput());
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::SetRandomMode(int mode)
{
  this->MaskPoints->SetRandomMode(mode);
}

//-----------------------------------------------------------------------------
int vtkPVGlyphFilter::GetRandomMode()
{
  return this->MaskPoints->GetRandomMode();
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::Execute()
{
  if (this->UseMaskPoints)
    {
    vtkPolyData* output = this->GetOutput();
    this->Superclass::SetInput(this->MaskPoints->GetOutput());
    vtkIdType maxNumPts = this->MaximumNumberOfPoints;
    vtkIdType numPts = this->MaskPoints->GetInput()->GetNumberOfPoints();
    // Although this is not perfectly process invariant, it is better
    // than we had before (divide by number of processes).
    vtkIdType totalNumPts = numPts;
    vtkMultiProcessController *controller;
    controller = vtkMultiProcessController::GetGlobalController();
    if (controller)
      {
      vtkIdType tmp;
      // This could be done much easier with MPI specific calls.
      if (controller->GetLocalProcessId() == 0)
        {
        int i;
        // Sum points on all processes.
        for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
          {
          controller->Receive(&tmp, 1, i, 349870);
          totalNumPts += tmp;
          }
        // Send results back to all processes.
        for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
          {
          controller->Send(&totalNumPts, 1, i, 349871);
          }
        }
      else
        {
        controller->Send(&numPts, 1, 0, 349870);
        controller->Receive(&totalNumPts, 1, 0, 349871);
        }
      // What fraction of the points will this processes get allocated?
      maxNumPts = (vtkIdType)(
        (double)(maxNumPts)*(double)(numPts)/(double)(totalNumPts));
      }

    maxNumPts = (maxNumPts < 1) ? 1 : maxNumPts;
    this->MaskPoints->SetMaximumNumberOfPoints(maxNumPts);
    this->MaskPoints->SetOnRatio(numPts / maxNumPts);
    // I do not like connecting internal filters to the actual input, but
    // This is the smallest change possible to fix the problem.
    // This update caused input to be executed with number of piecces of 1.
    this->MaskPoints->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
    this->MaskPoints->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
    this->MaskPoints->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
    this->MaskPoints->Update();
    }
  else
    {
    this->Superclass::SetInput(this->MaskPoints->GetInput());
    }
  
  this->Superclass::Execute();
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  collector->ReportReference(this->MaskPoints, "MaskPoints");
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::RemoveReferences()
{
  if(this->MaskPoints)
    {
    this->MaskPoints->Delete();
    this->MaskPoints = 0;
    }
  this->Superclass::RemoveReferences();
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputScalarsSelection: " 
     << (this->InputScalarsSelection ? this->InputScalarsSelection : "(none)")
     << endl;

  os << indent << "InputVectorsSelection: " 
     << (this->InputVectorsSelection ? this->InputVectorsSelection : "(none)")
     << endl;

  os << indent << "InputNormalsSelection: " 
     << (this->InputNormalsSelection ? this->InputNormalsSelection : "(none)")
     << endl;
  
  os << indent << "MaximumNumberOfPoints: " << this->GetMaximumNumberOfPoints()
     << endl;

  os << indent << "UseMaskPoints: " << (this->UseMaskPoints?"on":"off") << endl;

  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}
