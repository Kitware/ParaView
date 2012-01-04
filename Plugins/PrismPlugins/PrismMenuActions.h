
#ifndef _PrismMenuActions_h
#define _PrismMenuActions_h


#include <QActionGroup>
#include <QString>

class PrismMenuActions : public QActionGroup
{
  Q_OBJECT
public:
  PrismMenuActions(QObject* p);
  ~PrismMenuActions();

};
#endif


