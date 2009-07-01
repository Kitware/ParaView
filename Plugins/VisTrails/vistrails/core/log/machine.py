
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

from db.domain import DBMachine

class Machine(DBMachine):
    """ Class that stores info for logging a module execution. """

    def __init__(self, *args, **kwargs):
        DBMachine.__init__(self, *args, **kwargs)

    def __copy__(self):
        return self.do_copy()

    def do_copy(self):
        cp = DBMachine.__copy__(self)
        cp.__class__ = Machine
        return cp

    ##########################################################################
    # Properties

    def _get_id(self):
        return self.db_id
    def _set_id(self, id):
        self.db_id = id
    id = property(_get_id, _set_id)

    def _get_name(self):
        return self.db_name
    def _set_name(self, name):
        self.db_name = name
    name = property(_get_name, _set_name)

    def _get_os(self):
        return self.db_os
    def _set_os(self, os):
        self.db_os = os
    os = property(_get_os, _set_os)

    def _get_architecture(self):
        return self.db_architecture
    def _set_architecture(self, architecture):
        self.db_architecture = architecture
    architecture = property(_get_architecture, _set_architecture)

    def _get_processor(self):
        return self.db_processor
    def _set_processor(self, processor):
        self.db_processor = processor
    processor = property(_get_processor, _set_processor)

    def _get_ram(self):
        return self.db_ram
    def _set_ram(self, ram):
        self.db_ram = ram
    ram = property(_get_ram, _set_ram)

    ##########################################################################
    # Properties

    def equals_no_id(self, other):
        return (self.name == other.name and
                self.os == other.os and
                self.architecture == other.architecture and
                self.processor == other.processor and
                self.ram == other.ram)
