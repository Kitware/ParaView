/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqDoubleEdit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqDoubleEdit
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "pqDoubleEdit.h"

#include <QDoubleValidator>

pqDoubleEdit::pqDoubleEdit(QWidget* parentObject) :
  QLineEdit(parentObject)
{
  this->connect(this, SIGNAL(textChanged(const QString&)), this,
      SLOT(valueEdited(const QString&)));
}

pqDoubleEdit::~pqDoubleEdit()
{

}

double pqDoubleEdit::value()
{
  QString currentText = this->text();
  int currentPos = this->cursorPosition();
  QDoubleValidator dvalidator(NULL);
  QValidator::State state = dvalidator.validate(currentText, currentPos);
  if (state == QValidator::Acceptable || state == QValidator::Intermediate)
    {
    return currentText.toDouble();
    }
  return 0.0;
}

void pqDoubleEdit::setValue(double dvalue)
{
  // we do not want to modify the text when the text is Intermediate. Otherwise,
  // it erase the incomplete entries : 1. becomes 1, and it
  // is impossible to enter decimals
  QString currentText = this->text();
  int currentPos = this->cursorPosition();
  QDoubleValidator *dvalidator = new QDoubleValidator(NULL);
  QValidator::State state = dvalidator->validate(currentText, currentPos);
  delete dvalidator;
  if (state == QValidator::Acceptable)
    {
    double v = this->text().toDouble();
    if (v != dvalue)
      {
      this->setText(QString::number(dvalue));
      }
    }
  else if (state == QValidator::Intermediate && currentPos > 0)
    {
    return;
    }
  else
    {
    this->setText(QString::number(dvalue));
    }
}

void pqDoubleEdit::valueEdited(const QString& /*text*/)
{
  QString currentText = this->text();
  int currentPos = this->cursorPosition();
  QDoubleValidator* dvalidator = new QDoubleValidator(NULL);
  QValidator::State state = dvalidator->validate(currentText, currentPos);
  delete dvalidator;
  if (state == QValidator::Acceptable)
    {
    double dvalue = this->text().toDouble();
    emit valueChanged(dvalue);
    }
}
