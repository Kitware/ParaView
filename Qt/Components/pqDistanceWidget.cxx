/*=========================================================================

   Program: ParaView
   Module:    pqDistanceWidget.cxx

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
#include "pqDistanceWidget.h"

// Server Manager Includes.
#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMDoubleVectorProperty.h"

// Qt Includes.
#include <QVBoxLayout>
#include <QLabel>

// ParaView Includes.

//-----------------------------------------------------------------------------
pqDistanceWidget::pqDistanceWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p)
  :Superclass(o, pxy, p, "DistanceWidgetRepresentation")
{
  QVBoxLayout* l = qobject_cast<QVBoxLayout*>(this->layout());
  if (l)
    {
    QLabel* label =new QLabel("<b>Distance:</b> <i>na</i> ", this);
    l->insertWidget(0, label);
    this->Label = label;
    QLabel* notelabel =new QLabel("<b>Note: Move mouse and use 'P' key to change point position</b>", this);
    notelabel->setObjectName("ShortCutNoteLabel");
    notelabel->setWordWrap(1);
    l->addWidget(notelabel);
    }

  QObject::connect(this, SIGNAL(widgetInteraction()), this, SLOT(updateDistance()));
  QObject::connect(this, SIGNAL(modified()), this, SLOT(updateDistance()));
  this->updateDistance();
}

//-----------------------------------------------------------------------------
pqDistanceWidget::~pqDistanceWidget()
{
}


//-----------------------------------------------------------------------------
void pqDistanceWidget::updateDistance()
{
  vtkSMProxy* wproxy = this->getWidgetProxy();

  vtkSMDoubleVectorProperty* dvp1 = vtkSMDoubleVectorProperty::SafeDownCast(
    wproxy->GetProperty("Point1WorldPosition"));
  vtkSMDoubleVectorProperty* dvp2 = vtkSMDoubleVectorProperty::SafeDownCast(
    wproxy->GetProperty("Point2WorldPosition"));
  double distance =  sqrt(vtkMath::Distance2BetweenPoints(
      dvp1->GetElements(), dvp2->GetElements()));
  this->Label->setText(QString("<b>Distance:</b> <i>%1</i> ").arg(distance));
}
