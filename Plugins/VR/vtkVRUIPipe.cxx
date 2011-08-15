#include "vtkVRUIPipe.h"
#include <cassert>
#include <QTcpSocket>
#include "vtkVRUIServerState.h"

typedef unsigned short MessageTagPropocol;

// ----------------------------------------------------------------------------
vtkVRUIPipe::vtkVRUIPipe(QTcpSocket *socket)
{
  assert("pre: socket_exist" && socket!=0);
  this->Socket=socket;
}

// ----------------------------------------------------------------------------
vtkVRUIPipe::~vtkVRUIPipe()
{
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::Send(MessageTag m)
{
  MessageTagPropocol message=m;
  this->Socket->write(reinterpret_cast<const char*>(&message),
                      sizeof(MessageTagPropocol));
}

// ----------------------------------------------------------------------------
bool vtkVRUIPipe::WaitForServerReply(int msecs)
{
  return this->Socket->waitForReadyRead(msecs);
}

// ----------------------------------------------------------------------------
vtkVRUIPipe::MessageTag vtkVRUIPipe::Receive()
{
  MessageTagPropocol message;

  qint64 bytes=0;

  while(bytes!=sizeof(MessageTagPropocol)) // 2
    {
    bytes=this->Socket->read(reinterpret_cast<char *>(&message),
                             sizeof(MessageTagPropocol));
    }

//  cout << "bytes=" << bytes << endl;
//  cout << "sizeof=" << sizeof(MessageTagPropocol) <<  endl;

  return static_cast<MessageTag>(message);
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::ReadLayout(vtkVRUIServerState *state)
{
  assert("pre: state_exists" && state!=0);

  int value;
  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
  state->GetTrackerStates()->resize(value);
  int i=0;
  while(i<value)
    {
    (*(state->GetTrackerStates()))[i]=vtkVRUITrackerState::New();
    ++i;
    }

  cout << "number of trackers: " << value << endl;

  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
  state->GetButtonStates()->resize(value);

  cout << "number of buttons: " << value << endl;

  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
  state->GetValuatorStates()->resize(value);

  cout << "number of valuators: " << value << endl;
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::ReadState(vtkVRUIServerState *state)
{
  assert("pre: state_exists" && state!=0);

  // read all trackers states.
  vtkstd::vector<vtkSmartPointer<vtkVRUITrackerState> > *trackers=state->GetTrackerStates();
  size_t i=0;
  size_t c=trackers->size();
  while(i<c)
    {
    vtkVRUITrackerState *tracker=(*trackers)[i].GetPointer();
    this->Socket->read(reinterpret_cast<char *>(tracker->GetPosition()),
                       3*sizeof(float));
    this->Socket->read(reinterpret_cast<char *>(tracker->GetUnitQuaternion()),
                       4*sizeof(float));
    this->Socket->read(reinterpret_cast<char *>(tracker->GetLinearVelocity()),
                       3*sizeof(float));
    this->Socket->read(reinterpret_cast<char *>(tracker->GetAngularVelocity()),
                       3*sizeof(float));
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
