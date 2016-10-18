/*=========================================================================

   Program: ParaView
   Module:    pqNonEditableStyledItemDelegate.h

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

========================================================================*/

#ifndef pqNonEditableStyledItemDelegate_h
#define pqNonEditableStyledItemDelegate_h

#include "pqCoreModule.h"

#include <QStyledItemDelegate>

/**
* pqNonEditableStyledItemDelegate() can be used inside Table or TreeView as
* ItemDelegate to make them Copy/Paste friendly. Basically it will allow
* the user to enter in edit mode but without having the option to change the
* content so the content can only be selected and Copy to the clipboard.
*/

class PQCORE_EXPORT pqNonEditableStyledItemDelegate : public QStyledItemDelegate
{
  typedef QStyledItemDelegate Superclass;

  Q_OBJECT

public:
  pqNonEditableStyledItemDelegate(QObject* parent = 0);
  virtual QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
  Q_DISABLE_COPY(pqNonEditableStyledItemDelegate)
};

#endif
