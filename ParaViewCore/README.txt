/*=========================================================================

  Program:   ParaView
  Module:    ParaViewCore

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

The ParaViewCore directory aims to gather all the core components of ParaView.
Those components are divided in several groups.

--------------------
Common
--------------------

The Common directory represent the basic package that provide classes that
may be used by any other package. It provides classes for XML management,
and ParaView specific command line arguments, test utility and interpreter
initializer.
Common should contain the very core classes such as the Interpreter and
related classes that are needed by VTKExtensions. These have nothing to do
with ParaView or ParaView client-server per-say. The only things that this kit
can depend on are VTK and the Interpreter.

--------------------
ClientServerCore
--------------------

The ClientServerCore directory provides all the VTK classes that build
and define all the infrastructure of ParaView that will be used on the
client and server side. In this package the abstract session is defined
as well as some utility networking and plugin classes. It also contains
a set of specific representation that are used on both the client and server
side.
ClientServerCore contains classes essential for client-server mechanisms in
ParaView. This also includes additional extensions to VTK that depend on
ProcessModule or sessions.

--------------------
ServerManager
--------------------

The ServerManager directory provide all the Proxy abstraction that reside
on the client side. Proxies provide a unique API to manipulate and handle
objects that reside on the processing side which can either be local or
remote. The ServerManager classes should be prefixed by vtkSM.
The main class of that package is the vtkSMProxyManager that allow the user
to create and retreive proxies.

--------------------
ServerImplementation
--------------------

The ServerImplementation directory contain the code that will call the real
method of the underneath VTK objects. The classes in that directory should be
prefixed by vtkSI. A simple way to explain that package is to see it as a
mirror of the ServerManager one with its vtkSM class but this time on the side
where objects are really computing something. Those objects are not
necesseraly on the server side, just where the processing should occur.

--------------------
VTKExtensions
--------------------

The VTKExtensions directory provide VTK classes that may move to VTK at some
points. VTKExtensions should contain classes that extend VTK. This can only
depend on VTK and Common.

--------------------
Testing
--------------------

Allow to gather testing sub-directory of each module inside a simplified
CMakeList.txt file.
