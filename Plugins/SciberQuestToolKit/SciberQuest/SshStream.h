#ifndef __SshStream_h
#define __SshStream_h

#include <libssh/libssh.h> // for ssh_session

#include <string> // for string
#include <vector> // for vector

/**
Class that encapsulates an ssh connection and stream between
this and a remote host where commands are executed remotely
and the results may be read from the stream.
*/
class SshStream
{
public:
  SshStream()
       :
    Session(0),
    Channel(0)
      {
      ssh_init();
      }

  ~SshStream()
    {
    this->Disconnect();
    ssh_finalize();
    }


  /**
  Make a connection to a remote host. After connection you
  must authenticate and open a channel for use.
  */
  int Connect(std::string user, std::string host);

  /**
  Disconnect and free resources.
  */
  int Disconnect();

  /**
  Authenticate with the server.
  */
  int Authenticate(std::string user,std::string passwd);

  /**
  Open a channel, if successful an channel id >0 is returned.
  */
  int OpenChannel();

  /**
  Close a channel.
  */
  int CloseChannel(size_t i);
  int CloseChannels();

  /**
  Execute the command on the remote host over the channel identified by cid.
  */
  int Exec(int cid, std::string command, std::string &buffer);

  /**
  Validate the stream;
  */
//   bool Ok(){ return ((this->Session!=0)&&(this->Channel!=0)); }

protected:

private:
  ssh_session Session;
  std::vector<ssh_channel> Channel;
};

#endif

// VTK-HeaderTest-Exclude: SshStream.h
