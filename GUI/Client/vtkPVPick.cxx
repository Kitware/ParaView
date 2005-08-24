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
//the following are part of the temporal plot
#include "vtkPVArraySelection.h"
#include "vtkSMXYPlotDisplayProxy.h"
#include "vtkSMXYPlotActorProxy.h"
#include "vtkKWLoadSaveButton.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkCommand.h"
#include "vtkPVWindow.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVAnimationManager.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMStringListDomain.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkXYPlotActor.h"
#include "vtkXYPlotWidget.h"
#include "vtkSMIntVectorProperty.h"
// Some header file is defining CurrentTime so undef it
#undef CurrentTime
#include "vtkAnimationCue.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPick);
vtkCxxRevisionMacro(vtkPVPick, "1.24");


//*****************************************************************************
//TODO: merge this with the identical code in vtkPVProbe
class vtkTemporalPickObserver : public vtkCommand
{
public:
  static vtkTemporalPickObserver* New()
    {
    return new vtkTemporalPickObserver;
    }
  void SetTemporalProbeProxy(vtkSMProxy * proxy)
    {
    this->TemporalProbeProxy = proxy;
    }
  virtual void Execute(vtkObject * vtkNotUsed(obj), unsigned long event, void* calldata)
    {
    if (this->TemporalProbeProxy)
      {
      switch(event)
        {
        case vtkCommand::StartAnimationCueEvent:
          {
          //Tell the proxy, to tell the TemporalProbe, to get ready to make a set of samples.
          vtkSMProperty *prop = vtkSMProperty::SafeDownCast(
            this->TemporalProbeProxy->GetProperty("AnimateInit"));
          if (prop) prop->Modified();
          this->TemporalProbeProxy->UpdateVTKObjects();
          break;
          }
        case vtkCommand::AnimationCueTickEvent:
          {
          //Tell the proxy, to tell the TemporalProbe, to take a time sample.
          vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
            vtkAnimationCue::AnimationCueInfo*>(calldata);
          vtkSMDoubleVectorProperty *prop = vtkSMDoubleVectorProperty::SafeDownCast(
            this->TemporalProbeProxy->GetProperty("AnimateTick"));
          if (prop) prop->SetElement(0, cueInfo->CurrentTime);
          this->TemporalProbeProxy->UpdateVTKObjects();
          break;
          }
        }
      }
    }
protected:
  vtkTemporalPickObserver()
    {
    this->TemporalProbeProxy = 0;
    }
  vtkSMProxy* TemporalProbeProxy;
};


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

  //the following are part of the temporal plot
  this->XYPlotFrame = vtkKWFrameWithLabel::New();
  this->ShowXYPlotToggle = vtkKWCheckButton::New();
  this->PlotDisplayProxy = 0; 
  this->PlotDisplayProxyName = 0;
  this->TemporalProbeProxy = 0;
  this->TemporalProbeProxyName = 0;
  this->ArraySelection = vtkPVArraySelection::New();
  this->SaveButton = vtkKWLoadSaveButton::New();
  this->Observer = 0;
  this->LastPorC = -1;
  this->LastUseId = -1;
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

  //the following are part of the temporal plot
  this->XYPlotFrame->Delete();
  this->XYPlotFrame = NULL;
  this->ShowXYPlotToggle->Delete();
  this->ShowXYPlotToggle = NULL;
  if (this->PlotDisplayProxy)
    {
    if (this->GetPVApplication() && this->GetPVApplication()->GetRenderModuleProxy())
      {
      this->RemoveDisplayFromRenderModule(this->PlotDisplayProxy);
      }
    if  (this->PlotDisplayProxyName)
      {
      vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
      pxm->UnRegisterProxy("displays", this->PlotDisplayProxyName);
      this->SetPlotDisplayProxyName(0);
      }
    this->PlotDisplayProxy->Delete();
    this->PlotDisplayProxy = NULL;
    }
  if (this->TemporalProbeProxy)
    {
    if  (this->TemporalProbeProxyName)
      {
      vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
      pxm->UnRegisterProxy("filters", this->TemporalProbeProxyName);
      this->SetTemporalProbeProxyName(0);
      }
    this->TemporalProbeProxy->Delete();
    this->TemporalProbeProxy = NULL;
    }
  this->ArraySelection->Delete();
  this->ArraySelection = NULL;
  this->SaveButton->Delete();
  this->SaveButton =  NULL;
  if (this->Observer) 
    {
    vtkPVAnimationScene *animScene = 
      this->GetPVApplication()->GetMainWindow()->GetAnimationManager()->GetAnimationScene();
    animScene->RemoveObserver(this->Observer);   
    this->Observer->Delete();
    this->Observer = NULL;
    }

}

//----------------------------------------------------------------------------
void vtkPVPick::DeleteCallback()
{
  this->Superclass::DeleteCallback();
}

