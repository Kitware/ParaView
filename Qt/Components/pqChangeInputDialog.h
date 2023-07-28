// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqChangeInputDialog_h
#define pqChangeInputDialog_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QList>
#include <QMap>

class pqOutputPort;
class vtkSMProxy;

/**
 * pqChangeInputDialog is the dialog used to allow the user to change/set the
 * input(s) for a filter. It does not actually change the inputs, that is left
 * for the caller to do when the exec() returns with with QDialog::Accepted.
 */
class PQCOMPONENTS_EXPORT pqChangeInputDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  /**
   * Constructor. \c filterProxy is the proxy for the filter whose inputs are
   * being changed using this dialog. The filterProxy can be a prototype proxy
   * when using this dialog to set up the inputs during filter creation.
   * The values from the input properties of the \c filterProxy are used as the
   * default values shown by this dialog.
   */
  pqChangeInputDialog(vtkSMProxy* filterProxy, QWidget* parent = nullptr);
  ~pqChangeInputDialog() override;

  /**
   * Returns the map of selected inputs. The key in this map is the name of the
   * input property, while the values in the map are the list of output ports
   * that are chosen to be the input for that property. This list will contain
   * at most 1 item, when the input property indicates that it can accept only
   * 1 value.
   */
  const QMap<QString, QList<pqOutputPort*>>& selectedInputs() const;

protected Q_SLOTS:
  void inputPortToggled(bool);
  void selectionChanged();

protected: // NOLINT(readability-redundant-access-specifiers)
  void buildPortWidgets();

private:
  Q_DISABLE_COPY(pqChangeInputDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
