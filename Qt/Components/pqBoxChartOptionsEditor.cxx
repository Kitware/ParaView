/*=========================================================================

   Program: ParaView
   Module:    pqBoxChartOptionsEditor.cxx

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

#include "pqBoxChartOptionsEditor.h"
#include "ui_pqBoxChartOptionsWidget.h"


class pqBoxChartOptionsEditorForm : public Ui::pqBoxChartOptionsWidget
{
public:
  pqBoxChartOptionsEditorForm();
  ~pqBoxChartOptionsEditorForm() {}
};


//-----------------------------------------------------------------------------
pqBoxChartOptionsEditorForm::pqBoxChartOptionsEditorForm()
  : Ui::pqBoxChartOptionsWidget()
{
}


//-----------------------------------------------------------------------------
pqBoxChartOptionsEditor::pqBoxChartOptionsEditor(QWidget *widgetParent)
  : pqOptionsPage(widgetParent)
{
  this->Form = new pqBoxChartOptionsEditorForm();
  this->Form->setupUi(this);

  // Listen for user changes.
  this->connect(this->Form->HelpFormat, SIGNAL(textChanged(const QString &)),
    this, SIGNAL(helpFormatChanged(const QString &)));
  this->connect(this->Form->OutlierFormat,
    SIGNAL(textChanged(const QString &)),
    this, SIGNAL(outlierFormatChanged(const QString &)));
  this->connect(this->Form->OutlineStyle, SIGNAL(currentIndexChanged(int)),
    this, SLOT(convertOutlineStyle(int)));
  this->connect(this->Form->WidthFraction, SIGNAL(valueChanged(double)),
    this, SLOT(convertWidthFraction(double)));
}

pqBoxChartOptionsEditor::~pqBoxChartOptionsEditor()
{
  delete this->Form;
}

void pqBoxChartOptionsEditor::getHelpFormat(QString &format) const
{
  format = this->Form->HelpFormat->text();
}

void pqBoxChartOptionsEditor::setHelpFormat(const QString &format)
{
  this->Form->HelpFormat->setText(format);
}

void pqBoxChartOptionsEditor::getOutlierFormat(QString &format) const
{
  format = this->Form->OutlierFormat->text();
}

void pqBoxChartOptionsEditor::setOutlierFormat(const QString &format)
{
  this->Form->OutlierFormat->setText(format);
}

vtkQtStatisticalBoxChartOptions::OutlineStyle
pqBoxChartOptionsEditor::getOutlineStyle() const
{
  switch(this->Form->OutlineStyle->currentIndex())
    {
    default:
    case 0:
      {
      return vtkQtStatisticalBoxChartOptions::Darker;
      }
    case 1:
      {
      return vtkQtStatisticalBoxChartOptions::Black;
      }
    }
}

void pqBoxChartOptionsEditor::setOutlineStyle(
  vtkQtStatisticalBoxChartOptions::OutlineStyle outline)
{
  switch(outline)
    {
    case vtkQtStatisticalBoxChartOptions::Darker:
      {
      this->Form->OutlineStyle->setCurrentIndex(0);
      break;
      }
    case vtkQtStatisticalBoxChartOptions::Black:
      {
      this->Form->OutlineStyle->setCurrentIndex(1);
      break;
      }
    }
}

float pqBoxChartOptionsEditor::getBoxWidthFraction() const
{
  return (float)this->Form->WidthFraction->value();
}

void pqBoxChartOptionsEditor::setBoxWidthFraction(float fraction)
{
  this->Form->WidthFraction->setValue(fraction);
}

void pqBoxChartOptionsEditor::convertOutlineStyle(int)
{
  emit this->outlineStyleChanged(this->getOutlineStyle());
}

void pqBoxChartOptionsEditor::convertWidthFraction(double fraction)
{
  emit this->boxWidthFractionChanged((float)fraction);
}


