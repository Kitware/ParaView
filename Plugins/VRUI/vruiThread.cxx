#include "vruiThread.h"
#include "vruiPipe.h"
#include "vruiServerState.h"
#include <QMutex>

// ----------------------------------------------------------------------------
vruiThread::vruiThread()
{
  this->Pipe=0;
  this->StateMutex=0;
  this->ServerState=0;
}

// ----------------------------------------------------------------------------
void vruiThread::SetPipe(vruiPipe *p)
{
  this->Pipe=p;
}

// ----------------------------------------------------------------------------
void vruiThread::SetStateMutex(QMutex *m)
{
  this->StateMutex=m;
}
// ----------------------------------------------------------------------------
void vruiThread::SetServerState(vruiServerState *s)
{
  this->ServerState=s;
}

// ----------------------------------------------------------------------------
void vruiThread::run()
{
  bool done=false;
  QMutex *stateLock;
  while(!done)
    {
    vruiPipe::MessageTag m=this->Pipe->Receive();
    switch(m)
      {
      case vruiPipe::PACKET_REPLY:
        cout << "thread:PACKET_REPLY ok : tag=" << m << endl;
        this->StateMutex->lock();
        this->Pipe->ReadState(this->ServerState);
        this->StateMutex->unlock();
        break;
      case vruiPipe::STOPSTREAM_REPLY:
        cout << "thread:STOPSTREAM_REPLY ok : tag=" << m << endl;
        done=true;
        break;
      default:
        cerr << "thread: Mismatching message while waiting for PACKET_REPLY: tag=" << m << endl;
        done=true;
        break;
      }
    }
}
