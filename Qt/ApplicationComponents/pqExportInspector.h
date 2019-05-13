/*=========================================================================

   Program: ParaView
   Module:  pqExportInspector.h

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
#ifndef pqExportInspector_h
#define pqExportInspector_h

#include "pqApplicationComponentsModule.h" // for exports
#include <QWidget>

/**
 * @class pqExportInspector
 * @brief widget to setup Catalyst export scripts and data products
 *
 * pqExportInspector is a central place where the user can set up
 * a set of data products that ParaView will produce via the Export Now
 * action or that Catalyst or a Temporal batch script will eventually
 * produce when they are run.
 */

class PQAPPLICATIONCOMPONENTS_EXPORT pqExportInspector : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqExportInspector(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), bool autotracking = true);
  ~pqExportInspector() override;

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
  Q_DISABLE_COPY(pqExportInspector);

  // helpers to maintain the content
  void PopulateWriterFormats();
  void PopulateViewFormats();
  void UpdateGlobalOptions();
  void UpdateGlobalOptions(const QString& searchString);
  void InternalScreenshotCheckbox(int i);
  void InternalWriterCheckbox(int i);
  bool IsWriterChecked(const QString& filter, const QString& writer);
  bool IsScreenShotChecked(const QString& filter, const QString& writer);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
