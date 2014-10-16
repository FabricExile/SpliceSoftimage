#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import(
  'parentEnv',
  'FABRIC_DIR',
  'FABRIC_SPLICE_VERSION',
  'STAGE_DIR',
  'FABRIC_BUILD_OS',
  'FABRIC_BUILD_TYPE',
  'SOFTIMAGE_INCLUDE_DIR',
  'SOFTIMAGE_LIB_DIR',
  'SOFTIMAGE_VERSION',
  'sharedCapiFlags',
  'spliceFlags'
  )

env = parentEnv.Clone()

softimageFlags = {
  'CPPPATH': [
      SOFTIMAGE_INCLUDE_DIR
    ],
  'LIBPATH': [
    SOFTIMAGE_LIB_DIR,
  ],
}

softimageFlags['LIBS'] = ['sicoresdk', 'sicppsdk']
if FABRIC_BUILD_OS == 'Windows':
  softimageFlags['CCFLAGS'] = ['/DNT_PLUGIN']
elif FABRIC_BUILD_OS == 'Linux':
  softimageFlags['CCFLAGS'] = ['-DLINUX']

env.MergeFlags(softimageFlags)
env.Append(CPPDEFINES = ["_SPLICE_SOFTIMAGE_VERSION="+str(SOFTIMAGE_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

if FABRIC_BUILD_OS == 'Linux':
  env.Append(LIBS=['boost_filesystem', 'boost_system'])

target = 'FabricSpliceSoftimage'

softimageModule = env.SharedLibrary(target = target, source = Glob('*.cpp'), SHLIBPREFIX='')

softimageFiles = []

installedModule = env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'), softimageModule)
softimageFiles.append(installedModule)

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'UI'), os.path.join('UI', 'FE_logo.bmp')))

softimageFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

# also install the FabricCore dynamic library
if FABRIC_BUILD_OS == 'Linux':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../../../lib/')])
if FABRIC_BUILD_OS == 'Windows':
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      env.Glob(os.path.join(FABRIC_DIR, 'bin', '*.dll'))
      )
    )

# install PDB files on windows
if FABRIC_BUILD_TYPE == 'Debug' and FABRIC_BUILD_OS == 'Windows':
  env['CCPDBFLAGS']  = ['${(PDB and "/Fd%s_incremental.pdb /Zi" % File(PDB)) or ""}']
  pdbSource = softimageModule[0].get_abspath().rpartition('.')[0]+".pdb"
  pdbTarget = os.path.join(STAGE_DIR.abspath, os.path.split(pdbSource)[1])
  copyPdb = env.Command( 'copy', None, 'copy "%s" "%s" /Y' % (pdbSource, pdbTarget) )
  env.Depends( copyPdb, installedModule )
  env.AlwaysBuild(copyPdb)

# todo: install the python client

alias = env.Alias('splicesoftimage', softimageFiles)
spliceData = (alias, softimageFiles)
Return('spliceData')
