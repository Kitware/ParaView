/*=========================================================================

   Program: ParaView
   Module:    pqClipPanel.cxx

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

#include "pqClipPanel.h"

#include "pqProxySelectionWidget.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

pqClipPanel::pqClipPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p)
{
  pqProxySelectionWidget* clipFunc = 
    this->findChild<pqProxySelectionWidget*>("ClipFunction");
  QObject::connect(clipFunc, SIGNAL(proxyChanged(pqSMProxy)),
                   this, SLOT(clipTypeChanged(pqSMProxy)));

  this->setScalarWidgetsVisibility(clipFunc->proxy());
}

pqClipPanel::~pqClipPanel()
{
}

void pqClipPanel::setScalarWidgetsVisibility(pqSMProxy aproxy)
{
  if (!aproxy)
    {
    return;
    }

  QLabel* label = this->findChild<QLabel*>("_labelForSelectInputScalars");
  QComboBox* arraySel = this->findChild<QComboBox*>("SelectInputScalars");
  QLabel* label2 = this->findChild<QLabel*>("_labelForValue");
  QLineEdit* value = this->findChild<QLineEdit*>("Value");
  if (strcmp(aproxy->GetXMLName(), "Scalar") == 0)
    {
    label->show();
    arraySel->show();
    label2->show();
    value->show();
    }
  else
    {
    label->hide();
    arraySel->hide();
    label2->hide();
    value->hide();
    }
}

void pqClipPanel::clipTypeChanged(pqSMProxy aproxy)
{
  this->setScalarWidgetsVisibility(aproxy);
}

