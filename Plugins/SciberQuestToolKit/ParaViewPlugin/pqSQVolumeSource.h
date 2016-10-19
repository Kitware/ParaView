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
#ifndef pqSQVolumeSource_h
#define pqSQVolumeSource_h

#include "pqNamedObjectPanel.h"

#include "ui_pqSQVolumeSourceForm.h"
using Ui::pqSQVolumeSourceForm;

class pqProxy;
class pqPropertyLinks;
class QWidget;

class pqSQVolumeSource : public pqNamedObjectPanel
{
  Q_OBJECT
public:
  pqSQVolumeSource(pqProxy* proxy, QWidget* p = NULL);
  ~pqSQVolumeSource();

  // Description:
  // Set/Get values to/from the UI.
  void GetOrigin(double* o);
  void SetOrigin(double* o);

  void GetPoint1(double* p1);
  void SetPoint1(double* p1);

  void GetPoint2(double* p2);
  void SetPoint2(double* p2);

  void GetPoint3(double* p3);
  void SetPoint3(double* p3);

  void GetResolution(int* res);
  void SetResolution(int* res);

  void GetSpacing(double* dx);
  void SetSpacing(double* dx);

  // Description:
  // dispatch context menu events.
  void contextMenuEvent(QContextMenuEvent* event);

protected slots:
  // Description:
  // Transfer configuration to and from the clip board.
  void CopyConfiguration();
  void PasteConfiguration();

  // Description:
  // read/write configuration from disk.
  void loadConfiguration();
  void saveConfiguration();

  // Description:
  // check if cooridnates produce a good volume.
  int ValidateCoordinates();

  // Description:
  // calculate plane's dimensions for display. Retun 0 in case
  // an error occured.
  void DimensionsModified();

  // Description:
  // update and display computed values, and enforce aspect ratio lock.
  void SpacingModified();
  void ResolutionModified();

  // Description:
  // Update the UI with values from the server.
  void PullServerConfig();
  void PushServerConfig();

  // Description:
  // This is where we have to communicate our state to the server.
  virtual void accept();

  // Description:
  // Pull config from proxy
  virtual void reset();

private:
  double Dims[3];
  double Dx[3];
  int Nx[3];

private:
  pqSQVolumeSourceForm* Form;
  pqPropertyLinks* Links;
};

#endif
