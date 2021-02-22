/*=========================================================================

   Program: ParaView
   Module:  pqViewFrameActionsInterface.h

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
#ifndef pqViewFrameActionsInterface_h
#define pqViewFrameActionsInterface_h

#include "pqComponentsModule.h"
#include <QtPlugin>

class pqViewFrame;
class pqView;

/**
* pqViewFrameActionsInterface is an interface used by pqMultiViewWidget
* to add actions/toolbuttons to pqViewFrame placed in a pqMultiViewWidget.
* Thus, if you want to customize the buttons shown at the top of a view frame
* in your application/plugin, this is the interface to implement.
*/
class PQCOMPONENTS_EXPORT pqViewFrameActionsInterface
{
public:
  virtual ~pqViewFrameActionsInterface();

  /**
  * This method is called after a frame is assigned to a view. The view may be
  * nullptr, indicating the frame has been assigned to an empty view. Frames are
  * never reused (except a frame assigned to an empty view).
  */
  virtual void frameConnected(pqViewFrame* frame, pqView* view) = 0;

private:
};
Q_DECLARE_INTERFACE(pqViewFrameActionsInterface, "com.kitware/paraview/viewframeactions");
#endif
