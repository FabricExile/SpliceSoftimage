# FabricPython

from win32com.client import Dispatch
from win32com.client import constants

import sys

def XSILoadPlugin( in_reg ):
    in_reg.Author = "Eric Thivierge"
    in_reg.Name = "FabricSplice_Python"
    in_reg.Major = 1
    in_reg.Minor = 0

    #RegistrationInsertionPoint - do not remove this line


    # Add Fabric Splice Python module to system path.
    pluginPath = in_reg.OriginPath
    modulesPath = XSIUtils.BuildPath(pluginPath, "..", "..", "python27")

    if modulesPath not in sys.path:
        sys.path.append(modulesPath)

    return True