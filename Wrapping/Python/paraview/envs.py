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

  --list: List every environment that has been created or installed:

    $ pvpython -m paraview.envs --list

  --create: Create a named venv from a requirements.txt (does not run anything):

    $ pvpython -m paraview.envs --create trame ./requirements.txt

  --use: Enter a venv created with --create and run a script provided after `--`:

    $ pvpython -m paraview.envs --use trame [args...] -- ./example.py

  --install: Install a script as a named, reusable application. Its dependencies are
    declared as PEP 723 inline metadata (lines starting with `#` at the top of
    the file, e.g. `# dependencies = [...]`) and installed via `uv run`.
        --name defaults to the file's stem;
        --replace overwrites an existing environment of the same name;

    $ pvpython -m paraview.envs --install ./cone.py [--name cone] [--replace]

  --run: Run a previously installed application by name. Add `--enable-ssl` if ssl is
    required. `--enable-ssl` starts a new Python interpreter directly from the
    virtual environment and add ParaView environment on top of it:

    $ pvpython -m paraview.envs --run cone [--enable-ssl]

  --remove: Delete an environment (install or create) and its venv:

    $ pvpython -m paraview.envs --remove cone

Main commands are: --list / --create / --use / --install / --run / --remove
"""

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

USAGE = __doc__

USAGE_INSTALL = (
    "\n--install expects a path to a Python file containing PEP 723 inline script metadata."
    "\n  (?) --name [defaults to the file's stem] - name of application"
    "\n  (?) --replace - override any existing install/environment"
    "\n"
)
USAGE_CREATE = (
    "\n--create expects 2 arguments:"
    "\n  (1) name of the environment to create"
    "\n  (2) path to a requirements.txt file to use for initializing "
    "\n"
)
USAGE_REMOVE = "\n--remove expects the name of the environment/application to delete\n"
USAGE_RUN = (
    "\n--run expect the name of the environment/application to run"
    "\n  (?) --enable-ssl - starts a new Python interpreter"
    "\n                     directly from the virtual environment and"
    "\n                     add ParaView environment on top of it"
    "\n"
)
USAGE_USE = (
    "\n--use expect the name of the environment to run a script in"
    "\n  `--` should be used before the path of the script to execute"
    "\n"
)


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


def has_args(cmd, *keys, remove=False):
    """Return True if every key in `keys` is present in `cmd`.

    With remove=True, also pops each matched key out of `cmd` (used for
    boolean flags like --replace/--enable-ssl once they've been read).
    """
    for key in keys:
        if key not in cmd:
            return False

    if remove:
        for key in keys:
            cmd.remove(key)

    return True


def extract_arg(cmd, key, default=None, size=1, usage=None):
    """Pop `key` and the `size` values that follow it out of `cmd`.

    Returns the single popped value when size == 1, a list of `size`
    popped values otherwise, or `default` if `key` isn't present.
    """
    if key in cmd:
        idx = cmd.index(key)
        cmd.pop(idx)
        if len(cmd) - idx < size:
            print(usage)
            sys.exit(0)
        if size > 1:
            return [cmd.pop(idx) for _ in range(size)]
        return cmd.pop(idx)
    return default

def _create_venv(venv_path):
    """Create venv and return activate command as string"""
    result = subprocess.run(
        [str(UV_EXEC), "venv", str(venv_path.resolve()), "-p", PY_VERSION],
        capture_output=True,
        text=True,
    )
    lines = result.stderr.strip().splitlines()
    commands = None
    for line in lines:
        if "Activate" in line:
            cmd = line.split(":")[1].strip()
            commands = cmd
        else:
            print(line)

    return commands

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
    `--run <name>`. Fails without side effects if `app` doesn't exist or an
    environment named `name` already exists (unless replace=True).
    """
    if replace:
        remove(name)

    workspace = UV_VENV_PATH / name
    app = Path(app)
    if not app.exists():
        print(f"\nCan not install application with invalid path '{app}'.")
        print(USAGE_INSTALL)
        return

    if workspace.exists():
        print(f"\nCan not install application '{name}' as it already exist.")
        print(USAGE_INSTALL)
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
    activate_cmd = _create_venv(venv_path)
    commands = f"{activate_cmd} && uv run --active {dst_dep.resolve()}"
    print("Install dependencies:")
    install_dep = subprocess.run(commands, shell=True, capture_output=True, text=True)
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


def run(name, enable_ssl, cmd):
    """Launch a previously `--install`-ed application by name.

    By default the app's app.py is loaded and its `main()` executed
    in-process, after grafting the app's venv onto pvpython via
    load_venv(). With enable_ssl=True, the app is instead launched as a
    standalone subprocess using the venv's own `python`, with ParaView's
    modules injected via PYTHONPATH (see paraview_env()); this is needed
    because some apps (e.g. ones starting their own SSL/HTTPS server) must
    own their process rather than share pvpython's.
    """
    app_file = UV_VENV_PATH / name / "app.py"
    venv_base = (UV_VENV_PATH / name / "venv").resolve()

    if not venv_base.exists() or not app_file.exists():
        print(f"\nNo application found with name `{name}`.")
        print(USAGE_RUN)
        return

    if enable_ssl:
        # Run venv as main Python interpreter
        cmd.pop(0)  # remove executable
        python_exec = venv_base / "bin" / EXEC_PYTHON
        subprocess.run([str(python_exec), str(app_file), *cmd], env=paraview_env())
        return
    else:
        # Extend the current pvpython process with the venv's packages
        load_venv(venv_base)

        # Load an exec application file
        name = app_file.stem
        spec = importlib.util.spec_from_file_location(name, app_file)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        main = getattr(module, "main", None)
        if main:
            main()


def list_apps():
    """Print the names of all environments (install or create) on disk."""
    print(f"Available environments in ({UV_VENV_PATH})")
    for dir in UV_VENV_PATH.iterdir():
        if dir.is_dir():
            print(f"  - {dir.name}")


def create(name, requirement):
    """Create a named venv named `name` and `uv pip install -r requirement`.

    Unlike install(), this doesn't copy or run any script - it just
    prepares an environment meant to be entered later with `--use <name>`
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
    activate_cmd = _create_venv(venv_path)
    commands = f"{activate_cmd} && uv pip install -r {dst_req.resolve()}"
    print("Install dependencies:")
    install_dep = subprocess.run(commands, shell=True, capture_output=True, text=True)
    print(install_dep.stderr)


def use_venv(name, cmd):
    """Graft the venv `name` (built with `--create`) onto pvpython, then
    run the script following `--` in `cmd`, in-process, with the rest of
    `cmd` (after the script path) left in sys.argv for the script to read.
    """
    venv_base = (UV_VENV_PATH / name / "venv").resolve()

    if not venv_base.exists():
        print(f"\nNo application found for name: {name}.")
        print(USAGE_USE)
        return

    load_venv(venv_base)

    sub_cmd = []
    for token in cmd:
        if token == "--" or sub_cmd:
            sub_cmd.append(token)

    if len(sub_cmd) > 1:
        sub_cmd.pop(0)  # remove --
        script_file = Path(sub_cmd[0]).resolve()

        # Load an exec application file
        name = script_file.stem
        spec = importlib.util.spec_from_file_location(name, script_file)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        main = getattr(module, "main", None)
        if main:
            main()
    else:
        print("\nNo file to execute.")
        print(USAGE_USE)


def main():
    """Parse sys.argv for one of the top-level commands (--list, --create,
    --use, --install, --run, --remove, --help) and dispatch to it. See the
    module docstring (USAGE) for the full command reference.
    """
    cmd = sys.argv
    if has_args(cmd, "--help") or has_args(cmd, "-h"):
        print(USAGE)
    elif has_args(cmd, "--install"):
        app = extract_arg(cmd, "--install", usage=USAGE_INSTALL)
        name = extract_arg(cmd, "--name", default=Path(app).stem)
        install(app, name, has_args(cmd, "--replace"))
    elif has_args(cmd, "--remove"):
        app = extract_arg(cmd, "--remove", usage=USAGE_REMOVE)
        remove(app)
    elif has_args(cmd, "--run"):
        app = extract_arg(cmd, "--run", usage=USAGE_RUN)
        run(app, has_args(cmd, "--enable-ssl", remove=True), cmd)
    elif has_args(cmd, "--list"):
        list_apps()
    elif has_args(cmd, "--create"):
        name, requirement = extract_arg(cmd, "--create", size=2, usage=USAGE_CREATE)
        create(name, requirement)
    elif has_args(cmd, "--use"):
        name = extract_arg(cmd, "--use", usage=USAGE_USE)
        use_venv(name, cmd)
    else:
        print(USAGE)


if __name__ == "__main__":
    main()
