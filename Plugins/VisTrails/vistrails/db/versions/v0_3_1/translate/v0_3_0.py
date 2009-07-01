
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

import copy

def translateVistrail(_vistrail):
    # FIXME should this be a deepcopy?
    vistrail = copy.deepcopy(_vistrail)
    for action in vistrail.db_get_actions():
#        print 'translating action %s' % action.db_time
        if action.db_what == 'addModule':
            if action.db_datas[0].db_cache == 0:
                action.db_datas[0].db_cache = 1
    vistrail.db_version = '0.3.1'
    return vistrail
