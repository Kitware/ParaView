#ifndef _PrismToolBarActions_h
#define _PrismToolBarActions_h

#include <QActionGroup>
#include <QString>

class PrismToolBarActions : public QActionGroup
{
  Q_OBJECT
public:
  PrismToolBarActions(QObject* p);
  ~PrismToolBarActions();

};
#endif

