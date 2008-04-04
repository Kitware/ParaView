/*=========================================================================

   Program: ParaView
   Module:    pqWriterDialog.cxx

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

#include "pqWriterDialog.h"

// Qt includes
#include <QGridLayout>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include "ui_pqWriterDialog.h"

// ParaView Server Manager includes
#include <vtkSMProxy.h>

#include "pqPropertyManager.h"
#include "pqNamedWidgets.h"

class pqWriterDialog::pqImplementation
{
public:
  pqImplementation()
  {
  }
  ~pqImplementation()
  {
    delete this->PropertyManager;
  }
  
  vtkSMProxy *Proxy;
  Ui::pqWriterDialog UI;
  pqPropertyManager* PropertyManager;

};

//-----------------------------------------------------------------------------
pqWriterDialog::pqWriterDialog(vtkSMProxy *proxy, QWidget *p) :
   QDialog(p), 
  Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(this);

  this->Implementation->PropertyManager = new pqPropertyManager(this);

  QGridLayout *propertyFrameLayout = new QGridLayout(this->Implementation->UI.PropertyFrame);
  this->Implementation->Proxy = proxy;

  // Create the widgets inside "propertyFrameLayout"
  pqNamedWidgets::createWidgets(propertyFrameLayout,this->Implementation->Proxy);
  pqNamedWidgets::link(this->Implementation->UI.PropertyFrame, 
                       this->Implementation->Proxy, 
                       this->Implementation->PropertyManager);
}

//-----------------------------------------------------------------------------
pqWriterDialog::~pqWriterDialog()
{
  pqNamedWidgets::unlink(this->Implementation->UI.PropertyFrame, 
                         this->Implementation->Proxy, 
                         this->Implementation->PropertyManager);

  delete this->Implementation;
}

//-----------------------------------------------------------------------------
bool pqWriterDialog::hasConfigurableProperties()
{
  // If there are no configurable properties then the frame's only child  
  //  should be the QGridLayout object.

  return (this->Implementation->UI.PropertyFrame->children().count()>1) ? true : false;
}

//-----------------------------------------------------------------------------
void pqWriterDialog::accept()
{
  this->Implementation->PropertyManager->accept();

  this->done(QDialog::Accepted);
}

//-----------------------------------------------------------------------------
void pqWriterDialog::reject()
{
  this->Implementation->PropertyManager->reject();

  pqNamedWidgets::unlink(this->Implementation->UI.PropertyFrame, 
                         this->Implementation->Proxy, 
                         this->Implementation->PropertyManager);

  this->done(QDialog::Rejected);
}

//-----------------------------------------------------------------------------
QSize pqWriterDialog::sizeHint() const
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
  return (style()->sizeFromContents(
              QStyle::CT_LineEdit, &opt, QSize(w, h).
              expandedTo(QApplication::globalStrut()), this));
}


