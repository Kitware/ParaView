/*=========================================================================

   Program: ParaView
   Module:    pqAlwaysConnectedBehavior.h

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
#ifndef pqAlwaysConnectedBehavior_h
#define pqAlwaysConnectedBehavior_h

#include <QObject>

#include "pqApplicationComponentsModule.h"
#include "pqServerResource.h"
#include "pqTimer.h"

/**
* @ingroup Behaviors
* pqAlwaysConnectedBehavior ensures that the client always remains connected
* to a server.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqAlwaysConnectedBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAlwaysConnectedBehavior(QObject* parent = 0);
  ~pqAlwaysConnectedBehavior() override;

  /**
  * Get/Set the default server resource to connect to.
  */
  void setDefaultServer(const pqServerResource& resource) { this->DefaultServer = resource; }
  const pqServerResource& defaultServer() const { return this->DefaultServer; }

protected slots:
  void serverCheck();

protected:
  pqServerResource DefaultServer;
  pqTimer Timer;

private:
  Q_DISABLE_COPY(pqAlwaysConnectedBehavior)
};

#endif
