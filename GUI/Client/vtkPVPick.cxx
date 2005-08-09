/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPick.cxx

  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPick.h"
#include "vtkObjectFactory.h"
#include "vtkPVDisplayGUI.h"
#include "vtkSMPointLabelDisplayProxy.h"
#include "vtkPVApplication.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSourceNotebook.h"
#include "vtkSMProxyManager.h"
#include "vtkSMInputProperty.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWEntry.h"
#include "vtkPVRenderView.h"
#include "vtkPVTraceHelper.h"
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPick);
vtkCxxRevisionMacro(vtkPVPick, "1.22");


//----------------------------------------------------------------------------
vtkPVPick::vtkPVPick()
{
  this->ReplaceInputOff();

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part yet.
  this->RequiredNumberOfInputParts = 1;
  
  this->PointLabelFrame = vtkKWFrameWithLabel::New();
  this->PointLabelCheck = vtkKWCheckButton::New();
  this->PointLabelVisibility = 1;

  this->DataFrame = vtkKWFrame::New();
  this->LabelCollection = vtkCollection::New();

  this->PointLabelFontSizeLabel = vtkKWLabel::New();
  this->PointLabelFontSizeThumbWheel = vtkKWThumbWheel::New();

}

//----------------------------------------------------------------------------
vtkPVPick::~vtkPVPick()
{ 
  this->DataFrame->Delete();
  this->DataFrame = NULL;
  this->ClearDataLabels();
  this->LabelCollection->Delete();
  this->LabelCollection = NULL;
  this->LabelRow = 1;

  this->PointLabelFrame->Delete();
  this->PointLabelFrame = NULL;
  this->PointLabelCheck->Delete();
  this->PointLabelCheck = NULL;
  this->PointLabelFontSizeLabel->Delete();
  this->PointLabelFontSizeLabel = NULL;
  this->PointLabelFontSizeThumbWheel->Delete();
  this->PointLabelFontSizeThumbWheel = NULL;
}

//----------------------------------------------------------------------------
void vtkPVPick::DeleteCallback()
{
  this->Superclass::DeleteCallback();
}

//----------------------------------------------------------------------------
void vtkPVPick::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->Superclass::CreateProperties();

  this->PointLabelFrame->SetParent(this->ParameterFrame->GetFrame());
  this->PointLabelFrame->Create(pvApp);
  this->PointLabelFrame->SetLabelText("Point Label Display");
  this->Script("pack %s -fill x -expand true",
               this->PointLabelFrame->GetWidgetName());
  this->PointLabelFrame->CollapseButtonCallback();

  this->PointLabelCheck->SetParent(this->PointLabelFrame->GetFrame());
  this->PointLabelCheck->Create(pvApp);
  this->PointLabelCheck->SetText("Label Point Ids");
  this->PointLabelCheck->SetCommand(this, "PointLabelCheckCallback");
  this->PointLabelCheck->SetBalloonHelpString(
    "Toggle the visibility of point id labels for this dataset.");
  this->Script("grid %s -sticky wns",
               this->PointLabelCheck->GetWidgetName());
  this->PointLabelCheck->SetSelectedState(this->GetPointLabelVisibility());

  this->PointLabelFontSizeLabel->SetParent(this->PointLabelFrame->GetFrame());
  this->PointLabelFontSizeLabel->Create(pvApp);
  this->PointLabelFontSizeLabel->SetText("Point Id size:");
  this->PointLabelFontSizeLabel->SetBalloonHelpString(
    "This scale adjusts the size of the point ID labels.");

  this->PointLabelFontSizeThumbWheel->SetParent(this->PointLabelFrame->GetFrame());
  this->PointLabelFontSizeThumbWheel->PopupModeOn();
  this->PointLabelFontSizeThumbWheel->SetValue(this->GetPointLabelFontSize());
  this->PointLabelFontSizeThumbWheel->SetResolution(1.0);
  this->PointLabelFontSizeThumbWheel->SetMinimumValue(4.0);
  this->PointLabelFontSizeThumbWheel->ClampMinimumValueOn();
  this->PointLabelFontSizeThumbWheel->Create(pvApp);
  this->PointLabelFontSizeThumbWheel->DisplayEntryOn();
  this->PointLabelFontSizeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PointLabelFontSizeThumbWheel->SetBalloonHelpString("Set the point ID label font size.");
  this->PointLabelFontSizeThumbWheel->GetEntry()->SetWidth(5);
  this->PointLabelFontSizeThumbWheel->SetCommand(this, "ChangePointLabelFontSize");
  this->PointLabelFontSizeThumbWheel->SetEndCommand(this, "ChangePointLabelFontSize");
  this->PointLabelFontSizeThumbWheel->SetEntryCommand(this, "ChangePointLabelFontSize");
  this->PointLabelFontSizeThumbWheel->SetBalloonHelpString(
    "This scale adjusts the font size of the point ID labels.");

  this->Script("grid %s %s -sticky wns",
               this->PointLabelFontSizeLabel->GetWidgetName(),
               this->PointLabelFontSizeThumbWheel->GetWidgetName());
  this->Script("grid %s -sticky wns -padx 1 -pady 2",
               this->PointLabelFontSizeThumbWheel->GetWidgetName());



  this->DataFrame->SetParent(this->ParameterFrame->GetFrame());
  this->DataFrame->Create(pvApp);
  this->Script("pack %s",
               this->DataFrame->GetWidgetName());
  
}


