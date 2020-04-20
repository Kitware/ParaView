/*=========================================================================

   Program: ParaView
   Module:    pqListWidgetItemObject.h

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

#ifndef _pqListWidgetItemObject_h
#define _pqListWidgetItemObject_h

#include "pqWidgetsModule.h"
#include <QListWidgetItem>
#include <QObject>

/**
* QListWidgetItem subclass with additional signals, slots, and properties
*/
class PQWIDGETS_EXPORT pqListWidgetItemObject : public QObject, public QListWidgetItem
{
  Q_OBJECT
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
public:
  /**
  * construct list widget item to for QListWidget with a string
  */
  pqListWidgetItemObject(const QString& t, QListWidget* p);
  /**
  * overload setData() to emit changed signal
  */
  void setData(int role, const QVariant& v) override;

public Q_SLOTS:
  /**
  * get the check true/false
  */
  bool isChecked() const;
  /**
  * set the check state true/false
  */
  void setChecked(bool v);

Q_SIGNALS:
  /**
  * signal check state changed
  */
  void checkedStateChanged(bool);
};

#endif
