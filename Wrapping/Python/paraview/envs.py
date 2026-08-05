"""
---------------------------------------------------------------------
 paraview.envs - virtual environments manager for ParaView's Python
---------------------------------------------------------------------

ParaView ships its own bundled Python interpreter, which makes it hard to
pip-install extra packages (e.g. trame, custom apps) without touching the
ParaView install itself. This module uses `uv` to create separate virtual
environments and layers their site-packages on top of pvpython's own
`sys.path`, so scripts can `import paraview` (and its C++ bindings) *and*
whatever was pip-installed in the venv.

Environments are stored under
    ~/.config/ParaView/uv-venvs/<python-version>/<name>/

Commands
---------------------------------------------------------------------

  list: List every environment that has been created or installed:

    $ pvpython -m paraview.envs list

  create: Create a named venv from a requirements.txt (does not run anything):

    $ pvpython -m paraview.envs create trame ./requirements.txt

  use: Enter a venv created with `create` and run a script provided after `--`:

    $ pvpython -m paraview.envs use trame -- ./example.py [args...]

  install: Install a script as a named, reusable application. Its dependencies are
    declared as PEP 723 inline metadata (lines starting with `#` at the top of
    the file, e.g. `# dependencies = [...]`) and installed via `uv run`.
    --name defaults to the file's stem;
    --replace overwrites an existing environment of the same name;

    $ pvpython -m paraview.envs install ./cone.py [--name cone] [--replace]

  run: Run a previously installed application by name. Add `--enable-ssl` if ssl is
    required. `--enable-ssl` starts a new Python interpreter directly from the
    virtual environment and add ParaView environment on top of it:

    $ pvpython -m paraview.envs run cone [--enable-ssl]

  remove: Delete an environment (install or create) and its venv:

    $ pvpython -m paraview.envs remove cone

Main commands are: list / create / use / install / run / remove

Run `pvpython -m paraview.envs <command> --help` for a command's full options.
"""

import argparse
import platform
import sys
import os
import site
from pathlib import Path
import shutil
import subprocess
import importlib.util

PY_VERSION = f"{sys.version_info.major}.{sys.version_info.minor}"

CURRENT_OS = platform.system()
IS_WINDOWS = CURRENT_OS == "Windows"

if IS_WINDOWS:
    BASE_PATH = Path(os.environ.get("APPDATA")) / "ParaView"
else:
    BASE_PATH = Path("~/.config/ParaView").expanduser()

UV_EXEC_CACHE = BASE_PATH / "uv-path"
UV_VENV_PATH = BASE_PATH / "uv-venvs" / PY_VERSION
UV_VENV_PATH.mkdir(parents=True, exist_ok=True)

# Use System uv if available
sys_uv = shutil.which('uv')
if sys_uv:
    UV_EXEC_CACHE.write_text(sys_uv)

EXEC_EXTENSION = ".exe" if IS_WINDOWS else ""
EXEC_UV = f"uv{EXEC_EXTENSION}"
EXEC_PYTHON = f"python{EXEC_EXTENSION}"


def find_uv_exec():
    """Locate the `uv` executable, caching the result on disk.

    Looks up UV_EXEC_CACHE first; if missing or stale, walks up from
    sys.executable (pvpython) up to 3 parent directories and searches each
    subtree for a `uv` binary, since `uv` is typically bundled alongside or
    near the ParaView Python install.
    """
    if UV_EXEC_CACHE.exists():
        uv_path = Path(UV_EXEC_CACHE.read_text())
        if uv_path.exists():
            return uv_path

    CURRENT_DIRECTORY = Path(sys.executable).parent
    for _ in range(3):
        for file_path in CURRENT_DIRECTORY.rglob(EXEC_UV):
            UV_EXEC_CACHE.write_text(str(file_path.resolve()))
            return file_path.resolve()

        CURRENT_DIRECTORY = CURRENT_DIRECTORY.parent


UV_EXEC = find_uv_exec()


def paraview_env():
    """Build an environment dict that exposes ParaView's Python modules
    (PYTHONPATH) to a *separate* Python process, e.g. the standalone
    interpreter spawned by `run(..., enable_ssl=True)`. This is only needed
    for code paths that run outside of the current pvpython process, since
    in-process execution already has ParaView on sys.path.
    """
    envs = {**os.environ.copy()}
    if CURRENT_OS == "Windows":
        PV_BIN = UV_EXEC.parent.resolve()
        # envs["PATH"] = str(PV_BIN)
        envs["PYTHONPATH"] = str(PV_BIN / "Lib" / "site-packages")
    if CURRENT_OS == "Darwin":
        PV_HOME = UV_EXEC.parent.parent.resolve()
        # envs["DYLD_LIBRARY_PATH"] = str(PV_HOME / "Libraries")
        envs["PYTHONPATH"] = str(PV_HOME / "Python")
    if CURRENT_OS == "Linux":
        PV_HOME = UV_EXEC.parent.parent.resolve()
        # envs["LD_LIBRARY_PATH"] = str(PV_HOME / "lib")
        envs["PYTHONPATH"] = str(
            PV_HOME / "lib" / f"python{PY_VERSION}" / "site-packages"
        )
    return envs


