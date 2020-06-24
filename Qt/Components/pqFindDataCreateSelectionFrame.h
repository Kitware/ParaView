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
#ifndef pqFindDataCreateSelectionFrame_h
#define pqFindDataCreateSelectionFrame_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOutputPort;
class QComboBox;

/**
* pqFindDataCreateSelectionFrame is designed to be used by pqFindDataDialog.
* pqFindDataDialog uses this as the component to create a new selection based
* on the query. This class encapsulates the logic for the UI to create new
* query based selections.
* Users can construct queries to create new selections. When user "runs" the
* query, we create a new selection and update the global application selection
* by notifying pqSelectionManager instance, is available.
* If the global selection changes from outside pqFindDataCreateSelectionFrame
* then we reset any existing query the user may have set.
*/
class PQCOMPONENTS_EXPORT pqFindDataCreateSelectionFrame : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqFindDataCreateSelectionFrame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqFindDataCreateSelectionFrame() override;

  /**
  * Helper method used to fill up a combo-box showing allowed selection types
  * based on the data-type produced on the port.
  */
  static void populateSelectionTypeCombo(QComboBox* bbox, pqOutputPort* port);

public Q_SLOTS:
  /**
  * Set the port to create a query selection on. If the port is different from
  * the current one, it clears any existing query.
  */
  void setPort(pqOutputPort*);

private Q_SLOTS:
  /**
  * marks if the underlying data has changed
  */
  void dataChanged();

  /**
  * refreshes the query widget.
  */
  void refreshQuery();

  /**
  * run the active query.
  */
  void runQuery();

  /**
  * called when the global selection changes. We reset the UI if the
  * pqFindDataCreateSelectionFrame didn't create that selection.
  */
  void onSelectionChanged(pqOutputPort*);

private:
  Q_DISABLE_COPY(pqFindDataCreateSelectionFrame)

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
};

#endif
