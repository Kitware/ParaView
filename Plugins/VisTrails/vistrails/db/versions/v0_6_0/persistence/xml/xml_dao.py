
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

from datetime import date, datetime
from time import strptime

class XMLDAO:
    def __init__(self):
  pass

    def hasAttribute(self, node, attr):
        return node.hasAttribute(attr)

    def getAttribute(self, node, attr):
  try:
            attribute = node.attributes.get(attr)
            if attribute is not None:
                return attribute.value
  except KeyError:
      pass
  return None

    def convertFromStr(self, value, type):
  if value is not None:
      if type == 'str':
    return str(value)
      elif value.strip() != '':
    if type == 'long':
        return long(value)
    elif type == 'float':
        return float(value)
    elif type == 'int':
        return int(value)
    elif type == 'date':
        return date(*strptime(value, '%Y-%m-%d')[0:3])
    elif type == 'datetime':
        return datetime(*strptime(value, '%Y-%m-%d %H:%M:%S')[0:6])
  return None

    def convertToStr(self, value, type):
  if value is not None:
      if type == 'date':
    return value.isoformat()
      elif type == 'datetime':
    return value.strftime('%Y-%m-%d %H:%M:%S')
      else:
    return str(value)
  return ''