def _create_venv(venv_path):
    """Create a venv at `venv_path`, printing uv's own progress output."""
    result = subprocess.run(
        [str(UV_EXEC), "venv", str(venv_path.resolve()), "-p", PY_VERSION],
        capture_output=True,
        text=True,
    )
    print(result.stderr)


def _venv_env(venv_path):
    """Environment for running `uv` against `venv_path` without needing to
    `source`/activate it through a shell (which uv only offers a hint for
    when it recognizes $SHELL, e.g. it is silent in most CI containers)."""
    return {**os.environ, "VIRTUAL_ENV": str(venv_path.resolve())}


def _exec_script(script_file, script_args):
    """Graft `script_args` onto sys.argv (as a normal interpreter invocation
    would: script path as argv[0], its own args following) and exec
    `script_file` in-process, then call its main() if it defines one."""
    sys.argv[:] = [str(script_file)] + script_args
    spec = importlib.util.spec_from_file_location(script_file.stem, script_file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    main = getattr(module, "main", None)
    if main:
        main()

# -----------------------------------------------------------------------------

def remove(name):
    """Delete the environment (venv + app/dependency files) named `name`."""
    path_to_remove = UV_VENV_PATH / name
    if path_to_remove.exists():
        shutil.rmtree(path_to_remove)


def install(app, name, replace):
    """Install `app` (a .py file) as a persistent, named application.

    Copies `app` into a new workspace as app.py, extracts its leading
    `#`-comment lines (PEP 723 inline script metadata, e.g. `# dependencies
    = [...]`) into dependencies.py, creates a fresh venv, and runs
    `uv run` on dependencies.py so uv resolves and installs those
    dependencies into the venv. The app can later be launched with
    `run <name>`. Fails without side effects if `app` doesn't exist or an
    environment named `name` already exists (unless replace=True).
    """
    if replace:
        remove(name)

    workspace = UV_VENV_PATH / name
    app = Path(app)
    if not app.exists():
        print(f"\nCan not install application with invalid path '{app}'.")
        return

    if workspace.exists():
        print(f"\nCan not install application '{name}' as it already exist.")
        return

    # Create directory and capture app
    workspace.mkdir(parents=True, exist_ok=True)
    venv_path = workspace / "venv"
    dst_app = workspace / "app.py"
    dst_dep = workspace / "dependencies.py"
    dst_app.write_text(app.read_text())

    comments = []
    for line in app.read_text().splitlines(True):
        if line.startswith("#"):
            comments.append(line)
    dst_dep.write_text("".join(comments))

    # Create venv
    _create_venv(venv_path)
    print("Install dependencies:")
    install_dep = subprocess.run(
        [str(UV_EXEC), "run", "--active", str(dst_dep.resolve())],
        env=_venv_env(venv_path),
        capture_output=True,
        text=True,
    )
    print(install_dep.stderr)


def load_venv(venv_base):
    """Graft a venv onto the *current*, already-running pvpython process.

    Unlike a normal venv activation, this doesn't spawn a new interpreter:
    it prepends the venv's `bin` to PATH and adds its site-packages to
    sys.path, so packages pip-installed in the venv become importable
    alongside ParaView's own bundled modules in this same process.
    site.addsitedir() appends new entries to the end of sys.path, so they
    are moved back to the front afterwards to make the venv take priority
    over pvpython's built-ins when both provide the same package.
    """
    bin_dir = str(venv_base / "bin")
    os.environ["PATH"] = os.pathsep.join(
        [bin_dir] + os.environ.get("PATH", "").split(os.pathsep)
    )
    os.environ["VIRTUAL_ENV"] = str(venv_base)
    prev_length = len(sys.path)

    if sys.platform == "win32":
        python_libs = venv_base / "Lib/site-packages"
    else:
        python_libs = venv_base / f"lib/python{PY_VERSION}/site-packages"

    site.addsitedir(python_libs)
    sys.path[:] = sys.path[prev_length:] + sys.path[0:prev_length]
    sys.real_prefix = sys.prefix
    sys.prefix = venv_base


def run(name, enable_ssl, extra_args):
    """Launch a previously `install`-ed application by name.

    By default the app's app.py is loaded and its `main()` executed
    in-process, after grafting the app's venv onto pvpython via
    load_venv(). With enable_ssl=True, the app is instead launched as a
    standalone subprocess using the venv's own `python`, with ParaView's
    modules injected via PYTHONPATH (see paraview_env()); this is needed
    because some apps (e.g. ones starting their own SSL/HTTPS server) must
    own their process rather than share pvpython's. `extra_args` (anything
    following `run <name>` on the command line) is passed through to the app.
    """
    app_file = UV_VENV_PATH / name / "app.py"
    venv_base = (UV_VENV_PATH / name / "venv").resolve()

    if not venv_base.exists() or not app_file.exists():
        print(f"\nNo application found with name `{name}`.")
        return

    if enable_ssl:
        # Run venv as main Python interpreter
        python_exec = venv_base / "bin" / EXEC_PYTHON
        subprocess.run([str(python_exec), str(app_file), *extra_args], env=paraview_env())
    else:
        # Extend the current pvpython process with the venv's packages
        load_venv(venv_base)
        _exec_script(app_file, extra_args)


def list_apps():
    """Print the names of all environments (install or create) on disk."""
    print(f"Available environments in ({UV_VENV_PATH})")
    for dir in UV_VENV_PATH.iterdir():
        if dir.is_dir():
            print(f"  - {dir.name}")


def create(name, requirement):
    """Create a named venv named `name` and `uv pip install -r requirement`.

    Unlike install(), this doesn't copy or run any script - it just
    prepares an environment meant to be entered later with `use <name>`
    to run arbitrary scripts. Fails without side effects if `requirement`
    doesn't exist or an environment named `name` already exists.
    """
    workspace = UV_VENV_PATH / name
    requirement = Path(requirement)
    if not requirement.exists():
        print(f"Can not install requirements with invalid path {requirement}.")
        return

    if workspace.exists():
        print(f"Can not install environment {name} as it already exist.")
        return

    # Create directory and capture app
    workspace.mkdir(parents=True, exist_ok=True)
    venv_path = workspace / "venv"
    dst_req = workspace / "requirement.txt"
    dst_req.write_text(requirement.read_text())

    # Create venv
    _create_venv(venv_path)
    print("Install dependencies:")
    install_dep = subprocess.run(
        [str(UV_EXEC), "pip", "install", "-r", str(dst_req.resolve())],
        env=_venv_env(venv_path),
        capture_output=True,
        text=True,
    )
    print(install_dep.stderr)


def use_venv(name, script_args):
    """Graft the venv `name` (built with `create`) onto pvpython, then run
    the script in `script_args[0]` in-process, with script_args[1:] left in
    sys.argv for the script to read.
    """
    venv_base = (UV_VENV_PATH / name / "venv").resolve()

    if not venv_base.exists():
        print(f"\nNo application found for name: {name}.")
        return

    if not script_args:
        print("\nNo file to execute. Pass a script after `--`.")
        return

    load_venv(venv_base)
    _exec_script(Path(script_args[0]).resolve(), script_args[1:])


def build_parser():
    """Build the `pvpython -m paraview.envs` argument parser: one subcommand
    per top-level command (list/create/use/install/run/remove), each
    documented via argparse's own --help rather than hand-written usage
    strings. See the module docstring for the full command reference.
    """
    parser = argparse.ArgumentParser(
        prog="pvpython -m paraview.envs",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    sub.add_parser(
        "list", help="list every environment that has been created or installed")

    p_create = sub.add_parser(
        "create", help="create a named venv from a requirements.txt (does not run anything)")
    p_create.add_argument("name", help="name of the environment to create")
    p_create.add_argument("requirement", help="path to a requirements.txt file to use for initializing")

    p_use = sub.add_parser(
        "use", help="enter a venv created with `create` and run a script provided after `--`")
    p_use.add_argument("name", help="name of the environment to run a script in")

    p_install = sub.add_parser(
        "install", help="install a script as a named, reusable application")
    p_install.add_argument(
        "app", help="path to a Python file containing PEP 723 inline script metadata")
    p_install.add_argument("--name", help="defaults to the file's stem")
    p_install.add_argument(
        "--replace", action="store_true", help="override any existing install/environment")

    p_run = sub.add_parser(
        "run", help="run a previously installed application by name")
    p_run.add_argument("name", help="name of the environment/application to run")
    p_run.add_argument(
        "--enable-ssl", action="store_true",
        help="start a new Python interpreter directly from the virtual "
             "environment and add ParaView's environment on top of it")

    p_remove = sub.add_parser(
        "remove", help="delete an environment (install or create) and its venv")
    p_remove.add_argument("name", help="name of the environment/application to delete")

    return parser


def main():
    """Parse sys.argv for one of the top-level commands (list, create, use,
    install, run, remove) and dispatch to it. Anything left over after a
    known command and its own options are parsed (e.g. the script and args
    following `use <name> --`, or extra args after `run <name>`) is passed
    through untouched, so it never has to survive being re-parsed by
    whatever argument parsing the invoked script/app does on its own.
    """
    parser = build_parser()
    args, extra_args = parser.parse_known_args(sys.argv[1:])

    if args.command == "list":
        list_apps()
    elif args.command == "create":
        create(args.name, args.requirement)
    elif args.command == "use":
        use_venv(args.name, extra_args)
    elif args.command == "install":
        install(args.app, args.name or Path(args.app).stem, args.replace)
    elif args.command == "run":
        run(args.name, args.enable_ssl, extra_args)
    elif args.command == "remove":
        remove(args.name)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
