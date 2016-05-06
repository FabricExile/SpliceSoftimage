#
# Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
#

import os, sys, platform, copy

Import(
  'parentEnv',
  'FABRIC_DIR',
  'FABRIC_SPLICE_VERSION',
  'STAGE_DIR',
  'FABRIC_BUILD_OS',
  'FABRIC_BUILD_TYPE',
  'FABRIC_BUILD_ARCH',
  'SOFTIMAGE_INCLUDE_DIR',
  'SOFTIMAGE_LIB_DIR',
  'SOFTIMAGE_VERSION',
  'sharedCapiFlags',
  'spliceFlags',
  'commandsFlags',
  'astWrapperFlags',
  'codeCompletionFlags',
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

shibokenPysideDir = env.Dir('')
if FABRIC_BUILD_OS == 'Linux':
  shibokenPysideDir = env.Dir(os.environ['SHIBOKEN_PYSIDE_DIR'])

qtDir = None
if FABRIC_BUILD_OS == 'Windows':
  if FABRIC_BUILD_TYPE == 'Release':
    qtDir = env.Dir('#').Dir('ThirdParty').Dir('PreBuilt').Dir(FABRIC_BUILD_OS).Dir(FABRIC_BUILD_ARCH).Dir('VS2013').Dir('Release').Dir('qt-fabric').Dir('4.8.6')
  else:
    qtDir = env.Dir('#').Dir('ThirdParty').Dir('PreBuilt').Dir(FABRIC_BUILD_OS).Dir(FABRIC_BUILD_ARCH).Dir('VS2013').Dir('Release').Dir('qt').Dir('4.8.6')

if os.environ.has_key('QT_DIR'):
  qtDir = env.Dir(os.environ['QT_DIR'])

if qtDir:
  env.Append(CPPPATH = [os.path.join(qtDir.abspath, 'include')])
  env.Append(LIBPATH = [os.path.join(qtDir.abspath, 'lib')])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

if FABRIC_BUILD_OS == 'Linux':
  exportsFile = env.File('Linux.exports').srcnode()
  env.Append(SHLINKFLAGS = ['-Wl,--version-script='+str(exportsFile)])
elif FABRIC_BUILD_OS == 'Windows':
  env.Append(LIBS = ['OpenGL32.lib'])
  env.Append(LIBS = ['user32.lib'])

if len(commandsFlags.keys()) > 0:
  env.MergeFlags(commandsFlags)
  env.MergeFlags(astWrapperFlags)
  env.MergeFlags(codeCompletionFlags)
else:
  if FABRIC_BUILD_OS == 'Windows':
    if FABRIC_BUILD_TYPE == 'Release':
      env.Append(LIBS = ['FabricServices-MSVC-'+env['MSVC_VERSION']+'-mt'])
    else:
      env.Append(LIBS = ['FabricServices-MSVC-'+env['MSVC_VERSION']+'-mtd'])
  else:
    env.Append(LIBS = ['FabricServices'])
env.Append(LIBS = ['FabricSplitSearch'])

uiLibPrefix = 'uiSoftimage'+str(SOFTIMAGE_VERSION)

uiDir = env.Dir('#').Dir('Native').Dir('FabricUI')
if os.environ.has_key('FABRIC_UI_DIR'):
  uiDir = env.Dir(os.environ['FABRIC_UI_DIR'])
uiSconscript = uiDir.File('SConscript')
if not os.path.exists(uiSconscript.abspath):
  print "Error: You need to have FabricUI checked out to "+uiSconscript.dir.abspath

env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib')])
env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_DIR'], 'include')])
env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_DIR'], 'include', 'FabricServices')])
env.Append(CPPPATH = [uiSconscript.dir])

if qtDir:
  qtIncludeDir = os.path.join(qtDir.abspath, 'include', 'Qt')
  qtMocBin = os.path.join(qtDir.abspath, 'bin', 'moc')

uiLibs = SConscript(uiSconscript, exports = {
  'parentEnv': env, 
  'uiLibPrefix': uiLibPrefix, 
  'qtDir': qtIncludeDir,
  'qtMOC': qtMocBin,
  'qtFlags': {
  },
  'fabricFlags': sharedCapiFlags,
  'buildType': FABRIC_BUILD_TYPE,
  'buildOS': FABRIC_BUILD_OS,
  'buildArch': FABRIC_BUILD_ARCH,
  'stageDir': env.Dir('#').Dir('stage').Dir('lib'),
  'shibokenPysideDir': shibokenPysideDir,
  },
  duplicate=0,
  variant_dir = env.Dir('FabricUI')
  )

# import the softimage specific libraries
Import(uiLibPrefix+'Flags')

# ui flags
env.MergeFlags(locals()[uiLibPrefix + 'Flags'])

if qtDir:
  env.Append(LIBPATH = [os.path.join(qtDir.abspath, 'lib')])

if FABRIC_BUILD_OS == 'Windows':
  if FABRIC_BUILD_TYPE == 'Release':
    if os.path.exists(os.path.join(qtDir.abspath, 'lib', 'QtCoreFabric4.lib')):
      env.Append(LIBS = ['QtCoreFabric4'])
      env.Append(LIBS = ['QtGuiFabric4'])
      env.Append(LIBS = ['QtOpenGLFabric4'])
    elif os.path.exists(os.path.join(qtDir.abspath, 'lib', 'QtCore4.lib')):
      env.Append(LIBS = ['QtCore4'])
      env.Append(LIBS = ['QtGui4'])
      env.Append(LIBS = ['QtOpenGL4'])
    else:
      raise(Exception('QtCoreFabric4.lib or QtCore4.lib not found on '+qtDir.abspath))
  else:
    env.Append(LIBS = ['QtCored4'])
    env.Append(LIBS = ['QtGuid4'])
    env.Append(LIBS = ['QtOpenGLd4'])
else:
  env.Append(LIBS = ['QtCore'])
  env.Append(LIBS = ['QtGui'])
  env.Append(LIBS = ['QtOpenGL'])

target = 'FabricSoftimage'

softimageModule = env.SharedLibrary(target = target, source = Glob('*.cpp'), SHLIBPREFIX='')

if FABRIC_BUILD_OS == 'Linux':
  env.Depends(softimageModule, exportsFile)

softimageFiles = []

installedModule = env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'), softimageModule)
softimageFiles.append(installedModule)

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'UI'), os.path.join('UI', 'FE_logo.bmp')))
softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'UI'), os.path.join('UI', 'FE_exclamation.bmp')))

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Data', 'Compounds'), os.path.join('ICE_Compounds', 'Fabric Canvas Set Normals.xsicompound')))
softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Data', 'Compounds'), os.path.join('ICE_Compounds', 'Fabric Canvas Set UVWs.xsicompound')))
softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Data', 'Compounds'), os.path.join('ICE_Compounds', 'Fabric Canvas Set Colors.xsicompound')))

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'), 'FabricCanvasInspectOp.vbs'))

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'), 'FabricEnginePython.py'))

softimageFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

# also install the FabricCore dynamic library
if FABRIC_BUILD_OS == 'Linux':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../../../lib/')])
if FABRIC_BUILD_OS == 'Windows':
  FABRIC_CORE_VERSION = FABRIC_SPLICE_VERSION.rpartition('.')[0]
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      [
        env.Glob(os.path.join(FABRIC_DIR, 'bin', 'Fabric*.dll')),
        env.Glob(os.path.join(qtDir.abspath, 'lib', 'QtCore*.dll')),
        env.Glob(os.path.join(qtDir.abspath, 'lib', 'QtGui*.dll')),
        env.Glob(os.path.join(qtDir.abspath, 'lib', 'QtOpenGL*.dll')),
        os.path.join(FABRIC_DIR, 'bin', 'tbb.dll'),
        os.path.join(FABRIC_DIR, 'bin', 'tbbmalloc.dll'),
      ]
      )
    )
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      os.path.join(FABRIC_DIR, 'bin', 'FabricCore-' + FABRIC_CORE_VERSION + '.pdb')
      )
    )

# todo: install the python client

alias = env.Alias('splicesoftimage', softimageFiles)
spliceData = (alias, softimageFiles)
Return('spliceData')
