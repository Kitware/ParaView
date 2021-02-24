/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqCollaborationEventPlayer_h
#define pqCollaborationEventPlayer_h

#include "pqCoreModule.h"
#include "pqWidgetEventPlayer.h"

/**
* pqCollaborationEventPlayer is used to playback events that make
* collaborative-testing possible. These events cannot be recorded by the
* test-recorder, but are manually added. But once added, they enable the
* playback to wait for appropriate actions to happen.
*/
class PQCORE_EXPORT pqCollaborationEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqCollaborationEventPlayer(QObject* parent = nullptr);
  ~pqCollaborationEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* object, const QString& command, const QString& arguments, bool& error) override;

  /**
  * used to wait till the process becomes a master.
  */
  static void waitForMaster(int ms = 500);

  /**
  * used to wait until there are num_connection connections.
  */
  static void waitForConnections(int num_connections);

  static void wait(int milli_seconds);

private:
  Q_DISABLE_COPY(pqCollaborationEventPlayer)
};

#endif
