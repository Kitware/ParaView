/*=========================================================================

   Program: ParaView
   Module:    pqSelectionLinkDialog.h

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

========================================================================*/
#ifndef pqSelectionLinkDialog_h
#define pqSelectionLinkDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

/**
* Dialog used to ask the user for selection link options.
*/
class PQCOMPONENTS_EXPORT pqSelectionLinkDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqSelectionLinkDialog(QWidget* parent, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqSelectionLinkDialog() override;

  /**
  * Specify if the convert to indices flag should be set to on
  * Default behaviour is on. It can sometimes be useful to disable it,
  * eg. A frustum selection over multiple datasets.
  */
  void setEnableConvertToIndices(bool enable);

  /**
  * Returns if the user requested to convert to indices.
  */
  bool convertToIndices() const;

private:
  Q_DISABLE_COPY(pqSelectionLinkDialog)

  class pqInternal;
  pqInternal* Internal;
};

#endif
