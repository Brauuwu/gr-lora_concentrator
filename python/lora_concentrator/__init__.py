#
# Copyright 2024
#

# The presence of this file turns this directory into a Python package

# import pybind11 generated symbols into the lora_concentrator namespace
try:
    from .lora_concentrator_python import *
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from lora_concentrator_python import *
