/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextNamedOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMContextNamedOptionsProxy.h"

#include "vtkContextNamedOptions.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStdString.h"
#include "vtkStringList.h"
#include "vtkTable.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMContextNamedOptionsProxy);
//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::vtkSMContextNamedOptionsProxy()
{
}

//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::~vtkSMContextNamedOptionsProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::UpdatePropertyInformationInternal(
  vtkSMProperty* prop)
{
  vtkContextNamedOptions* options = vtkContextNamedOptions::SafeDownCast(
    this->GetClientSideObject());

  if (prop == NULL)
    {
    // update property information for all info properties.
    vtkSMPropertyIterator* iter = this->NewPropertyIterator();
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      if (iter->GetProperty() && iter->GetProperty()->GetInformationOnly())
        {
        this->UpdatePropertyInformationInternal(iter->GetProperty());
        }
      }
    iter->Delete();
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp || !svp->GetInformationOnly() || !options)
    {
    return;
    }

  vtkTable* table = options->GetTable();
  if (!table)
    {
    return;
    }

  bool skip = false;
  const char* propertyName = this->GetPropertyName(prop);
  vtkSmartPointer<vtkStringList> newValues = vtkSmartPointer<vtkStringList>::New();

  // Note: we could iterate over this->Internals->PlotMap, but just for
  // kicks we'll iterate over the table columns in order to respect the
  // column ordering, probably not needed...
  int numberOfColumns = table->GetNumberOfColumns();
  for (int i = 0; i < numberOfColumns; ++i)
    {
    const char* seriesName = table->GetColumnName(i);
    if (!seriesName)
      {
      continue;
      }

    newValues->AddString(seriesName);

    if (strcmp(propertyName, "VisibilityInfo") == 0)
      {
      newValues->AddString(options->GetVisibility(seriesName) ? "1" : "0");
      }
    else if (strcmp(propertyName, "LabelInfo") == 0)
      {
      newValues->AddString(options->GetLabel(seriesName));
      }
    else if (strcmp(propertyName, "LineThicknessInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << options->GetLineThickness(seriesName);
      newValues->AddString(string.str().c_str());
      }
    else if (strcmp(propertyName, "ColorInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      double color[3];
      options->GetColor(seriesName, color);
      for (int j = 0; j < 3; ++j)
        {
        string << color[j];
        newValues->AddString(string.str().c_str());
        string.str("");
        }
      }
    else if (strcmp(propertyName, "LineStyleInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << options->GetLineStyle(seriesName);
      newValues->AddString(string.str().c_str());
      }
    else if (strcmp(propertyName, "MarkerStyleInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << options->GetMarkerStyle(seriesName);
      newValues->AddString(string.str().c_str());
      }
    else if (strcmp(propertyName, "PlotCornerInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << options->GetAxisCorner(seriesName);
      newValues->AddString(string.str().c_str());
      }
    else
      {
      skip = true;
      break;
      }
    }
  if (!skip)
    {
    svp->SetElements(newValues);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
