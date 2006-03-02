/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProbe.h"

#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWLabel.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"

#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPVTraceHelper.h"

#include "vtkSMXYPlotDisplayProxy.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVArraySelection.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMPropertyIterator.h"

#include "vtkKWLoadSaveButton.h"
#include "vtkKWLoadSaveDialog.h"

#include "vtkCommand.h"
#include "vtkAnimationCue.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVAnimationManager.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>
 
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.164.2.1");

#define PV_TAG_PROBE_OUTPUT 759362


//*****************************************************************************
class vtkTemporalProbeFilterObserver : public vtkCommand
{
public:
  static vtkTemporalProbeFilterObserver* New()
    {
    return new vtkTemporalProbeFilterObserver;
    }
  void SetTemporalProbeProxy(vtkSMProxy * proxy)
    {
    this->TemporalProbeProxy = proxy;
    }
  void SetPVProbe(vtkPVProbe *probe)
    {
    this->PVProbe = probe;
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
          if (prop) 
            {
            prop->Modified();
            }
          this->TemporalProbeProxy->UpdateVTKObjects();
          break;
          }
        case vtkCommand::AnimationCueTickEvent:
          {
          double timestamp = 0.0;
          vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
            vtkAnimationCue::AnimationCueInfo*>(calldata);
          if (!PVProbe->GetSourceTimeNow(timestamp))
            {
            timestamp = cueInfo->AnimationTime;
            }

          vtkSMDoubleVectorProperty *prop = 
            vtkSMDoubleVectorProperty::SafeDownCast(
            this->TemporalProbeProxy->GetProperty("AnimateTick"));
          if (prop) 
            {
            prop->SetElement(0, timestamp);
            }

          this->TemporalProbeProxy->UpdateVTKObjects();
          break;
          }
        }
      }
    }
protected:
  vtkTemporalProbeFilterObserver()
    {
    this->TemporalProbeProxy = 0;
    this->PVProbe = 0;
    }
  vtkSMProxy* TemporalProbeProxy;
  vtkPVProbe* PVProbe;
};

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  this->SelectedPointFrame = vtkKWFrame::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->ShowXYPlotToggle = vtkKWCheckButton::New();

  
  this->ProbeFrame = vtkKWFrame::New();
  
  
  this->ReplaceInputOff();

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part.
  this->RequiredNumberOfInputParts = 1;
  
  this->PlotDisplayProxy = 0; 
  this->PlotDisplayProxyName = 0;
  this->ArraySelection = vtkPVArraySelection::New();

  this->SaveButton = vtkKWLoadSaveButton::New();

  this->TemporalProbeProxy = 0;
  this->TemporalProbeProxyName = 0;
  this->Observer = 0;
}

//----------------------------------------------------------------------------
vtkPVProbe::~vtkPVProbe()
{  
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

  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;  
  
  this->ShowXYPlotToggle->Delete();
  this->ShowXYPlotToggle = NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;  

  this->ArraySelection->Delete();
  this->ArraySelection = NULL;

  this->SaveButton->Delete();
  this->SaveButton =  NULL;

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
void vtkPVProbe::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
    
  this->Superclass::CreateProperties();
  // We do not support probing groups and multi-block objects. Therefore,
  // we use the first VTKSource id.
  stream << vtkClientServerStream::Invoke 
         <<  this->GetVTKSourceID(0) << "SetSpatialMatch" << 2
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  this->ProbeFrame->SetParent(this->ParameterFrame->GetFrame());
  this->ProbeFrame->Create(pvApp);
  
  this->Script("pack %s -fill x -expand true",
               this->ProbeFrame->GetWidgetName());

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp);

  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp);
  this->SelectedPointLabel->SetText("Point");

  this->Script("pack %s -side left",
               this->SelectedPointLabel->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp);

  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp);
  this->ShowXYPlotToggle->SetText("Show XY-Plot");
  this->ShowXYPlotToggle->SetSelectedState(1);
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
    this->Observer = vtkTemporalProbeFilterObserver::New();
    this->Observer->SetTemporalProbeProxy(this->TemporalProbeProxy);
    this->Observer->SetPVProbe(this);

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

    // We cannot set the input here itself,
    // since the Probe proxy is not yet created and setting it as input
    // calls a CreateParts() on it, which leads to errors.
    // Hence, we defer it until after initializaiton.
    }

  this->ArraySelection->SetParent(this->ProbeFrame);
  this->ArraySelection->SetPVSource(this);
  this->ArraySelection->SetLabelText("Point Scalars");
  this->ArraySelection->SetModifiedCommand(this->GetTclName(), "ArraySelectionInternalCallback");


  // Add a button to save XYPlotActor as CSV file
  this->SaveButton->SetParent(this->ParameterFrame->GetFrame());
  this->SaveButton->Create(pvApp); //, "foo");
  this->SaveButton->SetCommand(this, "SaveDialogCallback");
  this->SaveButton->SetText("Save as CSV");
  vtkKWLoadSaveDialog *dlg = this->SaveButton->GetLoadSaveDialog();
  dlg->SetDefaultExtension(".csv");
  dlg->SetFileTypes("{{CSV Document} {.csv}}");
  dlg->SaveDialogOn();
  this->Script("pack %s",
               this->SaveButton->GetWidgetName());

  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetVisibilityNoTrace(int val)
{
  if (this->PlotDisplayProxy && this->ShowXYPlotToggle->GetSelectedState())
    {
    this->PlotDisplayProxy->SetVisibilityCM(val);
    }
  this->Superclass::SetVisibilityNoTrace(val);
}


