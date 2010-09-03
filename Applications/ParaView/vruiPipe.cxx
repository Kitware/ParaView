#include "vruiPipe.h"
#include <cassert>
#include <QTcpSocket>
#include "vruiServerState.h"

typedef unsigned short MessageTagPropocol;

// ----------------------------------------------------------------------------
vruiPipe::vruiPipe(QTcpSocket *socket)
{
  assert("pre: socket_exist" && socket!=0);
  this->Socket=socket;
}

// ----------------------------------------------------------------------------
vruiPipe::~vruiPipe()
{
}

// ----------------------------------------------------------------------------
void vruiPipe::Send(MessageTag m)
{
  MessageTagPropocol message=m;
  this->Socket->write(reinterpret_cast<const char*>(&message),
                      sizeof(MessageTagPropocol));
}

// ----------------------------------------------------------------------------
bool vruiPipe::WaitForServerReply(int msecs)
{
  return this->Socket->waitForReadyRead(msecs);
}

// ----------------------------------------------------------------------------
vruiPipe::MessageTag vruiPipe::Receive()
{
  MessageTagPropocol message;
  this->Socket->read(reinterpret_cast<char *>(&message),
                     sizeof(MessageTagPropocol));

  return static_cast<MessageTag>(message);
}

// ----------------------------------------------------------------------------
void vruiPipe::ReadLayout(vruiServerState *state)
{
  assert("pre: state_exists" && state!=0);

  int value;
  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
  state->GetTrackerStates()->resize(value);

  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
  state->GetButtonStates()->resize(value);

  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
  state->GetValuatorStates()->resize(value);
}

// ----------------------------------------------------------------------------
void vruiPipe::ReadState(vruiServerState *state)
{
  assert("pre: state_exists" && state!=0);

  // read all trackers states.
  vtkstd::vector<vtkSmartPointer<vruiTrackerState> > *trackers=state->GetTrackerStates();
  size_t i=0;
  size_t c=trackers->size();
  while(i<c)
    {
    this->Socket->read(reinterpret_cast<char *>((*trackers)[i].GetPointer()),
                       sizeof(vruiTrackerState));
    ++i;
    }
  // read all buttons states.
  vtkstd::vector<bool> *buttons=state->GetButtonStates();
  i=0;
  c=buttons->size();
  while(i<c)
    {
    bool value;
    this->Socket->read(reinterpret_cast<char *>(&value),
                       sizeof(bool));
    (*buttons)[i]=value;
    ++i;
    }

  // read all valuators states.
  vtkstd::vector<float> *valuators=state->GetValuatorStates();
  i=0;
  c=valuators->size();
  while(i<c)
    {
    float value;
    this->Socket->read(reinterpret_cast<char *>(&value),
                       sizeof(float));
    (*valuators)[i]=value;
    ++i;
    }
}
