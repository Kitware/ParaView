/*=========================================================================

   Program: ParaView
   Module:  pqReloadFilesReaction.h

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
#ifndef pqReloadFilesReaction_h
#define pqReloadFilesReaction_h

#include "pqReaction.h"
class vtkSMSourceProxy;

/**
* @ingroup Reactions
*
* pqReloadFilesReaction adds handler code to reload the active reader.
* It calls the "reload" property, identified by hints, if present, or calls
* vtkSMProxy::RecreateVTKObjects().
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqReloadFilesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqReloadFilesReaction(QAction* parent = 0);
  virtual ~pqReloadFilesReaction();

  /**
  * reload the active proxy if it supports reload. Returns true on success.
  */
  static bool reload();

  /**
  * reload the \c proxy if it supports reload. Returns true on success.
  */
  static bool reload(vtkSMSourceProxy* proxy);

protected:
  virtual void onTriggered() { this->reload(); }
  virtual void updateEnableState();

private:
  Q_DISABLE_COPY(pqReloadFilesReaction)
};

#endif
