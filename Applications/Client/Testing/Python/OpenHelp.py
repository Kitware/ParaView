#/usr/bin/env python

import QtTesting
import time

object1 = 'pqClientMainWindow/menubar/menuHelp'
QtTesting.playCommand(object1, 'activate', 'actionHelpHelp')

# since this starts a new process, we'll wait for a bit
# before finishing the test, to give the other process
# time to report an error condition
time.sleep(5)

