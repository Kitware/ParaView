
/// \file pqObjectInspectorDelegate.cxx
/// \brief
///   The pqObjectInspectorDelegate class is used to edit the
///   pqObjectInspector model from the view.
///
/// \date 11/7/2005

#include "pqObjectInspectorDelegate.h"

#include "pqObjectInspector.h"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QLineEdit>
#include <QList>
#include <QModelIndex>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QVariant>


pqObjectInspectorDelegate::pqObjectInspectorDelegate(QObject *p)
  : QItemDelegate(p)
{
}

pqObjectInspectorDelegate::~pqObjectInspectorDelegate()
{
}

QWidget *pqObjectInspectorDelegate::createEditor(QWidget *p,
    const QStyleOptionViewItem &, const QModelIndex &index) const
{
  if(!index.isValid())
    return 0;

  // Make sure the model is the correct type.
  const pqObjectInspector *model = qobject_cast<const pqObjectInspector *>(
      index.model());
  if(!model)
    return 0;

  // Get the data for the given index. Use the data type and domain
  // information to create the correct widget type.
  QWidget *editor = 0;
  QVariant data = model->data(index, Qt::EditRole);
  QVariant domain = model->domain(index);
  if(data.type() == QVariant::Bool)
    {
    // Use a combo box with true/false in it.
    QComboBox *combo = new QComboBox(p);
    combo->addItem("false");
    combo->addItem("true");
    editor = combo;
    }
  else if(data.type() == QVariant::Int)
    {
    // Use a spin box for an int with a min and max. Use a combo
    // box for an int with enum names or values. Otherwise, use
    // a line edit.
    if(domain.type() == QVariant::List)
      {
      QList<QVariant> list = domain.toList();
      if(list.size() == 2 && list[0].isValid() && list[1].isValid())
        {
        QSpinBox *spin = new QSpinBox(p);
        spin->setMinimum(list[0].toInt());
        spin->setMaximum(list[1].toInt());
        editor = spin;
        }
      }
    else if(domain.type() == QVariant::StringList)
      {
      QStringList names = domain.toStringList();
      QComboBox *combo = new QComboBox(p);
      for(QStringList::Iterator it = names.begin(); it != names.end(); ++it)
        combo->addItem(*it);
      editor = combo;
      }
    }
  else if(data.type() == QVariant::String && 
         (domain.type() == QVariant::StringList || domain.type() == QVariant::List ))
    {
    QStringList names = domain.toStringList();
    QComboBox *combo = new QComboBox(p);
    for(QStringList::Iterator it = names.begin(); it != names.end(); ++it)
      combo->addItem(*it);
    editor = combo;
    }

  // Use a line edit for the default case.
  if(!editor)
    editor = new QLineEdit(p);

  if(editor)
    editor->installEventFilter(const_cast<pqObjectInspectorDelegate *>(this));
  return editor;
}

void pqObjectInspectorDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
  QVariant data = index.model()->data(index, Qt::EditRole);
  QComboBox *combo = qobject_cast<QComboBox *>(editor);
  if(combo)
    {
    if(data.type() == QVariant::Bool)
      {
      combo->setCurrentIndex(data.toBool() ? 1 : 0);
      }
    else if(data.type() == QVariant::Int)
      {
      combo->setCurrentIndex(data.toInt());
      }
    else if(data.type() == QVariant::String)
      {
      combo->setCurrentIndex(combo->findText(data.toString()));
      }
    return;
    }

  QSpinBox *spin = qobject_cast<QSpinBox *>(editor);
  if(spin)
    {
    spin->setValue(data.toInt());
    return;
    }

  QLineEdit *line = qobject_cast<QLineEdit *>(editor);
  if(line)
    {
    line->setText(data.toString());
    return;
    }
}

void pqObjectInspectorDelegate::setModelData(QWidget *editor,
    QAbstractItemModel *model, const QModelIndex &index) const
{
  QVariant value;
  QVariant data = model->data(index, Qt::EditRole);
  QComboBox *combo = qobject_cast<QComboBox *>(editor);
  if(combo)
    {
    if(data.type() == QVariant::String)
      {
      value = combo->currentText();
      }
    else
      {
      value = combo->currentIndex();
      }
    model->setData(index, value);
    return;
    }

  QSpinBox *spin = qobject_cast<QSpinBox *>(editor);
  if(spin)
    {
    value = spin->value();
    model->setData(index, value);
    return;
    }

  QLineEdit *line = qobject_cast<QLineEdit *>(editor);
  if(line)
    {
    value = line->text();
    model->setData(index, value);
    return;
    }
}


