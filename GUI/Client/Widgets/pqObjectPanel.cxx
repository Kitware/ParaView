/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectPanel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

// this include
#include "pqObjectPanel.h"

// Qt includes
#include <QApplication>
#include <QStyle>
#include <QStyleOption>

// VTK includes
#include "QVTKWidget.h"

// paraview includes
#include "vtkSMSourceProxy.h"

// paraq includes
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"


//-----------------------------------------------------------------------------
/// constructor
pqObjectPanel::pqObjectPanel(QWidget* p)
  : QWidget(p), Proxy(NULL)
{
  this->PropertyManager = new pqPropertyManager(this);
}

//-----------------------------------------------------------------------------
/// destructor
pqObjectPanel::~pqObjectPanel()
{
  delete this->PropertyManager;
}

//-----------------------------------------------------------------------------
QSize pqObjectPanel::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = 20 * (qMax(fm.lineSpacing(), 14));
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), 
                                    this));
}

//-----------------------------------------------------------------------------
/// set the proxy to display properties for
void pqObjectPanel::setProxy(pqSMProxy p)
{
  if(p != this->Proxy)
    {
    this->setProxyInternal(p);
    }
}

//-----------------------------------------------------------------------------
void pqObjectPanel::setProxyInternal(pqSMProxy p)
{
  this->Proxy = p;
  vtkSMSourceProxy* sp;
  sp = vtkSMSourceProxy::SafeDownCast(this->Proxy);
  if(sp)
    {
    sp->UpdatePipelineInformation();
    }
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqSMProxy pqObjectPanel::proxy()
{
  return this->Proxy;
}

//-----------------------------------------------------------------------------
/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqObjectPanel::accept()
{
  if(!this->Proxy)
    {
    return;
    }

  this->PropertyManager->accept();
}

//-----------------------------------------------------------------------------
/// reset the changes made
/// editor will query properties from the server manager
void pqObjectPanel::reset()
{
  if(!this->Proxy)
    {
    return;
    }
  this->PropertyManager->reject();
}

