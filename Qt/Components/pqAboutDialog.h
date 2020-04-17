/*=========================================================================

   Program: ParaView
   Module:    $RCS $

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
#ifndef _pqAboutDialog_h
#define _pqAboutDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

namespace Ui
{
class pqAboutDialog;
}

class pqServer;
class QTreeWidget;

/**
* pqAboutDialog is the about dialog used by ParaView.
* It provides information about ParaView and current configuration.
*/
class PQCOMPONENTS_EXPORT pqAboutDialog : public QDialog
{
  Q_OBJECT

public:
  pqAboutDialog(QWidget* Parent);
  ~pqAboutDialog() override;

  /**
   * Format the about dialog content into textual form
   */
  QString formatToText();

public Q_SLOTS:

  /**
   * Saves about dialog formatted output to a file.
   */
  void saveToFile();

  /**
   * Copy about dialog formatted output to the clipboard.
   */
  void copyToClipboard();

protected:
  void AddClientInformation();
  void AddServerInformation();
  void AddServerInformation(pqServer* server, QTreeWidget* tree);
  QString formatToText(QTreeWidget* tree);

private:
  Q_DISABLE_COPY(pqAboutDialog)
  Ui::pqAboutDialog* const Ui;
};

#endif // !_pqAboutDialog_h
