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
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkPVOrientScaleWidget);
vtkCxxRevisionMacro(vtkPVOrientScaleWidget, "1.24");

vtkCxxSetObjectMacro(vtkPVOrientScaleWidget, SMScalarProperty, vtkSMProperty);
vtkCxxSetObjectMacro(vtkPVOrientScaleWidget, SMVectorProperty, vtkSMProperty);
vtkCxxSetObjectMacro(vtkPVOrientScaleWidget, SMOrientModeProperty,
                     vtkSMProperty);
vtkCxxSetObjectMacro(vtkPVOrientScaleWidget, SMScaleModeProperty,
                     vtkSMProperty);
vtkCxxSetObjectMacro(vtkPVOrientScaleWidget, SMScaleFactorProperty,
                     vtkSMProperty);

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
  this->ScalarArrayName = NULL;
  this->VectorArrayName = NULL;
  this->CurrentScalars = 0;
  this->CurrentVectors = 0;
  this->CurrentOrientMode = 0;
  this->CurrentScaleMode = 0;
  this->SMScalarPropertyName = 0;
  this->SMVectorPropertyName = 0;
  this->SMOrientModePropertyName = 0;
  this->SMScaleModePropertyName = 0;
  this->SMScaleFactorPropertyName = 0;
  this->SMScalarProperty = 0;
  this->SMVectorProperty = 0;
  this->SMOrientModeProperty = 0;
  this->SMScaleModeProperty = 0;
  this->SMScaleFactorProperty = 0;
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
  this->SetScalarArrayName(NULL);
  this->SetVectorArrayName(NULL);
  this->SetSMScalarPropertyName(NULL);
  this->SetSMVectorPropertyName(NULL);
  this->SetSMOrientModePropertyName(NULL);
  this->SetSMScaleModePropertyName(NULL);
  this->SetSMScaleFactorPropertyName(NULL);
  this->SetSMScalarProperty(NULL);
  this->SetSMVectorProperty(NULL);
  this->SetSMOrientModeProperty(NULL);
  this->SetSMScaleModeProperty(NULL);
  this->SetSMScaleFactorProperty(NULL);
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
  int scalarArrayFound = 0;
  int vectorArrayFound = 0;
  const char *firstScalar = NULL;
  const char *firstVector = NULL;

  // Regenerate the menus, and look for the specified array.
  this->ScalarsMenu->DeleteAllEntries();
  this->VectorsMenu->DeleteAllEntries();

  vtkSMProperty *scalarProp = this->GetSMScalarProperty();
  vtkSMProperty *vectorProp = this->GetSMVectorProperty();
  vtkSMArrayListDomain *scalarDom = 0;
  vtkSMArrayListDomain *vectorDom = 0;
  
  if (scalarProp)
    {
    scalarDom = vtkSMArrayListDomain::SafeDownCast(
      scalarProp->GetDomain("array_list"));
    }
  if (vectorProp)
    {
    vectorDom = vtkSMArrayListDomain::SafeDownCast(
      vectorProp->GetDomain("array_list"));
    }

  if (!scalarProp || !vectorProp || !scalarDom || !vectorDom)
    {
    vtkErrorMacro("One of the properties or required domains (array_list) "
                  "could not be found.");
    this->ScalarsMenu->SetValue("None");
    this->SetCurrentScalars("None");
    this->VectorsMenu->SetValue("None");
    this->SetCurrentVectors("None");
    return;
    }

  if (scalarDom)
    {
    num = scalarDom->GetNumberOfStrings();
    for (i = 0; i < num; i++)
      {
      if (scalarDom->GetString(i))
        {
        sprintf(methodAndArgs, "ScalarsMenuEntryCallback");
        this->ScalarsMenu->AddEntryWithCommand(scalarDom->GetString(i),
                                               this, methodAndArgs);
        if (firstScalar == NULL)
          {
          firstScalar = scalarDom->GetString(i);
          }
        if (this->ScalarArrayName &&
            strcmp(this->ScalarArrayName, scalarDom->GetString(i)) == 0)
          {
          scalarArrayFound = 1;
          }
        }
      }
    if (!scalarArrayFound)
      {
      if (firstScalar)
        {
        this->SetScalarArrayName(firstScalar);
        this->ScalarsMenu->SetValue(firstScalar);
        this->SetCurrentScalars(firstScalar);
        this->ModifiedCallback();
        }
      else
        {
        this->SetScalarArrayName(NULL);
        this->ScalarsMenu->SetValue("None");
        this->SetCurrentScalars("None");
        }
      }
    else
      {
      this->ScalarsMenu->SetValue(this->ScalarArrayName);
      }
    }
  
  if (vectorDom)
    {
    num = vectorDom->GetNumberOfStrings();
    for (i = 0; i < num; i++)
      {
      if (vectorDom->GetString(i))
        {
        sprintf(methodAndArgs, "VectorsMenuEntryCallback");
        this->VectorsMenu->AddEntryWithCommand(vectorDom->GetString(i),
                                               this, methodAndArgs);
        if (firstVector == NULL)
          {
          firstVector = vectorDom->GetString(i);
          }
        if (this->VectorArrayName &&
            strcmp(this->VectorArrayName, vectorDom->GetString(i)) == 0)
          {
          vectorArrayFound = 1;
          }
        }
      }
    if (!vectorArrayFound)
      {
      if (firstVector)
        {
        this->SetVectorArrayName(firstVector);
        this->VectorsMenu->SetValue(firstVector);
        this->SetCurrentVectors(firstVector);
        this->ModifiedCallback();
        }
      else
        {
        this->SetVectorArrayName(NULL);
        this->VectorsMenu->SetValue("None");
        this->SetCurrentVectors("None");
        }
      }
    else
      {
      this->VectorsMenu->SetValue(this->VectorArrayName);
      }
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
  
  this->UpdateScaleFactor();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::UpdateScaleFactor()
{
  vtkSMProperty *prop = this->GetSMScaleFactorProperty();
  vtkSMArrayRangeDomain *scalarRangeDom = 0;
  vtkSMArrayRangeDomain *vectorRangeDom = 0;
  vtkSMBoundsDomain *boundsDom = 0;

  if (prop)
    {
    scalarRangeDom = vtkSMArrayRangeDomain::SafeDownCast(
      prop->GetDomain("scalar_range"));
    vectorRangeDom = vtkSMArrayRangeDomain::SafeDownCast(
      prop->GetDomain("vector_range"));
    boundsDom = vtkSMBoundsDomain::SafeDownCast(prop->GetDomain("bounds"));
    }

  if (!prop || !scalarRangeDom || !vectorRangeDom || !boundsDom)
    {
    vtkErrorMacro("One of the properties or required domains (scalar_range, "
                  "vector_range, bounds) could not be found.");
    return;
    }
  
  double bnds[6];
  int exists, i;
  for (i = 0; i < 3; i++)
    {
    bnds[2*i] = boundsDom->GetMinimum(i, exists);
    if (!exists)
      {
      bnds[2*i] = 0;
      }
    bnds[2*i+1] = boundsDom->GetMaximum(i, exists);
    if (!exists)
      {
      bnds[2*i+1] = 1;
      }
    }
  
  double maxBnds = bnds[1] - bnds[0];
  maxBnds = (bnds[3] - bnds[2] > maxBnds) ? (bnds[3] - bnds[2]) : maxBnds;
  maxBnds = (bnds[5] - bnds[4] > maxBnds) ? (bnds[5] - bnds[4]) : maxBnds;
  maxBnds *= 0.1;

  double absMaxRange = 0;
  const char* scaleMode = this->ScaleModeMenu->GetValue();

  vtkSMStringVectorProperty *scalarProp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetSMScalarProperty());
  vtkSMStringVectorProperty *vectorProp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetSMVectorProperty());
  
  if (!strcmp(scaleMode, "Scalar") && scalarProp)
    {
    const char *arrayName = this->ScalarsMenu->GetValue();
    scalarProp->SetUncheckedElement(0, arrayName);
    scalarProp->UpdateDependentDomains();
    if (arrayName)
      {
      double range[2];
      range[0] = scalarRangeDom->GetMinimum(0, exists);
      range[1] = scalarRangeDom->GetMaximum(0, exists);
      absMaxRange = fabs(range[0]);
      absMaxRange = (fabs(range[1]) > absMaxRange) ? fabs(range[1]) :
        absMaxRange;
      }
    }
  else if (!strcmp(scaleMode, "Vector Magnitude") && vectorProp)
    {
    const char *arrayName = this->VectorsMenu->GetValue();
    vectorProp->SetUncheckedElement(0, arrayName);
    vectorProp->UpdateDependentDomains();
    if (arrayName)
      {
      double range[2];
      range[0] = vectorRangeDom->GetMinimum(3, exists);
      range[1] = vectorRangeDom->GetMaximum(3, exists);
      absMaxRange = fabs(range[0]);
      absMaxRange = (fabs(range[1]) > absMaxRange) ? fabs(range[1]) :
        absMaxRange;
      }
    }
  else if (!strcmp(scaleMode, "Vector Components") && vectorProp)
    {
    const char *arrayName = this->VectorsMenu->GetValue();
    vectorProp->SetUncheckedElement(0, arrayName);
    vectorProp->UpdateDependentDomains();
    if (arrayName)
      {
      double range0[2], range1[2], range2[2];
      range0[0] = vectorRangeDom->GetMinimum(0, exists);
      range0[1] = vectorRangeDom->GetMaximum(0, exists);
      range1[0] = vectorRangeDom->GetMinimum(1, exists);
      range1[1] = vectorRangeDom->GetMaximum(1, exists);
      range2[0] = vectorRangeDom->GetMinimum(2, exists);
      range2[1] = vectorRangeDom->GetMaximum(2, exists);
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
  
  if (absMaxRange != 0)
    {
    maxBnds /= absMaxRange;
    }

  this->ScaleFactorEntry->SetValue(maxBnds);
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
    pvosw->SetSMScalarPropertyName(this->SMScalarPropertyName);
    pvosw->SetSMVectorPropertyName(this->SMVectorPropertyName);
    pvosw->SetSMOrientModePropertyName(this->SMOrientModePropertyName);
    pvosw->SetSMScaleModePropertyName(this->SMScaleModePropertyName);
    pvosw->SetSMScaleFactorPropertyName(this->SMScaleFactorPropertyName);
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
    imw->Delete();
    }

  const char *scalar_property = element->GetAttribute("scalar_property");
  if (scalar_property)
    {
    this->SetSMScalarPropertyName(scalar_property);
    }
  const char *vector_property = element->GetAttribute("vector_property");
  if (vector_property)
    {
    this->SetSMVectorPropertyName(vector_property);
    }
  const char *orient_mode_property =
    element->GetAttribute("orient_mode_property");
  if (orient_mode_property)
    {
    this->SetSMOrientModePropertyName(orient_mode_property);
    }
  const char *scale_mode_property =
    element->GetAttribute("scale_mode_property");
  if (scale_mode_property)
    {
    this->SetSMScaleModePropertyName(scale_mode_property);
    }
  const char *scale_factor_property =
    element->GetAttribute("scale_factor_property");
  if (scale_factor_property)
    {
    this->SetSMScaleFactorPropertyName(scale_factor_property);
    }
  
  return 1;
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
void vtkPVOrientScaleWidget::Accept()
{
  vtkSMStringVectorProperty *scalarProp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetSMScalarProperty());
  vtkSMStringVectorProperty *vectorProp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetSMVectorProperty());
  vtkSMIntVectorProperty *orientModeProp =
    vtkSMIntVectorProperty::SafeDownCast(this->GetSMOrientModeProperty());
  vtkSMIntVectorProperty *scaleModeProp =
    vtkSMIntVectorProperty::SafeDownCast(this->GetSMScaleModeProperty());
  vtkSMDoubleVectorProperty *scaleFactorProp =
    vtkSMDoubleVectorProperty::SafeDownCast(this->GetSMScaleFactorProperty());

  if (scalarProp)
    {
    scalarProp->SetElement(0, this->ScalarsMenu->GetValue());
    }
  if (vectorProp)
    {
    vectorProp->SetElement(0, this->VectorsMenu->GetValue());
    }
  if (orientModeProp)
    {
    orientModeProp->SetElement(0, this->OrientModeMenu->GetMenu()->GetIndex(
      this->OrientModeMenu->GetValue()));
    }
  if (scaleModeProp)
    {
    scaleModeProp->SetElement(0, this->ScaleModeMenu->GetMenu()->GetIndex(
      this->ScaleModeMenu->GetValue()));
    }
  if (scaleFactorProp)
    {
    scaleFactorProp->SetElement(0, this->ScaleFactorEntry->GetValueAsFloat());
    }

  this->Superclass::Accept();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::Initialize()
{
  this->Update();
  // Push the values to the property so that reset works properly
  this->Accept();
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::ResetInternal()
{

  vtkSMStringVectorProperty *scalarProp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetSMScalarProperty());
  vtkSMStringVectorProperty *vectorProp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetSMVectorProperty());
  vtkSMIntVectorProperty *orientModeProp =
    vtkSMIntVectorProperty::SafeDownCast(this->GetSMOrientModeProperty());
  vtkSMIntVectorProperty *scaleModeProp =
    vtkSMIntVectorProperty::SafeDownCast(this->GetSMScaleModeProperty());
  vtkSMDoubleVectorProperty *scaleFactorProp =
    vtkSMDoubleVectorProperty::SafeDownCast(this->GetSMScaleFactorProperty());

  if (orientModeProp)
    {
    this->OrientModeMenu->SetValue(
      this->OrientModeMenu->GetEntryLabel(orientModeProp->GetElement(0)));
    this->SetCurrentOrientMode(this->OrientModeMenu->GetValue());
    }
  if (scaleModeProp)
    {
    this->ScaleModeMenu->SetValue(
      this->ScaleModeMenu->GetEntryLabel(scaleModeProp->GetElement(0)));
    this->SetCurrentScaleMode(this->ScaleModeMenu->GetValue());
    }

  if (scalarProp)
    {
    this->ScalarsMenu->SetValue(scalarProp->GetElement(0));
    this->SetCurrentScalars(scalarProp->GetElement(0));
    }
  if (vectorProp)
    {
    this->VectorsMenu->SetValue(vectorProp->GetElement(0));
    this->SetCurrentVectors(vectorProp->GetElement(0));
    }    
  if (scaleFactorProp)
    {
    this->ScaleFactorEntry->SetValue(scaleFactorProp->GetElement(0));
    }

  this->ModifiedFlag = 0;
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
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::SaveInBatchScript(ofstream* file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMScalarPropertyName ||
      !this->SMVectorPropertyName || !this->SMOrientModePropertyName ||
      !this->SMScaleModePropertyName || !this->SMScaleFactorPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->SMScalarPropertyName << "] SetElement 0 {" 
        << this->ScalarsMenu->GetValue() << "}" << endl;
  *file << "  " << "[$pvTemp" << sourceID <<" GetProperty " 
        << this->SMVectorPropertyName << "] SetElement 0 {" 
        << this->VectorsMenu->GetValue() << "}" << endl;
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->SMOrientModePropertyName << "] SetElement 0 " 
        << this->OrientModeMenu->GetMenu()->GetIndex(
          this->OrientModeMenu->GetValue())
        << endl;;
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->SMScaleModePropertyName << "] SetElement 0 " 
        << this->ScaleModeMenu->GetMenu()->GetIndex(
          this->ScaleModeMenu->GetValue())
        << endl;
  *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << this->SMScaleFactorPropertyName
        << "] SetElement 0 " << this->ScaleFactorEntry->GetValueAsFloat()
        << endl;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkPVOrientScaleWidget::GetSMScalarProperty()
{
  if (this->SMScalarProperty)
    {
    return this->SMScalarProperty;
    }

  if (!this->GetPVSource() || !this->GetPVSource()->GetProxy())
    {
    return 0;
    }

  this->SetSMScalarProperty(this->GetPVSource()->GetProxy()->GetProperty(
    this->GetSMScalarPropertyName()));

  return this->SMScalarProperty;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkPVOrientScaleWidget::GetSMVectorProperty()
{
  if (this->SMVectorProperty)
    {
    return this->SMVectorProperty;
    }

  if (!this->GetPVSource() || !this->GetPVSource()->GetProxy())
    {
    return 0;
    }

  this->SetSMVectorProperty(this->GetPVSource()->GetProxy()->GetProperty(
    this->GetSMVectorPropertyName()));

  return this->SMVectorProperty;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkPVOrientScaleWidget::GetSMOrientModeProperty()
{
  if (this->SMOrientModeProperty)
    {
    return this->SMOrientModeProperty;
    }

  if (!this->GetPVSource() || !this->GetPVSource()->GetProxy())
    {
    return 0;
    }

  this->SetSMOrientModeProperty(this->GetPVSource()->GetProxy()->GetProperty(
    this->GetSMOrientModePropertyName()));

  return this->SMOrientModeProperty;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkPVOrientScaleWidget::GetSMScaleModeProperty()
{
  if (this->SMScaleModeProperty)
    {
    return this->SMScaleModeProperty;
    }

  if (!this->GetPVSource() || !this->GetPVSource()->GetProxy())
    {
    return 0;
    }

  this->SetSMScaleModeProperty(this->GetPVSource()->GetProxy()->GetProperty(
    this->GetSMScaleModePropertyName()));

  return this->SMScaleModeProperty;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkPVOrientScaleWidget::GetSMScaleFactorProperty()
{
  if (this->SMScaleFactorProperty)
    {
    return this->SMScaleFactorProperty;
    }

  if (!this->GetPVSource() || !this->GetPVSource()->GetProxy())
    {
    return 0;
    }

  this->SetSMScaleFactorProperty(this->GetPVSource()->GetProxy()->GetProperty(
    this->GetSMScaleFactorPropertyName()));

  return this->SMScaleFactorProperty;
}

//----------------------------------------------------------------------------
void vtkPVOrientScaleWidget::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
