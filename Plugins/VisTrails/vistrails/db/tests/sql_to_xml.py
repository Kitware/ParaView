
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
if __name__ != '__main__':
    import tests
    raise tests.NotModule('This should not be imported as a module')
    
import os
os.environ['EXECUTABLEPATH'] = '/vistrails/VisTrails.app/Contents/MacOS'

import MySQLdb

from db.services import io

def convert_sql_to_xml(filename, id):
    config = {'host': 'localhost', 
              'port': 3306,
              'user': 'vistrails',
              'passwd': 'vistrailspwd',
              'db': 'vistrails'}
    try:
        dbConnection = io.open_db_connection(config)        
        vistrail = io.open_vistrail_from_db(dbConnection, id)
        io.setDBParameters(vistrail, config)
        io.save_vistrail_to_xml(vistrail, filename)
        io.close_db_connection(dbConnection)
    except MySQLdb.Error, e:
        print e

if __name__ == '__main__':
    convert_sql_to_xml('/vistrails/vtk_http_from_db.xml', 1)
    # convert_sql_to_xml('/vistrails/head_from_db.xml', 1)
    # convert_sql_to_xml('/vistrails/lung_from_db.xml', 2)
