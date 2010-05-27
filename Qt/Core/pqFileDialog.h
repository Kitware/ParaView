/*=========================================================================

   Program: ParaView
   Module:    pqFileDialog.h

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

#ifndef _pqFileDialog_h
#define _pqFileDialog_h

#include "pqCoreExport.h"
#include <QDialog>

class QModelIndex;
class QPoint;
class pqServer;

/**
  Provides a standard file dialog "front-end" for the pqFileDialogModel
  "back-end", i.e. it can be used for both local and remote file browsing.

  pqFileDialog can be used in both "modal" and "non-modal" operations.
  For "non-modal" operation, create an instance of pqFileDialog on the heap,
  set the Qt::WA_DeleteOnClose flag, connect to the fileSelected() signal,
  and show the dialog.  The dialog will be automatically destroyed when the
  user completes their file selection, and your slot will be called with
  the files the user selected:

  /code
  pqFileDialog* dialog = new pqFileDialog(NULL, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);

  QObject::connect(
    dialog,
    SIGNAL(filesSelected(const QStringList&)),
    this,
    SLOT(onOpenSessionFile(const QStringList&)));

  dialog->show();
  /endcode

  For "modal" operation, create an instance of pqFileDialog on the stack,
  call its exec() method, and retrieve the user's file selection with the
  getSelectedFiles() method:

  /code
  pqFileDialog dialog(NULL, this);
  if(Qt::Accepted == dialog.exec())
    {
    QStringList files = dialog.getSelectedFiles();
    }
  /endcode

  \sa pqFileDialogModel
*/

class PQCORE_EXPORT pqFileDialog :
  public QDialog
{
  typedef QDialog Superclass;
  Q_OBJECT
public:

  /// choose mode for selecting file/folder.
  /// AnyFile: The name of a file, whether it exists or not.
  ///   Typically used by "Save As..."
  /// ExistingFile: The name of a single existing file.
  ///   Typically used by "Open..."
  /// ExistingFiles: The names of zero or more existing files.
  /// Directory: The name of a directory.
  enum FileMode { AnyFile, ExistingFile, ExistingFiles, Directory };

  /// Creates a file dialog with the specified server
  /// if the server is NULL, files are browsed locally
  /// the title, and start directory may be specified
  /// the filter is a string of semi-colon separated filters
  pqFileDialog(pqServer*,
    QWidget* Parent,
    const QString& Title = QString(),
    const QString& Directory = QString(),
    const QString& Filter = QString());
  ~pqFileDialog();

  /// set the file mode
  void setFileMode(FileMode);

  /// set the most recently used file extension
  void setRecentlyUsedExtension(const QString& fileExtension);

  /// Returns the set of files
  QStringList getSelectedFiles();

  /// accept this dialog
  void accept();

  /// set a file current to support test playback
  bool selectFile(const QString&);

  /// set if we show hidden files and holders
  void setShowHidden( const bool& hidden);

  ///returns the state of the show hidden flag
  bool getShowHidden();

signals:
  /// Signal emitted when the user has chosen a set of files
  /// and accepted the dialog
  void filesSelected(const QStringList&);

  /// signal emitted when user has chosen a set of files and accepted the
  /// dialog.  This signal includes only the path and file string as is
  /// This is to support test recording
  void fileAccepted(const QString&);

private slots:
  void onModelReset();
  void onNavigate(const QString&);
  void onNavigateUp();
  void onNavigateBack();
  void onNavigateForward();
  void onNavigateDown(const QModelIndex&);
  void onFilterChange(const QString&);

  void onClickedRecent(const QModelIndex&);
  void onClickedFavorite(const QModelIndex&);
  void onClickedFile(const QModelIndex&);

  void onActivateFavorite(const QModelIndex&);
  void onActivateRecent(const QModelIndex&);
  void onDoubleClickFile( const QModelIndex& );

  void onTextEdited(const QString&);

  void onShowHiddenFiles( const bool &hide );

  // Called when the user changes the file selection.
  void fileSelectionChanged();

  // Called when the user right-clicks in the file qtreeview
  void onContextMenuRequested(const QPoint &pos);

  // Called when the user requests to create a new directory in the cwd
  void onCreateNewFolder();

  /// Emits the filesSelected() signal and closes the dialog,
  void emitFilesSelected(const QStringList&);

private:
  pqFileDialog(const pqFileDialog&);
  pqFileDialog& operator=(const pqFileDialog&);

  class pqImplementation;
  pqImplementation* const Implementation;

  void acceptInternal(QStringList& selected_files, const bool &doubleclicked);
  QString fixFileExtension(const QString& filename, const QString& filter);
};

#endif // !_pqFileDialog_h

