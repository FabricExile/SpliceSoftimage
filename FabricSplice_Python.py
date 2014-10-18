#
#  Copyright 2010-2014 Fabric Engine Inc. All rights reserved.
#


from win32com.client import Dispatch
from win32com.client import constants

import sys

def XSILoadPlugin( in_reg ):
    in_reg.Author = "Fabric Engine Inc."
    in_reg.Name = "FabricSplice_Python"
    in_reg.Major = 1
    in_reg.Minor = 0

    #RegistrationInsertionPoint - do not remove this line

    # Add FabricEngine Python module to system path.
    pluginPath = in_reg.OriginPath
    modulesPath = XSIUtils.BuildPath(pluginPath, "..", "..", "python", "2.7")

    if modulesPath not in sys.path:
        sys.path.append(modulesPath)

    return True