/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOrientScaleWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOrientScaleWidget.h"

#include "vtkDataSetAttributes.h"
#include "vtkKWApplication.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVStringAndScalarListWidgetProperty.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkPVOrientScaleWidget);
vtkCxxRevisionMacro(vtkPVOrientScaleWidget, "1.17");

vtkCxxSetObjectMacro(vtkPVOrientScaleWidget, InputMenu, vtkPVInputMenu);

//----------------------------------------------------------------------------
vtkPVOrientScaleWidget::vtkPVOrientScaleWidget()
{
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->LabeledFrame->SetParent(this);
  this->ScalarsFrame = vtkKWWidget::New();
  this->ScalarsFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ScalarsLabel = vtkKWLabel::New();
  this->ScalarsLabel->SetParent(this->ScalarsFrame);
  this->ScalarsMenu = vtkKWOptionMenu::New();
  this->ScalarsMenu->SetParent(this->ScalarsFrame);
  this->VectorsFrame = vtkKWWidget::New();
  this->VectorsFrame->SetParent(this->LabeledFrame->GetFrame());
  this->VectorsLabel = vtkKWLabel::New();
  this->VectorsLabel->SetParent(this->VectorsFrame);
  this->VectorsMenu = vtkKWOptionMenu::New();
  this->VectorsMenu->SetParent(this->VectorsFrame);
  this->OrientModeFrame = vtkKWWidget::New();
  this->OrientModeFrame->SetParent(this->LabeledFrame->GetFrame());
  this->OrientModeLabel = vtkKWLabel::New();
  this->OrientModeLabel->SetParent(this->OrientModeFrame);
  this->OrientModeMenu = vtkKWOptionMenu::New();
  this->OrientModeMenu->SetParent(this->OrientModeFrame);
  this->ScaleModeFrame = vtkKWWidget::New();
  this->ScaleModeFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ScaleModeLabel = vtkKWLabel::New();
  this->ScaleModeLabel->SetParent(this->ScaleModeFrame);
  this->ScaleModeMenu = vtkKWOptionMenu::New();
  this->ScaleModeMenu->SetParent(this->ScaleModeFrame);
  this->ScaleFactorFrame = vtkKWWidget::New();
  this->ScaleFactorFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ScaleFactorLabel = vtkKWLabel::New();
  this->ScaleFactorLabel->SetParent(this->ScaleFactorFrame);
  this->ScaleFactorEntry = vtkKWEntry::New();
  this->ScaleFactorEntry->SetParent(this->ScaleFactorFrame);
  this->InputMenu = NULL;
  this->ScalarArrayName = NULL;
  this->VectorArrayName = NULL;
  this->Property = NULL;
  this->ScalarsCommand = NULL;
  this->VectorsCommand = NULL;
  this->OrientCommand = NULL;
  this->ScaleModeCommand = NULL;
  this->ScaleFactorCommand = NULL;
  this->DefaultOrientMode = 0;
  this->DefaultScaleMode = 0;
  this->CurrentScalars = 0;
  this->CurrentVectors = 0;
  this->CurrentOrientMode = 0;
  this->CurrentScaleMode = 0;
}

