import os

if os.name == 'posix':
	from libvtkPVServerManagerPython import *
	from libvtkPVServerCommonPython import *
else:
	from vtkPVServerManagerPython import *
	from vtkPVServerCommonPython import *

