/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXDMFParameters.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXDMFParameters.h"

#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkPVStringAndScalarListWidgetProperty.h"

#include "vtkSMProperty.h"
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/string>
#include <vtkstd/map>

//============================================================================
//----------------------------------------------------------------------------
class vtkPVXDMFParametersInternals : public vtkObject
{
public:
  vtkTypeMacro(vtkPVXDMFParametersInternals, vtkObject);
  static vtkPVXDMFParametersInternals* New();

  struct Parameter
    {
    int Value;
    int Min;
    int Step;
    int Max;
    };

  typedef vtkstd::map<vtkstd::string, Parameter> 
    ParametersMap;

  void AddParameter(const char* pname, int value, int min, int step, int max)
    {
    Parameter* p =  &this->Parameters[pname];
    p->Value = value;
    p->Min = min;
    p->Step = step;
    p->Max = max;
    if ( p->Value < p->Min )
      {
      p->Value = p->Min;
      }
    if ( p->Value > p->Max )
      {
      p->Value = p->Max;
      }
    //cout << "Add XDMF Parameter: " << pname 
    //  << " = " << p->Value << " (" << p->Min << ", " << p->Step << ", " << p->Max << ")" << endl;
    }

  void Update(vtkPVXDMFParameters* parent)
    {
    // Clear out any old check buttons.
    parent->Script("catch {eval pack forget [pack slaves %s]}",
      parent->GetFrame()->GetFrame()->GetFrame()->GetWidgetName());
    vtkCollectionIterator* sit = this->GetWidgetsIterator();
    for ( sit->InitTraversal(); !sit->IsDoneWithTraversal(); sit->GoToNextItem() )
      {
      vtkKWScale* scale = (vtkKWScale*)sit->GetObject();
      if ( scale )
        {
        scale->SetParent(0);
        }
      }
    this->Widgets->RemoveAllItems();
    ParametersMap::iterator it;
    vtkKWScale* scale;
    for ( it = this->Parameters.begin(); it != this->Parameters.end(); it ++ )
      {
      Parameter* p = &it->second;
      const vtkstd::string *name  = &it->first;
      scale = vtkKWScale::New();
      scale->SetParent(parent->GetFrame()->GetFrame()->GetFrame());
      scale->SetRange(p->Min, p->Max);
      scale->SetResolution(1);
      scale->Create(parent->GetApplication(), 0);
      scale->DisplayRangeOn();
      scale->DisplayEntry();
      scale->SetValue(p->Value);
      scale->DisplayLabel(name->c_str());
      scale->SetCommand(parent, "ModifiedCallback");
      parent->Script("pack %s -fill x -expand 1 -side top", scale->GetWidgetName());
      this->Widgets->AddItem(scale);
      scale->Delete();
      }
    }
  vtkGetObjectMacro(WidgetsIterator, vtkCollectionIterator);

  Parameter* GetParameter(const char* key)
    {
    if ( this->Parameters.find(key) == this->Parameters.end() )
      {
      return 0;
      }
    return &this->Parameters[key];
    }

protected:
  vtkPVXDMFParametersInternals()
    {
    this->Widgets = vtkCollection::New();
    this->WidgetsIterator = this->Widgets->NewIterator();
    }

  ~vtkPVXDMFParametersInternals()
    {
    this->WidgetsIterator->Delete();
    this->Widgets->Delete();
    }

  ParametersMap Parameters;
  vtkCollection* Widgets;
  vtkCollectionIterator* WidgetsIterator;

private:
  vtkPVXDMFParametersInternals(const vtkPVXDMFParametersInternals&);
  void operator=(const vtkPVXDMFParametersInternals&);
};
//----------------------------------------------------------------------------
//============================================================================
vtkStandardNewMacro(vtkPVXDMFParametersInternals);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVXDMFParameters);
vtkCxxRevisionMacro(vtkPVXDMFParameters, "1.23");

