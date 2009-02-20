/*=========================================================================

   Program: ParaView
   Module:    pqBoxChartViewContextMenuHandler.h

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

#ifndef _pqBoxChartViewContextMenuHandler_h
#define _pqBoxChartViewContextMenuHandler_h


#include "pqComponentsExport.h"
#include "pqChartViewContextMenuHandler.h"


/// \class pqBoxChartViewContextMenuHandler
/// \brief
///   The pqBoxChartViewContextMenuHandler class sets up and cleans up
///   context menus for the statistical box chart view.
class PQCOMPONENTS_EXPORT pqBoxChartViewContextMenuHandler :
    public pqChartViewContextMenuHandler
{
public:
  /// \brief
  ///   Creates a statistical box chart view context menu handler.
  /// \param parent The parent object.
  pqBoxChartViewContextMenuHandler(QObject *parent=0);
  virtual ~pqBoxChartViewContextMenuHandler() {}

protected:
  /// \brief
  ///   Creates the box chart context menu setup object for the view.
  /// \param view The chart view to set up.
  /// \return
  ///   A new chart context menu setup object.
  virtual pqChartViewContextMenu *createContextMenu(pqView *view);
};

#endif