//----------------------------------------------------------------------------
void vtkPVPick::AcceptCallbackInternal()
{
  int initialized = this->GetInitialized();
  int vis = this->GetPointLabelVisibility();
  int fsize = this->GetPointLabelFontSize();

  // call the superclass's method
  this->Superclass::AcceptCallbackInternal();

  if (!initialized)
    {
    this->PointLabelDisplayProxy->SetVisibilityCM(vis);
    this->PointLabelDisplayProxy->SetFontSizeCM(fsize);
    this->SetPointLabelFontSize(fsize);
    }

  // We need to update manually for the case we are probing one point.
  this->PointLabelDisplayProxy->Update();
  this->Notebook->GetDisplayGUI()->DrawWireframe();
  this->Notebook->GetDisplayGUI()->ColorByProperty();
  this->Notebook->GetDisplayGUI()->ChangeActorColor(0.8, 0.0, 0.2);
  this->Notebook->GetDisplayGUI()->SetLineWidth(2);

  this->UpdateGUI();
}

//----------------------------------------------------------------------------
void vtkPVPick::Select()
{
  // Update the GUI incase the input has changed.
  this->UpdateGUI();
  this->Superclass::Select();
}

//----------------------------------------------------------------------------
void vtkPVPick::UpdateGUI()
{
  this->UpdatePointLabelCheck();
  this->UpdatePointLabelFontSize();

  this->ClearDataLabels();
  // Get the collected data from the display.
  if (!this->GetInitialized())
    {
    return;
    }

  vtkUnstructuredGrid* d = this->PointLabelDisplayProxy->GetCollectedData();
  if (d == 0)
    {
    return;
    }
  vtkIdType num, idx;
  num = d->GetNumberOfCells();
  for (idx = 0; idx < num; ++idx)
    {
    this->InsertDataLabel("Cell", idx, d->GetCellData());
    }
  num = d->GetNumberOfPoints();
  for (idx = 0; idx < num; ++idx)
    {
    double x[3];
    d->GetPoints()->GetPoint(idx, x);
    this->InsertDataLabel("Point", idx, d->GetPointData(), x);
    }
}
 
