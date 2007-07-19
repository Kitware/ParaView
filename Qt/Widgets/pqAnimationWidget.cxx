/*=========================================================================

   Program: ParaView
   Module:    pqAnimationWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqAnimationWidget.h"

#include <QResizeEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>

#include "pqAnimationModel.h"

pqAnimationWidget::pqAnimationWidget(QWidget* p) 
  : QScrollArea(p) 
{
  QWidget* cont = new QWidget();
  cont->setSizePolicy(QSizePolicy::MinimumExpanding,
                      QSizePolicy::MinimumExpanding);
  QHBoxLayout* wlayout = new QHBoxLayout(cont);
  wlayout->setSizeConstraint(QLayout::SetMinimumSize);
  this->View = new QGraphicsView(cont);
  this->View->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->View->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->View->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  this->View->setFrameShape(QFrame::NoFrame);
  this->Model = new pqAnimationModel(this->View);
  this->View->setScene(this->Model);
  this->Header = new QHeaderView(Qt::Vertical, cont);
  this->Header->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);
  this->View->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);
  this->Header->setResizeMode(QHeaderView::Fixed);
  this->Header->setMinimumSectionSize(0);
  this->Header->setModel(this->Model->header());
  this->Model->setRowHeight(this->Header->sectionSize(0));
  wlayout->addWidget(this->Header);
  wlayout->addWidget(this->View);
  wlayout->setMargin(0);
  wlayout->setSpacing(0);
  this->setWidget(cont);
  this->setWidgetResizable(true);
  QObject::connect(this->Header->model(),
                   SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(updateSizes()));

}

pqAnimationWidget::~pqAnimationWidget()
{
}

pqAnimationModel* pqAnimationWidget::animationModel() const
{
  return this->Model;
}

void pqAnimationWidget::updateSizes()
{
  int sz = 0;
  for(int i=0; i<this->Header->count(); i++)
    {
    sz += this->Header->sectionSize(i);
    }
  this->widget()->setMinimumHeight(sz);
  this->widget()->layout()->invalidate();
}


