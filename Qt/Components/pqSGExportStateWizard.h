/*=========================================================================

   Program: ParaView
   Module:    pqSGExportStateWizard.h

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
#ifndef pqSGExportStateWizard_h
#define pqSGExportStateWizard_h

#include "pqComponentsModule.h"
#include <QString>
#include <QStringList>
#include <QWizard>
#include <QWizardPage>

class QLabel;
class pqImageOutputInfo;

class PQCOMPONENTS_EXPORT pqSGExportStateWizard : public QWizard
{
  Q_OBJECT
  typedef QWizard Superclass;

public:
  pqSGExportStateWizard(QWidget* parentObject = 0, Qt::WindowFlags parentFlags = 0);
  ~pqSGExportStateWizard() override;

  bool validateCurrentPage() override;

  virtual void customize() = 0;

protected slots:
  void updateAddRemoveButton();
  void onShowAllSources(bool);
  void onAdd();
  void onRemove();
  void toggleCinema(bool);

protected:
  virtual bool getCommandString(QString& command) = 0;
  QList<pqImageOutputInfo*> getImageOutputInfos();

  class pqInternals;
  pqInternals* Internals;

private:
  Q_DISABLE_COPY(pqSGExportStateWizard)

  friend class pqSGExportStateWizardPage2;
  friend class pqSGExportStateWizardPage3;
};

class pqSGExportStateWizardPage2 : public QWizardPage
{
  pqSGExportStateWizard::pqInternals* Internals;

public:
  pqSGExportStateWizardPage2(QWidget* _parent = 0);

  void initializePage() override;

  bool isComplete() const override;

  void emitCompleteChanged() { emit this->completeChanged(); }
};

class pqSGExportStateWizardPage3 : public QWizardPage
{
  pqSGExportStateWizard::pqInternals* Internals;

public:
  pqSGExportStateWizardPage3(QWidget* _parent = 0);

  void initializePage() override;
};

#endif
