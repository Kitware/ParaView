/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCutEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCutEntry.h"

#include "vtkPVInputMenu.h"
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVDataInformation.h"
#include "vtkPVImplicitPlaneWidget.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCutEntry);
vtkCxxRevisionMacro(vtkPVCutEntry, "1.6.2.2");

vtkCxxSetObjectMacro(vtkPVCutEntry, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
int vtkPVCutEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//-----------------------------------------------------------------------------
vtkPVCutEntry::vtkPVCutEntry()
{
  this->CommandFunction = vtkPVCutEntryCommand;
  this->InputMenu = NULL;
}

//-----------------------------------------------------------------------------
vtkPVCutEntry::~vtkPVCutEntry()
{
  this->SetInputMenu(NULL);
}

//-----------------------------------------------------------------------------
int vtkPVCutEntry::ComputeWidgetRange()
{
  if (!this->InputMenu)
    {
    return 0;
    }
  vtkPVSource* input = this->InputMenu->GetCurrentValue();
  if (!input)
    {
    return 0;
    }
  
  double bounds[6];
  input->GetDataInformation()->GetBounds(bounds);
  float length = sqrt(
    static_cast<double>(
      (bounds[1]-bounds[0])*(bounds[1]-bounds[0])+
      (bounds[3]-bounds[2])*(bounds[3]-bounds[2])+
      (bounds[5]-bounds[4])*(bounds[5]-bounds[4])));
  
  this->WidgetRange[0] = -length;
  this->WidgetRange[1] =  length;
  this->UseWidgetRange = 1;
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVCutEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }

  char methodAndArgs[500];

  sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName());
  ai->GetResetRangeButton()->SetCommand(this, methodAndArgs);
  ai->SetResetRangeButtonState(1);
  ai->UpdateEnableState();

  vtkPVSource* input = this->InputMenu->GetCurrentValue();
  if (!input)
    {
    return;
    }

  double bounds[6];
  input->GetDataInformation()->GetBounds(bounds);

  vtkPVSelectWidget* sw = vtkPVSelectWidget::SafeDownCast(
    this->GetPVSource()->GetPVWidget("Cut Function"));

  vtkPVImplicitPlaneWidget* ipw = 0;
  if (sw)
    {
    ipw = vtkPVImplicitPlaneWidget::SafeDownCast(
      sw->GetPVWidget(sw->GetCurrentValue()));
    }

  double min = 0, max = 0;

  if (ipw)
    {
    double originv[3], normalv[3];

    ipw->GetLastAcceptedCenter(originv);
    ipw->GetLastAcceptedNormal(normalv);

    double points[8][3];

    double xmin = bounds[0];
    double xmax = bounds[1];
    double ymin = bounds[2];
    double ymax = bounds[3];
    double zmin = bounds[4];
    double zmax = bounds[5];

    points[0][0] = xmin; points[0][1] = ymin; points[0][2] = zmin;
    points[1][0] = xmax; points[1][1] = ymax; points[1][2] = zmax;
    points[2][0] = xmin; points[2][1] = ymin; points[2][2] = zmax;
    points[3][0] = xmin; points[3][1] = ymax; points[3][2] = zmax;
    points[4][0] = xmin; points[4][1] = ymax; points[4][2] = zmin;
    points[5][0] = xmax; points[5][1] = ymax; points[5][2] = zmin;
    points[6][0] = xmax; points[6][1] = ymin; points[6][2] = zmin;
    points[7][0] = xmax; points[7][1] = ymin; points[7][2] = zmax;

    int i;
    int j;
    double dist[8];

    for(i=0; i<8; i++)
      {
      dist[i] = 0;
      for(j=0; j<3; j++)
        {
        dist[i] += (points[i][j] - originv[j])*normalv[j];
        }
      }

    min = dist[0];
    max = dist[0];
    for (i=1; i<8; i++)
      {
      if ( dist[i] < min )
        {
        min = dist[i];
        }
      if ( dist[i] > max )
        {
        max = dist[i];
        }
      }
    }

  ai->SetLabelAndScript(this->GetTraceName(), NULL, this->GetTraceName());
  ai->SetAnimationElement(0);

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDoubleRangeDomain *rangeDomain = vtkSMDoubleRangeDomain::SafeDownCast(
    prop->GetDomain("range"));
    

  if (!rangeDomain)
    {
    return;
    }

  ai->SetCurrentSMProperty(prop);
  ai->SetCurrentSMDomain(rangeDomain);

  if (ipw)
    {
    ai->SetTimeStart(min);
    ai->SetTimeEnd(max);
    }
  else
    {
    int minExists, maxExists;
    min = rangeDomain->GetMinimum(0, minExists);
    max = rangeDomain->GetMaximum(0, maxExists);
    if (minExists && maxExists)
      {
      ai->SetTimeStart(min);
      ai->SetTimeEnd(max);
      }
    }

  ai->Update();

}

//----------------------------------------------------------------------------
void vtkPVCutEntry::CopyProperties(
  vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVCutEntry* pvce = vtkPVCutEntry::SafeDownCast(clone);
  if (pvce)
    {
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvce->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVCutEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVCutEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if (!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }

  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  if (!ime)
    {
    vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
    return 0;
    }
  
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVCutEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  
  this->PropagateEnableState(this->InputMenu);
}

//-----------------------------------------------------------------------------
void vtkPVCutEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
}
