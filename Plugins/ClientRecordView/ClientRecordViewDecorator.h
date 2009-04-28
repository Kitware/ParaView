#ifndef _ClientRecordViewDecorator_h
#define _ClientRecordViewDecorator_h

#include <QObject>

class ClientRecordView;
class pqOutputPort;
class pqDataRepresentation;

class ClientRecordViewDecorator : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  ClientRecordViewDecorator(ClientRecordView* view);
  ~ClientRecordViewDecorator();

protected slots:
  void currentIndexChanged(pqOutputPort*);
  void showing(pqDataRepresentation*);
  void dataUpdated();

protected:
  ClientRecordView* View;

private:
  ClientRecordViewDecorator(const ClientRecordViewDecorator&); // Not implemented.
  void operator=(const ClientRecordViewDecorator&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;

};
#endif