//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  this->GetTraceHelper()->AddEntry("[$kw(%s) GetShowXYPlotToggle] SetSelectedState %d",
                      this->GetTclName(),
                      this->ShowXYPlotToggle->GetSelectedState());
 
  int initialized = this->GetInitialized();
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();

  if (!initialized)
    {
    // This needs to be done on first accept, because it can't be done 
    // in CreateProperties.

    vtkSMInputProperty* tip = vtkSMInputProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("Input"));
    if (!tip)
      {
      vtkErrorMacro("Failed to find property Input on TemporalProbeProxy.");
      return;
      }
    tip->AddProxy(this->GetProxy());    

    this->PlotDisplayProxy->SetVisibilityCM(0); // also calls UpdateVTKObjects().
    this->AddDisplayToRenderModule(this->PlotDisplayProxy);

    this->GetTraceHelper()->AddEntry("set kw(%s) [$kw(%s) GetShowXYPlotToggle ]",
                                     this->ShowXYPlotToggle->GetTclName(),
                                     this->GetTclName());
    }


    {
    //get correct value for plot
    vtkSMProperty *prop = vtkSMProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("AnimateInit"));
    if (prop) 
      {
      prop->Modified();
      }

    vtkPVAnimationScene *animScene = 
      this->GetPVApplication()->GetMainWindow()->GetAnimationManager()->GetAnimationScene();
    double currTime = animScene->GetAnimationTime();

    vtkSMDoubleVectorProperty *prop2 = vtkSMDoubleVectorProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("AnimateTick"));
    if (prop2) 
      {
      prop2->SetElement(0, currTime);
      }
    this->TemporalProbeProxy->UpdateVTKObjects();
    }

  // Use this to determine if acting as a point probe or line probe.
  int numPts = this->GetDataInformation()->GetNumberOfPoints();
   
  //Tell the plot which input to take, the probe's for line or the temporal probe's for point.
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->PlotDisplayProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on PlotDisplayProxy.");
    return;
    }
  ip->RemoveAllProxies();  
  if (numPts == 1)
    ip->AddProxy(this->TemporalProbeProxy);
  else
    ip->AddProxy(this->GetProxy());      
  
  if (numPts == 1)
    { // Put the array information in the UI. 
    // Get the collected data from the display.
    this->TemporalProbeProxy->UpdateVTKObjects();
    this->PlotDisplayProxy->Update();
    vtkDataSet* d = this->PlotDisplayProxy->GetCollectedData();
    vtkPointData* pd = d->GetPointData();
  
    // update the ui to see the point data for the probed point
    vtkIdType j, numComponents;

    // use vtkstd::string since 'label' can grow in length arbitrarily
    vtkstd::string label;
    vtkstd::string arrayData;
    vtkstd::string tempArray;

    int numArrays = pd->GetNumberOfArrays();
    for (int i = 0; i < numArrays; i++)
      {
      vtkDataArray* array = pd->GetArray(i);
      if (array->GetName())
        {
        numComponents = array->GetNumberOfComponents();
        int mostRecentSample = array->GetNumberOfTuples() - 1;
        if (numComponents > 1)
          {
          // make sure we fill buffer from the beginning
          vtksys_ios::ostringstream arrayStrm;
          arrayStrm << array->GetName() << ": ( ";
          arrayData = arrayStrm.str();

          for (j = 0; j < numComponents; j++)
            {
            // make sure we fill buffer from the beginning
            vtksys_ios::ostringstream tempStrm;
            tempStrm << array->GetComponent( mostRecentSample, j );
            tempArray = tempStrm.str();

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
          vtksys_ios::ostringstream arrayStrm;
          arrayStrm << array->GetName() << ": " << array->GetComponent( mostRecentSample, 0 ) << endl;

          label += arrayStrm.str();
          }
        }
      }
    this->PointDataLabel->SetText( label.c_str() );

    //make sure placement of pt info and array widgets stay consistant
    if (initialized)
      {
      this->Script("pack forget %s", this->ArraySelection->GetWidgetName());  
      }

    this->Script("pack %s", this->PointDataLabel->GetWidgetName());

    this->PlotDisplayProxy->SetXAxisLabel(true);
    }
  else
    {
    this->PointDataLabel->SetText("");
    this->Script("pack forget %s", this->PointDataLabel->GetWidgetName());

    this->PlotDisplayProxy->SetXAxisLabel(false);
    }

  // Fill up the ArrayNames of the XYPlotActorProxy (subproxy of XYPlotDisplayProxy) from the
  // input of PVProbe
  if (!initialized)
    {
    int numArrays = this->GetDataInformation()->GetPointDataInformation()->GetNumberOfArrays();
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("ArrayNames"));
    if (svp)
      {      
      vtkSMStringListDomain *arrayList = vtkSMStringListDomain::SafeDownCast(svp->GetDomain( "array_list" ));

      int e=0;
      for(int i=0; i<numArrays; i++)
        {
        vtkPVArrayInformation* arrayInfo = 
          this->GetDataInformation()->GetPointDataInformation()->GetArrayInformation(i);
        if( arrayInfo->GetNumberOfComponents() == 1 )
          {
          svp->SetElement(e++, arrayInfo->GetName());
          arrayList->AddString(arrayInfo->GetName());
          }
        }

      this->ArraySelection->SetSMProperty(svp);
      this->ArraySelection->Create(pvApp);
      }
    else
      {
      vtkErrorMacro("Failed to find property ArrayNames.");
      }
    }

  if (this->ShowXYPlotToggle->GetSelectedState() && !(!initialized && (numPts == 1)))
    {
    this->PlotDisplayProxy->SetVisibilityCM(1);
    this->Script("pack %s -fill x -expand true", this->ArraySelection->GetWidgetName());  
    this->SaveButton->SetEnabled(1);
    this->GetTraceHelper()->AddEntry("$kw(%s) SetSelectedState 1",
                                     this->ShowXYPlotToggle->GetTclName());
    }
  else
    {
    this->PlotDisplayProxy->SetVisibilityCM(0);
    this->Script("pack forget %s", this->ArraySelection->GetWidgetName());  
    this->SaveButton->SetEnabled(0);
    this->ShowXYPlotToggle->SetSelectedState(0);
    this->GetTraceHelper()->AddEntry("$kw(%s) SetSelectedState 0",
                                     this->ShowXYPlotToggle->GetTclName());
    }

}
 
