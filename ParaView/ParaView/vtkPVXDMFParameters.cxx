/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVXDMFParameters.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
vtkCxxRevisionMacro(vtkPVXDMFParameters, "1.8.2.8");

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
    pm->SendStreamToServer();
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
  if (this->Application)
    {
    vtkErrorMacro("XDMFParameters already created");
    return;
    }

  this->SetApplication(pvApp);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

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
    pm->SendStreamToServer();  // was a rootscript
    vtkClientServerStream parameters;
    if(!pm->GetLastServerResult().GetArgument(0, 0, &parameters))
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
      this->AddXDMFParameter(name, index, range[0], range[1], range[2]);
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
void vtkPVXDMFParameters::AcceptInternal(vtkClientServerID)
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();

  int some = 0;
  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScale* scale = (vtkKWScale*)it->GetObject();
    const char* label = scale->GetShortLabel();
    vtkPVXDMFParametersInternals::Parameter* par =
      this->Internals->GetParameter(label);
    par->Value = static_cast<int>(scale->GetValue());
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->VTKReaderID << "SetParameterIndex"
                    << label << par->Value
                    << vtkClientServerStream::End;
    some = 1;
    }
  if ( some )
    {
    pm->SendStreamToServer();
    }
  this->UpdateFromReader();
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
  pm->SendStreamToServer();
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
    pvs->VTKReaderID = pvSource->GetVTKSourceID();
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
void vtkPVXDMFParameters::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai, const char *name)
{
  char script[1024];

  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s) {%s}", 
      this->GetTclName(), ai->GetTclName(), name);
    }

  sprintf(script, "%s SetParameterIndex {%s} $pvTime", this->GetTclName(),
          name);
  vtkPVXDMFParametersInternals::Parameter *p = this->Internals->GetParameter(name);
  ai->SetLabelAndScript(this->GetTraceName(), script);
  ai->SetTimeStart(0);
  //ai->SetCurrentTime(p->Value);
  ai->SetTimeEnd(p->Max);
  ai->SetTypeToInt();
  sprintf(script, "AnimationMenuCallback $kw(%s)", 
    ai->GetTclName());
  ai->SetSaveStateScript(script);
  ai->SetSaveStateObject(this);
  ai->Update();
  //cout << "Set time to: " << ai->GetTimeStart() << " - " << ai->GetTimeEnd() << endl;
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::SaveInBatchScript(ofstream *file)
{
  if (!this->VTKReaderID.ID)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  if ( ! this->InitializeTrace(file))
    {
    return;
    }
  vtkCollectionIterator* it = this->Internals->GetWidgetsIterator();
  *file << "\t" << "pvTemp" << this->VTKReaderID.ID << " UpdateInformation" << endl;
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem() )
    {
    vtkKWScale* scale = (vtkKWScale*)it->GetObject();
    const char* label = scale->GetShortLabel();
    //cout << "Looking at scale: " << label << endl;
    vtkPVXDMFParametersInternals::Parameter* p = this->Internals->GetParameter(label);
    *file << "\t" << "pvTemp" << this->VTKReaderID.ID << " SetParameterIndex {" << label << "} "
      << p->Value << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVXDMFParameters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VTKReaderID: " << this->VTKReaderID.ID << endl;
  os << indent << "Frame: " << this->Frame << endl;
}
