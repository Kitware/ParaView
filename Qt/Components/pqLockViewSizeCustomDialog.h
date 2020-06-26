/*=========================================================================

   Program: ParaView
   Module:    pqLockViewSizeCustomDialog.h

   Copyright (c) 2005-2010 Sandia Corporation, Kitware Inc.
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
#ifndef pqLockViewSizeCustomDialog_h
#define pqLockViewSizeCustomDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

/**
* Dialog used to ask the user what resolution to lock the views to.
*/
class PQCOMPONENTS_EXPORT pqLockViewSizeCustomDialog : public QDialog
{
  Q_OBJECT;
  typedef QDialog Superclass;

public:
  pqLockViewSizeCustomDialog(QWidget* parent, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqLockViewSizeCustomDialog() override;

  /**
  * The custom resolution currently entered by the user.
  */
  QSize customResolution() const;

public Q_SLOTS:
  /**
  * Sets the view size to the displayed resolution.
  */
  virtual void apply();

  /**
  * Applies the resolution and accepts the dialog.
  */
  void accept() override;

  /**
  * Unlocks the size on the view.
  */
  virtual void unlock();

private:
  Q_DISABLE_COPY(pqLockViewSizeCustomDialog)

  class pqUI;
  pqUI* ui;
};

#endif // pqLockViewSizeCustomDialog_h
