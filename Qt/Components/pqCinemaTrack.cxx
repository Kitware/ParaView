/*=========================================================================

   Program: ParaView
   Module:    pqSGExportStateWizard.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#include <string>

#include "pqCinemaTrack.h"

#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtksys/SystemTools.hxx>

#include <pqPipelineFilter.h>
#include <pqScalarValueListPropertyWidget.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>

#include "ui_pqCinemaTrack.h"

// widget and state for a cinema property track

//-----------------------------------------------------------------------------
pqCinemaTrack::pqCinemaTrack(
  QWidget* parentObject, Qt::WindowFlags parentFlags, pqPipelineFilter* filter)
  : QWidget(parentObject, parentFlags)
  , Track(new Ui::CinemaTrack())
  , valsWidget(NULL)
{
  this->Track->setupUi(this);

  vtkSMProxy* prox = filter->getProxy();
  std::string vtkClassName = prox->GetVTKClassName();

  // only the following are currently supported by cinema
  std::string propName;
  if (vtkClassName == "vtkPVMetaSliceDataSet" || vtkClassName == "vtkPVContourFilter")
  {
    propName = "ContourValues";
  }
  else if (vtkClassName == "vtkPVMetaClipDataSet")
  {
    propName = "Value";
  }

  vtkSMProperty* prop = prox->GetProperty(propName.c_str());
  if (prop)
  {
    auto dom = prop->FindDomain<vtkSMDoubleRangeDomain>();
    if (dom)
    {
      this->Track->label->setText(filter->getSMName());

      pqScalarValueListPropertyWidget* vals = new pqScalarValueListPropertyWidget(prop, prox, this);
      vals->setRangeDomain(dom);
      vals->setEnabled(false);
      vals->setMinimumHeight(100);
      vals->setFixedHeight(100);
      vals->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
      this->layout()->addWidget(vals);
      this->valsWidget = vals;
      this->valsWidget->setEnabled(true);
    }
  }
};

//-----------------------------------------------------------------------------
pqCinemaTrack::~pqCinemaTrack()
{
}

//-----------------------------------------------------------------------------
bool pqCinemaTrack::explore() const
{
  return (this->valsWidget && !this->valsWidget->scalars().isEmpty());
}

//-----------------------------------------------------------------------------
QVariantList pqCinemaTrack::scalars() const
{
  return this->valsWidget->scalars();
}

//-----------------------------------------------------------------------------
QString pqCinemaTrack::filterName() const
{
  return this->Track->label->text();
}

//-----------------------------------------------------------------------------
void pqCinemaTrack::setFilterName(QString const& name)
{
  this->Track->label->setText(name);
}
