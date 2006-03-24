/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqObjectInspectorDelegate.h
/// \brief
///   The pqObjectInspectorDelegate class is used to edit the
///   pqObjectInspector model from the view.
///
/// \date 11/7/2005

#ifndef _pqObjectInspectorDelegate_h
#define _pqObjectInspectorDelegate_h

#include "QtWidgetsExport.h"
#include <QItemDelegate>

/// \class pqObjectInspectorDelegate
/// \brief
///   The pqObjectInspectorDelegate class is used to edit the
///   pqObjectInspector model from the view.
///
/// When the user wants to edit a property value, the view requests
/// an editor. The type of editor created is based on the property
/// value and domain. The delegate is also used to move the property
/// value back and forth between the model and the editor.
class QTWIDGETS_EXPORT pqObjectInspectorDelegate : public QItemDelegate
{
public:
  /// \brief
  ///   Creates a pqObjectInspectorDelegate instance.
  /// \param parent The parent object.
  pqObjectInspectorDelegate(QObject *parent=0);
  virtual ~pqObjectInspectorDelegate();

  /// \name QItemDelegate Methods
  //@{
  /// \brief
  ///   Creates a QWidget instance to edit the property value.
  ///
  /// The type of editor created is based on the property value and
  /// domain. For instance, if the value type is a boolean, a combo
  /// box is created with two entries: true and false. A spin box is
  /// used for int values that have a min and max. A combo box is used
  /// for int values associated with an enumeration.
  ///
  /// \param parent The editor parent widget.
  /// \param option The editor style options.
  /// \param index The model index being edited.
  virtual QWidget *createEditor(QWidget *parent,
      const QStyleOptionViewItem &option, const QModelIndex &index) const;

  /// \brief
  ///   Sets the editor value using the model data.
  /// \param editor The editor used to edit the value.
  /// \param index The model index being edited.
  virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;

  /// \brief
  ///   Sets the model data using the editor value.
  ///
  /// This method is called when the user finishes editing the value.
  ///
  /// \param editor The editor used to edit the value.
  /// \param model The model to be modified.
  /// \param index The model index being modified.
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
      const QModelIndex &index) const;
  //@}
};

#endif
