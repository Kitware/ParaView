
//--------------------------------------------------------------------------
//
// This file is part of the Vistrails ParaView Plugin.
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of
// this file.  Please review the following to ensure GNU General Public
// Licensing requirements will be met:
// http://www.opensource.org/licenses/gpl-2.0.php
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// Copyright (C) 2009 VisTrails, Inc. All rights reserved.
//
//--------------------------------------------------------------------------

#include "PluginMain.h"
#include "ToolBarStub.h"
#include "ResourceData.h"


// When the plugin gets loaded, ParaView instantiates one of these
// objects - we just create a small toolbar with a vistrails icon.
ToolBarStub::ToolBarStub(QObject* p) : QActionGroup(p)
{
  qRegisterResourceData(0x01,
    (unsigned char*)qt_resource_struct,
    (unsigned char*)qt_resource_name,
    (unsigned char*)qt_resource_data);

  QIcon icon(":/images/logo.png");

    QAction *a = new QAction(icon, "VisTrails", this);
    this->addAction(a);
}
