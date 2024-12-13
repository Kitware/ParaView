## Refactoring of simple.py

The paraview.simple is the core of ParaView for creating [trame](https://kitware.github.io/trame/) applications, catalyst scripts, Python states and traces.

It was time to rework its structure to promote clarity and enable a deprecation path while empowering developers to create new and better helpers.

The new structure convert the single simple.py file into a package split up with common responsability and where deprecation methods get centrelized within their own file.

During that restructuration, we are also exposing a new set of methods on proxy directly to streamline their usage. A blog post will capture the add-on with some concreate example for Catalyst and Trame.
