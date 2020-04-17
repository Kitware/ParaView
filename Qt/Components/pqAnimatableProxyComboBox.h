/*=========================================================================

   Program: ParaView
   Module:    pqAnimatableProxyComboBox.h

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
#ifndef pqAnimatableProxyComboBox_h
#define pqAnimatableProxyComboBox_h

#include "pqComponentsModule.h"
#include "pqSMProxy.h"
#include <QComboBox>

class vtkSMProxy;
class pqPipelineSource;
class pqServerManagerModelItem;

/**
* pqAnimatableProxyComboBox is a combo box that can list the animatable
* proxies.  All pqPipelineSources are automatically in this list
* Any other proxies must be manually added.
*/
class PQCOMPONENTS_EXPORT pqAnimatableProxyComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqAnimatableProxyComboBox(QWidget* parent = 0);
  ~pqAnimatableProxyComboBox() override;

  /**
  * Returns the current source
  */
  vtkSMProxy* getCurrentProxy() const;

  void addProxy(int index, const QString& label, vtkSMProxy*);
  void removeProxy(const QString& label);
  int findProxy(vtkSMProxy*);

protected Q_SLOTS:
  void onSourceAdded(pqPipelineSource* src);
  void onSourceRemoved(pqPipelineSource* src);
  void onNameChanged(pqServerManagerModelItem* src);
  void onCurrentSourceChanged(int idx);

Q_SIGNALS:
  void currentProxyChanged(vtkSMProxy*);

private:
  Q_DISABLE_COPY(pqAnimatableProxyComboBox)
};

#endif
