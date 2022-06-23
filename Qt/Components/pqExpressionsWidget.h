/*=========================================================================

   Program: ParaView
   Module:    pqExpressionsWidget.h

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
#ifndef pqExpressionsWidget_h
#define pqExpressionsWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOneLinerTextEdit;

/**
 * pqExpressionsWidget is a widget to edit expression.
 *
 * It is a container for a line edit and buttons linked to the ExpressionsManager.
 */
class PQCOMPONENTS_EXPORT pqExpressionsWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqExpressionsWidget(QWidget* parent = nullptr, const QString& groupName = "");
  ~pqExpressionsWidget() override = default;

  /**
   * Get the internal line edit.
   */
  pqOneLinerTextEdit* lineEdit();

  /**
   * Set buttons up for "groupName" expressions group.
   */
  void setupButtons(const QString& groupName);

private:
  Q_DISABLE_COPY(pqExpressionsWidget)

  pqOneLinerTextEdit* OneLiner;
};

#endif
