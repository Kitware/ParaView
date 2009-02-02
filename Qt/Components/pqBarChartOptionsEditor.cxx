/*=========================================================================

   Program: ParaView
   Module:    pqBarChartOptionsEditor.cxx

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

#include "pqBarChartOptionsEditor.h"
#include "ui_pqBarChartOptionsWidget.h"


class pqBarChartOptionsEditorForm : public Ui::pqBarChartOptionsWidget
{
public:
  pqBarChartOptionsEditorForm();
  ~pqBarChartOptionsEditorForm() {}
};


//-----------------------------------------------------------------------------
pqBarChartOptionsEditorForm::pqBarChartOptionsEditorForm()
  : Ui::pqBarChartOptionsWidget()
{
}


//-----------------------------------------------------------------------------
pqBarChartOptionsEditor::pqBarChartOptionsEditor(QWidget *widgetParent)
{
  this->Form = new pqBarChartOptionsEditorForm();
  this->Form->setupUi(this);

  // Listen for user changes.
  this->connect(this->Form->HelpFormat, SIGNAL(textChanged(const QString &)),
    this, SIGNAL(helpFormatChanged(const QString &)));
  this->connect(this->Form->OutlineStyle, SIGNAL(currentIndexChanged(int)),
    this, SLOT(convertOutlineStyle(int)));
  this->connect(this->Form->GroupFraction, SIGNAL(valueChanged(double)),
    this, SLOT(convertGroupFraction(double)));
  this->connect(this->Form->WidthFraction, SIGNAL(valueChanged(double)),
    this, SLOT(convertWidthFraction(double)));
}

pqBarChartOptionsEditor::~pqBarChartOptionsEditor()
{
  delete this->Form;
}

void pqBarChartOptionsEditor::getHelpFormat(QString &format) const
{
  format = this->Form->HelpFormat->text();
}

void pqBarChartOptionsEditor::setHelpFormat(const QString &format)
{
  this->Form->HelpFormat->setText(format);
}

vtkQtBarChartOptions::OutlineStyle
pqBarChartOptionsEditor::getOutlineStyle() const
{
  switch(this->Form->OutlineStyle->currentIndex())
    {
    default:
    case 0:
      {
      return vtkQtBarChartOptions::Darker;
      }
    case 1:
      {
      return vtkQtBarChartOptions::Black;
      }
    }
}

void pqBarChartOptionsEditor::setOutlineStyle(
  vtkQtBarChartOptions::OutlineStyle outline)
{
  switch(outline)
    {
    case vtkQtBarChartOptions::Darker:
      {
      this->Form->OutlineStyle->setCurrentIndex(0);
      break;
      }
    case vtkQtBarChartOptions::Black:
      {
      this->Form->OutlineStyle->setCurrentIndex(1);
      break;
      }
    }
}

float pqBarChartOptionsEditor::getBarGroupFraction() const
{
  return (float)this->Form->GroupFraction->value();
}

void pqBarChartOptionsEditor::setBarGroupFraction(float fraction)
{
  this->Form->GroupFraction->setValue(fraction);
}

float pqBarChartOptionsEditor::getBarWidthFraction() const
{
  return (float)this->Form->WidthFraction->value();
}

void pqBarChartOptionsEditor::setBarWidthFraction(float fraction)
{
  this->Form->WidthFraction->setValue(fraction);
}

void pqBarChartOptionsEditor::convertOutlineStyle(int)
{
  emit this->outlineStyleChanged(this->getOutlineStyle());
}

void pqBarChartOptionsEditor::convertGroupFraction(double fraction)
{
  emit this->barGroupFractionChanged((float)fraction);
}

void pqBarChartOptionsEditor::convertWidthFraction(double fraction)
{
  emit this->barWidthFractionChanged((float)fraction);
}


