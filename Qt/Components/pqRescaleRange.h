/*=========================================================================

   Program: ParaView
   Module:    pqRescaleRange.h

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

/// \file pqRescaleRange.h
/// \date 3/28/2007

#ifndef _pqRescaleRange_h
#define _pqRescaleRange_h


#include "pqComponentsExport.h"
#include <QDialog>

class pqRescaleRangeForm;
class QTimer;


class PQCOMPONENTS_EXPORT pqRescaleRange : public QDialog
{
  Q_OBJECT

public:
  pqRescaleRange(QWidget *parent=0);
  virtual ~pqRescaleRange();

  double getMinimum() const {return this->Minimum;}
  double getMaximum() const {return this->Maximum;}
  void setRange(double min, double max);

protected:
  virtual void hideEvent(QHideEvent *e);

private slots:
  void handleMinimumEdited();
  void handleMaximumEdited();
  void applyTextChanges();

private:
  void setMinimum();
  void setMaximum();

private:
  pqRescaleRangeForm *Form;
  QTimer *EditDelay;
  double Minimum;
  double Maximum;
  bool MinChanged;
  bool MaxChanged;
};

#endif
