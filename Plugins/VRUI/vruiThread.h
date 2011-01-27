#ifndef vruiThread_h
#define vruiThread_h

#include <QThread>

class vruiPipe;
class QMutex;
class vruiServerState;

class vruiThread : public QThread
{
  Q_OBJECT
public:
  vruiThread();
  void SetPipe(vruiPipe *p);
  void SetStateMutex(QMutex *m);
  void SetServerState(vruiServerState *s);

protected:
  // Entry point of the thread. Called by start().
  void run();
  vruiPipe *Pipe;
  QMutex *StateMutex;
  vruiServerState *ServerState;
};

#endif // #ifndef vruiThread_h
