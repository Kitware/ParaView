/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourWidgetProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVContourWidgetProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkPVSource.h"
#include "vtkPVWidget.h"

vtkStandardNewMacro(vtkPVContourWidgetProperty);
vtkCxxRevisionMacro(vtkPVContourWidgetProperty, "1.6");

void vtkPVContourWidgetProperty::SetAnimationTime(float time)
{
  if (this->NumberOfCommands > 1)
    {
    this->Scalars[1] = 0;
    this->Scalars[2] = time;
    }
  else
    {
    float scalars[3];
    scalars[0] = 1;
    scalars[1] = 0;
    scalars[2] = time;
    this->SetScalars(3, scalars);
    char **commands = new char *[2];
    int numScalars[2];
    numScalars[0] = 1;
    numScalars[1] = 2;
    commands[0] = new char[strlen(this->VTKCommands[0])+1];
    strcpy(commands[0], this->VTKCommands[0]);
    commands[1] = new char[9];
    sprintf(commands[1], "SetValue");
    this->SetVTKCommands(2, commands, numScalars);
    delete [] commands[0];
    delete [] commands[1];
    delete [] commands;
    }
  this->Widget->ModifiedCallback();
  this->Widget->Reset();
}

void vtkPVContourWidgetProperty::AcceptInternal()
{
  this->Superclass::AcceptInternal();

  vtkPVProcessModule *pm =
    this->Widget->GetPVApplication()->GetProcessModule();
  
  vtkPVClassNameInformation *cni = vtkPVClassNameInformation::New();
  pm->GatherInformation(cni, this->VTKSourceID);
  if (!strcmp(cni->GetVTKClassName(), "vtkPVContourFilter") ||
      !strcmp(cni->GetVTKClassName(), "vtkPVKitwareContourFilter"))
    {
    this->Widget->SetUseWidgetRange(1);
    vtkPVScalarRangeLabel *label = vtkPVScalarRangeLabel::SafeDownCast(
      this->Widget->GetPVSource()->GetPVWidget("ScalarRangeLabel"));
    if (label)
      {
      double *tmpRange = label->GetRange();
      double range[2];
      range[0] = tmpRange[0];
      range[1] = tmpRange[1];
      this->Widget->SetWidgetRange(range);
      }
    }
  cni->Delete();
}

void vtkPVContourWidgetProperty::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
