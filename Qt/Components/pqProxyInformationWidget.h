/*=========================================================================

   Program: ParaView
   Module:    pqProxyInformationWidget.h

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
#ifndef pqProxyInformationWidget_h
#define pqProxyInformationWidget_h

#include "pqComponentsModule.h"
#include <QScopedPointer>
#include <QWidget>

class pqOutputPort;
class vtkPVDataInformation;

/**
 * @class pqProxyInformationWidget
 * @brief Widget to show information about data produced by an algorithm
 *
 * pqProxyInformationWidget is intended to show meta-data about the data
 * produced by a VTK algorithm. In ParaView, that maps to representing
 * meta-data available in `vtkPVDataInformation`.
 *
 * pqProxyInformationWidget uses `pqActiveObjects` to monitor the active
 * output-port and updates to show data-information for the data produced by the
 * active output-port, if any.
 *
 */
class PQCOMPONENTS_EXPORT pqProxyInformationWidget : public QWidget
{
  Q_OBJECT;
  using Superclass = QWidget;

public:
  pqProxyInformationWidget(QWidget* p = 0);
  ~pqProxyInformationWidget() override;

  /**
   * Returns the output port from whose data information this widget is
   * currently showing.
   */
  pqOutputPort* outputPort() const;

  /**
   * Returns the `vtkPVDataInformation` currently shown.
   */
  vtkPVDataInformation* dataInformation() const;

public Q_SLOTS:
  /**
  * Set the display whose properties we want to edit.
  */
  void setOutputPort(pqOutputPort* outputport);

private Q_SLOTS:

  /**
   * Updates the UI using current vtkPVDataInformation.
   */
  void updateUI();
  void updateSubsetUI();

private:
  Q_DISABLE_COPY(pqProxyInformationWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
