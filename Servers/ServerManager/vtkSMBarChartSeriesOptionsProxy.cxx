/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartSeriesOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBarChartSeriesOptionsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkQtBarChartSeriesOptions.h"
#include "vtkQtChartNamedSeriesOptionsModel.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"

#include <QBrush>
#include <QColor>

vtkStandardNewMacro(vtkSMBarChartSeriesOptionsProxy);
vtkCxxRevisionMacro(vtkSMBarChartSeriesOptionsProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMBarChartSeriesOptionsProxy::vtkSMBarChartSeriesOptionsProxy()
{
}

//----------------------------------------------------------------------------
vtkSMBarChartSeriesOptionsProxy::~vtkSMBarChartSeriesOptionsProxy()
{
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkSMBarChartSeriesOptionsProxy::NewOptions()
{
  return new vtkQtBarChartSeriesOptions();
}

//----------------------------------------------------------------------------
void vtkSMBarChartSeriesOptionsProxy::SetColor(
  const char* name, double r, double g, double b)
{
  vtkQtChartSeriesOptions* options = this->GetOptions(name);
  QBrush brush = options->getBrush();
  brush.setColor(QColor::fromRgbF(r, g, b));
  options->setBrush(brush);
}

//----------------------------------------------------------------------------
void vtkSMBarChartSeriesOptionsProxy::UpdatePropertyInformationInternal(
  vtkSMProperty* prop)
{
  this->Superclass::UpdatePropertyInformationInternal(prop);

  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && svp->GetInformationOnly())
    {
    vtkStringList* new_values = vtkStringList::New();
    vtkQtChartNamedSeriesOptionsModel* optionsModel = this->GetOptionsModel();
    int num_options = optionsModel->getNumberOfOptions();
    const char* propname = this->GetPropertyName(prop);
    bool skip = false;
    for (int cc=0; cc < num_options; cc++)
      {
      QString name = optionsModel->getSeriesName(cc);
      vtkQtBarChartSeriesOptions* options =
        qobject_cast<vtkQtBarChartSeriesOptions*>(
          this->GetOptions(name.toAscii().data()));
      if (strcmp(propname, "ColorInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());

        QBrush brush = options->getBrush();
        new_values->AddString(QString::number(brush.color().redF()).toAscii().data());
        new_values->AddString(QString::number(brush.color().greenF()).toAscii().data());
        new_values->AddString(QString::number(brush.color().blueF()).toAscii().data());
        }
      else
        {
        skip = true;
        break;
        }
      }
    if (!skip)
      {
      svp->SetElements(new_values);
      }
    new_values->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMBarChartSeriesOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


