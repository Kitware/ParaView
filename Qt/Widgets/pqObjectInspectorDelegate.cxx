
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


pqObjectInspectorDelegate::pqObjectInspectorDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

pqObjectInspectorDelegate::~pqObjectInspectorDelegate()
{
}

QWidget *pqObjectInspectorDelegate::createEditor(QWidget *parent,
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
  QVariant domain = model->getDomain(index);
  if(data.type() == QVariant::Bool)
    {
    // Use a combo box with true/false in it.
    QComboBox *combo = new QComboBox(parent);
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
        QSpinBox *spin = new QSpinBox(parent);
        spin->setMinimum(list[0].toInt());
        spin->setMaximum(list[1].toInt());
        editor = spin;
        }
      }
    else if(domain.type() == QVariant::StringList)
      {
      QStringList names = domain.toStringList();
      QComboBox *combo = new QComboBox(parent);
      for(QStringList::Iterator it = names.begin(); it != names.end(); ++it)
        combo->addItem(*it);
      editor = combo;
      }
    }

  // TODO: What editor should be used for StringList domains?
  // Use a line edit for the default case.
  if(!editor)
    editor = new QLineEdit(parent);

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
      combo->setCurrentIndex(data.toBool() ? 1 : 0);
    else if(data.type() == QVariant::Int)
      combo->setCurrentIndex(data.toInt());
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
  QComboBox *combo = qobject_cast<QComboBox *>(editor);
  if(combo)
    {
    value = combo->currentIndex();
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


