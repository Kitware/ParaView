/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidgetItem.h

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
#ifndef __pqTreeWidgetItem_h 
#define __pqTreeWidgetItem_h

#include "QtWidgetsExport.h"
#include <QTreeWidgetItem>

/// pqTreeWidgetItem is a QTreeWidgetItem with callbacks for whenever the data
/// for the pqTreeWidgetItem changes. This is designed as a replacement for
/// pqTreeWidgetItemObject avoid the need for pqTreeWidgetItem to be a QObject
/// subclass, thus keeping them light-weight.
class QTWIDGETS_EXPORT pqTreeWidgetItem : public QTreeWidgetItem
{
  typedef QTreeWidgetItem Superclass;
public:
  pqTreeWidgetItem(int type=UserType):
    Superclass(type), CallbackHandler(0) { }
  pqTreeWidgetItem(const QStringList& strings, int type=UserType):
    Superclass(strings, type), CallbackHandler(0) { }
  pqTreeWidgetItem(QTreeWidget *parent, int type=UserType):
    Superclass(parent, type), CallbackHandler(0) { }
  pqTreeWidgetItem(QTreeWidget *parent, const QStringList &strings, int type=UserType):
    Superclass(parent, strings, type), CallbackHandler(0) { }
  pqTreeWidgetItem(QTreeWidget *parent, QTreeWidgetItem *preceding, int type=UserType):
    Superclass(parent, preceding, type), CallbackHandler(0) { }
  pqTreeWidgetItem(QTreeWidgetItem *parent, int type=UserType):
    Superclass(parent, type), CallbackHandler(0) { }
  pqTreeWidgetItem(QTreeWidgetItem *parent, const QStringList& strings, int type=UserType):
    Superclass(parent, strings, type), CallbackHandler(0) { }
  pqTreeWidgetItem(QTreeWidgetItem *parent, QTreeWidgetItem* preceding, int type=UserType):
    Superclass(parent, preceding, type), CallbackHandler(0) { }

  /// overload setData() to call callbacks if set.
  virtual void setData(int column, int role, const QVariant& v);

public:
  class pqCallbackHandler
    {
  public:
    virtual void checkStateChanged(pqTreeWidgetItem* item, int column) {};
    virtual void dataChanged(pqTreeWidgetItem* item, int column, int role) {};
    virtual bool acceptChange(pqTreeWidgetItem* item,
      const QVariant& curValue, const QVariant& newValue,
      int column, int role)
      { return true; }
    };

  /// Set the name of the callback slot to call
  void setCallbackHandler(pqCallbackHandler* hdlr)
    { this->CallbackHandler = hdlr; }

protected:
  pqCallbackHandler* CallbackHandler;
};

#endif


