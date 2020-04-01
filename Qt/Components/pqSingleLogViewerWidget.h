/*=========================================================================

   Program: ParaView
   Module:  pqSingleLogViewerWidget.h

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

#ifndef pqSingleLogViewWidget_h
#define pqSingleLogViewWidget_h

#include "pqComponentsModule.h"
#include "pqLogViewerWidget.h"

#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

/**
 * @class pqSingleLogViewerWidget
 * @brief A single log viewer widget which has a reference to the log recorder proxy.
 */
class PQCOMPONENTS_EXPORT pqSingleLogViewerWidget : public pqLogViewerWidget
{
  Q_OBJECT
  using Superclass = pqLogViewerWidget;

public:
  /**
   * @param parent The parent of this widget.
   * @param logRecorderProxy has the information of server location,
   * and the rank setting indicates which rank whose log is shown in
   * this widget.
   * @param rank
   */
  pqSingleLogViewerWidget(QWidget* parent, vtkSmartPointer<vtkSMProxy> logRecorderProxy, int rank);
  ~pqSingleLogViewerWidget() override = default;

  /**
   * Refresh the log viewer
   */
  void refresh();

  /**
   * Get the log recorder proxy reference hold by this widget.
   */
  const vtkSmartPointer<vtkSMProxy>& getLogRecorderProxy() const;

  /**
   * Get the rank of the process whose log is shown in this widget.
   */
  int getRank() const;

  /**
   * Disable log recording of the specific rank when the widget is closed.
   */
  void closeEvent(QCloseEvent*) override;

private:
  Q_DISABLE_COPY(pqSingleLogViewerWidget);

  vtkSmartPointer<vtkSMProxy> LogRecorderProxy;
  int Rank;
};

#endif // pqSingleLogViewWidget_h
