/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformationGUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInformationGUI.h"
#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLabel.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWNotebook.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInformationGUI);
vtkCxxRevisionMacro(vtkPVInformationGUI, "1.3");

int vtkPVInformationGUICommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVInformationGUI::vtkPVInformationGUI()
{
  this->CommandFunction = vtkPVInformationGUICommand;

  this->StatsFrame = vtkKWFrameLabeled::New();
  this->TypeLabel = vtkKWLabel::New();
  this->NumDataSetsLabel = vtkKWLabel::New();
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  this->MemorySizeLabel = vtkKWLabel::New();
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  this->BoundsDisplay->ShowHideFrameOn();
  this->ExtentDisplay = vtkKWBoundsDisplay::New();
  this->ExtentDisplay->ShowHideFrameOn();
}

//----------------------------------------------------------------------------
vtkPVInformationGUI::~vtkPVInformationGUI()
{    
  this->StatsFrame->Delete();
  this->StatsFrame = NULL;
  
  this->TypeLabel->Delete();
  this->TypeLabel = NULL;
  
  this->NumDataSetsLabel->Delete();
  this->NumDataSetsLabel = NULL;

  this->NumCellsLabel->Delete();
  this->NumCellsLabel = NULL;
  
  this->NumPointsLabel->Delete();
  this->NumPointsLabel = NULL;
  
  this->MemorySizeLabel->Delete();
  this->MemorySizeLabel = NULL;
  
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;
  
  this->ExtentDisplay->Delete();
  this->ExtentDisplay = NULL;  
}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::Create(vtkKWApplication* app, const char* options)
{  
  if (this->GetApplication())
    {
    vtkErrorMacro("Widget has already been created.");
    return;
    }
  
  this->Superclass::Create(app, options);

  this->StatsFrame->SetParent(this->GetFrame());
  this->StatsFrame->ShowHideFrameOn();
  this->StatsFrame->Create(this->GetApplication(), 0);
  this->StatsFrame->SetLabel("Statistics");

  this->TypeLabel->SetParent(this->StatsFrame->GetFrame());
  this->TypeLabel->Create(this->GetApplication(), "");

  this->NumDataSetsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumDataSetsLabel->Create(this->GetApplication(), "");

  this->NumCellsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumCellsLabel->Create(this->GetApplication(), "");

  this->NumPointsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumPointsLabel->Create(this->GetApplication(), "");
  
  this->MemorySizeLabel->SetParent(this->StatsFrame->GetFrame());
  this->MemorySizeLabel->Create(this->GetApplication(), "");

  this->BoundsDisplay->SetParent(this->GetFrame());
  this->BoundsDisplay->Create(this->GetApplication(), "");
  
  this->ExtentDisplay->SetParent(this->GetFrame());
  this->ExtentDisplay->Create(this->GetApplication(), "");
  this->ExtentDisplay->SetLabel("Extents");
  
  this->Script("pack %s %s %s %s -side top -anchor nw",
               this->TypeLabel->GetWidgetName(),
               this->NumDataSetsLabel->GetWidgetName(),
               this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName(),
               this->MemorySizeLabel->GetWidgetName());

  this->Script("pack %s %s -fill x -expand t -pady 2", 
               this->StatsFrame->GetWidgetName(),
               this->BoundsDisplay->GetWidgetName());

}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::Update(vtkPVSource* source)
{
  vtkPVDataInformation* dataInfo = source->GetDataInformation();
  double bounds[6];  
  ostrstream type;
  type << "Type: ";

  // Put the data type as the label of the top frame.
  int dataType = dataInfo->GetDataSetType();
  if (dataType == VTK_POLY_DATA)
    {
    type << "Polygonal";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    type << "Unstructured Grid";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_STRUCTURED_GRID)
    {
    type << "Curvilinear";
    this->ExtentDisplay->SetExtent(dataInfo->GetExtent());
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_RECTILINEAR_GRID)
    {
    type << "Nonuniform Rectilinear";
    this->ExtentDisplay->SetExtent(dataInfo->GetExtent());
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_IMAGE_DATA)
    {
    int *ext = dataInfo->GetExtent();
    if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
      {
      type << "Image (Uniform Rectilinear)";
      }
    else
      {
      type << "Volume (Uniform Rectilinear)";
      }
    this->ExtentDisplay->SetExtent(ext);
    this->Script("pack %s -fill x -expand t -pady 2", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_MULTI_BLOCK_DATA_SET)
    {
    type << "Multi-block composite";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    type << "Hierarchical Uniform AMR";
    this->Script("pack forget %s", 
                 this->ExtentDisplay->GetWidgetName());
    }
  else
    {
    type << "Unknown";
    }
  type << ends;
  this->TypeLabel->SetLabel(type.str());
  delete[] type.str();
  
  ostrstream numcells;
  if (dataType == VTK_MULTI_BLOCK_DATA_SET ||
      dataType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    ostrstream numds;
    numds << "Number of datasets: " 
             << dataInfo->GetNumberOfDataSets() 
             << ends;
    this->NumDataSetsLabel->SetLabel(numds.str());
    delete[] numds.str();
    }
  else
    {
    this->Script("pack forget %s", 
                 this->NumDataSetsLabel->GetWidgetName());
    }

  numcells << "Number of cells: " << dataInfo->GetNumberOfCells() << ends;
  this->NumCellsLabel->SetLabel(numcells.str());
  delete[] numcells.str();

  ostrstream numpts;
  numpts << "Number of points: " << dataInfo->GetNumberOfPoints() << ends;
  this->NumPointsLabel->SetLabel(numpts.str());
  delete[] numpts.str();
  
  ostrstream memsize;
  memsize << "Memory: " << ((float)(dataInfo->GetMemorySize())/1000.0) << " MBytes" << ends;
  this->MemorySizeLabel->SetLabel(memsize.str());
  delete[] memsize.str();

  dataInfo->GetBounds(bounds);
  this->BoundsDisplay->SetBounds(bounds);
}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->TypeLabel);
  this->PropagateEnableState(this->StatsFrame);
  this->PropagateEnableState(this->NumDataSetsLabel);
  this->PropagateEnableState(this->NumCellsLabel);
  this->PropagateEnableState(this->NumPointsLabel);
  this->PropagateEnableState(this->MemorySizeLabel);
  this->PropagateEnableState(this->BoundsDisplay);
  this->PropagateEnableState(this->ExtentDisplay);  
}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

