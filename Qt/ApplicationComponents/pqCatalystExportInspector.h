/*=========================================================================

   Program: ParaView
   Module:  pqCatalystExportInspector.h

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
#ifndef pqCatalystExportInspector_h
#define pqCatalystExportInspector_h

#include "pqApplicationComponentsModule.h" // for exports
#include <QWidget>

/**
 * @class pqCatalystExportInspector
 * @brief widget to setup Catalyst export scripts and data products
 *
 * pqCatalystExportInspector is a central place where the user can set up
 * a Catalyst script including all of the data products that the script
 * will produce when a Catalyzed simulation runs it.
 */

class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystExportInspector : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqCatalystExportInspector(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), bool autotracking = true);
  ~pqCatalystExportInspector() override;

protected:
  /**
  * receive notification whenever made visible.
  */
  void showEvent(QShowEvent* event) override;

private slots:
  /**
  * Populates the widget content with global options and sourceproxy and view lists.
  */
  void Update();

  /**
  * Maintains state of writer proxy's enable property
  */
  void UpdateWriterCheckbox(int i = -1);
  /**
  * User pressed on checkbox to turn on export of a filter
  */
  void ExportFilter(bool);
  /**
  * User pressed on ... button to configure export of a filter
  */
  void ConfigureWriterProxy();

  /**
  * Maintains state of screenshot proxy's enable property
  */
  void UpdateScreenshotCheckbox(int i = -1);
  /**
  * User pressed on checkbox to turn on export of a view
  */
  void ExportView(bool);
  /**
  * User pressed on ... button to configure export of a view
  */
  void ConfigureScreenshotProxy();

  /**
  * User pressed on advanced button
  */
  void Advanced(bool);

  /**
  * User pressed on help button.
  */
  void Help();

  /**
  * Search text changed.
  */
  void Search(const QString&);

private:
  Q_DISABLE_COPY(pqCatalystExportInspector);

  // helpers to maintain the content
  void PopulateWriterFormats();
  void PopulateViewFormats();
  void UpdateGlobalOptions();
  void UpdateGlobalOptions(const QString& searchString);
  void InternalScreenshotCheckbox(int i);
  void InternalWriterCheckbox(int i);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
