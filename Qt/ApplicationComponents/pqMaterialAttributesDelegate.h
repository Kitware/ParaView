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
#ifndef pqMaterialAttributesDelegate_h
#define pqMaterialAttributesDelegate_h

#include "pqApplicationComponentsModule.h"
#include <QStyledItemDelegate>

/**
 * pqMaterialAttributesDelegate is used to customize material attributes table view.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMaterialAttributesDelegate : public QStyledItemDelegate
{
  Q_OBJECT
  typedef QStyledItemDelegate Superclass;

public:
  pqMaterialAttributesDelegate(QObject* parent = nullptr);
  ~pqMaterialAttributesDelegate() override = default;

  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * Create the editor with two columns : one for the property selection and one to modify the
   * value of the property.
   */
  QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * do nothing, everything is handled in createEditor method
   */
  void setEditorData(QWidget* editor, const QModelIndex& index) const override
  {
    Q_UNUSED(editor);
    Q_UNUSED(index);
  };

  /**
   * Gets data from the editor widget and stores it in the specified model at the item index
   */
  void setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
  Q_DISABLE_COPY(pqMaterialAttributesDelegate)
};

#endif
