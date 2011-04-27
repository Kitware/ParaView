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
#include "vtkPVServerInformation.h"
#include "vtkstd/map"
#include "vtkstd/string"
#include <vtksys/ios/sstream>

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
    this->addEntry(this->userID, this->message->text());
    this->message->clear();
    }

  void addEntry(int userId, const QString& txt)
    {
    //QString message = QString("<table bgcolor='%2' width='100\%'><tr><td>%1</td></tr></table>").arg(txt,this->userPalette.at(userId%this->userPalette.size()));
    QString message = QString("<b>%1:</b> %2 <br/>\n\n").arg(this->getUserName(userId), txt.trimmed());
    this->content->textCursor().atEnd();
    this->content->insertHtml(message);
    this->content->textCursor().atEnd();
    this->content->textCursor().movePosition(QTextCursor::End);
    this->content->ensureCursorVisible();
    }

  const char* getUserName()
    {
    return this->getUserName(this->userID);
    }

  const char* getUserName(int id)
    {
    if(id == 0)
      {
      return "Nobody";
      }
    vtkstd::map<int, vtkstd::string>::iterator iter = this->userMap.find(id);
    if(iter != this->userMap.end())
      {
      return iter->second.c_str();
      }
    vtksys_ios::ostringstream name;
    name << "User " << id;
    this->userMap[id] = name.str().c_str();
    return this->userMap[id].c_str();
    }

  void updateUserName(int id, const char* name)
    {
    if(this->userMap[id] == name)
      {
      return;
      }
    this->userMap[id] = name;
    this->updateMembers();
    }

  void broadcastUserName(bool requestUpdate)
    {
    vtkSMMessage updateMsg;
    updateMsg.SetExtension(ClientInformation::req_update, requestUpdate);
    this->sendToOtherClients(&updateMsg);
    }

  void sendToOtherClients(vtkSMMessage* msg)
    {
    if(!this->server.isNull())
      {
      msg->SetExtension(ClientInformation::user, this->userID);
      msg->SetExtension(ClientInformation::name, this->getUserName());
      this->server->sendToOtherClients(msg);
      }
    }

  void updateMembers()
    {
    this->members->setRowCount(this->userMap.size());
    vtkstd::map<int, vtkstd::string>::iterator iter = this->userMap.begin();
    for(unsigned int cc = 0; iter != this->userMap.end(); iter++, cc++)
      {
      QTableWidgetItem* item = new QTableWidgetItem(iter->second.c_str());
      item->setData(Qt::UserRole, iter->first);
      if(iter->first == this->userID)
        {
        item->setFlags( item->flags() | Qt::ItemIsEditable );
        QFont font = item->font();
        font.setBold(true);
        font.setItalic(true);
        item->setFont(font);
        }
      else
        {
        item->setFlags( item->flags() & ~Qt::ItemIsEditable );
        }
      this->members->setItem(cc, 0, item);

      // Disable other column editing
      item = new QTableWidgetItem("");
      item->setFlags( item->flags() & ~Qt::ItemIsEditable );
      this->members->setItem(cc, 1, item);

      item = new QTableWidgetItem("");
      item->setFlags( item->flags() & ~Qt::ItemIsEditable );
      this->members->setItem(cc, 2, item);
      }

    this->members->horizontalHeader()->resizeSections(QHeaderView::Stretch);
    }

  int userID;
  vtkstd::map<int, vtkstd::string> userMap;
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
                    this, SLOT(onUserMessage()));

  QObject::connect(this->Internal->members, SIGNAL(itemChanged(QTableWidgetItem*)),
                   this, SLOT(itemChanged(QTableWidgetItem*)));
}

//-----------------------------------------------------------------------------
pqCollaborationPanel::~pqCollaborationPanel()
{
  if(this->Internal->server)
    {
    QObject::disconnect( this->Internal->server,
                         SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                         this, SLOT(onClientMessage(vtkSMMessage*)));
    }
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqCollaborationPanel::onUserMessage()
  {
  if(this->Internal->message->text().trimmed().length() == 0)
    {
    return;
    }

  if(!this->Internal->server.isNull())
    {
    vtkSMMessage msg;
    msg.SetExtension( ChatMessage::txt,
                      this->Internal->message->text().toStdString());
    this->Internal->sendToOtherClients(&msg);
    }
  this->Internal->validateEntry();
  }
//-----------------------------------------------------------------------------
void pqCollaborationPanel::onClientMessage(vtkSMMessage* msg)
{
  if(msg->HasExtension(ClientInformation::user))
    {
    // Handle client informations of the message
    int userId = msg->GetExtension(ClientInformation::user);
    vtkstd::string userName;
    if(msg->HasExtension(ClientInformation::name))
      {
      userName = msg->GetExtension(ClientInformation::name);
      this->Internal->updateUserName(userId, userName.c_str());
      }

    // Chat message detected... Just publish it in the UI
    if(msg->HasExtension(ChatMessage::txt))
      {
      this->Internal->addEntry( userId,
                                msg->GetExtension(ChatMessage::txt).c_str());
      }

    // Notify other client about your local name (if requested)
    if(msg->GetExtension(ClientInformation::req_update))
      {
      this->Internal->broadcastUserName(false);
      }
    }
}

//-----------------------------------------------------------------------------
// Set the active server. We need the active server only to determine if this is
// a multiprocess server connection or not.
void pqCollaborationPanel::setServer(pqServer* server)
{
  if(this->Internal->server)
    {
    QObject::disconnect( this->Internal->server,
                         SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                         this, SLOT(onClientMessage(vtkSMMessage*)));
    }
  this->Internal->server = server;
  if(server)
    {
    this->Internal->userID = server->getServerInformation()->GetClientId();
    QObject::connect( server,
                      SIGNAL(sentFromOtherClient(vtkSMMessage*)),
                      this, SLOT(onClientMessage(vtkSMMessage*)),
                      Qt::QueuedConnection);
    this->Internal->broadcastUserName(true);
    }
}
//-----------------------------------------------------------------------------
void pqCollaborationPanel::itemChanged(QTableWidgetItem* item)
  {
  if(item->column() == 0)
    {
    int id = item->data(Qt::UserRole).toInt();
    if(this->Internal->userID == id)
      {
      this->Internal->updateUserName(id, item->text().toAscii().data());
      this->Internal->broadcastUserName(true);
      }
    }
  }
