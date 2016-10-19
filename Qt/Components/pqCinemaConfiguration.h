/*=========================================================================

   Program: ParaView
   Module:  pqCinemaConfiguration.h

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
#ifndef pqCinemaConfiguration_h
#define pqCinemaConfiguration_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"

namespace Ui
{
class CinemaConfiguration;
}

/**
* @brief PropertyWidget used to define specifics of a Cinema database to be exported.
*
* This widget is used as the panel_widget of the vtkCinemaExporter (see SMApplication/
* Resources/utilities.xml), which is called from the top menu "Export Scene...". Some of
* its main components are also used in pqSGExportWizard.
*/
class PQCOMPONENTS_EXPORT pqCinemaConfiguration : public pqPropertyWidget
{
  Q_OBJECT;
  Q_PROPERTY(QString viewSelection READ viewSelection);
  Q_PROPERTY(QString trackSelection READ trackSelection);
  Q_PROPERTY(QString arraySelection READ arraySelection);

  typedef pqPropertyWidget Superclass;

public:
  pqCinemaConfiguration(vtkSMProxy* proxy_, vtkSMPropertyGroup* smpgroup, QWidget* parent_ = NULL);
  virtual ~pqCinemaConfiguration();

  virtual void updateWidget(bool showing_advanced_properties);

  /**
  * Get method for the viewSelection Q_PROPERTY. Defines a python script extract describing
  * the user-selected view options.
  */
  QString viewSelection();

  /**
  * Get method for the trackSelection Q_PROPERTY. Defines a python script extract describing
  * the user-selected track options.
  */
  QString trackSelection();

  /**
  * Get method for the arraySelection Q_PROPERTY. Defines a python script extract describing
  * the user-selected array options.
  */
  QString arraySelection();

protected:
  /**
  * Updates the vtkCinemaExporter proxy by emitting pqPropertyWidget's changeFinished() signal.
  */
  void hideEvent(QHideEvent* event_);

signals:

  void viewSelectionChanged();

  void trackSelectionChanged();

  void arraySelectionChanged();

private:
  void populateElements();

  Ui::CinemaConfiguration* Ui;

  Q_DISABLE_COPY(pqCinemaConfiguration)
};

#endif
