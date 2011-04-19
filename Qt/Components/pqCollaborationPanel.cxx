/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationPanel.cxx

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
#include "pqCollaborationPanel.h"

// Qt Includes.
#include <QTextDocument>
#include <QTextCursor>
#include <QColor>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>

//// ParaView Includes.
#include "pqActiveObjects.h"
#include "pqServer.h"
#include "vtkSMMessage.h"

//#include "pqApplicationCore.h"
//#include "pqSignalAdaptors.h"
//#include "pqSMProxy.h"
//#include "pqView.h"

//-----------------------------------------------------------------------------
#include "ui_pqCollaborationPanel.h"
class pqCollaborationPanel::pqInternal : public Ui::pqCollaborationPanel
{
public:
  pqInternal()
    {
    this->userID = 0;
    this->userPalette.append("#CC1010");
    this->userPalette.append("#10CC10");
    this->userPalette.append("#1010CC");
    this->userPalette.append("#AABBCC");
    this->userPalette.append("#FF00FF");
    this->userPalette.append("#AACCAA");
    }

  void validateEntry()
    {
    this->addEntry(this->userID++, this->message->text());
    this->message->clear();
    }

  void addEntry(int userId, const QString& txt)
    {
    QString message = QString("<table bgcolor='%2' width='100\%'><tr><td>%1</td></tr></table>").arg(txt,this->userPalette.at(userId%this->userPalette.size()));
    this->content->textCursor().atEnd();
    this->content->insertHtml(message);
    this->content->textCursor().atEnd();
    this->content->ensureCursorVisible();
    }

  int userID;
  QList<QString> userPalette;
  QPointer<pqServer> server;
};
//-----------------------------------------------------------------------------
pqCollaborationPanel::pqCollaborationPanel(QWidget* p):Superclass(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);

  QObject::connect( &pqActiveObjects::instance(),
                    SIGNAL(serverChanged(pqServer*)),
                    this, SLOT(setServer(pqServer*)));

  QObject::connect( this->Internal->message, SIGNAL(returnPressed()),
                    this, SLOT(onUserMessageAvailable()));
}

//-----------------------------------------------------------------------------
pqCollaborationPanel::~pqCollaborationPanel()
{
  if(this->Internal->server)
    {
    QObject::disconnect(this->Internal->server, SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                        this, SLOT(onChatMessage(vtkSMMessage*)));
    }
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserMessageAvailable()
  {
  if(!this->Internal->server.isNull())
    {
    vtkSMMessage msg;
    msg.SetExtension(ChatMessage::txt, this->Internal->message->text().toStdString());
    msg.SetExtension(ChatMessage::user, this->Internal->userID);
    this->Internal->server->sendToOtherClients(&msg);
    }
  this->Internal->validateEntry();
  }
//-----------------------------------------------------------------------------
void pqCollaborationPanel::onChatMessage(vtkSMMessage* msg)
{
  if(msg->HasExtension(ChatMessage::txt) && msg->HasExtension(ChatMessage::user))
    {
    this->Internal->addEntry( msg->GetExtension(ChatMessage::user),
                              msg->GetExtension(ChatMessage::txt).c_str());
    }
}

//-----------------------------------------------------------------------------
// Set the active server. We need the active server only to determine if this is
// a multiprocess server connection or not.
void pqCollaborationPanel::setServer(pqServer* server)
{
  if(this->Internal->server)
    {
    QObject::disconnect(this->Internal->server, SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                        this, SLOT(onChatMessage(vtkSMMessage*)));
    }
  this->Internal->server = server;
  if(server)
    {
    QObject::connect(server, SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                     this, SLOT(onChatMessage(vtkSMMessage*)));
    }
}
