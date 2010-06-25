/*=========================================================================

   Program: ParaView
   Module:    pqOptionsPage.h

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

/// \file pqOptionsPage.h
/// \date 7/26/2007

#ifndef _pqOptionsPage_h
#define _pqOptionsPage_h


#include "pqComponentsExport.h"
#include <QWidget>


/// \class pqOptionsPage
/// \brief
///   The pqOptionsPage class is used to add a single page of options
///   to the pqOptionsDialog.
class PQCOMPONENTS_EXPORT pqOptionsPage : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs an options page.
  /// \param parent The parent widget.
  pqOptionsPage(QWidget *parent=0);
  virtual ~pqOptionsPage() {}

  /// \brief
  ///   Gets whether or not the apply button is used by the options.
  /// \return
  ///   True if the apply button is used by the options.
  virtual bool isApplyUsed() const {return false; }

  /// Sends a signal that changes are available to apply.
  void sendChangesAvailable();

public slots:
  /// \brief
  ///   Applies changes to the options data.
  ///
  /// The apply handler is used to save the changes. Sub-classes can
  /// override this method to save the changes directly instead of
  /// using an apply handler.
  virtual void applyChanges()=0;

  /// \brief
  ///   Resets the changes to the options data.
  /// \sa pqOptionsPage::applyChanges()
  virtual void resetChanges()=0;

signals:
  /// Emitted when there are changes to be applied.
  void changesAvailable();

private:
};

#endif
