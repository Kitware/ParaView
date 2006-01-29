# module for kwwidgets

import os

if os.name == 'posix':
    from libKWWidgetsPython import *
else:
    from KWWidgetsPython import *



