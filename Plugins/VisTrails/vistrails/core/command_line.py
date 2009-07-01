
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
"""Very thin convenience wrapper around optparse.OptionParser."""

import optparse

class CommandLineParserSingleton(object):
    """CommandLineParser is a very thin wrapper around
    optparse.OptionParser to make easier the parsing of command line
    parameters."""
    def __call__(self):
        return self
    
    def __init__(self):
        self.parser = optparse.OptionParser()
        self.options_were_read = False
        self.args = []

    def init_options(self):
        """self.init_options() -> None. Initialize option dictionary,
        by parsing command line arguments according to the options set
        by previous add_option calls.

        Few programs should call this. Call self.parse_options() unless
        you know what you're doing."""
        (self.options, self.args) = self.parser.parse_args()
        self.options_were_read = True

    def add_option(self, *args, **kwargs):
        """self.add_option(*args, **kwargs) -> None. Adds a new option
        to the command line parser. Behaves identically to the
        optparse.OptionParser.add_option."""
        self.parser.add_option(*args, **kwargs)

    def get_option(self, key):
        """self.get_option(key) -> value. Returns a value corresponding
        to the given key that was parsed from the command line. Throws
        AttributeError if key is not present."""
        self.parse_options()
        return getattr(self.options, key)

    def parse_options(self):
        """self.parse_options() -> None. Parse command line arguments,
        according to the options set by previous add_option calls."""
        if not self.options_were_read:
            self.init_options()

    def get_arg(self,number):
        """self.get_arg(number) -> value. Returns the value corresponding
        to the argument at position number from the command line. Returns 
        None if number is greater or equal the number of arguments. """
        if len(self.args) > number:
            return self.args[number]
        else:
            return None 

    def positional_arguments(self):
        """positional_arguments() -> [string]. Returns a list of strings
        representing the positional arguments in the command line."""
        return self.args

# singleton trick
CommandLineParser = CommandLineParserSingleton()
