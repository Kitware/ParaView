// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSelectReaderDialog_h
#define pqSelectReaderDialog_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QScopedPointer> // needed for ivar

class pqServer;
class vtkSMReaderFactory;
class vtkStringList;

/**
 * a dialog that prompts for a reader type to open a file
 */
class PQCOMPONENTS_EXPORT pqSelectReaderDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * constructor
   */
  pqSelectReaderDialog(
    const QString& file, pqServer* s, vtkSMReaderFactory* factory, QWidget* p = nullptr);

  pqSelectReaderDialog(const QString& file, pqServer* s, vtkStringList* list, QWidget* p = nullptr);
  /**
   * destructor
   */
  ~pqSelectReaderDialog() override;

  /**
   * get the reader that was chosen to read a file
   */
  QString getReader() const;

  /**
   * get the group for the chosen reader.
   */
  QString getGroup() const;

  /**
   * Check if the user clicked the "Set reader as default" button
   */
  bool isSetAsDefault() const;

protected:
  class pqInternal;
  QScopedPointer<pqInternal> Internal;
};

#endif
