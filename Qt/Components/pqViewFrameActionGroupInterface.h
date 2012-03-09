/*=========================================================================

   Program: ParaView
   Module:    pqViewFrameActionGroupInterface.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqViewFrameActionGroupInterface_h
#define _pqViewFrameActionGroupInterface_h

#include <QtPlugin>
class pqViewFrameActionGroup;
class pqViewFrame;
class pqView;

/// interface class for plugins that create QActionGroups
/// for adding actions to view frames
class pqViewFrameActionGroupInterface
{
public:
  /// destructor
  virtual ~pqViewFrameActionGroupInterface() {}

  /// Add/remove plugin's actions to/from the frame 
  /// depending on the view type. Returns true if it did.
  /// Note that pqView* may be NULL, implying that the frame is not being
  /// assigned to any view.
  virtual bool connect(pqViewFrame*, pqView*) = 0;
  virtual bool disconnect(pqViewFrame*, pqView*) = 0;

  /// the instance of the QActionGroup that defines the actions
  virtual pqViewFrameActionGroup* actionGroup() = 0;
};

Q_DECLARE_INTERFACE(pqViewFrameActionGroupInterface, "com.kitware/paraview/viewframeactiongroup")

#endif

