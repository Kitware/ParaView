## pvpython can now manage uv-backed virtual environments

ParaView ships its own bundled Python interpreter, which has made it hard to
`pip install` extra packages (trame, custom apps, ...) without touching the
ParaView install itself. The new `paraview.envs` module uses
[`uv`](https://docs.astral.sh/uv/) to create separate virtual environments and
layers their site-packages on top of `pvpython`'s own `sys.path`, so a script
can `import paraview` (and its C++ bindings) *and* whatever was pip-installed
in the environment.

`paraview.envs` is a `pvpython` module with its own subcommands:

* `list` - list every environment that has been created or installed.

  ```
  pvpython -m paraview.envs list
  ```

* `create` - create a named environment from a `requirements.txt` (does not
  run anything).

  ```
  pvpython -m paraview.envs create trame ./requirements.txt
  ```

* `use` - enter an environment built with `create` and run a script provided
  after `--`.

  ```
  pvpython -m paraview.envs use trame -- ./example.py [args...]
  ```

* `install` - install a script as a named, reusable application. Its
  dependencies are declared as
  [PEP 723](https://packaging.python.org/en/latest/specifications/inline-script-metadata/)
  inline script metadata and installed via `uv run`.

  ```
  pvpython -m paraview.envs install ./cone.py [--name cone] [--replace]
  ```

* `run` - run a previously installed application by name. `--enable-ssl`
  starts a new Python interpreter directly from the environment (with
  ParaView's own modules made available on top of it) instead of running
  in-process, which some apps (e.g. ones starting their own SSL/HTTPS server)
  require.

  ```
  pvpython -m paraview.envs run cone [--enable-ssl]
  ```

* `remove` - delete an environment (`create`d or `install`ed) and its venv.

  ```
  pvpython -m paraview.envs remove cone
  ```

Run `pvpython -m paraview.envs <command> --help` for a command's full options.
Environments are stored under `~/.config/ParaView/uv-venvs/<python-version>/<name>/`
(`%APPDATA%/ParaView/...` on Windows). This feature requires a `uv` executable,
either bundled alongside ParaView or available on `PATH`.

Developer notes: `paraview.envs`'s CLI is implemented with `argparse`
subparsers. `use`/`run` load the target script/app in-process rather than as a
subprocess, so if that script does its own argument parsing it must not see
`paraview.envs`'s own flags mixed into `sys.argv` - the module explicitly
rewrites `sys.argv` to `[script_path, *script_args]` before executing it, the
same clean hand-off a normal `python script.py args...` invocation would give
it.