//----------------------------------------------------------------------------
void vtkPVPick::CreateProperties()
{
  //sets up the GUI
  //TODO: move as much of this as possible into XML
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->Superclass::CreateProperties();

  //point label controls
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


  //temporal plot
  this->XYPlotFrame->SetParent(this->ParameterFrame->GetFrame());
  this->XYPlotFrame->Create(pvApp);
  this->XYPlotFrame->SetLabelText("Temporal Plot");
  this->Script("pack %s -fill x -expand true",
               this->XYPlotFrame->GetWidgetName());
  this->XYPlotFrame->CollapseButtonCallback();

  this->ShowXYPlotToggle->SetParent(this->XYPlotFrame->GetFrame());
  this->ShowXYPlotToggle->Create(pvApp);
  this->ShowXYPlotToggle->SetText("Show XY-Plot");
  this->ShowXYPlotToggle->SetSelectedState(0);
  this->Script("%s configure -command {%s SetAcceptButtonColorToModified}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());
  this->Script("pack %s",
               this->ShowXYPlotToggle->GetWidgetName());

  if (!this->TemporalProbeProxy)
    {
    //create the proxy for the TemporalProbeFilter
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    this->TemporalProbeProxy = vtkSMProxy::SafeDownCast(
      pxm->NewProxy("filters", "TemporalProbe"));
    if (!this->TemporalProbeProxy)
      {
      vtkErrorMacro("Failed to create TemporalProbe Proxy!");
      return;
      }

    vtksys_ios::ostringstream str;
    str << this->GetSourceList() << "."
      << this->GetName() << "."
      << "TemporalProbeProxy";
    this->SetTemporalProbeProxyName(str.str().c_str());
    pxm->RegisterProxy("filters", this->TemporalProbeProxyName, this->TemporalProbeProxy);
    //Register the Proxy to receive events.
    this->Observer = vtkTemporalPickObserver::New();
    this->Observer->SetTemporalProbeProxy(this->TemporalProbeProxy);
    vtkPVAnimationScene *animScene = 
      this->GetPVApplication()->GetMainWindow()->GetAnimationManager()->GetAnimationScene();
    animScene->AddObserver(vtkCommand::StartAnimationCueEvent, this->Observer);
    animScene->AddObserver(vtkCommand::AnimationCueTickEvent, this->Observer);
    }

  if (!this->PlotDisplayProxy)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    this->PlotDisplayProxy = vtkSMXYPlotDisplayProxy::SafeDownCast(
      pxm->NewProxy("displays", "XYPlotDisplay"));
    if (!this->PlotDisplayProxy)
      {
      vtkErrorMacro("Failed to create Plot Display Proxy!");
      return;
      }
    vtksys_ios::ostringstream str;
    // SourceListName.SourceProxyName.XYPlotDisplay == name for the 
    // Display proxy.
    str << this->GetSourceList() << "."
      << this->GetName() << "."
      << "XYPlotDisplay";
    this->SetPlotDisplayProxyName(str.str().c_str());
    pxm->RegisterProxy("displays", this->PlotDisplayProxyName, this->PlotDisplayProxy);
    }
  
  this->ArraySelection->SetParent(this->XYPlotFrame->GetFrame());
  this->ArraySelection->SetPVSource(this);
  this->ArraySelection->SetLabelText("Cell Scalars");
  this->ArraySelection->SetModifiedCommand(this->GetTclName(), "ArraySelectionInternalCallback");

  this->SaveButton->SetParent(this->XYPlotFrame->GetFrame());
  this->SaveButton->Create(pvApp); 
  this->SaveButton->SetCommand(this, "SaveDialogCallback");
  this->SaveButton->SetText("Save as CSV");
  vtkKWLoadSaveDialog *dlg = this->SaveButton->GetLoadSaveDialog();
  dlg->SetDefaultExtension(".csv");
  dlg->SetFileTypes("{{CSV Document} {.csv}}");
  dlg->SaveDialogOn();
  this->Script("pack %s",
               this->SaveButton->GetWidgetName());

  //data values drawn on parameters tab
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
    //TODO: move as much of this as possible into XML

    //on first accept callback, set up auxiliary filters
    this->PointLabelDisplayProxy->SetVisibilityCM(vis);
    this->PointLabelDisplayProxy->SetFontSizeCM(fsize);
    this->SetPointLabelFontSize(fsize);

    //temporal plot
    vtkSMInputProperty* tip = vtkSMInputProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("Input"));
    if (!tip)
      {
      vtkErrorMacro("Failed to find property Input on TemporalProbeProxy.");
      return;
      }
    tip->AddProxy(this->GetProxy());    
    this->TemporalProbeProxy->UpdateVTKObjects();

    this->PlotDisplayProxy->SetPolyOrUGrid(1);
    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("Input"));
    if (!ip)
      {
      vtkErrorMacro("Failed to find property Input on PlotDisplayProxy.");
      return;
      }
    ip->RemoveAllProxies();  
    ip->AddProxy(this->TemporalProbeProxy);
    this->PlotDisplayProxy->SetXAxisLabel(true);
    this->AddDisplayToRenderModule(this->PlotDisplayProxy);
    }

  //choose either point or cell input to the temporal plot
  vtkSMIntVectorProperty *PorCIprop = 
    vtkSMIntVectorProperty::SafeDownCast(this->Proxy->GetProperty("PickCell"));
  int PorC = (PorCIprop)?PorCIprop->GetElement(0):0;
  vtkSMIntVectorProperty *PorCOprop = vtkSMIntVectorProperty::SafeDownCast(
    this->TemporalProbeProxy->GetProperty("PointOrCell"));
  if (PorCOprop) PorCOprop->SetElement(0, PorC);
  this->TemporalProbeProxy->UpdateVTKObjects();

  //pick by Id changes arrays too (by Id doesn't pass Id)
  vtkSMIntVectorProperty *UseIdprop = 
    vtkSMIntVectorProperty::SafeDownCast(this->Proxy->GetProperty("UseIdToPick"));
  int useId = (UseIdprop)?UseIdprop->GetElement(0):0;

  //set up the array selection area and pass names to plot
  if ((this->LastPorC != PorC) || (this->LastUseId != useId))
    {     
    this->LastPorC = PorC;
    this->LastUseId = useId;

    if (initialized) 
      {
      this->Script("pack forget %s", this->ArraySelection->GetWidgetName());  
      
      this->ArraySelection->Delete();

      this->ArraySelection = vtkPVArraySelection::New();
      this->ArraySelection->SetParent(this->XYPlotFrame->GetFrame());
      this->ArraySelection->SetPVSource(this);
      this->ArraySelection->SetLabelText("Cell Scalars");
      this->ArraySelection->SetModifiedCommand(this->GetTclName(), "ArraySelectionInternalCallback");
      }

    int numArrays;
    if (PorC) 
      {
      this->ArraySelection->SetLabelText("Cell Scalars");
      
      numArrays = this->GetDataInformation()->GetCellDataInformation()->GetNumberOfArrays();
      }
    else
      {
      this->ArraySelection->SetLabelText("Point Scalars");
      numArrays = this->GetDataInformation()->GetPointDataInformation()->GetNumberOfArrays();
      }
    
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("ArrayNames"));
    if (svp)
      {     
      vtkSMStringListDomain *arrayList = vtkSMStringListDomain::SafeDownCast(svp->GetDomain( "array_list" ));
      arrayList->RemoveAllStrings();
      
      int e=0;
      for(int i=0; i<numArrays; i++)
        {
        vtkPVArrayInformation* arrayInfo;
        if (PorC)
          {
          arrayInfo = this->GetDataInformation()->GetCellDataInformation()->GetArrayInformation(i);
          }
        else
          {
          arrayInfo = this->GetDataInformation()->GetPointDataInformation()->GetArrayInformation(i);
          }
        
        if( arrayInfo->GetNumberOfComponents() == 1 )
          {
          svp->SetElement(e++, arrayInfo->GetName());
          arrayList->AddString(arrayInfo->GetName());
          }
        }

      svp->SetNumberOfElements(e);

      this->ArraySelection->SetSMProperty(svp);
      this->ArraySelection->Create(this->GetPVApplication());
      }
    else
      {
      vtkErrorMacro("Failed to find property ArrayNames.");
      }
    
    this->Script("pack %s -fill x -expand true", this->ArraySelection->GetWidgetName());  

    //get correct value for plot
    vtkSMProperty *prop = vtkSMProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("AnimateInit"));
    if (prop) prop->Modified();

    vtkPVAnimationScene *animScene = 
      this->GetPVApplication()->GetMainWindow()->GetAnimationManager()->GetAnimationScene();
    double currTime = animScene->GetCurrentTime();

    vtkSMDoubleVectorProperty *prop2 = vtkSMDoubleVectorProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("AnimateTick"));
    if (prop2) prop2->SetElement(0, currTime);
    this->TemporalProbeProxy->UpdateVTKObjects();
    }

  //execute pipeline to have valid data to start off in plot
  this->TemporalProbeProxy->UpdateVTKObjects();
  this->PlotDisplayProxy->Update();
  if (this->ShowXYPlotToggle->GetSelectedState())
    {
    this->PlotDisplayProxy->SetVisibilityCM(1);
    this->SaveButton->SetEnabled(1);
    }
  else
    {
    this->PlotDisplayProxy->SetVisibilityCM(0);
    this->SaveButton->SetEnabled(0);
    }

  //We need to update manually for the case we are probing one point.
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
  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
}

//----------------------------------------------------------------------------
void vtkPVPick::ArraySelectionInternalCallback()
{
  this->ArraySelection->Accept();
  this->PlotDisplayProxy->UpdateVTKObjects();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVPick::SaveDialogCallback()
{
  vtkXYPlotActor *xy = this->PlotDisplayProxy->GetXYPlotWidget()->GetXYPlotActor();
  
  ofstream f;
  const char *filename = this->SaveButton->GetFileName();
  f.open( filename );
  xy->PrintAsCSV(f);
  f.close();
}

