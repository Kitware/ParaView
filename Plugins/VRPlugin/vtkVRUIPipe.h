/*=========================================================================

   Program: ParaView
   Module:    vtkVRUIPipe.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef vtkVRUIPipe_h
#define vtkVRUIPipe_h
#include <string>

class QTcpSocket;

class vtkVRUIServerState;

class vtkVRUIPipe
{
public:
  // Description:
  // VRUI Protocol messages
  enum MessageTag
  {
    CONNECT_REQUEST = 0, // Request to connect to server
    CONNECT_REPLY,       // Positive connect reply with server layout
    DISCONNECT_REQUEST,  // Polite request to disconnect from server
    ACTIVATE_REQUEST,    // Request to activate server (prepare for sending packets)
    DEACTIVATE_REQUEST,  // Request to deactivate server (no more packet requests)
    PACKET_REQUEST,      // Requests a single packet with current device state
    PACKET_REPLY,        // Sends a device state packet
    STARTSTREAM_REQUEST, // Requests entering stream mode (server sends packets automatically)
    STOPSTREAM_REQUEST,  // Requests leaving stream mode
    STOPSTREAM_REPLY     // Server's reply after last stream packet has been sent
  };

  // Description:
  // Constructor from tcp socket.
  // \pre socket_exists: socket!=0
  vtkVRUIPipe(QTcpSocket* socket);

  // Description:
  // Destructor.
  ~vtkVRUIPipe();

  // Description:
  // Send a VRUI protocol message.
  void Send(MessageTag m);

  // Description:
  // Wait for server's reply (with a msecs milliseconds timeout).
  // Return true if there is something to read.
  // Return false in case of timeout or error.
  bool WaitForServerReply(int msecs);

  // Description:
  // Read message from server.
  MessageTag Receive();

  // Description:
  // Read server's layout (whatever that means) and initialize current state
  // \pre state_exists: state!=0
  void ReadLayout(vtkVRUIServerState* state);

  // Description:
  // Read server state.
  // \pre state_exists: state!=0
  void ReadState(vtkVRUIServerState* state);

  // Description:
  // Prints the appropriate packet type
  std::string GetString(MessageTag m)
  {
    switch (m)
    {
      case CONNECT_REQUEST:
        return "CONNECT_REQUEST";
      case CONNECT_REPLY:
        return "CONNECT_REPLY";
      case DISCONNECT_REQUEST:
        return "DISCONNECT_REQUEST";
      case ACTIVATE_REQUEST:
        return "ACTIVATE_REQUEST";
      case DEACTIVATE_REQUEST:
        return "DEACTIVATE_REQUEST";
      case PACKET_REQUEST:
        return "PACKET_REQUEST";
      case PACKET_REPLY:
        return "PACKET_REPLY";
      case STARTSTREAM_REQUEST:
        return "STARTSTREAM_REQUEST";
      case STOPSTREAM_REQUEST:
        return "STOPSTREAM_REQUEST";
      case STOPSTREAM_REPLY:
        return "STOPSTREAM_REPLY";
      default:
        return "UNKNOWN";
    }
  }

protected:
  QTcpSocket* Socket;

private:
  vtkVRUIPipe() = delete;
  vtkVRUIPipe(const vtkVRUIPipe&) = delete;
  void operator=(const vtkVRUIPipe&) = delete;
};

#endif // #ifndef vtkVRUIPipe_h
