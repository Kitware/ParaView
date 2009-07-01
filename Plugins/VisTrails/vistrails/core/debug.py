
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
import logging
import sys
import inspect
from core.utils import VersionTooLow
import code
import threading
from core import system
import time
from PyQt4 import QtCore

################################################################################

class DebugPrintSingleton(QtCore.QObject):
    """ Class to be used for debugging information.

    Verboseness can be set in the following way:
        - DebugPrint.Critical
            Only critical messages will be shown
        - DebugPrint.Warning
            Warning and critical messages will be shown
        - DebugPrint.Log
            Information, warning and Critical messages will be shown
            
    As it uses information such as file name and line number, it should not be
    used interactively. Also, it goes up only one level in the traceback stack,
    so it will only get information of who called the DebugPrint functions. 

    Example of usage:
        >>> DebugPrint.set_message_level(DebugPrint.Warning)
        >>> DebugPrint.warning('This is a warning message') #message that will be shown
        >>> DebugPrint.log('This is a log message and it will not be shown') #only warnings and above are shown
        
    """
    (Critical, Warning, Log) = (logging.CRITICAL,
        logging.WARNING,
        logging.INFO) #python logging levels
    #Singleton technique
    def __call__(self):
  return self

    def make_logger_235(self, f):
        """self.make_logger_235(file) -> logger. Creates a logging object to
        be used within the DebugPrint class that sends the debugging
        output to file."""
        logger = logging.getLogger("VisLog")
        hdlr = logging.StreamHandler(f)
        formatter = logging.Formatter('VisTrails %(levelname)s %(message)s')
        hdlr.setFormatter(formatter)
        logger.addHandler(hdlr)
        logger.setLevel(logging.CRITICAL)
        return logger

    def make_logger_240(self, f):
        """self.make_logger_240(file) -> logger. Creates a logging object to
        be used within the DebugPrint class that sends the debugging
        output to file."""
        #setting basic configuration
        logging.basicConfig(level=logging.CRITICAL,
                            format='VisTrails %(levelname)s: %(message)s',
                            stream=f)
        return logging.getLogger("VisLog")

    if system.python_version() >= (2,4,0,'',0):
        make_logger = make_logger_240
    elif system.python_version() >= (2,3,5,'',0):
        make_logger = make_logger_235
    else:
        raise VersionTooLow('Python', '2.3.5')
                
    def __init__(self):
        QtCore.QObject.__init__(self)
        self.logger = self.make_logger(sys.stderr)
        self.level = logging.CRITICAL

    def redirect_to_file(self, f):
        """self.redirect_to_file(file) -> None. Redirects debugging
        output to file."""
        self.logger = self.make_logger(f)
        self.logger.set_message_level(self.level)
            
    def set_message_level(self,level):
        """self.set_message_level(level) -> None. Sets the logging
        verboseness.  level must be one of (DebugPrint.Critical,
        DebugPrint.Warning, DebugPrint.Log)."""
        self.level = logging.CRITICAL
  self.logger.setLevel(level)
        
    def message(self, caller, msg):
        """self.message(caller, msg) -> str. Returns a string with a
        formatted message to be send to the debugging output. This
        should not be called explicitly from userland. Consider using
        self.log(), self.warning() or self.critical() instead."""
        source = inspect.getsourcefile(caller)
        line = caller.f_lineno
        if source and line:
            return "File '" + source + "' at line " + str(line) + ": " + msg
        else:
            return "(File info not available): " + msg
        
    def log(self,msg):
        """self.log(str) -> None. Send information message (low
        importance) to log with appropriate call site information."""
  caller = inspect.currentframe().f_back # who called us?
  self.logger.info(self.message(caller, msg))
        
    def warning(self,msg):
        """self.warning(str) -> None. Send warning message (medium
        importance) to log with appropriate call site information."""
  caller = inspect.currentframe().f_back # who called us?
  self.logger.warning(self.message(caller, msg))
        
    def critical(self,msg):
        """self.critical(str) -> None. Send critical message (high
        importance) to log with appropriate call site information."""
  caller = inspect.currentframe().f_back # who called us?
  self.logger.critical(self.message(caller, msg))

    def watch_signal(self, obj, sig):
        """self.watch_signal(QObject, QSignal) -> None. Connects a debugging
        call to a signal so that every time signal is emitted, it gets
        registered on the log."""
        self.connect(obj, sig, self.__debugSignal)

    def __debugSignal(self, *args):
        self.critical(str(args))

DebugPrint = DebugPrintSingleton()

critical = DebugPrint().critical
warning  = DebugPrint().warning
log      = DebugPrint().log

################################################################################

def timecall(method):
    """timecall is a method decorator that wraps any call in timing calls
    so we get the total time taken by a function call as a debugging message."""
    def call(self, *args, **kwargs):
        caller = inspect.currentframe().f_back
        start = time.time()
        method(self, *args, **kwargs)
        end = time.time()
        DebugPrint.logger.critical(DebugPrint.message(caller, "time: %.5s" % (end-start)))
    call.__doc__ = method.__doc__
    return call

################################################################################

def object_at(desc):
    """object_at(id) -> object

    id is an int returning from id() or a hex string of id()

    Fetches all live objects, finds the one with given id, and returns
    it.  Warning: THIS IS FOR DEBUGGING ONLY. IT IS SLOW."""
    if type(desc) == int:
        target_id = desc
    elif type(desc) == str:
        target_id = int(desc, 16) # Reads desc as the hex address
    import gc
    for obj in gc.get_objects():
        if id(obj) == target_id:
            return obj
    raise Exception("Couldn't find object")