//----------------------------------------------------------------------------
void vtkPVProbe::ArraySelectionInternalCallback()
{
  this->ArraySelection->Accept();
  this->PlotDisplayProxy->UpdateVTKObjects();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveInBatchScript(ofstream* file)
{
  if( this->VisitedFlag )
    {
    return;
    }

  this->Superclass::SaveInBatchScript(file);

  *file << endl;
  *file << "  # Save the TemporalProbeProxy" << endl;
  this->SaveTemporalProbeInBatchScript(file);

  *file << endl;
  *file << "  # Save the XY Plot" << endl;
  this->PlotDisplayProxy->SaveInBatchScript(file);

  const char *filename = this->SaveButton->GetFileName();
  if (filename)
    {
    cout << filename << endl;
    *file << "  # Plot's .csv file name is " << filename << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveTemporalProbeInBatchScript(ofstream* file)
{
  // Some displays do not have VTKClassName set and hence only create Subproxies.
  // For such displays we use their self ids. 
  
  unsigned int count = this->TemporalProbeProxy->GetNumberOfIDs();
  vtkClientServerID id = (count)? this->TemporalProbeProxy->GetID(0) : this->TemporalProbeProxy->GetSelfID();
  count = (count)? count : 1;
   
  for (unsigned int kk = 0; kk < count ; kk++)
    {
    if (kk > 0)
      {
      id = this->TemporalProbeProxy->GetID(kk);
      }
    
    *file << endl;
    *file << "set pvTemp" << id
      << " [$proxyManager NewProxy " << this->TemporalProbeProxy->GetXMLGroup() << " "
      << this->TemporalProbeProxy->GetXMLName() << "]" << endl;
    *file << "  $proxyManager RegisterProxy " << this->TemporalProbeProxy->GetXMLGroup()
      << " pvTemp" << id <<" $pvTemp" << id << endl;
    *file << "  $pvTemp" << id << " UnRegister {}" << endl;

    //First set the input
    vtkSMInputProperty* ipp;
    ipp = vtkSMInputProperty::SafeDownCast(
      this->TemporalProbeProxy->GetProperty("Input"));
    if (ipp && ipp->GetNumberOfProxies() > 0)
      {
      *file << "  [$pvTemp" << id << " GetProperty Input] "
        " AddProxy $pvTemp" << ipp->GetProxy(0)->GetID(0)
        << endl;
      }
    else
      {
      *file << "# Input to Display Proxy not set properly or takes no Input." 
        << endl;
      }

    *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PlotDisplayProxy: " << this->PlotDisplayProxy << endl;
  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
  os << indent << "TemporalProbeProxy: " << this->TemporalProbeProxy << endl;
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveDialogCallback()
{
  const char *filename = this->SaveButton->GetFileName();
  if (filename)
    {
    this->PlotDisplayProxy->PrintAsCSV(filename);
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);
  //save the state of the plot display toggle if on
  if (this->ShowXYPlotToggle->GetSelectedState()) 
    {
    *file << "set kw(" << this->ShowXYPlotToggle->GetTclName() << ") [$kw(" << this->GetTclName() << ") GetShowXYPlotToggle ]" << endl;
    *file << "$kw(" << this->ShowXYPlotToggle->GetTclName() << ") SetSelectedState 1" << endl; 
    // Call accept.
    *file << "$kw(" << this->GetTclName() << ") AcceptCallback" << endl;
    }
}

//----------------------------------------------------------------------------
bool vtkPVProbe::GetSourceTimeNow(double &TimeNow)
{
  //trace backward to get the reader
  vtkPVSource *myinput = this->GetPVInput(0);
  vtkPVSource *pinput = myinput->GetPVInput(0);
  while (pinput != NULL)
    {
    myinput = pinput;    
    pinput = myinput->GetPVInput(0);
    }

  //get the readers time values array, and the current index into it
  vtkSMSourceProxy *proxy = myinput->GetProxy();
  vtkSMProperty *prop;
  vtkSMDoubleVectorProperty* tsvProp = NULL;
  vtkSMIntVectorProperty* tsiProp = NULL;
  
  prop = proxy->GetProperty("TimestepValues");
  if (prop) tsvProp = vtkSMDoubleVectorProperty::SafeDownCast(prop);

  prop = proxy->GetProperty("TimeStep");
  if (prop) tsiProp = vtkSMIntVectorProperty::SafeDownCast(prop);

  //if we've got them both, return the real time
  if (tsvProp && tsiProp)
    {
    int index = tsiProp->GetElement(0);
    double *timevalues = tsvProp->GetElements();
    TimeNow = timevalues[index];
    return true;
    }
  return false;

}
