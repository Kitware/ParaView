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
vtkCxxRevisionMacro(vtkPVXDMFParameters, "1.21.2.1");

//----------------------------------------------------------------------------
vtkPVXDMFParameters::vtkPVXDMFParameters()
{
  this->Internals = vtkPVXDMFParametersInternals::New();
  this->Frame = 0;
  this->FrameLabel = 0;
  this->SetFrameLabel("Parameters");
  this->VTKReaderID.ID = 0;
  this->ServerSideID.ID = 0;
  this->Property = 0;
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
void vtkPVXDMFParameters::UpdateFromReader()
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  if(this->VTKReaderID.ID)
    {
    // Create server-side helper if necessary.
    if(!this->ServerSideID.ID)
      {
      this->ServerSideID = pm->NewStreamObject("vtkPVServerXDMFParameters");
      }

    // Get the parameters from the server.
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ServerSideID << "GetParameters"
                    << this->VTKReaderID
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    vtkClientServerStream parameters;
    if(!pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &parameters))
      {
      vtkErrorMacro("Error getting parameters from server.");
      return;
      }

    // Add each parameter locally.
    int numParameters = parameters.GetNumberOfArguments(0)/3;
    for(int i=0; i < numParameters; ++i)
      {
      const char* name;
      int index;
      int range[3];
      if(!parameters.GetArgument(0, 3*i, &name))
        {
        vtkErrorMacro("Error parsing parameter name.");
        return;
        }
      if(!parameters.GetArgument(0, 3*i + 1, &index))
        {
        vtkErrorMacro("Error parsing parameter index.");
        return;
        }
      if(!parameters.GetArgument(0, 3*i + 2, range, 3))
        {
        vtkErrorMacro("Error parsing parameter range.");
        return;
        }
      // Use the property if available. This is done so that reset
      // is based on the property not the reader. However, we still
      // have to obtain the range from the reader since currently
      // properties do not have ranges.
      int numStrings = this->Property->GetNumberOfStrings();
      int idx;
      for(idx=0; idx<numStrings; idx++)
        {
        if ( strcmp(name, this->Property->GetString(idx)) == 0)
          {
          break;
          }
        }
      if (idx < numStrings)
        {
        this->AddXDMFParameter(
          name, 
          static_cast<int>(this->Property->GetScalar(idx)), 
          range[0], range[1], range[2]);
        }
      else
        {
        this->AddXDMFParameter(name, index, range[0], range[1], range[2]);
        }
      }
    this->Internals->Update(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::AddXDMFParameter(const char* pname, int value, int min, int step, int max)
{
  this->Internals->AddParameter(pname, value, min, step, max);
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVStringAndScalarListWidgetProperty::SafeDownCast(prop);
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVXDMFParameters::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVXDMFParameters::CreateAppropriateProperty()
{
  return vtkPVStringAndScalarListWidgetProperty::New();
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

  int numStrings = this->Property->GetNumberOfStrings();
  *file << "  [$pvTemp" << sourceID.ID 
        << " GetProperty ParameterIndex] SetNumberOfElements "
        << numStrings*2 << endl;;

  int idx;
  for(idx=0; idx<numStrings; idx++)
    {
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty ParameterIndex] SetElement " << idx*2
          <<  " " << this->Property->GetString(idx) << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty ParameterIndex] SetElement " << idx*2+1
          <<  " " << static_cast<int>(this->Property->GetScalar(idx)) << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::AcceptInternal(vtkClientServerID)
{
  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();

  int numParams = 0;
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    numParams++;
    }

  if (numParams > 0)
    {
    char **cmds = new char*[numParams];
    char **strings = new char*[numParams];
    float *scalars = new float[numParams];
    int *numStrings = new int[numParams];
    int *numScalars = new int[numParams];

    int idx=0;
    for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
      {
      vtkKWScale* scale = (vtkKWScale*)it->GetObject();
      const char* label = scale->GetShortLabel();
      vtkPVXDMFParametersInternals::Parameter* par =
        this->Internals->GetParameter(label);
      par->Value = static_cast<int>(scale->GetValue());

      cmds[idx] = new char[strlen("SetParameterIndex")+1];
      strcpy (cmds[idx] , "SetParameterIndex");

      strings[idx] = new char[strlen(label)+1];
      strcpy (strings[idx], label);

      scalars[idx] = par->Value;

      numStrings[idx] = 1;

      numScalars[idx] = 1;

      idx++;
      }

    this->Property->SetVTKCommands(numParams, cmds, numStrings, numScalars);
    this->Property->SetStrings(numParams, strings);
    this->Property->SetScalars(numParams, scalars);
    this->Property->SetVTKSourceID(this->VTKReaderID);
    this->Property->AcceptInternal();

    for (idx=0; idx<numParams; idx++)
      {
      delete[] cmds[idx];
      delete[] strings[idx];
      }
    delete[] cmds;
    delete[] strings;
    delete[] scalars;
    delete[] numStrings;
    delete[] numScalars;
    }

  this->ModifiedFlag = 0;
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
  this->UpdateFromReader();
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
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
void vtkPVXDMFParameters::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                  vtkPVAnimationInterfaceEntry *ai)
{
  if ( !this->VTKReaderID.ID )
    {
    return;
    }

  char methodAndArgs[1024];
  char name[1024];
  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  for ( it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScale* scale = (vtkKWScale*)it->GetObject();
    sprintf(methodAndArgs, "AnimationMenuCallback %s %s", ai->GetTclName(), scale->GetShortLabel());
    sprintf(name, "%s_%s", this->GetTraceName(), scale->GetShortLabel());
    menu->AddCommand(name, this, methodAndArgs, 0,"");
    }
}

//-----------------------------------------------------------------------------
void vtkPVXDMFParameters::AnimationMenuCallback(
  vtkPVAnimationInterfaceEntry *ai, const char *name)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s) {%s}", 
      this->GetTclName(), ai->GetTclName(), name);
    }

  vtkPVXDMFParametersInternals::Parameter *p = 
    this->Internals->GetParameter(name);
  ai->SetLabelAndScript(this->GetTraceName(), NULL, this->GetTraceName());
  ai->SetCurrentProperty(this->Property);
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
