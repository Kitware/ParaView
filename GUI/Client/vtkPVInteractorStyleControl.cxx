/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleControl.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInteractorStyleControl.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVCameraManipulator.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVScale.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWidget.h"
#include "vtkString.h"
#include "vtkTclUtil.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"

#include <vtkstd/vector>
#include <vtkstd/map>

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVInteractorStyleControl );
vtkCxxRevisionMacro(vtkPVInteractorStyleControl, "1.40");

vtkCxxSetObjectMacro(vtkPVInteractorStyleControl,ManipulatorCollection,
                     vtkCollection);

//===========================================================================
//***************************************************************************
class vtkPVInteractorStyleControlCmd : public vtkCommand
{
public:
  static vtkPVInteractorStyleControlCmd *New() 
    {return new vtkPVInteractorStyleControlCmd;};

  vtkPVInteractorStyleControlCmd()
    {
      this->InteractorStyleControl = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->InteractorStyleControl )
        {
        this->InteractorStyleControl->ExecuteEvent(wdg, event, calldata);
        }
    }

  vtkPVInteractorStyleControl* InteractorStyleControl;
};
//***************************************************************************
//===========================================================================

//===========================================================================
//***************************************************************************
class vtkPVInteractorStyleControlInternal
{
public:
//BTX
  typedef vtkstd::vector<vtkStdString> ArrayString;
  typedef vtkstd::map<vtkStdString,vtkSmartPointer<vtkPVCameraManipulator> > ManipulatorMap;
  typedef vtkstd::map<vtkStdString,vtkSmartPointer<vtkPVWidget> > WidgetsMap;
  typedef vtkstd::map<vtkStdString,ArrayString> MapStringToArrayString;

  ManipulatorMap          Manipulators;
  WidgetsMap              Widgets;
  MapStringToArrayString Arguments;
//ETX


};
//***************************************************************************
//===========================================================================

//-----------------------------------------------------------------------------
vtkPVInteractorStyleControl::vtkPVInteractorStyleControl()
{
  this->Internals = new vtkPVInteractorStyleControlInternal;
  this->InEvent = 0;
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->LabeledFrame->SetParent(this);
  this->OuterFrame = 0;

  this->Observer = vtkPVInteractorStyleControlCmd::New();
  this->Observer->InteractorStyleControl = this;
  
  int cc;

  for ( cc = 0; cc < 6; cc ++ )
    {
    this->Labels[cc] = vtkKWLabel::New();
    }
  for ( cc = 0; cc < 9; cc ++ )
    {
    this->Menus[cc] = vtkKWOptionMenu::New();
    }

  this->ManipulatorCollection = 0;
  this->DefaultManipulator = 0;
  this->RegisteryName = 0;

  this->ArgumentsFrame = vtkKWFrame::New();

  this->CurrentManipulator = 0;

}

