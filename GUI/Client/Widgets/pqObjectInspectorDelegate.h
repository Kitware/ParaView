
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
