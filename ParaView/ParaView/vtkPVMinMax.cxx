/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMinMax.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMinMax.h"

#include "vtkArrayMap.txx"
#include "vtkKWLabel.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVScalarListWidgetProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMinMax);
vtkCxxRevisionMacro(vtkPVMinMax, "1.28");

vtkCxxSetObjectMacro(vtkPVMinMax, ArrayMenu, vtkPVArrayMenu);

//----------------------------------------------------------------------------
vtkPVMinMax::vtkPVMinMax()
{
  this->MinFrame = vtkKWWidget::New();
  this->MinFrame->SetParent(this);
  this->MaxFrame = vtkKWWidget::New();
  this->MaxFrame->SetParent(this);
  this->MinLabel = vtkKWLabel::New();
  this->MaxLabel = vtkKWLabel::New();
  this->MinScale = vtkKWScale::New();
  this->MaxScale = vtkKWScale::New();

  this->GetMinCommand = NULL;
  this->GetMaxCommand = NULL;
  this->SetCommand = NULL;

  this->MinHelp = 0;
  this->MaxHelp = 0;

  this->PackVertically = 1;

  this->ShowMinLabel = 1;
  this->ShowMaxLabel = 1;

  this->MinLabelWidth = 18;
  this->MaxLabelWidth = 18;

  this->ArrayMenu = NULL;

  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkPVMinMax::~vtkPVMinMax()
{
  this->MinScale->Delete();
  this->MinScale = NULL;
  this->MaxScale->Delete();
  this->MaxScale = NULL;
  this->MinLabel->Delete();
  this->MinLabel = NULL;
  this->MaxLabel->Delete();
  this->MaxLabel = NULL;
  this->MinFrame->Delete();
  this->MinFrame = NULL;
  this->MaxFrame->Delete();
  this->MaxFrame = NULL;
  this->SetGetMinCommand(NULL);
  this->SetGetMaxCommand(NULL);
  this->SetSetCommand(NULL);
  this->SetMinHelp(0);
  this->SetMaxHelp(0);

  this->SetArrayMenu(NULL);
  
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinimumLabel(const char* label)
{
  this->MinLabel->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaximumLabel(const char* label)
{
  this->MaxLabel->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinimumHelp(const char* help)
{
  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (help != this->MinHelp)
    {
    this->SetMinHelp(help);
    }
  if (this->ShowMinLabel)
    {
    this->MinLabel->SetBalloonHelpString(help);
    }
  this->MinScale->SetBalloonHelpString(help);
  
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaximumHelp(const char* help)
{
  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (help != this->MaxHelp)
    {
    this->SetMaxHelp(help);
    }
  if (this->ShowMaxLabel)
    {
    this->MaxLabel->SetBalloonHelpString(help);
    }
  this->MaxScale->SetBalloonHelpString(help);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::Create(vtkKWApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("PVScale already created");
    return;
    }

  // For getting the widget in a script.
  const char* label = this->MinLabel->GetLabel();
  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->SetApplication(pvApp);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  this->MinFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x -expand t", 
               this->MinFrame->GetWidgetName());
  if (this->PackVertically)
    {
    this->MaxFrame->Create(pvApp, "frame", "");
    this->Script("pack %s -side top -fill x -expand t", 
                 this->MaxFrame->GetWidgetName());
    }
  
  // Now a label
  if ( this->ShowMinLabel )
    {
    this->MinLabel->SetParent(this->MinFrame);
    ostrstream opts;
    opts << "-width " << this->MinLabelWidth << " -justify right" << ends;
    this->MinLabel->Create(pvApp, opts.str());
    opts.rdbuf()->freeze(0);
    this->Script("pack %s -side left -anchor s", 
                 this->MinLabel->GetWidgetName());
    }

  this->MinScale->SetParent(this->MinFrame);
  this->MinScale->Create(this->Application, "");
  this->MinScale->SetDisplayEntryAndLabelOnTop(0);
  this->MinScale->DisplayEntry();
  this->MinScale->SetRange(-VTK_LARGE_FLOAT, VTK_LARGE_FLOAT);
  this->MinScale->SetCommand(this, "MinValueCallback");
  this->Script("pack %s -side left -fill x -expand t -padx 5", 
               this->MinScale->GetWidgetName());

  if ( this->ShowMaxLabel )
    {
    if (this->PackVertically)
      {
      this->MaxLabel->SetParent(this->MaxFrame);
      }
    else
      {
      this->MaxLabel->SetParent(this->MinFrame);
      }
    ostrstream opts;
    opts << "-width " << this->MaxLabelWidth << " -justify right" << ends;
    this->MaxLabel->Create(pvApp, opts.str());
    opts.rdbuf()->freeze(0);
    this->Script("pack %s -side left -anchor s", 
                 this->MaxLabel->GetWidgetName());
    }

  if (this->PackVertically)
    {
    this->MaxScale->SetParent(this->MaxFrame);
    }
  else
    {
    this->MaxScale->SetParent(this->MinFrame);
    }
  this->MaxScale->Create(this->Application, "");
  this->MaxScale->SetDisplayEntryAndLabelOnTop(0);
  this->MaxScale->DisplayEntry();
  this->MaxScale->SetRange(-VTK_LARGE_FLOAT, VTK_LARGE_FLOAT);
  this->MaxScale->SetCommand(this, "MaxValueCallback");
  this->Script("pack %s -side left -fill x -expand t -padx 5", 
               this->MaxScale->GetWidgetName());

  this->SetMinimumHelp(this->MinHelp);
  this->SetMaximumHelp(this->MaxHelp);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMinValue(float val)
{
  this->MinScale->SetValue(val);
  float *scalars = NULL;

  if (this->Property)
    {
    scalars = this->Property->GetScalars();
    }
  
  if (!this->AcceptCalled && scalars)
    {
    scalars[0] = val;
    }
  if (val > this->MaxScale->GetValue())
    {
    this->MaxScale->SetValue(val);
    if (!this->AcceptCalled && scalars)
      {
      scalars[1] = val;
      }
    }

  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetMaxValue(float val)
{
  this->MaxScale->SetValue(val);
  float *scalars = NULL;
  
  if (this->Property)
    {
    scalars = this->Property->GetScalars();
    }
  
  if (!this->AcceptCalled && scalars)
    {
    scalars[1] = val;
    }
  if (val < this->MinScale->GetValue())
    {
    this->MinScale->SetValue(val);
    if (!this->AcceptCalled && scalars)
      {
      scalars[0] = val;
      }
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::AcceptInternal(vtkClientServerID sourceID)
{
  float scalars[2];
  scalars[0] = this->GetMinValue();
  scalars[1] = this->GetMaxValue();
  this->Property->SetScalars(2, scalars);
  this->Property->SetVTKSourceID(sourceID);
  this->Property->AcceptInternal();
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVMinMax::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetMaxValue "
        << this->MaxScale->GetValue() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetMinValue "
        << this->MinScale->GetValue() << endl;
}


//----------------------------------------------------------------------------
void vtkPVMinMax::ResetInternal()
{
  if ( this->MinScale->IsCreated() )
    {
    // Command to update the UI.
    float *values = this->Property->GetScalars();
    this->SetMinValue(values[0]);
    this->SetMaxValue(values[1]);
    }
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}


//----------------------------------------------------------------------------
void vtkPVMinMax::Update()
{
  double range[2];
  double oldRange[2];

  vtkPVArrayInformation *ai;

  if (this->ArrayMenu == NULL)
    {
    return;
    }

  ai = this->ArrayMenu->GetArrayInformation();
  if (ai == NULL || ai->GetName() == NULL)
    {
    return;
    }

  ai->GetComponentRange(0, range);

  if (range[0] > range[1])
    {
    vtkErrorMacro("Invalid Data Range");
    return;
    }
  
  if (range[0] == range[1])
    {
    // Special case to avoid log(0).
    this->MinScale->SetRange(range);
    this->MaxScale->SetRange(range);

    this->SetMinValue(range[0]);
    this->SetMaxValue(range[1]);
    return;
    }

  // Find the place value resolution.
  int place = (int)(floor(log10((double)(range[1]-range[0])) - 1.5));
  double resolution = pow(10.0, (double)(place));
  // Now find the range at resolution values.
  range[0] = (floor((double)(range[0]) / resolution) * resolution);
  range[1] = (ceil((double)(range[1]) / resolution) * resolution);


  oldRange[1] = this->MinScale->GetRangeMax();
  oldRange[0] = this->MinScale->GetRangeMin();

  // Detect when the array has changed.
  if (oldRange[0] != range[0] || oldRange[1] != range[1])
    {
    this->MinScale->SetResolution(resolution);
    this->MinScale->SetRange(range);

    this->MaxScale->SetResolution(resolution);
    this->MaxScale->SetRange(range);

    this->SetMinValue(range[0]);
    this->SetMaxValue(range[1]);
    }
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetResolution(float res)
{
  this->MinScale->SetResolution(res);
  this->MaxScale->SetResolution(res);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SetRange(float min, float max)
{
  this->MinScale->SetRange(min, max);
  this->MaxScale->SetRange(min, max);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::GetRange(float range[2])
{
  this->MinScale->GetRange(range);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::MinValueCallback()
{
  if (this->MinScale->GetValue() > this->MaxScale->GetValue())
    {
    this->MaxScale->SetValue(this->MinScale->GetValue());
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::MaxValueCallback()
{
  if (this->MaxScale->GetValue() < this->MinScale->GetValue())
    {
    this->MinScale->SetValue(this->MaxScale->GetValue());
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::SaveInBatchScriptForPart(ofstream *file,
                                           vtkClientServerID sourceID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                  << this->GetMinCommand
                  << vtkClientServerStream::End; 
  pm->SendStreamToClient();
  {
  ostrstream result;
  pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
  result << ends;
  *file << "pvTemp" << sourceID << " " << this->SetCommand;
  *file << " " << result.str();
  }
  
  pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                  << this->GetMaxCommand
                  << vtkClientServerStream::End; 
  pm->SendStreamToClient();
  {
  ostrstream result;
  pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
  result << ends;
  *file << " " << result.str() << "\n";
  }
}


//----------------------------------------------------------------------------
vtkPVMinMax* vtkPVMinMax::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVMinMax::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVMinMax::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVMinMax* pvmm = vtkPVMinMax::SafeDownCast(clone);
  if (pvmm)
    {
    if (this->ArrayMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVArrayMenu* am = this->ArrayMenu->ClonePrototype(pvSource, map);
      pvmm->SetArrayMenu(am);
      am->Delete();
      }

    pvmm->SetMinimumLabel(this->MinLabel->GetLabel());
    pvmm->SetMaximumLabel(this->MaxLabel->GetLabel());
    pvmm->SetMinimumHelp(this->MinHelp);
    pvmm->SetMaximumHelp(this->MaxHelp);
    pvmm->SetResolution(this->MinScale->GetResolution());
    float min, max;
    this->MinScale->GetRange(min, max);
    pvmm->SetRange(min, max);
    pvmm->SetSetCommand(this->SetCommand);
    pvmm->SetGetMinCommand(this->GetMinCommand);
    pvmm->SetGetMaxCommand(this->GetMaxCommand);
    pvmm->SetMinValue(this->GetMinValue());
    pvmm->SetMaxValue(this->GetMaxValue());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVMinMax.");
    }
}

//----------------------------------------------------------------------------
int vtkPVMinMax::ReadXMLAttributes(vtkPVXMLElement* element,
                                   vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the ArrayMenu.
  const char* array_menu = element->GetAttribute("array_menu");
  if(!array_menu)
    {
    vtkErrorMacro("No array_menu attribute.");
    return 0;
    }
  vtkPVXMLElement* ame = element->LookupElement(array_menu);
  if (!ame)
    {
    vtkErrorMacro("Couldn't find ArrayMenu element " << array_menu);
    return 0;
    }
  vtkPVWidget* w = this->GetPVWidgetFromParser(ame, parser);
  vtkPVArrayMenu* amw = vtkPVArrayMenu::SafeDownCast(w);
  if(!amw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get ArrayMenu widget " << array_menu);
    return 0;
    }
  amw->AddDependent(this);
  this->SetArrayMenu(amw);
  amw->Delete();  

  // Setup the MinimumLabel.
  const char* min_label = element->GetAttribute("min_label");
  if(!min_label)
    {
    vtkErrorMacro("No min_label attribute.");
    return 0;
    }
  this->SetMinimumLabel(min_label);
  
  // Setup the MaximumLabel.
  const char* max_label = element->GetAttribute("max_label");
  if(!max_label)
    {
    vtkErrorMacro("No max_label attribute.");
    return 0;
    }
  this->SetMaximumLabel(max_label);
  
  // Setup the MinimumHelp.
  const char* min_help = element->GetAttribute("min_help");
  if(!min_help)
    {
    vtkErrorMacro("No min_help attribute.");
    return 0;
    }
  this->SetMinimumHelp(min_help);
  
  // Setup the MaximumHelp.
  const char* max_help = element->GetAttribute("max_help");
  if(!max_help)
    {
    vtkErrorMacro("No max_help attribute.");
    return 0;
    }
  this->SetMaximumHelp(max_help);
  
  // Setup the GetMinCommand.
  const char* get_min_command = element->GetAttribute("get_min_command");
  if(!get_min_command)
    {
    vtkErrorMacro("No get_min_command attribute.");
    return 0;
    }
  this->SetGetMinCommand(get_min_command);
  
  // Setup the GetMaxCommand.
  const char* get_max_command = element->GetAttribute("get_max_command");
  if(!get_max_command)
    {
    vtkErrorMacro("No get_max_command attribute.");
    return 0;
    }
  this->SetGetMaxCommand(get_max_command);
  
  // Setup the SetCommand.
  const char* set_command = element->GetAttribute("set_command");
  if(!set_command)
    {
    vtkErrorMacro("No set_command attribute.");
    return 0;
    }
  this->SetSetCommand(set_command);
  
  return 1;
}

//----------------------------------------------------------------------------
float vtkPVMinMax::GetMinValue() 
{ return this->MinScale->GetValue(); }

//----------------------------------------------------------------------------
float vtkPVMinMax::GetMaxValue() 
{ return this->MaxScale->GetValue(); }

//----------------------------------------------------------------------------
float vtkPVMinMax::GetResolution() 
{ return this->MinScale->GetResolution(); }

//----------------------------------------------------------------------------
void vtkPVMinMax::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVScalarListWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmd = new char[strlen(this->SetCommand)+1];
    strcpy(cmd, this->SetCommand);
    int numScalars = 2;
    this->Property->SetVTKCommands(1, &cmd, &numScalars);
    float scalars[2];
    scalars[0] = this->MinScale->GetValue();
    scalars[1] = this->MaxScale->GetValue();
    this->Property->SetScalars(2, scalars);
    delete [] cmd;
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVMinMax::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVMinMax::CreateAppropriateProperty()
{
  return vtkPVScalarListWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVMinMax::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "GetMaxCommand: " 
     << (this->GetGetMaxCommand()?this->GetGetMaxCommand():"none") << endl;
  os << "GetMinCommand: " 
     << (this->GetGetMinCommand()?this->GetGetMinCommand():"none") << endl;
  os << "SetCommand: " << (this->SetCommand?this->SetCommand:"none") << endl;
  os << "PackVertically: " << this->PackVertically << endl;
  os << "MinScale: " << this->MinScale << endl;
  os << "MaxScale: " << this->MaxScale << endl;
  os << "ShowMinLabel: " << this->ShowMinLabel << endl;
  os << "ShowMaxLabel: " << this->ShowMaxLabel << endl;
  os << "MinLabelWidth: " << this->MinLabelWidth << endl;
  os << "MaxLabelWidth: " << this->MaxLabelWidth << endl;
}
