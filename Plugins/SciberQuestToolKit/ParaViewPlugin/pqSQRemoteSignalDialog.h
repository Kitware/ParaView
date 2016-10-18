/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef pqSQRemoteSignalDialog_h
#define pqSQRemoteSignalDialog_h
// .NAME pqSQRemoteSignalDialog - Dialog for configuring server side signals
//
// .SECTION Description
//
// .SECTION See Also
//
// .SECTION Thanks

#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QString>
#include <QStringList>

class pqSQRemoteSignalDialogUI;

class pqSQRemoteSignalDialog : public QDialog
{
  Q_OBJECT

public:
  pqSQRemoteSignalDialog(QWidget* parent, Qt::WindowFlags f);
  ~pqSQRemoteSignalDialog();

  // Description:
  // Access to UI state
  void SetTrapFPEDivByZero(int enable);
  int GetTrapFPEDivByZero();

  void SetTrapFPEInexact(int enable);
  int GetTrapFPEInexact();

  void SetTrapFPEInvalid(int enable);
  int GetTrapFPEInvalid();

  void SetTrapFPEOverflow(int enable);
  int GetTrapFPEOverflow();

  void SetTrapFPEUnderflow(int enable);
  int GetTrapFPEUnderflow();

  int GetModified() { return this->Modified; }

private slots:
  void SetModified() { ++this->Modified; }

private:
  int Modified;
  pqSQRemoteSignalDialogUI* Ui;
};

#endif
