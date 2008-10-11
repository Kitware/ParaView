/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientRecordDisplay.h"
#include "ui_ClientRecordDisplay.h"

#include <pqApplicationCore.h>
#include <pqComboBoxDomain.h>
#include <pqNamedWidgets.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqPropertyHelper.h>
#include <pqPropertyLinks.h>
#include <pqPropertyManager.h>
#include <pqServerManagerModel.h>
#include <pqSignalAdaptors.h>

#include <vtkDataSetAttributes.h>
#include <vtkGraph.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

class ClientRecordDisplay::implementation
{
public:
  implementation()
  {
  }

  ~implementation()
  {
    delete this->PropertyManager;
  }

  Ui::ClientRecordDisplay Widgets;

  pqPropertyManager* PropertyManager;
  pqPropertyLinks Links;
};

ClientRecordDisplay::ClientRecordDisplay(pqRepresentation* representation, QWidget* p) :
  pqDisplayPanel(representation, p),
  Implementation(new implementation())
{
  this->Implementation->PropertyManager = new pqPropertyManager(this);

  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(representation->getProxy());

  this->Implementation->Widgets.setupUi(this);

  pqSignalAdaptorComboBox* attributeTypeAdaptor = 
    new pqSignalAdaptorComboBox(this->Implementation->Widgets.attribute_mode);
  attributeTypeAdaptor->setObjectName("ComboBoxAdaptor");

  this->Implementation->Links.addPropertyLink(
    attributeTypeAdaptor,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy,
    proxy->GetProperty("AttributeType"),0);

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));

  pqNamedWidgets::linkObject(this->Implementation->Widgets.attribute_mode, proxy, "AttributeType", this->Implementation->PropertyManager);

}

ClientRecordDisplay::~ClientRecordDisplay()
{
  delete this->Implementation;
}
