/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPick.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPick.h"
#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVPickDisplay.h"
#include "vtkPVApplication.h"
#include "vtkPVPart.h"
#include "vtkPVRenderModule.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVProcessModule.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPick);
vtkCxxRevisionMacro(vtkPVPick, "1.5");


//----------------------------------------------------------------------------
vtkPVPick::vtkPVPick()
{
  this->ReplaceInputOff();

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part yet.
  this->RequiredNumberOfInputParts = 1;
  
  this->PickDisplay = vtkPVPickDisplay::New();

  this->DataFrame = vtkKWWidget::New();
  this->LabelCollection = vtkCollection::New();
}

vtkPVPick::~vtkPVPick()
{  
  if (this->GetPVApplication() && this->GetPVApplication()->GetProcessModule()->GetRenderModule())
    {
    this->GetPVApplication()->GetProcessModule()->GetRenderModule()->RemoveDisplay(this->PickDisplay);
    }

  this->PickDisplay->SetPointLabelVisibility(0);
  this->PickDisplay->Delete();
  this->PickDisplay = NULL;

  this->DataFrame->Delete();
  this->DataFrame = NULL;
  this->ClearDataLabels();
  this->LabelCollection->Delete();
  this->LabelCollection = NULL;
  this->LabelRow = 1;
}

//----------------------------------------------------------------------------
void vtkPVPick::SetVisibilityInternal(int val)
{
  if (this->PickDisplay)
    {
    this->PickDisplay->SetPointLabelVisibility(val);
    }
  this->Superclass::SetVisibilityInternal(val);
}


//----------------------------------------------------------------------------
void vtkPVPick::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  this->PickDisplay->SetProcessModule(pvApp->GetProcessModule());

  this->Superclass::CreateProperties();

  this->DataFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->DataFrame->Create(pvApp, "frame", "");
  this->Script("pack %s",
               this->DataFrame->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVPick::AcceptCallbackInternal()
{
  // call the superclass's method
  this->Superclass::AcceptCallbackInternal();
    
  if (this->PickDisplay->GetPart() == NULL)
    {
    // Connect to the display.
    // These should be merged.
    this->PickDisplay->SetPart(this->GetPart(0)->GetSMPart());
    this->PickDisplay->SetInput(this->GetPart(0)->GetSMPart());
    this->GetPart(0)->AddDisplay(this->PickDisplay);
    this->GetPVApplication()->GetProcessModule()->GetRenderModule()->AddDisplay(this->PickDisplay);
    }

  // We need to update manually for the case we are probing one point.
  this->PickDisplay->Update();
  this->PickDisplay->SetPointLabelVisibility(1);
  this->GetPVOutput()->DrawWireframe();
  this->GetPVOutput()->ColorByProperty();
  this->GetPVOutput()->ChangeActorColor(0.8, 0.0, 0.2);
  this->GetPVOutput()->SetLineWidth(2);

  this->ClearDataLabels();
  // Get the collected data from the display.
  vtkUnstructuredGrid* d = this->PickDisplay->GetCollectedData();
  if (d->GetNumberOfCells() > 0)
    {
    this->InsertDataLabel("Cell", 0, d->GetCellData());
    }
  vtkIdType num, idx;
  num = d->GetNumberOfPoints();
  for (idx = 0; idx < num; ++idx)
    {
    this->InsertDataLabel("Point", idx, d->GetPointData());
    }
}
 
//----------------------------------------------------------------------------
void vtkPVPick::ClearDataLabels()
{
  vtkCollectionIterator* it = this->LabelCollection->NewIterator();
  for ( it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWLabel *label =
      static_cast<vtkKWLabel*>(it->GetObject());
    if (label == NULL)
      {
      vtkErrorMacro("Only labels should be in this collection.");
      }
    else
      {
      this->Script("grid forget %s", label->GetWidgetName());
      }
    }
  it->Delete();
  this->LabelCollection->RemoveAllItems();
  this->LabelRow = 1;
}

//----------------------------------------------------------------------------
void vtkPVPick::InsertDataLabel(const char* labelArg, vtkIdType idx, 
                                vtkDataSetAttributes* attr)
{
  // update the ui
  vtkIdType j, numComponents;
  // use vtkstd::string since 'label' can grow in length arbitrarily
  vtkstd::string label;
  vtkstd::string arrayData;
  vtkstd::string tempArray;
  vtkKWLabel* kwl;

  // First the point/cell index label.
  kwl = vtkKWLabel::New();
  kwl->SetParent(this->DataFrame);
  kwl->Create(this->GetPVApplication(), "");
  ostrstream kwlStr;
  kwlStr << labelArg  << ": " << idx << ends;
  kwl->SetLabel(kwlStr.str());
  kwlStr.rdbuf()->freeze(0);
  this->LabelCollection->AddItem(kwl);
  this->Script("grid %s -column 1 -row %d -sticky nw",
               kwl->GetWidgetName(), this->LabelRow++);
  kwl->Delete();
  kwl = NULL;

  // Now add a label for the attribute data.
  int numArrays = attr->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
    {
    vtkDataArray* array = attr->GetArray(i);
    if (array->GetName())
      {
      numComponents = array->GetNumberOfComponents();
      if (numComponents > 1)
        {
        // make sure we fill buffer from the beginning
        ostrstream arrayStrm;
        arrayStrm << array->GetName() << ": ( " << ends;
        arrayData = arrayStrm.str();
        arrayStrm.rdbuf()->freeze(0);
        for (j = 0; j < numComponents; j++)
          {
          // make sure we fill buffer from the beginning
          ostrstream tempStrm;
          tempStrm << array->GetComponent( idx, j ) << ends; 
          tempArray = tempStrm.str();
          tempStrm.rdbuf()->freeze(0);

          if (j < numComponents - 1)
            {
            tempArray += ",";
            if (j % 3 == 2)
              {
              tempArray += "\n\t";
              }
            else
              {
              tempArray += " ";
              }
            }
          else
            {
            tempArray += " )\n";
            }
          arrayData += tempArray;
          }
        label += arrayData;
        }
      else
        {
        // make sure we fill buffer from the beginning
        ostrstream arrayStrm;
        arrayStrm << array->GetName() << ": " << array->GetComponent( idx, 0 ) << endl << ends;
        label += arrayStrm.str();
        arrayStrm.rdbuf()->freeze(0);
        }
      }
    }


  kwl = vtkKWLabel::New();
  kwl->SetParent(this->DataFrame);
  kwl->Create(this->GetPVApplication(), "");
  kwl->SetLabel( label.c_str() );
  this->LabelCollection->AddItem(kwl);
  this->Script("grid %s -column 2 -row %d -sticky nw",
               kwl->GetWidgetName(), this->LabelRow++);
  kwl->Delete();
  kwl = NULL;
}

//----------------------------------------------------------------------------
void vtkPVPick::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
