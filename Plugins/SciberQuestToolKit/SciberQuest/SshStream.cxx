#include "SshStream.h"

#include <iostream>
#include <sstream>

#include <cstdio>
#include <cstring>
#include <cerrno>

//-----------------------------------------------------------------------------
int SshStream::Connect(std::string user, std::string host)
{
  this->Disconnect();

  this->Session=ssh_new();
  ssh_options_set(this->Session, SSH_OPTIONS_USER,user.c_str());
  ssh_options_set(this->Session, SSH_OPTIONS_HOST,host.c_str());

  int stat;

  stat=ssh_connect(this->Session);
  if (stat!=SSH_OK)
    {
    std::cerr
      << "Error:" << std::endl
      << "Failed to connect " << user << "@" << host << std::endl
      << ssh_get_error(this->Session) << std::endl
      << std::endl;
    return -1;
    }

  stat=ssh_is_server_known(this->Session);
  switch (stat)
    {
    // these are non-issues for this use case.
    case SSH_SERVER_KNOWN_OK:
    case SSH_SERVER_KNOWN_CHANGED:
    case SSH_SERVER_FOUND_OTHER:
      break;

    // update the known hosts.
    case SSH_SERVER_NOT_KNOWN:
      if (ssh_write_knownhost(this->Session)<0)
        {
        std::cerr
          << "Error:" << std::endl
          << "Write known hosts failed." << std::endl
          << strerror(errno) << std::endl;
        return -1;
        }
      break;

    // when can this occur??
    case SSH_SERVER_ERROR:
    case SSH_SERVER_FILE_NOT_FOUND:
    default:
      std::cerr
        << "Error:" << std::endl
        << ssh_get_error(this->Session) << std::endl
        << std::endl;
      return -1;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int SshStream::Authenticate(std::string user, std::string passwd)
{
  // authenticate
  int stat=ssh_userauth_password(this->Session,user.c_str(),passwd.c_str());
  if (stat!=SSH_OK)
    {
    std::cerr
      << "Error:" << std::endl
      << "Failed to authenticate using password." << std::endl
      << std::endl;
    return -1;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int SshStream::OpenChannel()
{
  // open a channel
  ssh_channel chan=channel_new(this->Session);
  if (chan==NULL)
    {
    std::cerr
      << "Error:" << std::endl
      << "Failed to open a channel." << std::endl
      << ssh_get_error(this->Session) << std::endl
      << std::endl;
    return -1;
    }

  int stat=channel_open_session(chan);
  if (stat!=SSH_OK)
    {
    std::cerr
      << "Error:" << std::endl
      << "Failed to open a channel." << std::endl
      << ssh_get_error(this->Session) << std::endl
      << std::endl;
    return -1;
    }

  this->Channel.push_back(chan);

  int cid=this->Channel.size()-1;

  return cid;
}

//-----------------------------------------------------------------------------
int SshStream::CloseChannel(size_t cid)
{
  if (cid>=this->Channel.size())
    {
    std::cerr
      << "Error: CloseChannel failed, no channel " << cid << "." << std::endl;
    return -1;
    }

  if (this->Channel[cid])
    {
    channel_send_eof(this->Channel[cid]);
    channel_close(this->Channel[cid]);
    channel_free(this->Channel[cid]);
    this->Channel[cid]=0;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int SshStream::CloseChannels()
{
  int nc=this->Channel.size();
  for (int i=0; i<nc; ++i)
    {
    this->CloseChannel(i);
    }
  return 0;
}

//-----------------------------------------------------------------------------
int SshStream::Disconnect()
{

  this->CloseChannels();

  if (this->Session)
    {
    ssh_free(this->Session);
    this->Session=0;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int SshStream::Exec(int cid, std::string command, std::string &buffer)
{
  int stat;

  stat=channel_request_exec(this->Channel[cid],command.c_str());
  if (stat!=SSH_OK)
    {
    std::cerr
      << "Error:" << std::endl
      << "Failed to exec \"" << command << "\" on channel " << cid << "." << std::endl
      << ssh_get_error(this->Session) << std::endl
      << std::endl;
    return -1;
    }

  int n=0;
  char buf[1024]={'\0'};

  std::ostringstream resss;
  while ((n=channel_read(this->Channel[cid],buf,sizeof(buf)-1,0))>0)
    {
    buf[n]='\0';
    resss << buf;
    }
  if (n<0)
    {
    std::cerr
      << "Error:" << std::endl
      << "Failed to read channel " << cid << "." << std::endl
      << ssh_get_error(this->Session) << std::endl
      << std::endl;
    return -1;
    }

  buffer=resss.str();

  return 0;
}