//----------------------------------------------------------------------------
vtkPVOrientScaleWidget::~vtkPVOrientScaleWidget()
{
  this->LabeledFrame->Delete();
  this->LabeledFrame = NULL;
  this->ScalarsFrame->Delete();
  this->ScalarsFrame = NULL;
  this->ScalarsLabel->Delete();
  this->ScalarsLabel = NULL;
  this->ScalarsMenu->Delete();
  this->ScalarsMenu = NULL;
  this->VectorsFrame->Delete();
  this->VectorsFrame = NULL;
  this->VectorsLabel->Delete();
  this->VectorsLabel = NULL;
  this->VectorsMenu->Delete();
  this->VectorsMenu = NULL;
  this->OrientModeFrame->Delete();
  this->OrientModeFrame = NULL;
  this->OrientModeLabel->Delete();
  this->OrientModeLabel = NULL;
  this->OrientModeMenu->Delete();
  this->OrientModeMenu = NULL;
  this->ScaleModeFrame->Delete();
  this->ScaleModeFrame = NULL;
  this->ScaleModeLabel->Delete();
  this->ScaleModeLabel = NULL;
  this->ScaleModeMenu->Delete();
  this->ScaleModeMenu = NULL;
  this->ScaleFactorFrame->Delete();
  this->ScaleFactorFrame = NULL;
  this->ScaleFactorLabel->Delete();
  this->ScaleFactorLabel = NULL;
  this->ScaleFactorEntry->Delete();
  this->ScaleFactorEntry = NULL;
  this->SetInputMenu(NULL);
  this->SetScalarArrayName(NULL);
  this->SetVectorArrayName(NULL);
  this->SetProperty(NULL);
  this->SetScalarsCommand(NULL);
  this->SetVectorsCommand(NULL);
  this->SetOrientCommand(NULL);
  this->SetScaleModeCommand(NULL);
  this->SetScaleFactorCommand(NULL);
  this->SetCurrentScalars(NULL);
  this->SetCurrentVectors(NULL);
  this->SetCurrentOrientMode(NULL);
  this->SetCurrentScaleMode(NULL);
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  this->LabeledFrame->Create(app, "");
  this->LabeledFrame->SetLabel("Orient / Scale");
  
  this->ScalarsFrame->Create(app, "frame", "");
  this->ScalarsLabel->Create(app, "-width 18 -justify center"); 
  this->ScalarsLabel->SetLabel("Scalars");
  this->ScalarsLabel->EnabledOff();
  this->ScalarsMenu->Create(app, "");
  this->ScalarsMenu->EnabledOff();
  
  this->VectorsFrame->Create(app, "frame", "");
  this->VectorsLabel->Create(app, "-width 18 -justify center");
  this->VectorsLabel->SetLabel("Vectors");
  this->VectorsMenu->Create(app, "");

  this->Script("pack %s -side left", this->ScalarsLabel->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand yes",
               this->ScalarsMenu->GetWidgetName());
  this->Script("pack %s -side left",
               this->VectorsLabel->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand yes",
               this->VectorsMenu->GetWidgetName());
  
  this->OrientModeFrame->Create(app, "frame", "");
  this->OrientModeLabel->Create(app, "-width 18 -justify center");
  this->OrientModeLabel->SetLabel("Orient Mode");
  this->OrientModeMenu->Create(app, "");
  this->OrientModeMenu->AddEntryWithCommand("Off", this,
                                            "OrientModeMenuCallback");
  this->OrientModeMenu->AddEntryWithCommand("Vector", this,
                                            "OrientModeMenuCallback");
  this->OrientModeMenu->SetValue("Vector");
  this->SetCurrentOrientMode("Vector");

  this->Script("pack %s -side left", this->OrientModeLabel->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand yes",
               this->OrientModeMenu->GetWidgetName());

  this->ScaleModeFrame->Create(app, "frame", "");
  this->ScaleModeLabel->Create(app, "-width 18 -justify center");
  this->ScaleModeLabel->SetLabel("Scale Mode");
  this->ScaleModeMenu->Create(app, "");
  this->ScaleModeMenu->AddEntryWithCommand("Scalar", this,
                                           "ScaleModeMenuCallback");
  this->ScaleModeMenu->AddEntryWithCommand("Vector Magnitude", this,
                                           "ScaleModeMenuCallback");
  this->ScaleModeMenu->AddEntryWithCommand("Vector Components", this,
                                           "ScaleModeMenuCallback");
  this->ScaleModeMenu->AddEntryWithCommand("Data Scaling Off", this,
                                           "ScaleModeMenuCallback");
  this->ScaleModeMenu->SetValue("Vector Magnitude");
  this->SetCurrentScaleMode("Vector Magnitude");
  
  this->Script("pack %s -side left", this->ScaleModeLabel->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand yes",
               this->ScaleModeMenu->GetWidgetName());
  
  this->ScaleFactorFrame->Create(app, "frame", "");
  this->ScaleFactorLabel->Create(app, "-width 18 -justify center");
  this->ScaleFactorLabel->SetLabel("Scale Factor");
  this->ScaleFactorEntry->Create(app, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->ScaleFactorEntry->GetWidgetName(), this->GetTclName());

  this->Script("pack %s -side left", this->ScaleFactorLabel->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand yes",
               this->ScaleFactorEntry->GetWidgetName());
  
  this->Script("pack %s %s %s %s %s -side top -anchor w -fill x",
               this->OrientModeFrame->GetWidgetName(),
               this->ScaleModeFrame->GetWidgetName(),
               this->ScaleFactorFrame->GetWidgetName(),
               this->ScalarsFrame->GetWidgetName(),
               this->VectorsFrame->GetWidgetName());
  this->Script("pack %s -side top -anchor w -fill x -pady 4",
               this->LabeledFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::UpdateActiveState()
{
  const char *orientMode = this->OrientModeMenu->GetValue();
  const char *scaleMode = this->ScaleModeMenu->GetValue();
  
  if (!strcmp(orientMode, "Vector") ||
      !strcmp(scaleMode, "Vector Magnitude") ||
      !strcmp(scaleMode, "Vector Components"))
    {
    this->VectorsLabel->EnabledOn();
    this->VectorsMenu->EnabledOn();
    }
  else
    {
    this->VectorsLabel->EnabledOff();
    this->VectorsMenu->EnabledOff();
    }
  
  if (!strcmp(scaleMode, "Scalar"))
    {
    this->ScalarsLabel->EnabledOn();
    this->ScalarsMenu->EnabledOn();
    }
  else
    {
    this->ScalarsLabel->EnabledOff();
    this->ScalarsMenu->EnabledOff();
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::Trace(ofstream *file)
{
  if (!this->InitializeTrace(file))
    {
    return;
    }
  
  *file << "$kw(" << this->GetTclName() << ") SetOrientMode {"
        << this->OrientModeMenu->GetValue() << "}" << endl;
  *file << "$kw(" << this->GetTclName() << ") SetScaleMode {"
        << this->ScaleModeMenu->GetValue() << "}" << endl;
  *file << "$kw(" << this->GetTclName() << ") SetScalars {"
        << this->ScalarsMenu->GetValue() << "}" << endl;
  *file << "$kw(" << this->GetTclName() << ") SetVectors {"
        << this->VectorsMenu->GetValue() << "}" << endl;
  *file << "$kw(" << this->GetTclName() << ") SetScaleFactor "
        << this->ScaleFactorEntry->GetValueAsFloat() << endl;
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::Update()
{
  this->UpdateArrayMenus();
  this->UpdateModeMenus();
  this->UpdateActiveState();
  this->UpdateScaleFactor();
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::UpdateArrayMenus()
{
  int i, num;
  char methodAndArgs[1024];
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *ai;
  int scalarArrayFound = 0;
  int vectorArrayFound = 0;
  const char *firstScalar = NULL;
  const char *firstVector = NULL;

  // Regenerate the menus, and look for the specified array.
  this->ScalarsMenu->ClearEntries();
  this->VectorsMenu->ClearEntries();

  attrInfo = this->GetPointDataInformation();
  if (attrInfo == NULL)
    {
    this->ScalarsMenu->SetValue("None");
    this->SetCurrentScalars("None");
    this->VectorsMenu->SetValue("None");
    this->SetCurrentVectors("None");
    this->Property->SetString(0, "None");
    this->Property->SetString(1, "None");
    return;
    }

  num = attrInfo->GetNumberOfArrays();
  for (i = 0; i < num; ++i)
    {
    ai = attrInfo->GetArrayInformation(i);
    // If the array does not have a name, then we can do nothing with it.
    if (ai->GetName())
      {
      if (ai->GetNumberOfComponents() == 1) // scalars
        {
        sprintf(methodAndArgs, "ScalarsMenuEntryCallback");
        this->ScalarsMenu->AddEntryWithCommand(ai->GetName(), 
                                               this, methodAndArgs);
        if (firstScalar == NULL)
          {
          firstScalar = ai->GetName();
          }
        if (this->ScalarArrayName &&
            strcmp(this->ScalarArrayName, ai->GetName()) == 0)
          {
          scalarArrayFound = 1;
          }
        }
      if (ai->GetNumberOfComponents() == 3) // vectors
        {
        sprintf(methodAndArgs, "VectorsMenuEntryCallback");
        this->VectorsMenu->AddEntryWithCommand(ai->GetName(), 
                                               this, methodAndArgs);
        if (firstVector == NULL)
          {
          firstVector = ai->GetName();
          }
        if (this->VectorArrayName &&
            strcmp(this->VectorArrayName, ai->GetName()) == 0)
          {
          vectorArrayFound = 1;
          }
        }
      }
    }

  // If the filter has not specified a valid array, then use the default
  // attribute.
  if (scalarArrayFound == 0)
    { // If the current value is not in the menu, then look for another to use.
    // First look for a default attribute.
    ai = attrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
    if (ai == NULL || ai->GetName() == NULL)
      { // lets just use the first in the menu.
      if (firstScalar)
        {
        ai = attrInfo->GetArrayInformation(firstScalar);
        }
      else
        {
        // Here we may want to keep the previous value.
        this->SetScalarArrayName(NULL);
        this->ScalarsMenu->SetValue("None");
        this->SetCurrentScalars("None");
        }
      }

    if (ai)
      {
      this->SetScalarArrayName(ai->GetName());

      // In this case, the widget does not match the object.
      this->ModifiedCallback();
      }
    
    // Now set the menu's value.
    this->ScalarsMenu->SetValue(this->ScalarArrayName);
    this->SetCurrentScalars(this->ScalarArrayName);
    }
  if (vectorArrayFound == 0)
    { // If the current value is not in the menu, then look for another to use.
    // First look for a default attribute.
    ai = attrInfo->GetAttributeInformation(vtkDataSetAttributes::VECTORS);
    if (ai == NULL || ai->GetName() == NULL)
      { // lets just use the first in the menu.
      if (firstVector)
        {
        ai = attrInfo->GetArrayInformation(firstVector);
        }
      else
        {
        // Here we may want to keep the previous value.
        this->SetVectorArrayName(NULL);
        this->VectorsMenu->SetValue("None");
        this->SetCurrentVectors("None");
        }
      }

    if (ai)
      {
      this->SetVectorArrayName(ai->GetName());

      // In this case, the widget does not match the object.
      this->ModifiedCallback();
      }
    
    // Now set the menu's value.
    this->VectorsMenu->SetValue(this->VectorArrayName);
    this->SetCurrentVectors(this->VectorArrayName);
    }

  if (!this->AcceptCalled &&
      (this->ScalarArrayName || this->VectorArrayName))
    {
    // property stuff
    this->Property->SetString(0, this->ScalarArrayName);
    this->Property->SetString(1, this->VectorArrayName);
    }
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::UpdateModeMenus()
{
  vtkKWMenu *scaleMenu = this->ScaleModeMenu->GetMenu();
  vtkKWMenu *orientMenu = this->OrientModeMenu->GetMenu();
  
  int numScalars = this->ScalarsMenu->GetNumberOfEntries();
  int numVectors = this->VectorsMenu->GetNumberOfEntries();

  const char *scaleMode = this->ScaleModeMenu->GetValue();
  
  if (numScalars == 0)
    {
    // disabled
    scaleMenu->SetState("Scalar", 2);
    if (!strcmp(scaleMode, "Scalar"))
      {
      if (numVectors == 0)
        {
        this->ScaleModeMenu->SetValue("Data Scaling Off");
        }
      else
        {
        this->ScaleModeMenu->SetValue("Vector Magnitude");
        }
      this->SetCurrentScaleMode(this->ScaleModeMenu->GetValue());
      }
    }
  else
    {
    // normal
    scaleMenu->SetState("Scalar", 0);
    }
  
  if (numVectors == 0)
    {
    // disabled
    orientMenu->SetState("Vector", 2);
    scaleMenu->SetState("Vector Magnitude", 2);
    scaleMenu->SetState("Vector Components", 2);
    if (!strcmp(this->OrientModeMenu->GetValue(), "Vector"))
      {
      this->OrientModeMenu->SetValue("Off");
      this->SetCurrentOrientMode("Off");
      }
    if (!strcmp(scaleMode, "Vector Magnitude") ||
        !strcmp(scaleMode, "Vector Components"))
      {
      if (numScalars == 0)
        {
        this->ScaleModeMenu->SetValue("Data Scaling Off");
        }
      else
        {
        this->ScaleModeMenu->SetValue("Scalar");
        }
      this->SetCurrentScaleMode(this->ScaleModeMenu->GetValue());
      }
    }
  else
    {
    // normal
    orientMenu->SetState("Vector", 0);
    scaleMenu->SetState("Vector Magnitude", 0);
    scaleMenu->SetState("Vector Components", 0);
    }
  
  if (!this->AcceptCalled)
    {
    this->DefaultOrientMode = this->OrientModeMenu->GetMenu()->GetIndex(
      this->OrientModeMenu->GetValue());
    this->DefaultScaleMode = this->ScaleModeMenu->GetMenu()->GetIndex(
      this->ScaleModeMenu->GetValue());
    }
  
  this->UpdateScaleFactor();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::UpdateScaleFactor()
{
  if (!this->InputMenu)
    {
    return;
    }
  
  vtkPVSource *input = this->InputMenu->GetCurrentValue();
  if (!input)
    {
    return;
    }
  
  double bnds[6];
  vtkPVDataInformation *dInfo = input->GetDataInformation();
  dInfo->GetBounds(bnds);
  vtkPVDataSetAttributesInformation *pdInfo = dInfo->GetPointDataInformation();
  vtkPVArrayInformation *aInfo;
  
  double maxBnds = bnds[1] - bnds[0];
  maxBnds = (bnds[3] - bnds[2] > maxBnds) ? (bnds[3] - bnds[2]) : maxBnds;
  maxBnds = (bnds[5] - bnds[4] > maxBnds) ? (bnds[5] - bnds[4]) : maxBnds;
  maxBnds *= 0.1;

  double absMaxRange = 0;
  const char* scaleMode = this->ScaleModeMenu->GetValue();

  if (!strcmp(scaleMode, "Scalar"))
    {
    const char *arrayName = this->ScalarsMenu->GetValue();
    if (arrayName)
      {
      aInfo = pdInfo->GetArrayInformation(arrayName);
      if (aInfo)
        {
        double *range = aInfo->GetComponentRange(0);
        absMaxRange = fabs(range[0]);
        absMaxRange = (fabs(range[1]) > absMaxRange) ? fabs(range[1]) :
          absMaxRange;
        }
      }
    }
  else if (!strcmp(scaleMode, "Vector Magnitude"))
    {
    const char *arrayName = this->VectorsMenu->GetValue();
    if (arrayName)
      {
      aInfo = pdInfo->GetArrayInformation(arrayName);
      if (aInfo)
        {
        double *range = aInfo->GetComponentRange(-1);
        absMaxRange = fabs(range[0]);
        absMaxRange = (fabs(range[1]) > absMaxRange) ? fabs(range[1]) :
          absMaxRange;
        }
      }
    }
  else if (!strcmp(scaleMode, "Vector Components"))
    {
    const char *arrayName = this->VectorsMenu->GetValue();
    if (arrayName)
      {
      aInfo = pdInfo->GetArrayInformation(arrayName);
      if (aInfo)
        {
        double *range0 = aInfo->GetComponentRange(0);
        double *range1 = aInfo->GetComponentRange(1);
        double *range2 = aInfo->GetComponentRange(2);
        absMaxRange = fabs(range0[0]);
        absMaxRange = (fabs(range0[1]) > absMaxRange) ? fabs(range0[1]) :
          absMaxRange;
        absMaxRange = (fabs(range1[0]) > absMaxRange) ? fabs(range1[0]) :
          absMaxRange;
        absMaxRange = (fabs(range1[1]) > absMaxRange) ? fabs(range1[1]) :
          absMaxRange;
        absMaxRange = (fabs(range2[0]) > absMaxRange) ? fabs(range2[0]) :
          absMaxRange;
        absMaxRange = (fabs(range2[1]) > absMaxRange) ? fabs(range2[1]) :
          absMaxRange;
        }
      }
    }
  
  if (absMaxRange != 0)
    {
    maxBnds /= absMaxRange;
    }

  this->ScaleFactorEntry->SetValue(maxBnds);
  if (!this->AcceptCalled)
    {
    float scalars[3];
    scalars[0] = this->DefaultOrientMode;
    scalars[1] = this->DefaultScaleMode;
    scalars[2] = maxBnds;
    this->Property->SetScalars(3, scalars);
    }
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::CopyProperties(
  vtkPVWidget *clone, vtkPVSource *pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVOrientScaleWidget *pvosw = vtkPVOrientScaleWidget::SafeDownCast(clone);
  if (pvosw)
    {
    if (this->InputMenu)
      {
      vtkPVInputMenu *im = this->InputMenu->ClonePrototype(pvSource, map);
      pvosw->SetInputMenu(im);
      im->Delete();
      }
    pvosw->SetScalarsCommand(this->ScalarsCommand);
    pvosw->SetVectorsCommand(this->VectorsCommand);
    pvosw->SetOrientCommand(this->OrientCommand);
    pvosw->SetScaleModeCommand(this->ScaleModeCommand);
    pvosw->SetScaleFactorCommand(this->ScaleFactorCommand);
    pvosw->SetDefaultOrientMode(this->DefaultOrientMode);
    pvosw->SetDefaultScaleMode(this->DefaultScaleMode);
    }
  else
    {
    vtkErrorMacro("Internal error. Could not downcast clont to PVOrientScaleWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVOrientScaleWidget::ReadXMLAttributes(vtkPVXMLElement *element,
                                              vtkPVXMLPackageParser *parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser))
    {
    return 0;
    }
  
  const char *input_menu = element->GetAttribute("input_menu");
  if (input_menu)
    {
    vtkPVXMLElement *ime = element->LookupElement(input_menu);
    if (!ime)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
      return 0;
      }
    
    vtkPVWidget *w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVInputMenu *imw = vtkPVInputMenu::SafeDownCast(w);
    if (!imw)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
      return 0;
      }
    imw->AddDependent(this);
    this->SetInputMenu(imw);
    imw->Delete();
    }

  this->SetScalarsCommand(element->GetAttribute("scalars_command"));
  this->SetVectorsCommand(element->GetAttribute("vectors_command"));
  this->SetOrientCommand(element->GetAttribute("orient_command"));
  this->SetScaleModeCommand(element->GetAttribute("scale_mode_command"));
  this->SetScaleFactorCommand(element->GetAttribute("scale_factor_command"));
  
  const char *orient_mode = element->GetAttribute("default_orient_mode");
  this->SetDefaultOrientMode(atoi(orient_mode));
  const char *scale_mode = element->GetAttribute("default_scale_mode");
  this->SetDefaultScaleMode(atoi(scale_mode));
  
  return 1;
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* vtkPVOrientScaleWidget::GetPointDataInformation()
{
  if (!this->InputMenu)
    {
    return NULL;
    }
  
  vtkPVSource *input = this->InputMenu->GetCurrentValue();
  if (!input)
    {
    return NULL;
    }
  
  return input->GetDataInformation()->GetPointDataInformation();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::ScalarsMenuEntryCallback()
{
  if (this->CurrentScalars &&
      !strcmp(this->ScalarsMenu->GetValue(), this->CurrentScalars))
    {
    return;
    }
  
  this->SetCurrentScalars(this->ScalarsMenu->GetValue());
  this->UpdateScaleFactor();
    
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::VectorsMenuEntryCallback()
{
  if (this->CurrentVectors &&
      !strcmp(this->VectorsMenu->GetValue(), this->CurrentVectors))
    {
    return;
    }
  
  this->SetCurrentVectors(this->VectorsMenu->GetValue());
  this->UpdateScaleFactor();
    
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::ScaleModeMenuCallback()
{
  if (this->CurrentScaleMode &&
      !strcmp(this->ScaleModeMenu->GetValue(), this->CurrentScaleMode))
    {
    return;
    }
  
  this->SetCurrentScaleMode(this->ScaleModeMenu->GetValue());
  this->UpdateActiveState();
  this->UpdateScaleFactor();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::OrientModeMenuCallback()
{
  if (this->CurrentOrientMode &&
      !strcmp(this->OrientModeMenu->GetValue(), this->CurrentOrientMode))
    {
    return;
    }
  
  this->SetCurrentOrientMode(this->OrientModeMenu->GetValue());
  this->UpdateActiveState();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::AcceptInternal(vtkClientServerID sourceID)
{
  float scalars[3];
  scalars[0] = this->OrientModeMenu->GetMenu()->GetIndex(
    this->OrientModeMenu->GetValue());
  scalars[1] = this->ScaleModeMenu->GetMenu()->GetIndex(
    this->ScaleModeMenu->GetValue());
  scalars[2] = this->ScaleFactorEntry->GetValueAsFloat();
  
  this->Property->SetVTKSourceID(sourceID);
  this->Property->SetString(0, (char*)(this->ScalarsMenu->GetValue()));
  this->Property->SetString(1, (char*)(this->VectorsMenu->GetValue()));
  this->Property->SetScalars(3, scalars);
  this->Property->AcceptInternal();
  
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::ResetInternal()
{
  if (!this->ModifiedFlag)
    {
    return;
    }
  
  float *scalars = this->Property->GetScalars();

  this->ScalarsMenu->SetValue(this->Property->GetString(0));
  this->SetCurrentScalars(this->Property->GetString(0));
  this->VectorsMenu->SetValue(this->Property->GetString(1));
  this->SetCurrentVectors(this->Property->GetString(1));
  this->OrientModeMenu->SetValue(
    this->OrientModeMenu->GetEntryLabel((int)scalars[0]));
  this->SetCurrentOrientMode(this->OrientModeMenu->GetValue());
  this->ScaleModeMenu->SetValue(
    this->ScaleModeMenu->GetEntryLabel((int)scalars[1]));
  this->SetCurrentScaleMode(this->ScaleModeMenu->GetValue());
  this->ScaleFactorEntry->SetValue(scalars[2]);
  
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVStringAndScalarListWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmds[5];
    cmds[0] = this->ScalarsCommand;
    cmds[1] = this->VectorsCommand;
    cmds[2] = this->OrientCommand;
    cmds[3] = this->ScaleModeCommand;
    cmds[4] = this->ScaleFactorCommand;
    int numStrings[5] = {1, 1, 0, 0, 0};
    int numScalars[5] = {0, 0, 1, 1, 1};
    this->Property->SetVTKCommands(5, cmds, numStrings, numScalars);
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVOrientScaleWidget::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVOrientScaleWidget::CreateAppropriateProperty()
{
  return vtkPVStringAndScalarListWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SetOrientMode(char *mode)
{
  this->OrientModeMenu->SetValue(mode);
  this->SetCurrentOrientMode(mode);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SetScaleMode(char *mode)
{
  this->ScaleModeMenu->SetValue(mode);
  this->SetCurrentScaleMode(mode);
  this->Update();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SetScalars(char *scalars)
{
  this->ScalarsMenu->SetValue(scalars);
  this->SetCurrentScalars(scalars);
  this->Update();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SetVectors(char *vectors)
{
  this->VectorsMenu->SetValue(vectors);
  this->SetCurrentVectors(vectors);
  this->Update();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SetScaleFactor(float factor)
{
  this->ScaleFactorEntry->SetValue(factor);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabeledFrame);
  this->PropagateEnableState(this->ScalarsFrame);
  this->PropagateEnableState(this->ScalarsLabel);
  this->PropagateEnableState(this->ScalarsMenu);
  this->PropagateEnableState(this->VectorsFrame);
  this->PropagateEnableState(this->VectorsLabel);
  this->PropagateEnableState(this->VectorsMenu);
  this->PropagateEnableState(this->OrientModeFrame);
  this->PropagateEnableState(this->OrientModeLabel);
  this->PropagateEnableState(this->OrientModeMenu);
  this->PropagateEnableState(this->ScaleModeFrame);
  this->PropagateEnableState(this->ScaleModeLabel);
  this->PropagateEnableState(this->ScaleModeMenu);
  this->PropagateEnableState(this->ScaleFactorFrame);
  this->PropagateEnableState(this->ScaleFactorLabel);
  this->PropagateEnableState(this->ScaleFactorEntry);

  this->PropagateEnableState(this->InputMenu);
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SaveInBatchScript(ofstream* file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->ScalarsCommand << "] SetElement 0 {" 
        << this->ScalarsMenu->GetValue() << "}" << endl;
  *file << "  " << "[$pvTemp" << sourceID <<" GetProperty " 
        << this->VectorsCommand << "] SetElement 0 {" 
        << this->VectorsMenu->GetValue() << "}" << endl;
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->OrientCommand << "] SetElement 0 " 
        << this->OrientModeMenu->GetMenu()->GetIndex(
          this->OrientModeMenu->GetValue())
        << endl;;
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->ScaleModeCommand << "] SetElement 0 " 
        << this->ScaleModeMenu->GetMenu()->GetIndex(
          this->ScaleModeMenu->GetValue())
        << endl;
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->ScaleFactorCommand
        << "] SetElement 0 " << this->ScaleFactorEntry->GetValueAsFloat()
        << endl;
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  os << indent << "ScalarsCommand: "
     << (this->ScalarsCommand ? this->ScalarsCommand : "(none)") << endl;
  os << indent << "VectorsCommand: "
     << (this->VectorsCommand ? this->VectorsCommand : "(none)") << endl;
  os << indent << "OrientCommand: "
     << (this->OrientCommand ? this->OrientCommand : "(none)") << endl;
  os << indent << "ScaleModeCommand: "
     << (this->ScaleModeCommand ? this->ScaleModeCommand : "(none)") << endl;
  os << indent << "ScaleFactorCommand: "
     << (this->ScaleFactorCommand ? this->ScaleFactorCommand : "(none)")
     << endl;
  os << indent << "DefaultScaleMode: " << this->DefaultScaleMode << endl;
  os << indent << "DefaultOrientMode: " << this->DefaultOrientMode << endl;
}
