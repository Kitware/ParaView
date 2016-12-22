Implementation Details: String Encoding    {#DesignStringEncoding}
=======================================

This page summarizes the design details of string encoding in ParaView, which
covers pq to SM string call and SI to VTK / VTK to SI string arguments passing.

##Motivation##

For a long time, ParaView used a simple implementation when passing string
arguments. pq Classes handles QString, which are a vory robust implementation
of string that need to be converted before being used as an actual c string.
For this conversion, ParaView used to use QString::toLatin1(), which works fine,
in order to pass the String to SM level.
Then on lower levels, all classes manipulate the same c string, without any
reencoding.

This works fine most of the time, except in some cases, when the string
contains special characters. And it can happens a lot with filenames.

They were two reasons for it not to work. First the client/server communication
is done via a library called protobuf, wich support only UTF8 string.
Second, Latin1 is a specific encoding, which can be different from the
local 8 Bits encoding of strings.

Special character are now supported in filenames in ParaView, and
one should take care of conversion when handling filenames.

##Design##

Once the problem is understood, the design is quite simple.
QString in pq classes given as an argument of non-pq classes should be encoded
to local 8 bit encoding when used locally.
QString in pq classes given as an argument of SM classes that will transfer it to the server
should be encoded to utf8.
In SI classes, string should then reencoded to local 8 bit before being given to vtk classes
SI classes recovering string from vtk classes should reencode string in utf8.

##Usage##

When developping a new feature, especially a Qt feature handling filenames, one should
use the following pattern :
 * Filename QString that need to be converted to c-string and used **locally** should be encoded
using **QString::toLocal8Bit**.
 * Filename QString that need to be converted to c-string and **transfered to the server**
should be encoded using **QString::toUtf8**.
 * All **other** QString should be converted **using QString::toUtf8** unless specific reasons.

The server side is done automatically for filename StringVectorProperty, and a few other cases.

Notes
------
This page is generated from *StringEncoding.md*.
