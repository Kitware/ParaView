/*=========================================================================

   Program: ParaView
   Module: pq3DWidgetPropertyWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pq3DWidgetPropertyWidget.h"

#include <QVBoxLayout>

#include "pq3DWidget.h"

pq3DWidgetPropertyWidget::pq3DWidgetPropertyWidget(pq3DWidget* widget, QWidget* parent_)
  : pqPropertyWidget(widget->proxy(), parent_)
{
  this->Widget = widget;
  this->connect(this->Widget, SIGNAL(modified()), this, SIGNAL(changeAvailable()));
  this->connect(this->Widget, SIGNAL(modified()), this, SIGNAL(changeFinished()));
  this->connect(this, SIGNAL(viewChanged(pqView*)), this->Widget, SLOT(setView(pqView*)));
  this->Widget->setView(this->view());

  QVBoxLayout* layout_ = new QVBoxLayout;
  layout_->setMargin(0);
  layout_->addWidget(widget);
  this->setLayout(layout_);
}

pq3DWidgetPropertyWidget::~pq3DWidgetPropertyWidget()
{
  delete this->Widget;
}

void pq3DWidgetPropertyWidget::apply()
{
  this->Widget->accept();
}

void pq3DWidgetPropertyWidget::reset()
{
  this->Widget->reset();
}

void pq3DWidgetPropertyWidget::select()
{
  this->Widget->select();
}

void pq3DWidgetPropertyWidget::deselect()
{
  this->Widget->deselect();
}
