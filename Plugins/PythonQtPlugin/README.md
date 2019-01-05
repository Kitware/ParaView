ParaView PythonQt Plugin
------------------------

The PythonQt plugin makes the Qt API available to users from the ParaView
Python console.  The plugin uses the PythonQt library.  The PythonQt homepage
is pythonqt.sourceforge.net

Note, PythonQt is different from PyQt4, it is a separate project and uses a
different technique to generate Qt bindings.


Build Instructions
------------------

To satisfy the PythonQt dependency, first build and install the PythonQt
library and headers.  I recommend using the PythonQt fork maintained by
the commontk project.  Commits from the commontk fork are migrated upstream
to PythonQt on sourceforge.  To clone PythonQt using git:

    git clone https://github.com/commontk/PythonQt

Create a build directory and then configure with CMake:

    mkdir build
    cd build
    cmake ../PythonQt

When configuring PythonQt, my recommended settings are:

    BUILD_SHARED_LIBS=ON
    PythonQt_Wrap_Qtcore=ON
    PythonQt_Wrap_Qtgui=ON
    PythonQt_Wrap_Qtuitools=ON

When configuring this PythonQt plugin for ParaView, CMake will look for the
installed PythonQt library and headers.  Set PYTHONQT_LIBRARY to the installed
library, and set PYTHONQT_INCLUDE_DIR to the installed header directory that
contains PythonQt.h.


Testing the Plugin
------------------

After loading the PythonQt plugin in ParaView, you can try out the functions
defined by samples/demo.py.


Wrapping the ParaView Qt Interface
----------------------------------

This plugin contains a file named wrapped_methods.txt which lists some ParaView
classes and functions that should be wrapped for PythonQt.  Using the wrapping it
is possible for Python to access classes in ParaView that are derived from QObjects,
the "pq" classes.  The python script named WrapPythonQt.py reads the methods files
and generates a "decorator" file.  See the PythonQt documentation for more details
on the "decorator" concept.  WrapPythonQt.py is invoked at build time to generate
a file named pqPluginDecorators.h in the build directory.  You should read the
contents of pqPluginDecorators.h to see what decorators look like.  WrapPythonQt.py
has support for wrapping class methods, static class methods, and class constructors
and destructors, but it's not a real C++ parser, so it might not work for all
method signatures that you want to wrap.  You could write your own decorator by hand
to handle these cases, or modify the target object to make the method a Qt slot
function or Qt invokable.

Note that using PythonQt you can easily can wrap method signatures that use both
VTK objects and QObjects as arguments and/or return values.  This plugin uses a
class named pqPythonQtWrapperFactory to translate between VTK objects and Python
objects using VTK's standard Python wrapping, making PythonQt wrapped methods
fully compatible with Python wrapped VTK modules.
