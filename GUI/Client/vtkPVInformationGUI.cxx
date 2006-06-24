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

#include "vtkKWBoundsDisplay.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWNotebook.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSource.h"
#include "vtkKWMultiColumnList.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInformationGUI);
vtkCxxRevisionMacro(vtkPVInformationGUI, "1.18");

//----------------------------------------------------------------------------
vtkPVInformationGUI::vtkPVInformationGUI()
{
  this->StatsFrame = 0;
  this->TypeLabel = 0;
  this->CompositeDataFrame = 0;
  this->NumBlocksLabel = 0;
  this->NumDataSetsLabel = 0;
  this->NumCellsLabel = 0;
  this->NumPointsLabel = 0;
  this->MemorySizeLabel = 0;
  this->PolygonCount = 0;
  this->BoundsDisplay = 0;
  this->ExtentDisplay = 0;
  this->ArrayInformationFrame = 0;
  this->ArrayInformationList = 0;
}

#define vtkKWRemoveIfExists(x) \
  if ( this->x ) \
    { \
    this->x->Delete(); \
    this->x = 0; \
    }

//----------------------------------------------------------------------------
vtkPVInformationGUI::~vtkPVInformationGUI()
{
  vtkKWRemoveIfExists(StatsFrame);
  vtkKWRemoveIfExists(TypeLabel);
  vtkKWRemoveIfExists(CompositeDataFrame);
  vtkKWRemoveIfExists(NumBlocksLabel);
  vtkKWRemoveIfExists(NumDataSetsLabel);
  vtkKWRemoveIfExists(NumCellsLabel);
  vtkKWRemoveIfExists(NumPointsLabel);
  vtkKWRemoveIfExists(MemorySizeLabel);
  vtkKWRemoveIfExists(PolygonCount);
  vtkKWRemoveIfExists(BoundsDisplay);
  vtkKWRemoveIfExists(ExtentDisplay);
  vtkKWRemoveIfExists(ArrayInformationFrame);
  vtkKWRemoveIfExists(ArrayInformationList);
}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::CreateWidget()
{  
  if (this->IsCreated())
    {
    vtkErrorMacro("Widget has already been created.");
    return;
    }
  
  this->Superclass::CreateWidget();

  this->StatsFrame = vtkKWFrameWithLabel::New();
  this->TypeLabel = vtkKWLabel::New();
  this->CompositeDataFrame = vtkKWFrame::New();
  this->NumBlocksLabel = vtkKWLabel::New();
  this->NumDataSetsLabel = vtkKWLabel::New();
  this->NumCellsLabel = vtkKWLabel::New();
  this->NumPointsLabel = vtkKWLabel::New();
  this->MemorySizeLabel = vtkKWLabel::New();
  this->PolygonCount = vtkKWLabel::New();
  this->BoundsDisplay = vtkKWBoundsDisplay::New();
  this->ExtentDisplay = vtkKWBoundsDisplay::New();
  this->ArrayInformationFrame = vtkKWFrameWithLabel::New();
  this->ArrayInformationList = vtkKWMultiColumnList::New();

  this->StatsFrame->SetParent(this->GetFrame());
  this->StatsFrame->Create();
  this->StatsFrame->SetLabelText("Statistics");

  this->TypeLabel->SetParent(this->StatsFrame->GetFrame());
  this->TypeLabel->Create();

  this->CompositeDataFrame->SetParent(this->StatsFrame->GetFrame());
  this->CompositeDataFrame->Create();

  this->NumBlocksLabel->SetParent(this->CompositeDataFrame);
  this->NumBlocksLabel->Create();

  this->NumDataSetsLabel->SetParent(this->CompositeDataFrame);
  this->NumDataSetsLabel->Create();

  this->NumCellsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumCellsLabel->Create();

  this->NumPointsLabel->SetParent(this->StatsFrame->GetFrame());
  this->NumPointsLabel->Create();
  
  this->MemorySizeLabel->SetParent(this->StatsFrame->GetFrame());
  this->MemorySizeLabel->Create();

  this->PolygonCount->SetParent(this->StatsFrame->GetFrame());
  this->PolygonCount->Create();

  this->BoundsDisplay->SetParent(this->GetFrame());
  this->BoundsDisplay->Create();
  
  this->ExtentDisplay->SetParent(this->GetFrame());
  this->ExtentDisplay->Create();
  this->ExtentDisplay->SetLabelText("Extents");
  
  this->Script("pack %s %s % s %s %s %s -side top -anchor nw",
               this->TypeLabel->GetWidgetName(),
               this->CompositeDataFrame->GetWidgetName(),
               this->NumCellsLabel->GetWidgetName(),
               this->NumPointsLabel->GetWidgetName(),
               this->MemorySizeLabel->GetWidgetName(),
               this->PolygonCount->GetWidgetName());

  this->Script("pack %s %s -fill x -expand t -pady 2", 
               this->StatsFrame->GetWidgetName(),
               this->BoundsDisplay->GetWidgetName());

  this->ArrayInformationFrame->SetParent(this->StatsFrame->GetFrame());
  this->ArrayInformationFrame->Create();

  this->ArrayInformationList->SetParent(this->ArrayInformationFrame->GetFrame());
  this->ArrayInformationFrame->SetLabelText("Data Arrays");
  this->ArrayInformationList->Create();
  this->ArrayInformationList->AddColumn("Name");
  this->ArrayInformationList->AddColumn("Type");
  this->ArrayInformationList->AddColumn("Data Type");
  this->ArrayInformationList->AddColumn("Data Range");
  this->ArrayInformationList->SetColumnAlignmentToCenter(1);
  this->ArrayInformationList->SetColumnAlignmentToCenter(2);
  this->ArrayInformationList->StretchableColumnsOn();

  this->Script("pack %s -side top -anchor nw -expand 1 -fill both -padx 2 -pady 2", 
    this->ArrayInformationList->GetWidgetName());
  this->Script("pack %s -fill x -expand t -pady 2 -side bottom", 
               this->ArrayInformationFrame->GetWidgetName());
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
  if (dataInfo->GetCompositeDataSetType() >= 0)
    {
    dataType = dataInfo->GetCompositeDataSetType();
    }

  if (dataType == VTK_POLY_DATA)
    {
    type << "Polygonal";
    this->Script("pack forget %s", 
      this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_HYPER_OCTREE)
    {
    type << "Octree";
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
  else if (dataType == VTK_MULTIGROUP_DATA_SET)
    {
    type << "Multi-group";
    this->Script("pack forget %s", 
      this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_MULTIBLOCK_DATA_SET)
    {
    type << "Multi-block";
    this->Script("pack forget %s", 
      this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_HIERARCHICAL_DATA_SET)
    {
    type << "Hierarchical AMR";
    this->Script("pack forget %s", 
      this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    type << "Hierarchical Uniform AMR";
    this->Script("pack forget %s", 
      this->ExtentDisplay->GetWidgetName());
    }
  else if (dataType == VTK_HYPER_OCTREE)
    {
    type << "Unknown";
    }
  type << ends;
  this->TypeLabel->SetText(type.str());
  delete[] type.str();

  ostrstream numcells;

  int packNumBlocks = 0;
  vtkPVCompositeDataInformation* cdi = 
    dataInfo->GetCompositeDataInformation();
  if (dataType == VTK_MULTIGROUP_DATA_SET && cdi)
    {
    ostrstream numBlocks;
    numBlocks << "Number of groups: " 
      << cdi->GetNumberOfGroups() 
      << ends;
    this->NumBlocksLabel->SetText(numBlocks.str());
    delete[] numBlocks.str();
    packNumBlocks = 1;
    }

  if (dataType == VTK_MULTIBLOCK_DATA_SET && cdi)
    {
    ostrstream numBlocks;
    numBlocks << "Number of blocks: " 
      << cdi->GetNumberOfGroups() 
      << ends;
    this->NumBlocksLabel->SetText(numBlocks.str());
    delete[] numBlocks.str();
    packNumBlocks = 1;
    }

  if (dataType == VTK_HIERARCHICAL_DATA_SET ||
    dataType == VTK_HIERARCHICAL_BOX_DATA_SET)
    {
    ostrstream numBlocks;
    numBlocks << "Number of levels: " 
      << cdi->GetNumberOfGroups() 
      << ends;
    this->NumBlocksLabel->SetText(numBlocks.str());
    delete[] numBlocks.str();
    packNumBlocks = 1;
    }

  if (packNumBlocks)
    {
    this->Script("pack %s -side top -anchor nw ", 
      this->NumBlocksLabel->GetWidgetName());
    }
  else
    {
    this->Script("pack forget %s", 
      this->NumBlocksLabel->GetWidgetName());
    }

  if (dataType == VTK_MULTIGROUP_DATA_SET ||
    dataType == VTK_MULTIBLOCK_DATA_SET ||
    dataType == VTK_HIERARCHICAL_DATA_SET ||
    dataType == VTK_HIERARCHICAL_BOX_DATA_SET ||
    dataInfo->GetNumberOfDataSets() > 1)
    {
    ostrstream numds;
    numds << "Number of datasets: " 
      << dataInfo->GetNumberOfDataSets() 
      << ends;
    this->NumDataSetsLabel->SetText(numds.str());
    delete[] numds.str();
    this->Script("pack %s -side top -anchor nw ", 
      this->NumDataSetsLabel->GetWidgetName());
    }
  else
    {
    this->Script("pack forget %s", 
      this->NumDataSetsLabel->GetWidgetName());
    if (!packNumBlocks)
      {
      // If it is empty, the fame should not occupy any vertical
      // space.
      this->CompositeDataFrame->SetHeight(1);
      }
    }

  numcells << "Number of cells: " << dataInfo->GetNumberOfCells() << ends;
  this->NumCellsLabel->SetText(numcells.str());
  delete[] numcells.str();

  ostrstream numpts;
  numpts << "Number of points: " << dataInfo->GetNumberOfPoints() << ends;
  this->NumPointsLabel->SetText(numpts.str());
  delete[] numpts.str();

  ostrstream memsize;
  memsize << "Memory: " << ((float)(dataInfo->GetMemorySize())/1000.0) << " MBytes" << ends;
  this->MemorySizeLabel->SetText(memsize.str());
  delete[] memsize.str();

  ostrstream polycount;
  polycount << "Polygon count: " << dataInfo->GetPolygonCount() << ends;
  this->PolygonCount->SetText(polycount.str());
  delete[] polycount.str();

  dataInfo->GetBounds(bounds);
  this->BoundsDisplay->SetBounds(bounds);

  int array;
  int totalArray = 0;
  vtkPVDataSetAttributesInformation* dataSetAttr[2];
  dataSetAttr[0] = dataInfo->GetPointDataInformation();
  dataSetAttr[1] = dataInfo->GetCellDataInformation();
  int dt;
  this->ArrayInformationList->DeleteAllRows();
  for ( dt = 0; dt < 2; ++ dt )
    {
    vtkPVDataSetAttributesInformation* dsa = dataSetAttr[dt];
    for ( array = 0; array < dsa->GetNumberOfArrays(); ++ array )
      {
      vtkPVArrayInformation* dataArray = dsa->GetArrayInformation(array);
      this->ArrayInformationList->InsertCellText(totalArray, 0, dataArray->GetName());
      this->ArrayInformationList->InsertCellText(totalArray, 1, dt?"cell":"point");
      char buffer[1024];
      sprintf(buffer, "%d - %s", dataArray->GetNumberOfComponents(), vtkImageScalarTypeNameMacro(dataArray->GetDataType()));
      this->ArrayInformationList->InsertCellText(totalArray, 2, buffer);
      vtkstd::string res;
      int comp;
      for ( comp = 0; comp < dataArray->GetNumberOfComponents(); ++ comp )
        {
        if ( comp > 0 )
          {
          res += ", ";
          }
        double* range = dataArray->GetComponentRange(comp);
        sprintf(buffer, "%f - %f", range[0], range[1]);
        res += buffer;
        }
      this->ArrayInformationList->InsertCellText(totalArray, 3, res.c_str());
      totalArray ++;
      }
    }
  this->ArrayInformationList->SetHeight(totalArray);
}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->TypeLabel);
  this->PropagateEnableState(this->StatsFrame);
  this->PropagateEnableState(this->NumBlocksLabel);
  this->PropagateEnableState(this->NumDataSetsLabel);
  this->PropagateEnableState(this->NumCellsLabel);
  this->PropagateEnableState(this->NumPointsLabel);
  this->PropagateEnableState(this->MemorySizeLabel);
  this->PropagateEnableState(this->PolygonCount);
  this->PropagateEnableState(this->BoundsDisplay);
  this->PropagateEnableState(this->ExtentDisplay);  
}

//----------------------------------------------------------------------------
void vtkPVInformationGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

