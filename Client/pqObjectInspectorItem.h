
#ifndef _pqObjectInspectorItem_h
#define _pqObjectInspectorItem_h

#include <QObject>
#include <QString>  // needed for Name
#include <QVariant> // needed for Value

class pqObjectInspectorItemInternal;


class pqObjectInspectorItem : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString propertyName READ GetPropertyName WRITE SetPropertyName)
  Q_PROPERTY(QVariant value READ GetValue WRITE SetValue)

public:
  pqObjectInspectorItem(QObject *parent=0);
  ~pqObjectInspectorItem();

  const QString &GetPropertyName() const {return this->Name;}
  void SetPropertyName(const QString &name);

  const QVariant &GetValue() const {return this->Value;}
  void SetValue(const QVariant &value);

  int GetChildCount() const;
  int GetChildIndex(pqObjectInspectorItem *child) const;
  pqObjectInspectorItem *GetChild(int index) const;

  void ClearChildren();
  void AddChild(pqObjectInspectorItem *child);

  pqObjectInspectorItem *GetParent() const {return this->Parent;}
  void SetParent(pqObjectInspectorItem *parent) {this->Parent = parent;}

signals:
  void nameChanged(pqObjectInspectorItem *item);
  void valueChanged(pqObjectInspectorItem *item);

private:
  QString Name;
  QVariant Value;
  pqObjectInspectorItemInternal *Internal;
  pqObjectInspectorItem *Parent;
};


#endif
