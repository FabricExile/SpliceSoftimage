# Fabric Engine Python
# This python plugin is required to import FabricEngine modules

import os
import sys

def XSILoadPlugin(in_reg):
    in_reg.Author = 'Fabric Engine'
    in_reg.Name = 'Fabric Engine Python'
    in_reg.Major = 1
    in_reg.Minor = 0


    pluginPath = in_reg.OriginPath
    fabricPythonDir = os.path.normpath(XSIUtils.BuildPath(pluginPath, "..", "..", "python", "2.7"))
    
    # Add the path to the module search paths so we can import the module.
    sys.path.append(fabricPythonDir)