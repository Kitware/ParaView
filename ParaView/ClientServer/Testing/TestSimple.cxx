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
  
  void ProcessMessage(const unsigned char*, size_t);
  void PrintObjects();
private:
  vtkClientServerInterpreter* ClientServerInterpreter;
};

  
class ClientManager
{
public:
  void SetServer(Server* s)
    {
      this->server = s;
    }
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

void Server::ProcessMessage(const unsigned char* msg, size_t length)
{
  if(this->ClientServerInterpreter->ProcessMessage(msg, length))
    {
    cerr << "error in process message\n";
    }
  else
    {
    vtkClientServerID id;
    id.ID=0;
    vtkClientServerMessage* ames = this->ClientServerInterpreter->GetMessageFromID(id);
    if(!ames)
      {
      cerr << "nothing returned\n";
      }
    else
      {
      unsigned int cc;
      cerr << "num args = " << ames->NumberOfArguments << "\n";
      for ( cc = 0; cc < ames->NumberOfArguments; cc ++ )
        {
        cerr << cc << ": ";
        if ( cc > 0 )
          {
          cerr << " ";
          }
        switch( ames->ArgumentTypes[cc] )
          {
          case vtkClientServerStream::string_value:
            {
            char* str = new char[ames->ArgumentSizes[cc]+1];
            str[ames->ArgumentSizes[cc]] = 0;
            strncpy(str, (const char*)ames->Arguments[cc], ames->ArgumentSizes[cc]);
            cerr << str << "\n";
            delete [] str;
            }
            break;
          case vtkClientServerStream::int_value:
            {
            cerr << *reinterpret_cast<const int*>(ames->Arguments[cc]) << "\n";
            }
            break;
          }
        }
      }
    }
}


void Server::PrintObjects()
{
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
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "SetReferenceCount" << 10 << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
  stream.Reset();
  stream << vtkClientServerStream::Invoke << instance_id << "GetReferenceCount" << vtkClientServerStream::End;
  stream.GetData(&data, &len);
  server->ProcessMessage(data, len);
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
    
    


