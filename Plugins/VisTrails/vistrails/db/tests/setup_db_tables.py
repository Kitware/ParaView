
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
##
############################################################################

# MACOSX binary install stuff
import os
os.environ['EXECUTABLEPATH'] = '/vistrails/VisTrails.app/Contents/MacOS'
import sys
sys.path.append("../..")
from db.services import io

def setup_tables(host, port, user, passwd, db):
    config = {'host': host, 
              'port': port,
              'user': user,
              'passwd': passwd,
              'db': db}

    try:
        db_connection = io.open_db_connection(config)
        io.setup_db_tables(db_connection)
    except Exception, e:
        print e

if __name__ == '__main__':
    args = sys.argv
    if len(args) > 4:
        host = str(args[1])
        port = 3306
        user = str(args[2])
        passwd = str(args[3])
        db = str(args[4])
        setup_tables(host, port, user, passwd, db)
    else:
        print "Usage: %s host user passwd db" % args[0]
