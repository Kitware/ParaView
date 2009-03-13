/*=========================================================================

   Program: ParaView
   Module:    StopListDialog.h

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

// .NAME StopListDialog - Editor for a list of stop words
//
// .SECTION Description
//
// This is a QListView-based editor for a list of strings. 

#ifndef __StopListDialog_h
#define __StopListDialog_h

#include "OverViewCoreExport.h"
#include "pqDialog.h"

#include <ui_StopListDialog.h>
#include <QStringList>

class OVERVIEW_CORE_EXPORT StopListDialog : public pqDialog, public Ui::StopListDialog
{
  Q_OBJECT
public:
  StopListDialog(QWidget *parent=0);
  ~StopListDialog();

  QStringList stopWords() const;
  void setStopWords(const QStringList &words);

  // Description:
  // Load the stop list from a file.  Each line of the file will be
  // treated as a separate stop word.  Returns true on success; false
  // if there was an error with the file.
  bool importStopList(const QString &filename, QString *errorMsg = 0);

  // Description:
  // Save the stop list to a file.  Writes one stopword per line.
  // Returns true on success; false if there was an error with the
  // file.
  bool exportStopList(const QString &filename, QString *errorMsg = 0);

signals:
  // This will be emitted when the user clicks the Apply button.
  void applyStopList(const QStringList &words);

  public slots:
  void handleImportButton();
  void handleExportButton();
  void handleAddItemButton();
  void handleApplyButton();

};

#endif
