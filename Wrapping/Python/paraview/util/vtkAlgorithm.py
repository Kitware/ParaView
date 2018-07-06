r"""
This module provides utility functions that can be used to expose a
`VTKPythonAlgorithmBase` derived Pythonic vtkAlgorithm subclass in ParaView.

Introduction
============

VTK provides a convenient Python-friendly mechanism to add new algorithms, such as data sources,
filters, readers, and writers via `vtkPythonAlgorithm`. In this approach, developers can
write subclasses of `VTKPythonAlgorithmBase` and then implement the
`vtkAlgorithm` API in Python.

.. code-block:: python

    from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
    class ContourShrink(VTKPythonAlgorithmBase):
      def __init__(self):
          VTKPythonAlgorithmBase.__init__(self)

      def RequestData(self, request, inInfo, outInfo):
          inp = vtk.vtkDataSet.GetData(inInfo[0])
          opt = vtk.vtkPolyData.GetData(outInfo)

          cf = vtk.vtkContourFilter()
          cf.SetInputData(inp)
          cf.SetValue(0, 200)

          sf = vtk.vtkShrinkPolyData()
          sf.SetInputConnection(cf.GetOutputPort())
          sf.Update()

          opt.ShallowCopy(sf.GetOutput())
          return 1


Such `VTKPythonAlgorithmBase`-based algorithms act as any other `vtkAlgorithm`
and can be used directly in VTK pipelines. To use them in ParaView, however, one
has to make ParaView aware of the capabilities of the algorithm. For
`vtkAlgorithm` subclasses in C++, this is done by writing an XML description of
the algorithm. For Python-based algorithms, this module provides us a mechanism
to build the ParaView-specific XML via decorators.

.. code-block:: python

    from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
    from paraview.util.vtkAlgorithm import smproxy, smproperty, smdomain

    @smproxy.filter(label="Contour Shrink")
    @smproperty.input(name="Input")
    @smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
    class ContourShrink(VTKPythonAlgorithmBase):
      def __init__(self):
          VTKPythonAlgorithmBase.__init__(self)

      def RequestData(self, request, inInfo, outInfo):
          ...
          return 1


.. note::

    ParaView's XML description provides a wide gamut of features.
    The decorators available currently only expose a small subset of them. The
    plan is to have complete feature parity in the future. If you encounter a
    missing capability, please raise an issue or better yet, a merge-request to address
    the same.


Decorator Basics
================

There are four main classes currently available: `smproxy`,
`smproperty`, `smdomain`, and `smhint`. Each provides a set of decorators to
declare a proxy, property, domain or a hint, respectively.

All available decorators exhibit the following characteristics:

#. With the exception of decorators defined in `smproxy`, all decorators are
   intended for decorating classes or their methods. `smproxy` decorators are
   only intended for classes.
#. Decorators take keyword arguments (positional arguments are discouraged).
   While there may be exceptions, the general pattern is that a decorator
   corresponds to an XML node in the ParaView XML configuration. Any attributes
   on the XML node itself can be specified as keyword arguments to the
   decorator. The decorator may explicitly define parameters that affect the
   generated node or to explicit limit the arguments provided.
#. Decorators can be chained. The order of chaining is significant and follows
   the direction of nesting in the XML e.g. to add a domain to a property, one
   would nest decorators as follows:

.. code-block:: python

    @smproperty.intvector(name="Radius", default_values=[0])
    @smdomain.intrange(min=0, max=10)
    @smhint.xml("<SomeHint/>")
    def SetRadius(self, radius):
        ...


   Note, since domains can't have nested hints, the hint specified after the
   `smdomain.intrange` invocation is intended for the `smproperty` and not the
   `smdomain`.

   When chaining `smproperty` decorators under a `smproxy`, the order in which
   the properties are shown in ParaView UI is reverse of the order in the
   Python code e.g. the in following snippets the UI will show "PolyDataInput"
   above "TableInput".

.. code-block:: python

    @smproxy.filter(...)
    @smproperty.input(name="TableInput", port_index=1, ...)
    @smdomain.datatype(dataTypes=["vtkTable"])
    @smproperty.input(name="PolyDataInput", port_index=0, ...)
    @smdomain.datatype(dataTypes=["vtkPolyData"])
    class MyFilter(VTKPythonAlgorithmBase):
       ...


`smproxy` Decorators
====================

To make `VTKPythonAlgorithmBase` subclass available in ParaView as a data-source,
filter, reader, or writer, one must use of the decorators in the `smproxy`
class.

Common decorator parameters
---------------------------

The following keyword parameters are supported by all `smproxy` decorators:

#. `name`: if present, provides the name to use of the Proxy. This is the name
   that ParaView uses to refer to your class. If none specified, the name is
   deduced from the class name.
#. `label`: if present, provides the label to use in the UI. If missing, the
   label is deduced from the `name`.
#. `class`: if present, is the fully qualified class name ParaView should use to
   instantiate this object. Except for exceptional situation, it's best to let
   ParaView deduce this argument.
#. `group`: if present, is the proxy-group under which the definition must be
   placed. This is often not needed since the decorators set a good default
   based on the role of the proxy.

As mentioned before, any keyword arguments passed any decorator that is not
processed by the decorator simply gets added to the generated XML node as
attribute.

`smproxy.source`
---------------

This decorator can be used to declare data sources. Data sources don't have any
inputs, but provide 1 or more outputs. While technically a reader is a source,
`smproxy.reader` is preferred way of adding readers.

`smproxy.filter`
----------------

This decorator can be used to declare filters that have one or more inputs and
one or more outputs. The inputs themselves must be declared using
`smproperty.input` decorators.

`smproxy.reader`
----------------

This decorator is used to declare reader. This is same as `smproxy.source`,
except it adds the appropriate hints to make ParaView aware that this is a
reader.

It takes the following keyword parameters, in addition to the standard ones:

#. `file_description`: (required) a user friendly text to use in the **File
   Open** dialog.
#. `extensions`:  a string or list of strings which define the supported
   extensions (without the leading `.`).
#. `filename_patterns`: wild card patten.

Either `extensions` or `filename_patterns` must be provided.

`smproxy.writer`
----------------

This decorator is used to declare a writer. Similar to `smproxy.reader`, one
provides `file_description` and `extensions` keyword parameters to indicate
which file extensions the writer supports.


`smproperty` Decorators
=======================

Decorators in `smproperty` are used to add properties to the proxies declared
using `smproxy` decorators. These can be added either to class methods or to
class objects. Multiple `smproperty` decorators may be chained when adding to
class objects.

Common decorator parameters
---------------------------

The following keyword parameters are supported by all `smproperty` decorators:

#. `name`: if present, provides the name to use of the Property. If missing, the
   name will be deduced from the object being decorated which will rarely be
   correct when decorating class objects. Hence, this parameter is optional
   when decorating class methods, but required when decorating class objects.
#. `label`: if present, provides the label to use in the UI. If missing, the
   label is deduced from the `name`.
#. `command`: if present, is the method name. If missing, ParaView can deduce it
   based on the method being decorated. Hence, as before, optional when
   decorating class methods, but generally required when decorating class
   objects (with the exception of `smproperty.input`).

`smproperty.xml`
---------------

This is a catch-all decorator. One can use this to add arbitrary XML to the
proxy definition.

.. code-block:: python

    @smproperty.xml(xmlstr="<IntVectorProperty name=..> ....  </IntVectorProperty>")
    def MyMethod(self, ...):
        ...


`smproperty.intvector`, `smproperty.doublevector`, `smproperty.idtypevector`, `smproperty.stringvector`
-------------------------------------------------------------------------------------------------------

These are used to declare various types of vector properties. The arguments are
simply the XML attributes.

`smproperty.proxy`
-----------------

This decorator can be used for adding a `ProxyProperty`, i.e. a method that
takes another VTK object as argument.

`smproperty.input`
-----------------

This decorator can be used to add inputs to a `vtkAlgorithm`. Since
`vtkAlgorithm` provides the appropriate API to set an input, often one doesn't
have custom methods in Python in the `VTKPythonAlgorithmBase` subclass. Hence,
this decorator is often simply added to the class definition rather than a
particular method.

.. code-block:: python

    @smproxy.filter(...)
    @smproperty.input(name="TableInput", port_index=0, multiple_input=True)
    @smdomain.datatype(dataTypes=["vtkTable"], composite_data_supported=False)
    class MyFilter(VTKPythonAlgorithmBase):
       ...

`smproperty.dataarrayselection`
------------------------------

This a convenience decorator that can be used to decorate a method that returns
a `vtkDataArraySelection` object often used to array selection in data sources
and readers. The decorator adds appropriate ParaView ServerManager XML
components to expose the array selection to the user.


`smdomain` Decorators
=======================

These decorators are used to add domains to `smproperty`. Domains often guide
the UI that ParaView generated for the specific property. The current set of
available domains, includes `smdomain.intrange`, `smdomain.doublerange`,
`smdomain.filelist`, and `smdomain.datatype`. More will be added as needed.
Additionally, one can use the `smdomain.xml` catch-all decorator to add XML
definition for the domain.


`smhint` Decorators
====================

These add elements under `<Hints/>` for `smproxy` and/or `smproperty` elements.
Currently `smhint.filechooser` and the generic `smhint.xml` are provided with
more to be added as needed.


Examples
========

**PythonAlgorithmExamples.py** provides a working example that demonstrates how
several of the decorators described here may be used.

.. literalinclude:: ../../../../Examples/Plugins/PythonAlgorithm/PythonAlgorithmExamples.py
   :caption:

"""
from __future__ import absolute_import, print_function

from ..detail.pythonalgorithm import \
        smproxy, \
        smproperty, \
        smdomain, \
        smhint


# import VTKPythonAlgorithmBase from vtkmodules, just to make it easier for
# importers of this module.
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
