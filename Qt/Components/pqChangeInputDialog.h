/*=========================================================================

   Program: ParaView
   Module:    pqChangeInputDialog.h

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
  pqChangeInputDialog(vtkSMProxy* filterProxy, QWidget* parent = 0);
  ~pqChangeInputDialog() override;

  /**
  * Returns the map of selected inputs. The key in this map is the name of the
  * input property, while the values in the map are the list of output ports
  * that are chosen to be the input for that property. This list will contain
  * at most 1 item, when the input property indicates that it can accept only
  * 1 value.
  */
  const QMap<QString, QList<pqOutputPort*> >& selectedInputs() const;

protected Q_SLOTS:
  void inputPortToggled(bool);
  void selectionChanged();

protected:
  void buildPortWidgets();

private:
  Q_DISABLE_COPY(pqChangeInputDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
