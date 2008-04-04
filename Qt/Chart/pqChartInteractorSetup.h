/*=========================================================================

   Program: ParaView
   Module:    pqChartInteractorSetup.h

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

/// \file pqChartInteractorSetup.h
/// \date 6/25/2007

#ifndef _pqChartInteractorSetup_h
#define _pqChartInteractorSetup_h


#include "QtChartExport.h"

class pqChartArea;
class pqChartMouseSelection;


/// \class pqChartInteractorSetup
/// \brief
///   The pqChartInteractorSetup class is used to set up the chart
///   interactor.
class QTCHART_EXPORT pqChartInteractorSetup
{
public:
  pqChartInteractorSetup() {}
  ~pqChartInteractorSetup() {}

  /// \brief
  ///   Creates the default interactor setup for the given chart.
  ///
  /// Selection is set on the left mouse button. All the zoom
  /// functionality is added to the middle button. The panning
  /// capability is added to the right button. The separate zooming
  /// functions are accessed using keyboard modifiers.
  ///   \li No modifiers: regular drag zoom.
  ///   \li Control: x-only drag zoom.
  ///   \li Alt: y-only drag zoom.
  ///   \li Shift: zoom box.
  ///
  /// The interactor is created as a child of the chart area. The
  /// mouse functions are created as children of the interactor.
  ///
  /// \param area The chart to add the interactor to.
  /// \return
  ///   A pointer to the mouse selection handler.
  static pqChartMouseSelection *createDefault(pqChartArea *area);

  /// \brief
  ///   Creates an interactor with the zoom functionality on separate
  ///   buttons.
  ///
  /// The panning capability is added to the left button along with
  /// selection. The left button interaction mode must be set to
  /// access the different functionality. The zoom box function is
  /// set on the right button. The rest of the zoom capability is
  /// added to the middle button. X-only and y-only zooms are
  /// accessed using the control and alt modifiers respectively. If
  /// no modifiers are pressed, regular drag zoom is activated.
  ///
  /// The interactor is created as a child of the chart area. The
  /// mouse functions are created as children of the interactor.
  ///
  /// \param area The chart to add the interactor to.
  /// \return
  ///   A pointer to the mouse selection handler.
  static pqChartMouseSelection *createSplitZoom(pqChartArea *area);
};

#endif