//-----------------------------------------------------------------------------
vtkPVInteractorStyleControl::~vtkPVInteractorStyleControl()
{
  if ( this->ManipulatorCollection )
    {
    vtkCollectionIterator *it = this->ManipulatorCollection->NewIterator();
    it->InitTraversal();
    while(!it->IsDoneWithTraversal())
      {
      vtkPVCameraManipulator* m = static_cast<vtkPVCameraManipulator*>(
        it->GetCurrentObject());
      m->RemoveObserver(this->Observer);
      it->GoToNextItem();
      }
    it->Delete();
    this->SetManipulatorCollection(0);
    }
  // So that events will not be called.
  this->InEvent = 1;
  this->StoreRegistery();
  int cc;
  if ( this->LabeledFrame )
    {
    this->LabeledFrame->Delete();
    this->LabeledFrame = 0;
    }
  if ( this->OuterFrame )
    {
    this->OuterFrame->Delete();
    this->OuterFrame = 0;
    }
  for ( cc = 0; cc < 6; cc ++ )
    {
    this->Labels[cc]->Delete();
    }
  for ( cc = 0; cc < 9; cc ++ )
    {
    this->Menus[cc]->Delete();
    }

  this->SetDefaultManipulator(0);
  this->SetRegisteryName(0);
  
  this->ArgumentsFrame->Delete();
  this->Observer->Delete();

  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::AddManipulator(const char* name, 
                                                 vtkPVCameraManipulator* object)
{
  this->Internals->Manipulators[name] = object;
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::UpdateMenus()
{
  if ( this->GetApplication() )
    {
    this->ReadRegistery();
    vtkPVInteractorStyleControlInternal::ManipulatorMap::iterator it;
    int cc;
    for ( cc = 0; cc < 9; cc ++ )
      {
      this->Menus[cc]->DeleteAllEntries();
      char command[100];
      for ( it = this->Internals->Manipulators.begin();
        it != this->Internals->Manipulators.end();
        ++it )
        {
        sprintf(command, "SetCurrentManipulator %d {%s}", cc, it->first.c_str());
        this->Menus[cc]->AddEntryWithCommand(it->first.c_str(), this, command);
        }
      if ( this->GetManipulator(cc) == 0 && this->DefaultManipulator )
        {
        this->SetCurrentManipulator(cc, this->DefaultManipulator);
        }
      }
    }

  if ( this->ArgumentsFrame->IsCreated() )
    {
    this->Script("catch { eval pack forget [ pack slaves %s ] }",
                 this->ArgumentsFrame->GetWidgetName());

    vtkPVInteractorStyleControlInternal::WidgetsMap::iterator it;
    for ( it = this->Internals->Widgets.begin();
      it != this->Internals->Widgets.end();
      ++it )
      {
      if ( !it->second->IsCreated() )
        {
        it->second->SetParent(this->ArgumentsFrame);
        it->second->Create(this->GetApplication());
        ostrstream str;
        str << "ChangeArgument " << it->first.c_str() << " " 
          << it->second->GetTclName() << ends;
        it->second->SetAcceptedCommand(this->GetTclName(), str.str());
        str.rdbuf()->freeze(0);

        char manipulator[100];
        char buffer[100];
        sprintf(manipulator, "Manipulator%s", it->first.c_str());
        if ( this->GetApplication()->GetRegisteryValue(2, "RunTime", manipulator,
            buffer) &&
          *buffer > 0 )
          {
          vtkPVScale *sc = vtkPVScale::SafeDownCast(it->second.GetPointer());
          if ( sc )
            {
            this->Script("%s SetValue %s", sc->GetTclName(),
              buffer);
            }
          }
        }
      this->Script("pack %s -fill x -expand true -side top",
        it->second->GetWidgetName());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::ExecuteEvent(
  vtkObject* wdg, unsigned long event, void* calldata)
{
  if ( this->InEvent )
    {
    return;
    }
  this->InEvent = 1;

  if ( event == vtkKWEvent::ManipulatorModifiedEvent )
    {
    const char* argument = static_cast<char*>(calldata);

    vtkPVCameraManipulator* manipulator = static_cast<vtkPVCameraManipulator*>(wdg);
    const char* name = manipulator->GetManipulatorName();
  
    vtkPVInteractorStyleControlInternal::MapStringToArrayString::iterator ait = 
      this->Internals->Arguments.find(argument);
    if ( ait != this->Internals->Arguments.end() )
      {
      vtkPVInteractorStyleControlInternal::ArrayString::iterator vit;
      for ( vit = ait->second.begin();
        vit != ait->second.end();
        ++ vit )
        {
        if ( *vit == name )
          {
          this->ResetWidget(manipulator, argument);
          }
        }
      }
    }
  this->InEvent = 0;
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::ChangeArgument(const char* name, 
                                                 const char* swidget)
{
  vtkPVInteractorStyleControlInternal::MapStringToArrayString::iterator sit
    = this->Internals->Arguments.find(name);
  if ( sit == this->Internals->Arguments.end() )
    {
    return;
    }

  int error =0;
  vtkPVWidget *widget = static_cast<vtkPVWidget*>(
    vtkTclGetPointerFromObject(swidget, "vtkPVWidget", 
                               this->GetApplication()->GetMainInterp(), error));
  if ( !widget )
    {
    vtkErrorMacro("Change argument called without valid widget");
    return;
    }
  vtkPVScale* scale = vtkPVScale::SafeDownCast(widget);
  vtkPVVectorEntry* vectorEntry = vtkPVVectorEntry::SafeDownCast(widget);
  char* value = 0;
  if ( scale )
    {
    ostrstream str;
    str << "[ " << scale->GetTclName() << " GetValue ]" << ends;
    value = vtkString::Duplicate(str.str());
    str.rdbuf()->freeze(0);
    }
  else if ( vectorEntry )
    {
    int cc;
    float f[6];
    vectorEntry->GetValue(f, vectorEntry->GetVectorLength());
    ostrstream str;
    str << "{";
    for ( cc = 0; cc < vectorEntry->GetVectorLength(); cc ++ )
      {
      str << f[cc] << " ";
      }
    str << "}" <<ends;
    value = vtkString::Duplicate(str.str());
    str.rdbuf()->freeze(0);
    }
  else
    {
    cout << "Unknown widget" << endl;
    return;
    }

  int found = 0;
  
  vtkPVInteractorStyleControlInternal::ArrayString::iterator vit;
  for ( vit = sit->second.begin();
    vit != sit->second.end();
    ++vit)
    {
    vtkCollectionIterator *cit = this->ManipulatorCollection->NewIterator();
    cit->InitTraversal();
    while ( !cit->IsDoneWithTraversal() )
      {
      vtkPVCameraManipulator* cman 
        = static_cast<vtkPVCameraManipulator*>(cit->GetCurrentObject());
      if ( *vit == cman->GetManipulatorName() )
        {
        this->CurrentManipulator = cman;
        this->Script("eval [ %s GetCurrentManipulator ] Set%s %s", 
          this->GetTclName(), name, value );
        this->CurrentManipulator = 0;
        found = 1;
        }
      cit->GoToNextItem();
      }
    cit->Delete();
    }

  if ( found )
    {
    // This is a hack. 
    if ( vtkString::Length(value) > 0 && !vectorEntry ) 
      {
      const char* val = this->GetApplication()->EvaluateString("%s", value);
      char *rname = vtkString::Append("Manipulator", name);      
      this->GetApplication()->SetRegisteryValue(2, "RunTime", rname, val);
      delete[] rname;
      }
    }
  delete [] value;
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SetCurrentManipulator(
  int mouse, int key, const char* name)
{
  if ( mouse < 0 || mouse > 2 || key < 0 || key > 2 )
    {
    vtkErrorMacro("Setting manipulator to the wrong key or mouse");
    return;
    }
  this->SetCurrentManipulator(mouse + key * 3, name);
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SetCurrentManipulator(
  int pos, const char* name)
{
  this->AddTraceEntry("$kw(%s) SetCurrentManipulator %d {%s}",
                      this->GetTclName(), pos, name);
  
  this->SetManipulator(pos, name);
  if ( pos < 0 || pos > 8 || !this->ManipulatorCollection )
    {
    return;
    }
  vtkPVCameraManipulator *manipulator = this->GetManipulator(name);
  if ( !manipulator )
    {
    return;
    }

  // Figure out mouse and keys layout
  int mouse = pos % 3;
  int key = static_cast<int>(pos / 3);
  int shift = (key == 1);
  int control = (key == 2);

  vtkCollectionIterator *it = this->ManipulatorCollection->NewIterator();
  it->InitTraversal();

  vtkPVCameraManipulator *clone = 0;
  while(!it->IsDoneWithTraversal())
    {
    vtkPVCameraManipulator* access 
      = static_cast<vtkPVCameraManipulator*>(it->GetCurrentObject());
    
    // Find previous one that matches the layout
    if ( access->GetButton() == mouse+1 &&
         access->GetShift() == shift &&
         access->GetControl() == control )
      {
      // If this is the same one, then just assign it.
      if ( vtkString::Equals(access->GetClassName(), 
                             manipulator->GetClassName()) )
        {
        clone = access;
        }
      else
        {
        // Otherwise remove it
        access->SetApplication(0);
        access->RemoveObserver(this->Observer);
        this->ManipulatorCollection->RemoveItem(access);
        }
      break;
      }

    it->GoToNextItem();
    }  
  it->Delete();
  
  // If this is new one, then clone it
  if ( !clone )
    {
    clone = manipulator->NewInstance();
    vtkPVApplication* pvApp = static_cast<vtkPVApplication*>(this->GetApplication());
    clone->SetApplication(pvApp);
    this->ManipulatorCollection->AddItem(clone); 
    clone->Delete();
    clone->AddObserver(vtkKWEvent::ManipulatorModifiedEvent, this->Observer);
    clone->SetManipulatorName(name);
    }
  // Set the mouse and key layout
  clone->SetButton(mouse+1);
  clone->SetShift(shift);
  clone->SetControl(control);
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SetLabel(const char* label)
{
  if ( this->LabeledFrame)
    {
    ostrstream str;
    str << "Camera Control for " << label << ends;
    this->LabeledFrame->SetLabel(str.str());
    str.rdbuf()->freeze(0);
    }
}

//-----------------------------------------------------------------------------
int vtkPVInteractorStyleControl::SetManipulator(int pos, const char* name)
{
  if ( pos < 0 || pos > 8 )
    {
    vtkErrorMacro("There are only 9 possible menus");
    return 0;
    }
  if ( !this->GetManipulator(name) ) 
    {
    return 0;
    }
  
  this->Menus[pos]->SetValue(name);
  return 1;
}

//-----------------------------------------------------------------------------
vtkPVCameraManipulator* vtkPVInteractorStyleControl::GetManipulator(int pos)
{
  if ( pos < 0 || pos > 8 )
    {
    vtkErrorMacro("There are only 9 possible menus");
    return 0;
    }
  const char* name = this->Menus[pos]->GetValue();
  return this->GetManipulator(name);
}

//-----------------------------------------------------------------------------
vtkPVCameraManipulator* 
vtkPVInteractorStyleControl::GetManipulator(const char* name)
{
  vtkPVInteractorStyleControlInternal::ManipulatorMap::iterator mit 
    = this->Internals->Manipulators.find(name);
  if ( mit == this->Internals->Manipulators.end() )
    {
    return 0;
    }
  return mit->second.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::Create(vtkKWApplication *app, const char*)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app, 0);
  this->LabeledFrame->SetLabel("Camera Manipulators Control");

  this->OuterFrame = vtkKWFrame::New();
  this->OuterFrame->SetParent(this->LabeledFrame->GetFrame());
  this->OuterFrame->Create(app, 0);
  
  int cc;

  for ( cc = 0; cc < 6; cc ++ )
    {
    this->Labels[cc]->SetParent(this->OuterFrame->GetFrame());
    this->Labels[cc]->Create(app, "");
    }

  for ( cc = 0; cc < 9; cc ++ )
    {
    this->Menus[cc]->SetParent(this->OuterFrame->GetFrame());
    this->Menus[cc]->Create(app, "-anchor w");
    }

  this->Labels[0]->SetLabel("Left Button");
  this->Labels[1]->SetLabel("Middle Button");
  this->Labels[2]->SetLabel("Right Button");
  this->Labels[4]->SetLabel("Shift");
  this->Labels[5]->SetLabel("Control");

  const char *grid_settings = " -sticky news -padx 1 -pady 1";

  this->Script("grid x %s %s %s %s", 
               this->Labels[0]->GetWidgetName(), 
               this->Labels[1]->GetWidgetName(), 
               this->Labels[2]->GetWidgetName(),
               grid_settings);
  this->Script("grid %s %s %s %s %s", 
               this->Labels[3]->GetWidgetName(), 
               this->Menus[0]->GetWidgetName(), 
               this->Menus[1]->GetWidgetName(), 
               this->Menus[2]->GetWidgetName(),
               grid_settings);
  this->Script("grid %s %s %s %s %s", 
               this->Labels[4]->GetWidgetName(), 
               this->Menus[3]->GetWidgetName(), 
               this->Menus[4]->GetWidgetName(), 
               this->Menus[5]->GetWidgetName(),
               grid_settings);
  this->Script("grid %s %s %s %s %s", 
               this->Labels[5]->GetWidgetName(), 
               this->Menus[6]->GetWidgetName(), 
               this->Menus[7]->GetWidgetName(), 
               this->Menus[8]->GetWidgetName(),
               grid_settings);
               
  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->OuterFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->OuterFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->OuterFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->OuterFrame->GetFrame()->GetWidgetName());
  

  this->Script("pack %s -expand true -fill both -side top", 
               this->OuterFrame->GetWidgetName());
  this->Script("pack %s -expand true -fill x -side top", 
               this->LabeledFrame->GetWidgetName());
  this->UpdateMenus();

  this->ArgumentsFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ArgumentsFrame->Create(app, 0);
  this->Script("pack %s -expand true -fill x -side top", 
               this->ArgumentsFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::ReadRegistery()
{
  if ( !this->GetApplication() || !this->RegisteryName )
    {
    vtkErrorMacro("Application and type of Interactor Style Controler"
                  " have to be defined");
    return;
    }
  int cc;
  char manipulator[100];
  char buffer[100];
  for ( cc = 0; cc < 9; cc ++ )
    {
    int mouse = cc % 3;
    int key = static_cast<int>(cc / 3);
    buffer[0] = 0;
    sprintf(manipulator, "ManipulatorT%sM%dK%d", 
            this->RegisteryName, mouse, key);
    if ( this->GetApplication()->GetRegisteryValue(2, "RunTime", manipulator,
                                              buffer) &&
         *buffer > 0 &&
         this->GetManipulator(buffer) )
      {
      this->SetCurrentManipulator(mouse, key, buffer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::StoreRegistery()
{
  if ( !this->GetApplication() || !this->RegisteryName )
    {
    return;
    }
  int cc;
  char manipulator[100];
  for ( cc = 0; cc < 9; cc ++ )
    {
    int mouse = cc % 3;
    int key = static_cast<int>(cc / 3);
    
    sprintf(manipulator, "ManipulatorT%sM%dK%d", 
            this->RegisteryName, mouse, key);
    this->GetApplication()->SetRegisteryValue(2, "RunTime", manipulator,
                                         this->Menus[cc]->GetValue());
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::AddArgument(
  const char* name, const char* manipulator, vtkPVWidget* widget)
{
  if ( !name || !manipulator || !widget )
    {
    vtkErrorMacro("Name, manipulator, or widget not specified");
    return;
    }
  // Add widget to the map
  vtkPVInteractorStyleControlInternal::WidgetsMap::iterator wit
    = this->Internals->Widgets.find(name);
  if ( wit != this->Internals->Widgets.end() )
    {
    wit->second->SetParent(0);
    wit->second->SetPVSource(0);
    }
  this->Internals->Widgets[name] = widget;

  char str[512];
  widget->SetTraceReferenceObject(this);
  sprintf(str, "GetWidget {%s}", name);
  widget->SetTraceReferenceCommand(str);
  
  // find vector of manipulators that respond to this argument
  vtkPVInteractorStyleControlInternal::MapStringToArrayString::iterator mit
    = this->Internals->Arguments.find(name);
  if ( mit == this->Internals->Arguments.end() )
    {
    // If there is none, create it.
    vtkPVInteractorStyleControlInternal::ArrayString nstr;
    this->Internals->Arguments[name] = nstr;
    mit = this->Internals->Arguments.find(name);
    }
  
  // Now check if this manipulator is already on the list
  vtkPVInteractorStyleControlInternal::ArrayString::iterator cnt;
  for ( cnt = mit->second.begin();
    cnt != mit->second.end();
    ++ cnt )
    {
    if ( *cnt == manipulator )
      {
      break;
      }
    }
  if ( cnt == mit->second.end() )
    {
    // if not add it.
    mit->second.push_back(manipulator);
    }
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVInteractorStyleControl::GetWidget(const char* name)
{
  vtkPVInteractorStyleControlInternal::WidgetsMap::iterator it =
    this->Internals->Widgets.find(name);
  if ( it == this->Internals->Widgets.end() )
    {
    return NULL;
    }
  return it->second.GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::ResetWidget(vtkPVCameraManipulator* man, 
                                              const char* name)
{
  vtkPVWidget *pw = 0;
  vtkPVInteractorStyleControlInternal::WidgetsMap::iterator it
    = this->Internals->Widgets.find(name);
  if ( it == this->Internals->Widgets.end() )
    {
    return;
    }
  //vtkPVScale* scale = vtkPVScale::SafeDownCast(pw);
  vtkPVVectorEntry* vectorEntry = vtkPVVectorEntry::SafeDownCast(pw);
  if ( vectorEntry )
    {
    this->CurrentManipulator = man;
    this->Script("[ %s GetCurrentManipulator ] Get%s", this->GetTclName(),
                 name);
    strstream str;
    str << this->GetApplication()->GetMainInterp()->result << ends;
    float f[6] = { 0, 0, 0, 0, 0, 0 };
    int cc;
    for ( cc = 0; cc < vectorEntry->GetVectorLength(); cc ++ )
      {
      float fn = 0;
      str >> fn;
      f[cc] = fn;
      }
    vectorEntry->SetValue(f, vectorEntry->GetVectorLength());
    this->CurrentManipulator = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SaveState(ofstream *file)
{
  if (!this->ManipulatorCollection)
    {
    return;
    }
  
  vtkCollectionIterator *it = this->ManipulatorCollection->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkPVCameraManipulator *m = static_cast<vtkPVCameraManipulator*>(
      it->GetCurrentObject());
    *file << "$kw(" << this->GetTclName() << ") SetCurrentManipulator "
          << m->GetButton() - 1 << " ";
    if (m->GetShift())
      {
      *file << "1 ";
      }
    else if (m->GetControl())
      {
      *file << "2 ";
      }
    else
      {
      *file << "0 ";
      }
    *file << "{" << m->GetManipulatorName() << "}" << endl;
    it->GoToNextItem();
    }
  it->Delete();
  
  if (this->ArgumentsFrame->IsCreated())
    {
    vtkPVInteractorStyleControlInternal::WidgetsMap::iterator widgetIt;
    for ( widgetIt = this->Internals->Widgets.begin();
      widgetIt != this->Internals->Widgets.end();
      ++ widgetIt )
      {
      widgetIt->second->SaveState(file);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabeledFrame);
  int cc;
  for ( cc = 0; cc < 6; cc ++ )
    {
    this->PropagateEnableState(this->Labels[cc]);
    }
  for ( cc = 0; cc < 9; cc ++ )
    {
    this->PropagateEnableState(this->Menus[cc]);
    }
  this->PropagateEnableState(this->ArgumentsFrame);

  vtkPVInteractorStyleControlInternal::WidgetsMap::iterator it;
  for ( it = this->Internals->Widgets.begin();
    it != this->Internals->Widgets.end();
    ++it )
    {
    it->second->SetEnabled(this->Enabled);
    }
  this->PropagateEnableState(this->OuterFrame);
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame: " << this->LabeledFrame << endl;
  os << indent << "DefaultManipulator: " << (this->DefaultManipulator?this->DefaultManipulator:"None") << endl;
  os << indent << "ManipulatorCollection: " << this->ManipulatorCollection << endl;
  os << indent << "RegisteryName: " << (this->RegisteryName?this->RegisteryName:"none") << endl;
  os << indent << "CurrentManipulator: " << this->CurrentManipulator << endl;
}