//----------------------------------------------------------------------------
vtkPVXDMFParameters::vtkPVXDMFParameters()
{
  this->Internals = vtkPVXDMFParametersInternals::New();
  this->Frame = 0;
  this->FrameLabel = 0;
  this->SetFrameLabel("Parameters");
  this->VTKReaderID.ID = 0;
  this->ServerSideID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVXDMFParameters::~vtkPVXDMFParameters()
{
  if ( this->Frame )
    {
    this->Frame->Delete();
    this->Frame = 0;
    }

  this->Internals->Delete();
  this->Internals = 0;

  this->SetFrameLabel(0);
  if(this->ServerSideID.ID)
    {
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    pm->DeleteStreamObject(this->ServerSideID);
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::CheckModifiedCallback()
{
  this->ModifiedCallback();
  this->AcceptedCallback();
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Frame = vtkKWLabeledFrame::New();
  this->Frame->SetParent(this);
  this->Frame->Create(pvApp, 0);
  this->Frame->SetLabel(this->FrameLabel);
  this->Script("pack %s -fill both -expand 1", this->Frame->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVXDMFParameters::UpdateParameters(int fromReader)
{

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    vtkSMStringListRangeDomain* dom = vtkSMStringListRangeDomain::SafeDownCast(
      svp->GetDomain("range"));
    if (dom)
      {
      unsigned int numStrings = dom->GetNumberOfStrings();
      
      // Obtain parameters from the domain (that obtained them
      // from the information property that obtained them from the server)
      for(unsigned int i=0; i < numStrings; ++i)
        {
        // Min, max and name are always taken from the domain
        int minexists, maxexists;
        int min = dom->GetMinimum(i, minexists);
        int max = dom->GetMaximum(i, maxexists);
        const char* name = dom->GetString(i);
        if (minexists && maxexists && name)
          {
          int val=0;
          // We are updating the value from the property not
          // the domain
          if (!fromReader)
            {
            // Find the right property comparing the name
            // to the property value
            int found=0;
            unsigned int idx = svp->GetElementIndex(name, found);
            if (found)
              {
              val = atoi(svp->GetElement(idx+1));
              }
            else
              {
              vtkErrorMacro("Could not find an appropriate property value "
                            "matching the domain. Can not update widget.");
              }
            }
          this->AddXDMFParameter(name, val, min, 1, max);
          }
        }
      }
    else
      {
      vtkErrorMacro("An appropriate domain (name: range) is not specified. "
                    "Can not update");
      }
    this->Internals->Update(this);
    }
  else
    {
    vtkErrorMacro("An appropriate property not specified. "
                  "Can not update");
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::AddXDMFParameter(const char* pname, int value, int min, int step, int max)
{
  this->Internals->AddParameter(pname, value, min, step, max);
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::SaveInBatchScript(ofstream *file)
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("SaveInBatchScript requires a PVSource.")
    return;
    }

  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    unsigned int numStrings = svp->GetNumberOfElements();
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty ParameterIndex] SetNumberOfElements "
          << numStrings << endl;
    
    unsigned int idx;
    for(idx=0; idx<numStrings; idx++)
      {
      *file << "  [$pvTemp" << sourceID.ID 
            << " GetProperty ParameterIndex] SetElement " << idx
            <<  " " << svp->GetElement(idx) << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::Accept()
{
  int modFlag = this->GetModifiedFlag();

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();

    int numParams = 0;
    for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
      {
      numParams++;
      }

    svp->SetNumberOfElements(0);
    if (numParams > 0)
      {
      svp->SetNumberOfElements(2*numParams);
      int idx=0;
      for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
        {
        vtkKWScale* scale = (vtkKWScale*)it->GetObject();
        const char* label = scale->GetShortLabel();
        vtkPVXDMFParametersInternals::Parameter* par =
          this->Internals->GetParameter(label);
        par->Value = static_cast<int>(scale->GetValue());
        svp->SetElement(2*idx, label);
        char value[128];
        sprintf(value, "%d", static_cast<int>(scale->GetValue()));
        svp->SetElement(2*idx+1, value);
        idx++;
        }
      }
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    }

  this->ModifiedFlag = 0;

  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//---------------------------------------------------------------------------
void vtkPVXDMFParameters::SetParameterIndex(const char* label, int value)
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke
                  << this->VTKReaderID << "SetParameterIndex"
                  << label << value
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}

//---------------------------------------------------------------------------
void vtkPVXDMFParameters::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }
  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScale* scale = (vtkKWScale*)it->GetObject();
    const char* label = scale->GetShortLabel();
    //cout << "Looking at scale: " << label << endl;
    vtkPVXDMFParametersInternals::Parameter* p = this->Internals->GetParameter(label);
    *file << "$kw(" << this->GetTclName() << ") SetParameterIndex {"
          << label << "} " << p->Value << endl;
    }
}


//----------------------------------------------------------------------------
void vtkPVXDMFParameters::ResetInternal()
{
  if (!this->AcceptCalled)
    {
    this->UpdateParameters(1);
    return;
    }
  this->UpdateParameters(0);
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVXDMFParameters* vtkPVXDMFParameters::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVXDMFParameters::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVXDMFParameters* pvs = vtkPVXDMFParameters::SafeDownCast(clone);
  if (pvs)
    {
    pvs->VTKReaderID = pvSource->GetVTKSourceID(0);
    //float min, max;
    //this->Scale->GetRange(min, max);
    //pvs->SetRange(min, max);
    //pvs->SetResolution(this->Scale->GetResolution());
    //pvs->SetLabel(this->EntryLabel);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to XDMFParameters.");
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::SetLabel(const char* label)
{
  this->SetFrameLabel(label);
  if ( this->Frame )
    {
    this->Frame->SetLabel(this->FrameLabel);
    }
}

//----------------------------------------------------------------------------
int vtkPVXDMFParameters::ReadXMLAttributes(vtkPVXMLElement* element,
                                  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->SetLabel(label);

  return 1;

}

//-----------------------------------------------------------------------------
void vtkPVXDMFParameters::AddAnimationScriptsToMenu(
  vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai)
{
  if ( !this->VTKReaderID.ID )
    {
    return;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    char methodAndArgs[1024];
    char name[1024];
    unsigned int numProps = svp->GetNumberOfElements()/2;
    for (unsigned int i=0; i<numProps; i++)
      {
      sprintf(methodAndArgs, 
              "AnimationMenuCallback %s %s %d", 
              ai->GetTclName(), 
              svp->GetElement(2*i),
              i);
      sprintf(name, "%s_%s", this->GetTraceName(), svp->GetElement(2*i));
      menu->AddCommand(name, this, methodAndArgs, 0,"");
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVXDMFParameters::AnimationMenuCallback(
  vtkPVAnimationInterfaceEntry *ai, const char *name, unsigned int idx)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s) {%s}", 
                        this->GetTclName(), ai->GetTclName(), name);
    }
  
  vtkPVXDMFParametersInternals::Parameter *p = 
    this->Internals->GetParameter(name);
  char label[1024];
  sprintf(label, "%s_%s", this->GetTraceName(), name);
  ai->SetLabelAndScript(label, NULL, this->GetTraceName());

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDomain *rangeDomain = prop->GetDomain("range");
  
  ai->SetCurrentSMProperty(prop);
  ai->SetCurrentSMDomain(rangeDomain);
  ai->SetAnimationElement(idx);
  ai->SetTimeStart(p->Min);
  ai->SetTimeEnd(p->Max);
  ai->SetTypeToInt();
  ai->Update();
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame);

  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScale* scale = (vtkKWScale*)it->GetObject();
    this->PropagateEnableState(scale);
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VTKReaderID: " << this->VTKReaderID.ID << endl;
  os << indent << "Frame: " << this->Frame << endl;
}
