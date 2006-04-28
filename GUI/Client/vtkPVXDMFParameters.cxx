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
#include "vtkCommand.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkPVTraceHelper.h"

#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
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
      parent->GetFrame()->GetFrame()->GetWidgetName());
    vtkCollectionIterator* sit = this->GetWidgetsIterator();
    for ( sit->InitTraversal(); !sit->IsDoneWithTraversal(); sit->GoToNextItem() )
      {
      vtkKWScaleWithEntry* scale = (vtkKWScaleWithEntry*)sit->GetCurrentObject();
      if ( scale )
        {
        scale->SetParent(0);
        }
      }
    this->Widgets->RemoveAllItems();
    ParametersMap::iterator it;
    vtkKWScaleWithEntry* scale;
    for ( it = this->Parameters.begin(); it != this->Parameters.end(); it ++ )
      {
      Parameter* p = &it->second;
      const vtkstd::string *name  = &it->first;
      scale = vtkKWScaleWithEntry::New();
      scale->SetParent(parent->GetFrame()->GetFrame());
      scale->SetRange(p->Min, p->Max);
      scale->SetResolution(1);
      scale->Create();
      scale->RangeVisibilityOn();
      scale->SetValue(p->Value);
      scale->SetLabelText(name->c_str());
      scale->SetCommand(parent, "ScaleModifiedCallback");
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
vtkCxxRevisionMacro(vtkPVXDMFParameters, "1.46");

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
    vtkProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    vtkClientServerStream stream;
    pm->DeleteStreamObject(this->ServerSideID, stream);
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::CheckModifiedCallback()
{
  this->ModifiedCallback();
  this->AcceptedCallback();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::ScaleModifiedCallback(double)
{
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->Frame = vtkKWFrameWithLabel::New();
  this->Frame->SetParent(this);
  this->Frame->Create();
  this->Frame->SetLabelText(this->FrameLabel);
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

  const char* sourceID = this->PVSource->GetProxy()->GetSelfIDAsString();

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    unsigned int numStrings = svp->GetNumberOfElements();
    *file << "  [$pvTemp" << sourceID 
          << " GetProperty ParameterIndex] SetNumberOfElements "
          << numStrings << endl;
    
    unsigned int idx;
    for(idx=0; idx<numStrings; idx++)
      {
      *file << "  [$pvTemp" << sourceID 
            << " GetProperty ParameterIndex] SetElement " << idx
            <<  " " << svp->GetElement(idx) << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::Accept()
{
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
        vtkKWScaleWithEntry* scale = 
          (vtkKWScaleWithEntry*)it->GetCurrentObject();
        const char* label = scale->GetLabelText();
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
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    }

  this->Superclass::Accept();
}

//---------------------------------------------------------------------------
void vtkPVXDMFParameters::SetParameterIndex(const char* label, int value)
{
  vtkProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->VTKReaderID << "SetParameterIndex" << label << value
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER, stream);
}

//---------------------------------------------------------------------------
void vtkPVXDMFParameters::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }
  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScaleWithEntry* scale = (vtkKWScaleWithEntry*)it->GetCurrentObject();
    const char* label = scale->GetLabelText();
    //cout << "Looking at scale: " << label << endl;
    vtkPVXDMFParametersInternals::Parameter* p = this->Internals->GetParameter(label);
    *file << "$kw(" << this->GetTclName() << ") SetParameterIndex {"
          << label << "} " << p->Value << endl;
    }
}


//----------------------------------------------------------------------------
void vtkPVXDMFParameters::Initialize()
{
  this->UpdateParameters(1);
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::ResetInternal()
{
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
    this->Frame->SetLabelText(this->FrameLabel);
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

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame);

  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScaleWithEntry* scale = (vtkKWScaleWithEntry*)it->GetCurrentObject();
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