//----------------------------------------------------------------------------
void vtkPVPick::ClearDataLabels()
{
  vtkCollectionIterator* it = this->LabelCollection->NewIterator();
  for ( it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWLabel *label =
      static_cast<vtkKWLabel*>(it->GetCurrentObject());
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
template <class T>
void vtkPVPickPrint(ostream& os, T* d)
{
  os << *d;
}
void vtkPVPickPrintChar(ostream& os, char* d)
{
  os << static_cast<short>(*d);
}
void vtkPVPickPrintUChar(ostream& os, unsigned char* d)
{
  os << static_cast<unsigned short>(*d);
}

//----------------------------------------------------------------------------
void vtkPVPickPrint(ostream& os, vtkDataArray* da,
                    vtkIdType index, vtkIdType j)
{
  // Print the component using its real type.
  void* d = da->GetVoidPointer(index*da->GetNumberOfComponents());
  switch(da->GetDataType())
    {
    case VTK_ID_TYPE:
      vtkPVPickPrint(os, static_cast<vtkIdType*>(d)+j); break;
    case VTK_DOUBLE:
      vtkPVPickPrint(os, static_cast<double*>(d)+j); break;
    case VTK_FLOAT:
      vtkPVPickPrint(os, static_cast<float*>(d)+j); break;
    case VTK_LONG:
      vtkPVPickPrint(os, static_cast<long*>(d)+j); break;
    case VTK_UNSIGNED_LONG:
      vtkPVPickPrint(os, static_cast<unsigned long*>(d)+j); break;
    case VTK_INT:
      vtkPVPickPrint(os, static_cast<int*>(d)+j); break;
    case VTK_UNSIGNED_INT:
      vtkPVPickPrint(os, static_cast<unsigned int*>(d)+j); break;
    case VTK_SHORT:
      vtkPVPickPrint(os, static_cast<short*>(d)+j); break;
    case VTK_UNSIGNED_SHORT:
      vtkPVPickPrint(os, static_cast<unsigned short*>(d)+j); break;
    case VTK_CHAR:
      vtkPVPickPrintChar(os, static_cast<char*>(d)+j); break;
    case VTK_UNSIGNED_CHAR:
      vtkPVPickPrintUChar(os, static_cast<unsigned char*>(d)+j); break;
    default:
      // We do not know about the type.  Just use double.
      os << da->GetComponent(index, j);
    }
}

//----------------------------------------------------------------------------
void vtkPVPick::InsertDataLabel(const char* labelArg, vtkIdType idx, 
                                vtkDataSetAttributes* attr, double* x)
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
  kwl->Create(this->GetPVApplication());
  ostrstream kwlStr;
  kwlStr << labelArg  << ": " << idx << ends;
  kwl->SetText(kwlStr.str());
  kwlStr.rdbuf()->freeze(0);
  this->LabelCollection->AddItem(kwl);
  this->Script("grid %s -column 1 -row %d -sticky nw",
               kwl->GetWidgetName(), this->LabelRow++);
  kwl->Delete();
  kwl = NULL;

  // Print the point location if given.
  if(x)
    {
    ostrstream arrayStrm;
    arrayStrm << "Location: ( " << x[0] << "," << x[1] << "," << x[2] << " )"
              << endl << ends;
    label += arrayStrm.str();
    arrayStrm.rdbuf()->freeze(0);
    }

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
          vtkPVPickPrint(tempStrm, array, idx, j);
          tempStrm << ends;
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
        arrayStrm << array->GetName() << ": ";
        vtkPVPickPrint(arrayStrm, array, idx, 0);
        arrayStrm << endl << ends;
        label += arrayStrm.str();
        arrayStrm.rdbuf()->freeze(0);
        }
      }
    }


  kwl = vtkKWLabel::New();
  kwl->SetParent(this->DataFrame);
  kwl->Create(this->GetPVApplication());
  kwl->SetText( label.c_str() );
  this->LabelCollection->AddItem(kwl);
  this->Script("grid %s -column 2 -row %d -sticky nw",
               kwl->GetWidgetName(), this->LabelRow++);
  kwl->Delete();
  kwl = NULL;
}

//----------------------------------------------------------------------------
void vtkPVPick::PointLabelCheckCallback()
{
  //PVSource does tracing for us
  this->SetPointLabelVisibility(this->PointLabelCheck->GetSelectedState());
}

//----------------------------------------------------------------------------
void vtkPVPick::UpdatePointLabelCheck()
{
  this->PointLabelCheck->SetSelectedState(this->GetPointLabelVisibility());
} 

//----------------------------------------------------------------------------
void vtkPVPick::ChangePointLabelFontSize()
{
  this->SetPointLabelFontSize(
    (int)this->PointLabelFontSizeThumbWheel->GetValue());
} 

//----------------------------------------------------------------------------
void vtkPVPick::UpdatePointLabelFontSize()
{
  this->PointLabelFontSizeThumbWheel->SetValue(this->GetPointLabelFontSize());
} 

//----------------------------------------------------------------------------
void vtkPVPick::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
