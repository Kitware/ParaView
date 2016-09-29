Python Version 2 versus 3
=========================

ParaView is working towards full Python 3 support. To that end, python code should
be written so it is compatible with Python 2.7 and Python 3.5.

Here are the changes we have needed making the transition to Python 3.

print() is a function
---------------------
::

    print "helpful message"

should be replaced by::

    print ("helpful message")

Re-directing to stderr is slightly more complicated. Python 2 did::

    print >> stderr, "Error: Failed to evaluate '%s'. " % query

To maintain python-2 compatibility, you need the future module::

    from __future__ import print_function
    ...
    print ("Error: Failed to evaluate '%s'. " % query, file=sys.stderr)

Exceptions
----------
::

    raise RuntimeError, "failed"

is replaced by::

    raise RuntimeError("failed")

Handling exceptions as objects must use the 'as' keyword::

   except AttributeError as attrErr:

Iterables
---------

To create an iterable, python 3 needs a `__next__()` method.
Assign the python2 `next()` method to it::

    __next__ = next # Python 3.X compatibility

Iterators, not lists
---------------------
Several methods now return an iterable object, not a list. Examples
include:

	| range(), map(), filter(), zip()
	| dictionary's .keys() .values() .items() methods

If you need a list, just wrap the return in a list::

	list(array_colors.keys())

Dictionaries also lost the .iterkeys(), .iteritems() and .itervalues() methods.
Just use keys to get values instead. ::

    for key, value in kwargs.iteritems():
        self.analysis[key] = value

becomes::

    for key in kwargs:
        self.analysis[key] = kwargs[key]

Submodule import
----------------
The only valid syntax in Python 3 for relative submodule import is::

    from .someModule import *

Be explicit when importing submodules whenever possible.

New modules
-----------
Any new modules or significant work on old code should add::

    from __future__ import absolute_import
    from __future__ import division
    from __future__ import print_function
    from __future__ import unicode_literals

to the top, to ensure forward compatibility with Python 3.

References
----------

https://docs.python.org/3.0/whatsnew/3.0.html

http://sebastianraschka.com/Articles/2014_python_2_3_key_diff.html
