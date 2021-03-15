/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqFindDataCurrentSelectionFrame_h
#define pqFindDataCurrentSelectionFrame_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOutputPort;

/**
* pqFindDataCurrentSelectionFrame is designed to be used by pqFindDataDialog.
* pqFindDataDialog uses this class to show the current selection in a
* spreadsheet view. This class encapsulates the logic to monitor the current
* selection by tracking the pqSelectionManager and then showing the results in
* the spreadsheet.
*/
class PQCOMPONENTS_EXPORT pqFindDataCurrentSelectionFrame : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqFindDataCurrentSelectionFrame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqFindDataCurrentSelectionFrame() override;

  /**
  * return the port from which this frame is showing the selected data, if
  * any.
  */
  pqOutputPort* showingPort() const;

Q_SIGNALS:
  /**
  * signal fired to indicate the selected port that currently being shown in
  * the frame.
  */
  void showing(pqOutputPort*);

private Q_SLOTS:
  /**
  * show the selected data from the given output port in the frame.
  */
  void showSelectedData(pqOutputPort*);

  /**
  * update the field-type set of the internal spreadsheet view based on the
  * value in the combo-box.
  */
  void updateFieldType();

  /**
  * update the data shown in the spreadsheet aka render the spreadsheet.
  */
  void updateSpreadSheet();

private:
  Q_DISABLE_COPY(pqFindDataCurrentSelectionFrame)

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
};

#endif
