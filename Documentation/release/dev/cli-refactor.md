# Command line option parsing

The command line options parsing code has been completely refactored. ParaView
now uses [CLI11](https://github.com/CLIUtils/CLI11). This has implications for
users and developers alike.

## Changes for users

There way to specify command line options is now more flexible. These can be
provided as follows:

* `-a` : a single flag / boolean option
* `-f filename`: option with value, separated by space
* `--long`:  long flag / boolean option
* `--file filename`: long option with value separated using a space
* `--file=filename`: long option with value separated by equals

Note this is a subset of ways supported by CLI11 itself. This is because
ParaView traditionally supported options of form `-long=value` i.e. `-` could be
used as the prefix for long-named options. This is non-standard and now
deprecated. Instead, one should add use `--` as the prefix for such options e.g.
`-url=...` becomes `--url=...`. Currently, this is done automatically to avoid
disruption and a warning is raised. Since this conflicts with some of the other
more flexible ways of specifying options in `CLI11`, we limit ourselves to the
ways listed above until this legacy behaviour is no longer supported.

The `--help` output for all ParaView executables is now better formatted.
Options are grouped, making it easier to inspect related options together.
Mutually exclusive options, deprecated options are clearly marked to minimize
confusion. Also, in most terminals, the text width is automatically adjusted to
the terminal width and text is wrapped to make it easier to read.

Several options supports overriding default values using environment variables.
If the option is not specified on the command line, then that denoted
environment variable will be used to fetch the value for that option (or flag).

## Changes for developers

`vtkPVOptions` and subclasses are deprecated. Instead of a single class that
handled the parsing of the defining of command line flags/options, command line arguments,
and then keep state for the flag/option selections, the new design uses two
different classes. `vtkCLIOptions` handle the parsing (using CLI11), while
singletons such as `vtkProcessModuleConfiguration`, `vtkRemotingCoreConfiguration`, `pqCoreConfiguration`
maintain the state and also populate `vtkCLIOptions`  with supported flags/options. Custom applications
and easily add their own `*Configuration` classes to populate `vtkCLIOptions` to
custom options of override the default ParaView ones. If your custom code was
simply checking user selections from `vtkPVOptions` or subclasses, change it to
using the corresponding `*Configuration` singleton.
