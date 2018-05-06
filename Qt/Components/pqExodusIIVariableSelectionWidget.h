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
#ifndef pqExodusIIVariableSelectionWidget_h
#define pqExodusIIVariableSelectionWidget_h

#include "vtkSetGet.h"
#if !defined(VTK_LEGACY_REMOVE)
#include "pqComponentsModule.h"
#include "pqTreeWidget.h"

/**
 * @class pqExodusIIVariableSelectionWidget
 * @deprecated ParaView 5.6
 *
 * pqExodusIIVariableSelectionWidget has been deprecated in ParaView 5.6 and has
 * been replaced by pqArraySelectionWidget. pqArraySelectionWidget a QTreeView
 * subclass, rather than a QTreeWidget subclass and is better suited for
 * consolidating user interactions when dealing with large lists.
*/
class PQCOMPONENTS_EXPORT pqExodusIIVariableSelectionWidget : public pqTreeWidget
{
  Q_OBJECT
  typedef pqTreeWidget Superclass;

public:
  VTK_LEGACY(pqExodusIIVariableSelectionWidget(QWidget* parent = 0));
  ~pqExodusIIVariableSelectionWidget() override;

signals:
  /**
  * fired when widget is modified.
  */
  void widgetModified();

protected slots:
  void updateProperty();
  void delayedUpdateProperty(bool check_state);

protected:
  /**
  * overridden to handle QDynamicPropertyChangeEvent when properties are
  * added/removed/updated.
  */
  bool eventFilter(QObject* object, QEvent* qevent) override;

  void propertyChanged(const QString& pname);

  void setStatus(const QString& key, const QString& text, bool value);

private:
  Q_DISABLE_COPY(pqExodusIIVariableSelectionWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif // !defined(VTK_LEGACY_REMOVE)

#endif // pqExodusIIVariableSelectionWidget_h
