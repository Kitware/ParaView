#ifndef _ClientTableViewDecorator_h
#define _ClientTableViewDecorator_h

#include <QObject>

class ClientTableView;
class pqOutputPort;
class pqDataRepresentation;

class ClientTableViewDecorator : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  ClientTableViewDecorator(ClientTableView* view);
  ~ClientTableViewDecorator();

protected slots:
  void currentIndexChanged(pqOutputPort*);
  void showing(pqDataRepresentation*);
  void dataUpdated();
  void setShowAllColumns(bool);

protected:
  ClientTableView* View;

private:
  ClientTableViewDecorator(const ClientTableViewDecorator&); // Not implemented.
  void operator=(const ClientTableViewDecorator&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;

};
#endif

