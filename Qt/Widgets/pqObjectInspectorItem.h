
#ifndef _pqObjectInspectorItem_h
#define _pqObjectInspectorItem_h

#include <QObject>
#include <QString>  // needed for Name
#include <QVariant> // needed for Value

class pqObjectInspectorItemInternal;


class pqObjectInspectorItem : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString propertyName READ getPropertyName WRITE setPropertyName)
  Q_PROPERTY(QVariant value READ getValue WRITE setValue)

public:
  pqObjectInspectorItem(QObject *parent=0);
  ~pqObjectInspectorItem();

  const QString &getPropertyName() const {return this->Name;}
  void setPropertyName(const QString &name);

  const QVariant &getValue() const {return this->Value;}
  void setValue(const QVariant &value);

  int getChildCount() const;
  int getChildIndex(pqObjectInspectorItem *child) const;
  pqObjectInspectorItem *getChild(int index) const;

  void clearChildren();
  void addChild(pqObjectInspectorItem *child);

  pqObjectInspectorItem *getParent() const {return this->Parent;}
  void setParent(pqObjectInspectorItem *parent) {this->Parent = parent;}

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
