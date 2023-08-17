Guidelines
==========
This document shows guidelines for code contributers, mainly focused on C++ code.

Licensing
---------
Each code file (header and source) requires SPDX license and copyright headers with a specific syntax

```
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
```

If a secondary copyright *must* be added, format as follows:

```
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) NAME_OF_PERSON_OR_INSTITUTE_CONTRIBUTING_THE_CODE
// SPDX-License-Identifier: BSD-3-Clause
```

Please note it is not required to add your own copyright line when modifying or adding a file.
Even without adding it you still own copyrights on the line you modified or added in the file
according to the git history. Consider instead adding a `@thanks` tag in the documentation of the class.

If the code *must* be contributed under another license than `BSD-3-Clause` license, contact
maintainers to find the right formalism and ensure license compatibility before contributing.

Find more information about Software Package Data Exchange (SPDX) and Software Bill of Materials (SBOM)
in the VTK documentation:
[SPDX & SBOM](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/docs/advanced/spdx_and_sbom.md)

C++ Preprocessor
----------------
* Use extensions .h and .cxx.
* Use `#include <...>` for includes of third-party headers, use `#include "..."` for includes of ParaView headers.
* Use no generic defines like `DEBUG`.
* Do not use double underscore in define or macro names, no `__MY_DEFINE`.
* Prefer `#if` checks on unconditionally set symbols rather than `#ifdef` checks on conditionally defined symbols.

C++ Language
------------
* Preferably use C++11.
* Use RAII, e.g. `std::array<char, 5> dat` but not `char* dat = new[5]`.
* Avoid `using` and write the full namespaces (e.g. `std::string`).
* Exceptions can be thrown, but should not leave the function as VTK (and therefore ParaView) is not exception-safe in general.
* You might run clang-tidy for hints, however clang-tidy is by default stricter than necessary. VTK and ParaView have their own `.clang-tidy` configuration files.

Naming Conventions
------------------
* Use `this->` for all member access.
* Use camel style for all members (functions, attributes) and start with a capital (e.g. `CapitalStart`).
* Don't abbreviate public functions (`GetNumberOfElements()` but not `GetNumElems()`).

Code Formatting
---------------
* Use separate lines for braces like
```
if ()
{
  ...;
}
```

* No superfluous empty lines.
* In the source files separate function implementations by an empty line and a line comment
```
//-----------------------------------------------------------------------------
```

Comments
--------
* Comment any function at least briefly in the header in [Doxygen](https://www.doxygen.nl), use the [Javadoc style](https://www.doxygen.nl/manual/docblocks.html)

```
/**
 * This comments the function
 */
void TheFunction();
```
* Use Doxygen's `@class` for a comprehensive class description (only header).
* Don't keep commented code.
