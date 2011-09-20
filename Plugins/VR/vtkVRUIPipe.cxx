#include "vtkVRUIPipe.h"
#include <cassert>
#ifdef QTSOCK
#include <QTcpSocket>
#else
#include <QDebug>
#endif
#include "vtkVRUIServerState.h"

typedef unsigned short MessageTagPropocol;

// ----------------------------------------------------------------------------
#ifdef QTSOCK
vtkVRUIPipe::vtkVRUIPipe(QTcpSocket *socket)
{
  assert("pre: socket_exist" && socket!=0);
  this->Socket=socket;
}
#else
vtkVRUIPipe::vtkVRUIPipe(int socket)
{
  this->Socket=socket;
}
#endif

// ----------------------------------------------------------------------------
vtkVRUIPipe::~vtkVRUIPipe()
{
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::Send(MessageTag m)
{
  MessageTagPropocol message=m;
  //std::cout << "Sending : " << this->GetString( m ) << std::endl;
#ifdef QTSOCK
  this->Socket->write(reinterpret_cast<const char*>(&message),
                      sizeof(MessageTagPropocol));
  this->Socket->flush();
#else
  write(this->Socket, reinterpret_cast<const char*>(&message),
                      sizeof(MessageTagPropocol));
#endif
}

// ----------------------------------------------------------------------------
bool vtkVRUIPipe::WaitForServerReply(int vtkNotUsed( msecs ))
{
#ifdef QTSOCK
  //std::cout<< "in" <<std::endl;
  bool status = this->Socket->waitForReadyRead(500);
  //std::cout<< "out : " << status <<std::endl;
  return status;
#else
  return true;
#endif
}

// ----------------------------------------------------------------------------
vtkVRUIPipe::MessageTag vtkVRUIPipe::Receive()
{
  MessageTagPropocol message;

  qint64 bytes=0;

  while(bytes!=sizeof(MessageTagPropocol)) // 2
    {
    //std::cout<< "Waiting to recieve" <<std::endl;
#ifdef QTSOCK
    bytes=this->Socket->read(reinterpret_cast<char *>(&message),
                             sizeof(MessageTagPropocol));
#else
    bytes=read(this->Socket,reinterpret_cast<char *>(&message),
                             sizeof(MessageTagPropocol));
    if (bytes < 0) {
        qDebug() << "Socket read error";
    }
#endif
#ifdef VRUI_ENABLE_DEBUG
    if ( bytes )
      cout << "bytes=" << bytes << endl;

  cout << "sizeof=" << sizeof(MessageTagPropocol) <<  endl;
  std::cout << "Recieved : "
            << this->GetString( static_cast<MessageTag>( message ) )
            << static_cast<MessageTag>( message )
            << std::endl;
#endif
    }
  return static_cast<MessageTag>(message);
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::ReadLayout(vtkVRUIServerState *state)
{
  assert("pre: state_exists" && state!=0);

  int value;
#ifdef QTSOCK
  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
#else
  ssize_t bytes;
  bytes = read(this->Socket, reinterpret_cast<char *>(&value),sizeof(int));
    if (bytes < 0) {
#ifdef VRUI_ENABLE_DEBUG
        qDebug() << "Socket readlayout tracker error";
#endif
    }
#endif
  state->GetTrackerStates()->resize(value);
  int i=0;
  while(i<value)
    {
    (*(state->GetTrackerStates()))[i]=vtkVRUITrackerState::New();
    ++i;
    }

  cout << "number of trackers: " << value << endl;

#ifdef QTSOCK
  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
#else
  bytes = read(this->Socket, reinterpret_cast<char *>(&value),sizeof(int));
    if (bytes < 0) {
        qDebug() << "Socket readlayout buttons error";
    }
#endif
  state->GetButtonStates()->resize(value);

  cout << "number of buttons: " << value << endl;

#ifdef QTSOCK
  this->Socket->read(reinterpret_cast<char *>(&value),sizeof(int));
#else
  bytes = read(this->Socket, reinterpret_cast<char *>(&value),sizeof(int));
    if (bytes < 0) {
        qDebug() << "Socket readlayout valuators error";
    }
#endif
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

  quint64 readSize;

  while(i<c)
    {
    vtkVRUITrackerState *tracker=(*trackers)[i].GetPointer();
#ifdef QTSOCK
    readSize = this->Socket->read(reinterpret_cast<char *>(tracker->GetPosition()), 3*sizeof(float));
    if(readSize < 3 * sizeof(float)){qDebug() << "position: " << readSize;}
    readSize = this->Socket->read(reinterpret_cast<char *>(tracker->GetUnitQuaternion()), 4*sizeof(float));
    if(readSize < 4 * sizeof(float)){qDebug() << "quat:" << readSize;}
    readSize = this->Socket->read(reinterpret_cast<char *>(tracker->GetLinearVelocity()), 3*sizeof(float));
    if(readSize < 3 * sizeof(float)){qDebug() << "lv: " << readSize;}
    readSize = this->Socket->read(reinterpret_cast<char *>(tracker->GetAngularVelocity()), 3*sizeof(float));
    if(readSize < 3 * sizeof(float)){qDebug() << "av: " << readSize;}
#else
    readSize = read(this->Socket, reinterpret_cast<char *>(tracker->GetPosition()), 3*sizeof(float));
    if(readSize < 3 * sizeof(float)){qDebug() << "position: " << readSize;}
    readSize = read(this->Socket, reinterpret_cast<char *>(tracker->GetUnitQuaternion()), 4*sizeof(float));
    if(readSize < 4 * sizeof(float)){qDebug() << "quat:" << readSize;}
    readSize = read(this->Socket, reinterpret_cast<char *>(tracker->GetLinearVelocity()), 3*sizeof(float));
    if(readSize < 3 * sizeof(float)){qDebug() << "lv: " << readSize;}
    readSize = read(this->Socket, reinterpret_cast<char *>(tracker->GetAngularVelocity()), 3*sizeof(float));
    if(readSize < 3 * sizeof(float)){qDebug() << "av: " << readSize;}
#endif
    ++i;
    }
  // read all buttons states.
  vtkstd::vector<bool> *buttons=state->GetButtonStates();
  i=0;
  c=buttons->size();
  while(i<c)
    {
    bool value;
#ifdef QTSOCK
    readSize = this->Socket->read(reinterpret_cast<char *>(&value), sizeof(bool));
#else
    readSize = read(this->Socket, reinterpret_cast<char *>(&value), sizeof(bool));
#endif
    if(readSize < sizeof(bool)){qDebug() << "button : " << i <<  readSize;}
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
#ifdef QTSOCK
    readSize  = this->Socket->read(reinterpret_cast<char *>(&value), sizeof(float));
#else
    readSize  = read(this->Socket, reinterpret_cast<char *>(&value), sizeof(float));
#endif
    if(readSize < sizeof(float)){qDebug() << "analog : " << i <<  readSize;}
    (*valuators)[i]=value;
    ++i;
    }
}
