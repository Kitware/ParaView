
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

from db import VistrailsDBException
from db.versions.v0_9_1.domain import DBAdd, DBAnnotation, DBChange, DBDelete

def translateVistrail(vistrail):
    id_remap = {}
    for action in vistrail.db_get_actions():
        for annotation in action.db_get_annotations():
            if annotation.db_key == 'notes':
                del action.db_annotations_key_index['notes']
                annotation.db_key = '__notes__'
                action.db_annotations_key_index['__notes__'] = annotation

    vistrail.db_version = '0.9.2'
    return vistrail
