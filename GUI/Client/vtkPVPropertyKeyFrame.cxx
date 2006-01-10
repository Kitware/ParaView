/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPropertyKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPropertyKeyFrame.h"

#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWThumbWheel.h"
#include "vtkObjectFactory.h"
#include "vtkPVCutEntry.h"
#include "vtkPVSelectionList.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXDMFPropertyDomain.h"

//*****************************************************************************
//Helper methods to down cast the property and set value.
inline static int DoubleVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, double val, int index = 0)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}

//-----------------------------------------------------------------------------
inline static int IntVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, int val, int index = 0)
{
  vtkSMIntVectorProperty* dvp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}


vtkCxxRevisionMacro(vtkPVPropertyKeyFrame, "1.10");
//-----------------------------------------------------------------------------
vtkPVPropertyKeyFrame::vtkPVPropertyKeyFrame()
{
  this->ValueLabel = vtkKWLabel::New();
  this->ValueWidget = NULL;
  this->MinButton = vtkKWPushButton::New();
  this->MaxButton = vtkKWPushButton::New();
}

//-----------------------------------------------------------------------------
vtkPVPropertyKeyFrame::~vtkPVPropertyKeyFrame()
{
  this->ValueLabel->Delete();
  if (this->ValueWidget)
    {
    this->ValueWidget->Delete();
    }
  this->MaxButton->Delete();
  this->MinButton->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::ChildCreate()
{
  this->Superclass::ChildCreate();

  this->ValueLabel->SetParent(this);
  this->ValueLabel->Create();
  this->ValueLabel->SetText("Value:");
  this->CreateValueWidget();

  this->MinButton->SetParent(this);
  this->MinButton->Create();
  this->MinButton->SetText("min");
  this->MinButton->SetBalloonHelpString(
    "Set the value to the minimum possible, given the "
    "current state of the system.");
  this->MinButton->SetCommand(this,"MinimumCallback");
  this->MaxButton->SetParent(this);
  this->MaxButton->Create();
  this->MaxButton->SetText("max");
  this->MaxButton->SetBalloonHelpString(
    "Set the value to the maximum possible, given the "
    "current state of the system.");
  this->MaxButton->SetCommand(this, "MaximumCallback");

  if (this->ValueWidget)
    {
    this->Script("grid %s %s x x x -sticky w",
      this->ValueLabel->GetWidgetName(),
      this->ValueWidget->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::CreateValueWidget()
{
  vtkSMAnimationCueProxy* cueProxy = this->AnimationCueProxy;
  vtkSMProperty* property = cueProxy->GetAnimatedProperty();
  vtkSMDomain* domain = cueProxy->GetAnimatedDomain();
  int animated_element = cueProxy->GetAnimatedElement();

  if (!domain || !property)
    {
    vtkErrorMacro("Animated domain/property not specified!");
    //don't create a value widget.
    return;
    }
  // 4 Types of widgets: SelectionList, Checkbox, Thumbwheel
  // and PVContourEntry.
  vtkSMBooleanDomain* bd = vtkSMBooleanDomain::SafeDownCast(domain);
  vtkSMEnumerationDomain* ed = vtkSMEnumerationDomain::SafeDownCast(domain);
  vtkSMStringListDomain* sld = vtkSMStringListDomain::SafeDownCast(domain);

  if (animated_element==-1)
    {
    // For now, I will only support multiple value widgets for double vector
    // property alone (since I use the vtkPVContourEntry (which accepts only
    // doubles).
    if (!vtkSMDoubleVectorProperty::SafeDownCast(property))
      {
      vtkWarningMacro("Array List domains are currently supported for "
        " vtkSMDoubleVectorProperty alone.");
      return;
      }
    vtkPVContourEntry* valueList;
    if (domain->IsA("vtkSMBoundsDomain"))
      {
      valueList = vtkPVCutEntry::New();
      }
    else
      {
      valueList = vtkPVContourEntry::New();
      }
    valueList->SetParent(this);
    // vtkPVContourEntry complains if the property is not set.
    valueList->SetSMProperty(property);
    valueList->Create();
    valueList->SetModifiedCommand(this->GetTclName(),"ValueChangedCallback");
    this->ValueWidget = valueList;
    }
  else
    {
    if (bd || ed || sld)
      {
      vtkPVSelectionList* pvList = vtkPVSelectionList::New();
      pvList->SetParent(this);
      pvList->SetLabelVisibility(0);
      pvList->Create();
      pvList->SetModifiedCommand(this->GetTclName(), "ValueChangedCallback");
      this->ValueWidget = pvList;
      }
    else // even for xdmfd we create a thumbwheel.
      {
      vtkKWThumbWheel* pvWheel = vtkKWThumbWheel::New();
      pvWheel->SetParent(this);
      pvWheel->PopupModeOn();
      pvWheel->Create();
      pvWheel->DisplayEntryOn();
      pvWheel->DisplayLabelOff();
      pvWheel->DisplayEntryAndLabelOnTopOff();
      pvWheel->ExpandEntryOn();
      pvWheel->SetEntryCommand(this, "ThumbWheelValueChangedCallback");
      pvWheel->SetEndCommand(this, "ThumbWheelValueChangedCallback");
      this->ValueWidget = pvWheel;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SetValueToMinimum()
{
  this->UpdateDomain();
  vtkKWThumbWheel* pvWheel = vtkKWThumbWheel::SafeDownCast(this->ValueWidget);
  vtkPVSelectionList *pvSelect = 
    vtkPVSelectionList::SafeDownCast(this->ValueWidget);
  vtkPVContourEntry* pvContour = vtkPVContourEntry::SafeDownCast(
    this->ValueWidget);
  
  if (pvWheel && pvWheel->GetClampMinimumValue())
    {
    this->SetKeyValue(pvWheel->GetMinimumValue());
    }
  else if (pvSelect && pvSelect->GetNumberOfItems() > 0)
    {
    this->SetKeyValue(0);
    }
  else if (pvContour)
    {
    vtkSMDoubleRangeDomain* domain = vtkSMDoubleRangeDomain::SafeDownCast(
      this->AnimationCueProxy->GetAnimatedDomain());
    if (domain)
      {
      int exists;
      double min = domain->GetMinimum(0, exists);
      if (exists)
        {
        this->SetKeyValue(0, min);
        }
      }
    }
  this->UpdateValuesFromProxy();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SetValueToMaximum()
{
  this->UpdateDomain();
  vtkKWThumbWheel* pvWheel = vtkKWThumbWheel::SafeDownCast(this->ValueWidget);
  vtkPVSelectionList *pvSelect = 
    vtkPVSelectionList::SafeDownCast(this->ValueWidget);
  vtkPVContourEntry* pvContour = vtkPVContourEntry::SafeDownCast(
    this->ValueWidget);

  if (pvWheel && pvWheel->GetClampMaximumValue())
    {
    this->SetKeyValue(pvWheel->GetMaximumValue());
    }
  else if (pvSelect && pvSelect->GetNumberOfItems() > 0)
    {
    this->SetKeyValue(pvSelect->GetNumberOfItems()-1);
    } 
  else if (pvContour)
    {
    vtkSMDoubleRangeDomain* domain = vtkSMDoubleRangeDomain::SafeDownCast(
      this->AnimationCueProxy->GetAnimatedDomain());
    if (domain)
      {
      int exists;
      double max = domain->GetMaximum(0, exists);
      if (exists)
        {
        this->SetKeyValue(0, max);
        }
      }
    }
  this->UpdateValuesFromProxy();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::MinimumCallback()
{
  this->SetValueToMinimum();
  this->GetTraceHelper()->AddEntry("$kw(%s) MinimumCallback", 
    this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::MaximumCallback()
{
  this->SetValueToMaximum();
  this->GetTraceHelper()->AddEntry("$kw(%s) MaximumCallback", 
    this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::InitializeKeyValueUsingProperty(
  vtkSMProperty* property, int index)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  if (!property )
    {
    vtkErrorMacro("Invalid property");
    return;
    }

  if (index == -1)
    {
    vtkPVContourEntry* contourEntry = vtkPVContourEntry::SafeDownCast(
      this->ValueWidget);
    if (contourEntry)
      {
      // This may be the unconventional way, but we achieve the 
      // updating of the animated property by letting the GUI update
      // itself first and then update the animated property values.
      vtkSMProperty* oldProp = contourEntry->GetSMProperty();
      contourEntry->SetSMProperty(property);
      contourEntry->Initialize(); // this will update the GUI using the property. 
      contourEntry->SetSMProperty(oldProp); // restore the property
      this->UpdateValueFromGUI();
      }
    return;
    }
  if (vtkSMVectorProperty::SafeDownCast(property))
    {
    if (
      static_cast<int>(
        (vtkSMVectorProperty::SafeDownCast(property)->GetNumberOfElements())) 
      <= index)
      {
      vtkErrorMacro(<<"Invalid index " << index << " for property.");
      return;
      }
    }
  if (vtkSMDoubleVectorProperty::SafeDownCast(property))
    {
    this->SetKeyValue(
      vtkSMDoubleVectorProperty::SafeDownCast(property)->GetElement(index));
    }
  else if (vtkSMIntVectorProperty::SafeDownCast(property))
    {
    this->SetKeyValue(static_cast<double>(vtkSMIntVectorProperty::SafeDownCast(
          property)->GetElement(index)));
    }
  else if (vtkSMIdTypeVectorProperty::SafeDownCast(property))
    {
    this->SetKeyValue(
      static_cast<double>(vtkSMIdTypeVectorProperty::SafeDownCast(
          property)->GetElement(index)));
    }
  else if (vtkSMStringVectorProperty::SafeDownCast(property))
    {
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      property);
    vtkSMAnimationCueProxy* cueProxy = this->AnimationCueProxy;
    vtkSMDomain* domain = cueProxy->GetAnimatedDomain();
    vtkSMXDMFPropertyDomain* xdmfd = 
      vtkSMXDMFPropertyDomain::SafeDownCast(domain);
    if (xdmfd)
      {
      const char* name = xdmfd->GetString(index);
      if (name)
        {
        int found = 0;
        unsigned int idx = svp->GetElementIndex(name, found);
        if (found)
          {
          int val = 0;
          val = atoi(svp->GetElement(idx + 1));
          this->SetKeyValue(static_cast<double>(val));
          }
        else
          {
          vtkErrorMacro("Could not find an appropriate property value "
            "matching the domain. Can not update keyframe.");
          }
        }
      }
    else
      {
      const char* string = svp->GetElement(index);
      vtkPVSelectionList* pvList = 
        vtkPVSelectionList::SafeDownCast(this->ValueWidget); 
      if (string && pvList)
        {
        // find the index for this string in the widget / or domain.
        int vindex = pvList->GetValue(string);
        if (vindex != -1)
          {
          this->SetKeyValue(static_cast<double>(vindex));
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::UpdateDomain()
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  if (!this->ValueWidget)
    {
    vtkErrorMacro("ValueWidget must be created before updating domain");
    return;
    }
  
  vtkSMAnimationCueProxy* cueProxy = this->AnimationCueProxy;
  vtkSMDomain* domain = cueProxy->GetAnimatedDomain();
  int index = cueProxy->GetAnimatedElement();
  
  vtkSMBooleanDomain* bd = vtkSMBooleanDomain::SafeDownCast(domain);
  vtkSMEnumerationDomain* ed = vtkSMEnumerationDomain::SafeDownCast(domain);
  vtkSMStringListDomain* sld = vtkSMStringListDomain::SafeDownCast(domain);
  vtkSMDoubleRangeDomain* drd = vtkSMDoubleRangeDomain::SafeDownCast(domain);
  vtkSMIntRangeDomain* ird = vtkSMIntRangeDomain::SafeDownCast(domain);
  vtkSMXDMFPropertyDomain* xdmfd =
    vtkSMXDMFPropertyDomain::SafeDownCast(domain);
  // TODO: Actually, it would have been neat if we could compare the MTimes for the
  // widgets and the domains, but so happens that none of the
  // PVWidgets or SMDomains update MTime properly. Should correct that first.
  if (index == -1)
    {
    return;
    }
  
  if (bd)
    {
    // Domain does not change for boolean.
    vtkPVSelectionList* pvList = 
      vtkPVSelectionList::SafeDownCast(this->ValueWidget);
    if (pvList->GetNumberOfItems() != 2)
      {
      pvList->RemoveAllItems();
      pvList->AddItem("Off", 0);
      pvList->AddItem("On", 1);
      }
    }
  else if (ed)
    {
    vtkPVSelectionList* pvList = 
      vtkPVSelectionList::SafeDownCast(this->ValueWidget);
    // Update PVSelectionList using emumerated elements.
    if (pvList && (pvList->GetMTime() <= ed->GetMTime() || 
                   pvList->GetNumberOfItems()==0))
      {
      pvList->RemoveAllItems();
      for (unsigned int cc=0; cc < ed->GetNumberOfEntries(); cc++)
        {
        const char* text = ed->GetEntryText(cc);
        int value = ed->GetEntryValue(cc);
        pvList->AddItem(text, value);
        }
      }
    }
  else if (sld)
    {
    vtkPVSelectionList* pvList = 
      vtkPVSelectionList::SafeDownCast(this->ValueWidget);
    // Update PVSelectionList using strings.
    if (pvList && (pvList->GetMTime() <= sld->GetMTime() || 
                   pvList->GetNumberOfItems()==0))
      {
      pvList->RemoveAllItems();
      for (unsigned int cc=0; cc < sld->GetNumberOfStrings(); cc++)
        {
        pvList->AddItem(sld->GetString(cc), cc);
        }
      }
    }
  else if (xdmfd)
    {
    vtkKWThumbWheel* wheel = vtkKWThumbWheel::SafeDownCast(this->ValueWidget);
    wheel->SetResolution(1);
    int minexists, maxexists;
    int min = xdmfd->GetMinimum(index, minexists);
    int max = xdmfd->GetMaximum(index, maxexists);
    const char* name = xdmfd->GetString(index);
    if (minexists && maxexists && name)
      {
      wheel->SetMinimumValue(min);
      wheel->SetMaximumValue(max);
      wheel->ClampMinimumValueOn();
      wheel->ClampMaximumValueOn();
      }
    }
  else if (drd || ird)
    {
    int hasmin=0;
    int hasmax=0;
    double min;
    double max;
    int column = 2;
    vtkKWThumbWheel* wheel = vtkKWThumbWheel::SafeDownCast(this->ValueWidget);
    if (drd)
      {
      min = drd->GetMinimum(index, hasmin);
      max = drd->GetMaximum(index, hasmax);
      wheel->SetResolution(0.01);
      }
    else //if(ird)
      {
      min = ird->GetMinimum(index, hasmin);
      max = ird->GetMaximum(index, hasmax);
      wheel->SetResolution(1);
      }
    if (hasmin)
      {
      wheel->SetMinimumValue(min);
      wheel->ClampMinimumValueOn();
      this->Script("grid %s -column %d -row 1", 
                   this->MinButton->GetWidgetName(), column);
      column++;
      }
    else
      {
      wheel->ClampMinimumValueOff();
      this->Script("grid forget %s", this->MinButton->GetWidgetName());
      }
    if (hasmax)
      {
      wheel->SetMaximumValue(max);
      wheel->ClampMaximumValueOn();
      this->Script("grid %s -column %d -row 1", 
                   this->MaxButton->GetWidgetName(), column);
      }
    else
      {
      wheel->ClampMaximumValueOff();
      this->Script("grid forget %s", this->MaxButton->GetWidgetName());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::InitializeKeyValueUsingCurrentState()
{
  if (!this->ValueWidget)
    {
    return;
    }
  vtkSMAnimationCueProxy* cueProxy = this->AnimationCueProxy;
  vtkSMProperty* property = cueProxy->GetAnimatedProperty();
  int index = cueProxy->GetAnimatedElement();
  this->InitializeKeyValueUsingProperty(property, index);
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::InitializeKeyValueDomainUsingCurrentState()
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  this->UpdateDomain();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::ValueChangedCallback()
{
  this->UpdateValueFromGUI();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::ThumbWheelValueChangedCallback(double)
{
  this->ValueChangedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::UpdateValueFromGUI()
{
  this->BlockUpdates = 1; //we are pushing GUI values on to proxy,
                          //hence, no need to update GUI when proxy changes.
  if (vtkPVSelectionList::SafeDownCast(this->ValueWidget))
    {
    this->SetKeyValueWithTrace(0, static_cast<double>(
      vtkPVSelectionList::SafeDownCast(this->ValueWidget)->GetCurrentValue()));
    }
  else if (vtkKWThumbWheel::SafeDownCast(this->ValueWidget))
    {
    this->SetKeyValueWithTrace(0, 
      vtkKWThumbWheel::SafeDownCast(this->ValueWidget)->GetEntry()->
      GetValueAsDouble());
    }
  else if (vtkPVContourEntry::SafeDownCast(this->ValueWidget))
    {
    vtkPVContourEntry* contourEntry = 
      vtkPVContourEntry::SafeDownCast(this->ValueWidget);
    
    int numContours = contourEntry->GetNumberOfValues();
    this->SetNumberOfKeyValuesWithTrace(numContours);
    for (int i=0; i < numContours; i++)
      {
      this->SetKeyValueWithTrace(i, contourEntry->GetValue(i));
      }
    }
  this->BlockUpdates = 0;
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::UpdateValuesFromProxy()
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  double keyvalue = this->KeyFrameProxy->GetKeyValue();
  if (vtkPVSelectionList::SafeDownCast(this->ValueWidget))
    {
    vtkPVSelectionList::SafeDownCast(this->ValueWidget)->SetCurrentValue(
      static_cast<int>(keyvalue));
    }
  else if (vtkKWThumbWheel::SafeDownCast(this->ValueWidget))
    {
    vtkKWThumbWheel::SafeDownCast(this->ValueWidget)->SetValue(
      keyvalue);
    }
  else if (vtkPVContourEntry::SafeDownCast(this->ValueWidget))
    {
    vtkPVContourEntry* pvContour = vtkPVContourEntry::SafeDownCast(
      this->ValueWidget);
    pvContour->SetModifiedCommand(0, 0);
    pvContour->RemoveAllValues();
    int numVals = this->GetNumberOfKeyValues();
    for (int i=0; i < numVals; i++)
      {
      pvContour->AddValue(this->GetKeyValue(i));
      }
    pvContour->SetModifiedCommand(this->GetTclName(),"ValueChangedCallback");
    }

  this->Superclass::UpdateValuesFromProxy();
}
//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SetKeyValueWithTrace(int index, double val)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  this->SetKeyValue(index, val);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyValueWithTrace %d %f", 
    this->GetTclName(), index, val);
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SetKeyValue(int index, double val)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  DoubleVectPropertySetElement(this->KeyFrameProxy, "KeyValues", val, index);
  this->KeyFrameProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SetNumberOfKeyValuesWithTrace(int num)
{

  this->SetNumberOfKeyValues(num);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetNumberOfKeyValuesWithTrace %d", 
    this->GetTclName(), num);
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SetNumberOfKeyValues(int num)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  IntVectPropertySetElement(this->KeyFrameProxy, "NumberOfKeyValues", num);
  this->KeyFrameProxy->UpdateVTKObjects();

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty("KeyValues"));
  dvp->SetNumberOfElements(num);
  
}

//-----------------------------------------------------------------------------
int vtkPVPropertyKeyFrame::GetNumberOfKeyValues()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty("NumberOfKeyValues"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property NumberOfKeyValues.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
double vtkPVPropertyKeyFrame::GetKeyValue(int index)
{
  return this->KeyFrameProxy->GetKeyValue(index);
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::Copy(vtkPVKeyFrame* fromKF)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  this->Superclass::Copy(fromKF);
  
  vtkPVPropertyKeyFrame* from = vtkPVPropertyKeyFrame::SafeDownCast(fromKF);
  if (from)
    {
    int nos = from->GetNumberOfKeyValues();
    this->SetNumberOfKeyValues(nos);
    for (int i=0; i < nos; i++)
      {
      this->SetKeyValue(i, from->GetKeyValue(i));
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  if (this->ValueWidget)
    {
    this->PropagateEnableState(this->ValueWidget);
    }
}

//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);
  for (unsigned int i=0; i < this->KeyFrameProxy->GetNumberOfKeyValues(); i++)
    {
    *file << "$kw(" << this->GetTclName() << ") SetKeyValue " << i << " "
      << this->GetKeyValue(i) << endl;
    }
}
//-----------------------------------------------------------------------------
void vtkPVPropertyKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

