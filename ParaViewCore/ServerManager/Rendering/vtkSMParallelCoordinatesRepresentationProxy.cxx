/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParallelCoordinatesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMParallelCoordinatesRepresentationProxy.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkContextView.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkPlot.h"
#include "vtkPen.h"
#include "vtkTable.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkStringArray.h"
#include "vtkStringList.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMParallelCoordinatesRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMParallelCoordinatesRepresentationProxy::vtkSMParallelCoordinatesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMParallelCoordinatesRepresentationProxy::~vtkSMParallelCoordinatesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::UpdatePropertyInformationInternal(
  vtkSMProperty* vtkNotUsed(prop))
{
#ifdef FIXME_COLLABORATION
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp || !svp->GetInformationOnly() ||
      !vtkTable::SafeDownCast(this->GetOutput()))
    {
    return;
    }

  vtkTable* table = vtkTable::SafeDownCast(this->GetOutput());

  bool skip = false;
  const char* propertyName = this->GetPropertyName(prop);
  vtkSmartPointer<vtkStringList> newValues = vtkSmartPointer<vtkStringList>::New();

  int numberOfColumns = table->GetNumberOfColumns();
  for (int i = 0; i < numberOfColumns; ++i)
    {
    const char* seriesName = table->GetColumnName(i);
    if (!seriesName)
      {
      continue;
      }

    newValues->AddString(seriesName);

    if (strcmp(propertyName, "SeriesVisibilityInfo") == 0)
      {
      if (i < 10)
        {
        newValues->AddString("1");
        }
      else
        {
        newValues->AddString("0");
        }
      }
    else if (strcmp(propertyName, "LabelInfo") == 0)
      {
      newValues->AddString(seriesName);
      }
    else if (strcmp(propertyName, "LineThicknessInfo") == 0)
      {
      newValues->AddString("2");
      }
    else if (strcmp(propertyName, "ColorInfo") == 0)
      {
      newValues->AddString("0");
      newValues->AddString("0");
      newValues->AddString("0");
      }
    else if (strcmp(propertyName, "OpacityInfo") == 0)
      {
      newValues->AddString("0.10");
      }
    else if (strcmp(propertyName, "LineStyleInfo") == 0)
      {
      newValues->AddString("1");
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
#endif
}
