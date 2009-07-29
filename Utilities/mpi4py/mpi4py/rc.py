# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com
"""
Runtime configuration parameters.
"""

initialize = True
"""
Automatic MPI initialization at import time

* Any of ``{True  | 1 | "yes" }``: initialize MPI at import time
* Any of ``{False | 0 | "no"  }``: do not initialize MPI at import time
"""


threaded = True
"""
Request for thread support at MPI initialization

* Any of ``{True  | 1 | "yes" }``: initialize MPI with ``MPI_Init_thread()``
* Any of ``{False | 0 | "no"  }``: initialize MPI with ``MPI_Init()``
"""


thread_level = "multiple"
"""
Level of thread support to request at MPI initialization

* ``"single"``     : use ``MPI_THREAD_SINGLE``
* ``"funneled"``   : use ``MPI_THREAD_FUNNELED``
* ``"serialized"`` : use ``MPI_THREAD_SERIALIZED``
* ``"multiple"``   : use ``MPI_THREAD_MULTIPLE``
"""


finalize = True
"""
Automatic MPI finalization at exit time

* Any of ``{True  | 1 | "yes" }``: call ``MPI_Finalize()`` at exit time
* Any of ``{False | 0 | "no"  }``: do not call ``MPI_Finalize()`` at exit time
"""
