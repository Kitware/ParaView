/*=========================================================================

   Program: ParaView
   Module:  pqDesktopServicesReaction.h

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
#ifndef pqDesktopServicesReaction_h
#define pqDesktopServicesReaction_h

#include "pqReaction.h"

#include <QUrl>

/**
* @ingroup Reactions
* pqDesktopServicesReaction can be used to open a file (or URL) using
* QDesktopServices. e.g. if your application wants to launch a PDF viewer to
* open the application's User Guide, you can hookup a menu QAction to the
* pqDesktopServicesReaction. e.g.
* @code
* QAction* action = ...
* new pqDesktopServicesReaction(QUrl("file:///..../doc/UsersGuide.pdf"), action);
* @endcode
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqDesktopServicesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqDesktopServicesReaction(const QUrl& url, QAction* parent);
  virtual ~pqDesktopServicesReaction();

  /**
  * Attempt to open a file (local or on the Web) using QDesktopServices.
  * Returns false if failed to open for some reason, other returns true.
  */
  static bool openUrl(const QUrl& url);

protected:
  virtual void onTriggered() { pqDesktopServicesReaction::openUrl(this->URL); }

private:
  Q_DISABLE_COPY(pqDesktopServicesReaction)
  QUrl URL;
};

#endif
