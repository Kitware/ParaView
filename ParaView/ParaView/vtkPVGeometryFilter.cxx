/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGeometryFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGeometryFilter.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGeometryFilter.h"
#ifdef PARAVIEW_BUILD_DEVELOPMENT
#include "vtkCompositeDataIterator.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHierarchicalBoxOutlineFilter.h"
#endif
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridOutlineFilter.h"
#include "vtkStripper.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkPVGeometryFilter, "1.16");
vtkStandardNewMacro(vtkPVGeometryFilter);

//----------------------------------------------------------------------------
vtkPVGeometryFilter::vtkPVGeometryFilter ()
{
  this->OutlineFlag = 0;
  this->UseOutline = 1;
  this->UseStrips = 0;
  this->NumberOfRequiredInputs = 0;
  this->DataSetSurfaceFilter = vtkDataSetSurfaceFilter::New();
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  this->HierarchicalBoxOutline = vtkHierarchicalBoxOutlineFilter::New();
#endif
}

//----------------------------------------------------------------------------
vtkPVGeometryFilter::~vtkPVGeometryFilter ()
{
  this->DataSetSurfaceFilter->Delete();
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  this->HierarchicalBoxOutline->Delete();
#endif
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPVGeometryFilter::SetInput(vtkDataObject *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataObject *vtkPVGeometryFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return this->Inputs[0];
}



//----------------------------------------------------------------------------
int vtkPVGeometryFilter::CheckAttributes(vtkDataObject* input)
{
  if (input->IsA("vtkDataSet"))
    {
    if (!static_cast<vtkDataSet*>(input)->CheckAttributes())
      {
      return 0;
      }
    }
//   else if (input->IsA("vtkCompositeDataSet"))
//     {
//     vtkCompositeDataSet* compInput = 
//       static_cast<vtkCompositeDataSet*>(input);
//     vtkCompositeDataIterator* iter = compInput->NewIterator();
//     while (!iter->IsDoneWithTraversal())
//       {
//       vtkDataObject* curDataSet = iter->GetCurrentDataObject();
//       if (curDataSet && !this->CheckAttributes(curDataSet))
//         {
//         return 0;
//         }
//       iter->GoToNextItem();
//       }
//     iter->Delete();
//     }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::Execute()
{
  vtkDataObject *input = this->GetInput();

  if (input == NULL)
    {
    return;
    }

  if (!this->CheckAttributes(input))
    {
    return;
    }

  if (input->IsA("vtkImageData"))
    {
    this->ImageDataExecute(static_cast<vtkImageData*>(input));
    return;
    }

  if (input->IsA("vtkStructuredGrid"))
    {
    this->StructuredGridExecute(static_cast<vtkStructuredGrid*>(input));
    return;
    }

  if (input->IsA("vtkRectilinearGrid"))
    {
    this->RectilinearGridExecute(static_cast<vtkRectilinearGrid*>(input));
    return;
    }

  if (input->IsA("vtkUnstructuredGrid"))
    {
    this->UnstructuredGridExecute(static_cast<vtkUnstructuredGrid*>(input));
    return;
    }
  if (input->IsA("vtkPolyData"))
    {
    this->PolyDataExecute(static_cast<vtkPolyData*>(input));
    return;
    }
  if (input->IsA("vtkDataSet"))
    {
    this->DataSetExecute(static_cast<vtkDataSet*>(input));
    return;
    }
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  if (input->IsA("vtkHierarchicalBoxDataSet"))
    {
    this->HierarchicalBoxExecute(static_cast<vtkHierarchicalBoxDataSet*>(input));
    return;
    }
#endif
  return;
}

#ifdef PARAVIEW_BUILD_DEVELOPMENT
//----------------------------------------------------------------------------
void vtkPVGeometryFilter::HierarchicalBoxExecute(vtkHierarchicalBoxDataSet *input)
{
  vtkHierarchicalBoxDataSet* ds = input->NewInstance();
  ds->ShallowCopy(input);
  this->HierarchicalBoxOutline->SetInput(ds);
  ds->Delete();
  this->HierarchicalBoxOutline->Update();
  this->GetOutput()->ShallowCopy(this->HierarchicalBoxOutline->GetOutput());
}
#endif

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::DataSetExecute(vtkDataSet *input)
{
  vtkOutlineSource* outline = vtkOutlineSource::New();
  vtkPolyData *output = this->GetOutput();

  outline->SetBounds(input->GetBounds());
  outline->Update();

  output->SetPoints(outline->GetOutput()->GetPoints());
  output->SetLines(outline->GetOutput()->GetLines());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::DataSetSurfaceExecute(vtkDataSet *input)
{
  vtkDataSet* ds = input->NewInstance();
  ds->ShallowCopy(input);
  this->DataSetSurfaceFilter->SetInput(ds);
  ds->Delete();
  this->DataSetSurfaceFilter->Update();
  this->GetOutput()->ShallowCopy(this->DataSetSurfaceFilter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ImageDataExecute(vtkImageData *input)
{
  float *spacing;
  float *origin;
  int *ext;
  float bounds[6];
  vtkPolyData *output = this->GetOutput();

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    this->DataSetSurfaceExecute(input);
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let OutlineSource do all the work
  //
  
  if (output->GetUpdatePiece() == 0)
    {
    spacing = input->GetSpacing();
    origin = input->GetOrigin();
    
    bounds[0] = spacing[0] * ((float)ext[0]) + origin[0];
    bounds[1] = spacing[0] * ((float)ext[1]) + origin[0];
    bounds[2] = spacing[1] * ((float)ext[2]) + origin[1];
    bounds[3] = spacing[1] * ((float)ext[3]) + origin[1];
    bounds[4] = spacing[2] * ((float)ext[4]) + origin[2];
    bounds[5] = spacing[2] * ((float)ext[5]) + origin[2];

    vtkOutlineSource *outline = vtkOutlineSource::New();
    outline->SetBounds(bounds);
    outline->Update();

    output->SetPoints(outline->GetOutput()->GetPoints());
    output->SetLines(outline->GetOutput()->GetLines());
    outline->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::StructuredGridExecute(vtkStructuredGrid *input)
{
  int *ext;
  vtkPolyData *output = this->GetOutput();

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    this->DataSetSurfaceExecute(input);
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //
  

  vtkStructuredGridOutlineFilter *outline = vtkStructuredGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::RectilinearGridExecute(vtkRectilinearGrid *input)
{
  int *ext;
  vtkPolyData *output = this->GetOutput();

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    this->DataSetSurfaceExecute(input);
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //

  vtkRectilinearGridOutlineFilter *outline = vtkRectilinearGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::UnstructuredGridExecute(vtkUnstructuredGrid* input)
{
  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;
    this->DataSetSurfaceExecute(input);
    return;
    }
  
  vtkPolyData *output = this->GetOutput();
  
  this->OutlineFlag = 1;
  float bounds[6];
  input->GetBounds(bounds);
  
  vtkOutlineSource *outline = vtkOutlineSource::New();
  outline->SetBounds(bounds);
  outline->Update();
  
  output->SetPoints(outline->GetOutput()->GetPoints());
  output->SetLines(outline->GetOutput()->GetLines());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PolyDataExecute(vtkPolyData *input)
{
  vtkPolyData *out = this->GetOutput(); 

  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;
    if (this->UseStrips)
      {
      vtkPolyData *inCopy = vtkPolyData::New();
      vtkStripper *stripper = vtkStripper::New();
      inCopy->ShallowCopy(input);
      inCopy->RemoveGhostCells(1);
      stripper->SetInput(inCopy);
      stripper->Update();
      out->CopyStructure(stripper->GetOutput());
      out->GetPointData()->ShallowCopy(stripper->GetOutput()->GetPointData());
      out->GetCellData()->ShallowCopy(stripper->GetOutput()->GetCellData());
      inCopy->Delete();
      stripper->Delete();
      }
    else
      {
      out->ShallowCopy(input);
      out->RemoveGhostCells(1);
      }
    return;
    }
  
  this->OutlineFlag = 1;
  float bounds[6];
  input->GetBounds(bounds);
  
  vtkOutlineSource *outline = vtkOutlineSource::New();
  outline->SetBounds(bounds);
  outline->Update();
  
  out->SetPoints(outline->GetOutput()->GetPoints());
  out->SetLines(outline->GetOutput()->GetLines());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->OutlineFlag)
    {
    os << indent << "OutlineFlag: On\n";
    }
  else
    {
    os << indent << "OutlineFlag: Off\n";
    }
  
  os << indent << "UseOutline: " << this->UseOutline << endl;
}
