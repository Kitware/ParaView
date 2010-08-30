#include "vruiPipe.h"
#include <cassert>
#include <QTcpSocket>

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
  this->Socket->write(&message,sizeof(MessageTagPropocol));
}

// ----------------------------------------------------------------------------
bool vruiPipe::WaitForServerReply()
{
  return this->Socket->waitForReadyRead(30000); // 30 secs expressed in msecs.
}

// ----------------------------------------------------------------------------
MessageTag vruiPipe::Receive()
{
  MessageTagPropocol message;
  this->Socket->read(&message,sizeof(MessageTagPropocol));

  return static_cast<MessageTag>(message);
}

// ----------------------------------------------------------------------------
void vruiPipe::ReadLayout(vruiServerState *state)
{
  assert("pre: state_exists" && state!=0);

  int value;
  this->Socket->readData(&value,sizeof(int));
  state->GetTrackerStates()->resize(value);

  this->Socket->readData(&value,sizeof(int));
  state->GetButtonStates()->resize(value);

  this->Socket->readData(&value,sizeof(int));
  state->GetValuatorStates()->resize(value);
}
