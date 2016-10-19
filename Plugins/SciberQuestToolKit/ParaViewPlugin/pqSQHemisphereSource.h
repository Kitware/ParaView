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
#ifndef pqSQHemisphereSource_h
#define pqSQHemisphereSource_h

#include "pqNamedObjectPanel.h"
#include "ui_pqSQHemisphereSourceForm.h" // for ui

using Ui::pqSQHemisphereSourceForm;

class pqProxy;
class pqPropertyLinks;
class vtkEventQtSlotConnect;
class QWidget;

class pqSQHemisphereSource : public pqNamedObjectPanel
{
  Q_OBJECT
public:
  pqSQHemisphereSource(pqProxy* proxy, QWidget* p = NULL);
  ~pqSQHemisphereSource();

protected slots:
  // Description:
  // read state from disk.
  void Restore();
  void loadConfiguration();
  // Description:
  // write state to disk.
  void Save();
  void saveConfiguration();

  // Description:
  // Update the UI with values from the server.
  void PullServerConfig();
  void PushServerConfig();

  // Description:
  // This is where we have to communicate our state to the server.
  void accept();

  // Description:
  // UI driven reset of widget to current server manager values.
  void reset();

private:
  pqSQHemisphereSourceForm* Form;
  pqPropertyLinks* Links;
};

#endif
