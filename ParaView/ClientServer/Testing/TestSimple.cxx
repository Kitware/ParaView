#include "vtkClientServerInterpreter.h"
#include "vtkClientServerMessage.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerArrayInformation.h"


class Server
{
public:
  Server();
  ~Server()
    {
      ClientServerInterpreter->Delete();
    }
  void GetResultMessageData(const unsigned char**, size_t*);
  void ProcessMessage(const unsigned char*, size_t);
  void PrintObjects();
private:
  vtkClientServerInterpreter* ClientServerInterpreter;
  vtkClientServerStream ServerStream;
};

  
class ClientManager
{
public:
  void SetServer(Server* s)
    {
      this->server = s;
    }
  vtkClientServerMessage* GetResultMessage();
  void RunTests();
  vtkClientServerStream stream;
  Server* server;
};


// Declare the initialization function as external
// this is defined in the PackageInit file
extern void Vtkparaviewcswrapped_Initialize(vtkClientServerInterpreter *arlu);

Server::Server()
{
  ClientServerInterpreter = vtkClientServerInterpreter::New();
  Vtkparaviewcswrapped_Initialize(this->ClientServerInterpreter);
}

void Server::GetResultMessageData(const unsigned char** data, size_t* len)
{ 
  vtkClientServerID id;
  id.ID=0;
  vtkClientServerMessage* ames = this->ClientServerInterpreter->GetMessageFromID(id);
  if(!ames)
    {
    *data  = 0;
    *len = 0;
    return;
    }
  this->ServerStream.Reset();
  this->ServerStream << ames << vtkClientServerStream::End;
  this->ServerStream.GetData(data, len);
}

void Server::ProcessMessage(const unsigned char* msg, size_t length)
{
  if(this->ClientServerInterpreter->ProcessMessage(msg, length))
    {
    cerr << "error in process message\n";
    }
}


void Server::PrintObjects()
{
}

vtkClientServerMessage* ClientManager::GetResultMessage()
{
  const unsigned char* data;
  size_t len;
  // simulate getting a message over a socket from the server
  server->GetResultMessageData(&data, &len);
  
  // now create a message on the client and print it out
  const unsigned char *nextPos;
  vtkClientServerMessage* ames
    = vtkClientServerMessage::GetMessage(data,len, &nextPos);
  return ames;
}



void ClientManager::RunTests()
{
  vtkClientServerID instance_id = this->stream.GetUniqueID();
  stream << vtkClientServerStream::New << "vtkObject" << instance_id << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << instance_id << "DebugOn" << vtkClientServerStream::End;
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "GetClassName" << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  vtkstd::string name;
  if(this->GetResultMessage()->GetArgument(0, &name))
    {
    cerr << name.c_str() << "\n";
    }
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "SetReferenceCount" << 10 << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "GetReferenceCount" << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  int refcount;
  if(this->GetResultMessage()->GetArgument(0, &refcount))
    {
    cerr << refcount << "\n";
    }
}

int main()
{
  Server server;
  ClientManager cmgr;
  cmgr.SetServer(&server);
  cmgr.RunTests();
  return 0;
}

// new names
// vtkClientServerInterpreter - vtkClientServerInterpreter - process data, len


// // vtkClientServerMessage - vtkClientServerMessage -
// // vtkClientServerStream - vtkClientServerStream  - has the unique id stuff and GetData(data, len)

// vtkPVClientServerModule - should have vtkClientServerStream
// If(this->ClientMode)
//   {
//     initialize
//       add method FlushMessages()
//       CallRMI
//       }
// else
// {
//   vtkClientServerInterpreter
    
//     add RMI
    
    


