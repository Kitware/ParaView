
/// \file pqObjectInspectorWidget.h
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable form.
///
/// \date 11/25/2005

#ifndef _pqObjectInspectorWidget_h
#define _pqObjectInspectorWidget_h

#include "QtWidgetsExport.h"
#include <QWidget>
#include <QListWidgetItem>

class pqObjectInspector;
class pqObjectInspectorDelegate;
class QTreeView;
class QTabWidget;
class vtkSMProxy;
class QListWidgetItem;


/// \class pqObjectInspectorWidget
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable form.
class QTWIDGETS_EXPORT pqObjectInspectorWidget : public QWidget
{
  Q_OBJECT
public:
  pqObjectInspectorWidget(QWidget *parent=0);
  virtual ~pqObjectInspectorWidget();

  pqObjectInspector *getObjectModel() const {return this->Inspector;}
  QTreeView *getTreeView() const {return this->TreeView;}

public slots:
  void setProxy(vtkSMProxy *proxy);


protected:
  void setupCustomForm(vtkSMProxy*, QWidget* w);

protected slots:
  // slot to help with custom forms
  void updateDisplayForPropertyChanged();

private:
  pqObjectInspector *Inspector;
  pqObjectInspectorDelegate *Delegate;
  QTreeView *TreeView;
  QTabWidget* TabWidget;
};


// internal class only  -- TODO support more than check states?
class pqCustomFormListItem : public QObject, public QListWidgetItem
{
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value WRITE setValue)
public:
  pqCustomFormListItem(const QString& t, QListWidget* p)
    : QListWidgetItem(t, p), Value(QVariant::Invalid) {}

  void setData(int role, const QVariant& v)
    {
    QListWidgetItem::setData(role, v);
    if(Qt::CheckStateRole == role)
      {
      if(Qt::Checked == v)
        {
        this->Value = true;
        }
      else if(Qt::Unchecked == v)
        {
        this->Value = false;
        }
      emit this->valueChanged();
      }
    }
  QVariant value() const
    {
    return this->Value;
    }
  void setValue(const QVariant& v)
    {
    // only set if it is different then what we have
    if((v.toBool() == true && this->checkState() == Qt::Checked ||
       v.toBool() == false && this->checkState() == Qt::Unchecked) &&
      Value.type() != QVariant::Invalid)
      {
      return;
      }
    if(v.toBool())
      {
      this->setCheckState(Qt::Checked);
      }
    else
      {
      this->setCheckState(Qt::Unchecked);
      }
    }
signals:
  void valueChanged();

private:
  QVariant Value;

};

#endif
