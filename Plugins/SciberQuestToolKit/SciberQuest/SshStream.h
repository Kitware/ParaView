/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SshStream_h
#define SshStream_h

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
