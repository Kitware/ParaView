/*=========================================================================

   Program: ParaView
   Module:    ParaViewVRUI.h

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
#ifndef __ParaViewVRUI_h
#define __ParaViewVRUI_h

#include <QObject>

/// VRUI client
class ParaViewVRUI : public QObject
{
  Q_OBJECT
public:
  ParaViewVRUI();
  ~ParaViewVRUI();

  // Description:
  // Name of the VRUI server. For example, "localhost"
  // Initial value is a NULL pointer.
  void SetName(const char *name);
  const char *GetName() const;

  // Description:
  // Port number of the VRUI server.
  // Initial value is 8555.
  void SetPort(int port);
  int GetPort() const;

  // Description:
  // Initialize the device with the name.
  void Init();

  // Description:
  // Tell if Init() was called succesfully
  bool GetInitialized() const;

  // Description:
  void Activate();

  // Description:
  void Deactivate();

  // Description:
  void StartStream();

  // Description:
  void StopStream();

protected slots:
  void callback();

protected:
  void PrintPositionOrientation();
  void GetNextPacket();

  char *Name;
  int Port;

  class pqInternals;
  pqInternals* Internals;

  bool Initialized;

private:
  ParaViewVRUI(const ParaViewVRUI&); // Not implemented.
  void operator=(const ParaViewVRUI&); // Not implemented.
};

#endif
