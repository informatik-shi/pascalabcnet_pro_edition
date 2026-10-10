"""Line-oriented bridge from compiled SPython plotting calls to CPython.

Keep this protocol generic: NumPy arrays and pandas objects can stay in the
same object table instead of being converted to large .NET arrays each call.
"""

import base64
import importlib
import json
import numbers
import operator
import os
import sys
import tempfile
import uuid

try:
    import matplotlib

    # Notebook cells need a deterministic off-screen renderer. The same backend
    # also makes savefig work in a console process without a window manager.
    matplotlib.use("Agg")
    import matplotlib.pyplot as pyplot
    import numpy as numpy
except Exception as exc:
    message = f"{type(exc).__name__}: {exc}"
    print("E\t" + base64.b64encode(message.encode()).decode(), flush=True)
    sys.exit(1)


objects = {}
next_id = 1
modules = {"pyplot": pyplot}


def decode(value):
    if isinstance(value, dict):
        if set(value) == {"ref"}:
            return objects[value["ref"]]
        if set(value) == {"float"}:
            return float(value["float"])
        if set(value) == {"tuple"}:
            return tuple(decode(item) for item in value["tuple"])
        if set(value) == {"map"}:
            return {decode(key): decode(item) for key, item in value["map"]}
        return {key: decode(item) for key, item in value.items()}
    if isinstance(value, list):
        return [decode(item) for item in value]
    return value


def respond(value, force_ref=False):
    global next_id
    # NumPy scalars need their Python str(), type, and operators. Converting
    # them to .NET doubles also loses distinctions such as 4.0 versus 4.
    force_ref = force_ref or isinstance(value, numpy.generic)
    if value is None and not force_ref:
        return "N"
    if isinstance(value, bool) and not force_ref:
        return "B\t" + ("1" if value else "0")
    if isinstance(value, numbers.Integral) and not force_ref:
        return "I\t" + str(value)
    if isinstance(value, numbers.Real) and not force_ref:
        return "D\t" + repr(float(value))
    if isinstance(value, str) and not force_ref:
        return "S\t" + base64.b64encode(value.encode()).decode()
    handle = next_id
    next_id += 1
    objects[handle] = value
    return "H\t" + str(handle)


print("R", flush=True)
for line in sys.stdin:
    try:
        request = json.loads(line)
        if os.environ.get("PABC_PYTHON_BRIDGE_DEBUG") == "1":
            print(repr(request), file=sys.stderr, flush=True)
        target_name = request["target"]
        if isinstance(target_name, str):
            if target_name not in modules:
                modules[target_name] = importlib.import_module(target_name)
            target = modules[target_name]
        else:
            target = objects[target_name]
        operation = request["op"]
        if operation == "call":
            args = [decode(item) for item in request.get("args", [])]
            kwargs = {key: decode(item) for key, item in request.get("kwargs", {}).items()}
            result = getattr(target, request["name"])(*args, **kwargs)
            if (target is pyplot and request["name"] == "show"
                    and os.environ.get("PABC_NOTEBOOK_INLINE") != "1"):
                for number in pyplot.get_fignums():
                    image = os.path.join(tempfile.gettempdir(),
                                         f"spython-plot-{uuid.uuid4().hex}.png")
                    pyplot.figure(number).savefig(image)
                    os.startfile(image)
        elif operation == "attr":
            result = getattr(target, request["name"])
        elif operation == "item":
            result = target[decode(request["index"])]
        elif operation == "binary":
            name = request["name"]
            other = decode(request["value"])
            result = (getattr(operator, name[1:])(other, target)
                      if name.startswith("r") else getattr(operator, name)(target, other))
        elif operation == "str":
            result = str(target)
        elif operation == "repr":
            result = repr(target)
        elif operation == "len":
            result = len(target)
        elif operation == "iter":
            result = list(target)
        else:
            raise ValueError(f"Unknown bridge operation: {operation}")
        print(respond(result, operation == "item"), flush=True)
    except Exception as exc:
        message = f"{type(exc).__name__}: {exc}"
        print("E\t" + base64.b64encode(message.encode()).decode(), flush=True)
