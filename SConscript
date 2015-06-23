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
  'FABRIC_BUILD_ARCH',
  'SOFTIMAGE_INCLUDE_DIR',
  'SOFTIMAGE_LIB_DIR',
  'SOFTIMAGE_VERSION',
  'sharedCapiFlags',
  'spliceFlags',
  'commandsFlags',
  'astWrapperFlags',
  'legacyBoostFlags',
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

qtDir = None
if FABRIC_BUILD_OS == 'Windows':
    qtDir = env.Dir('#').Dir('ThirdParty').Dir('PreBuilt').Dir(FABRIC_BUILD_OS).Dir(FABRIC_BUILD_ARCH).Dir('VS2013').Dir(FABRIC_BUILD_TYPE).Dir('qt').Dir('4.8.6')
if os.environ.has_key('QT_DIR'):
  qtDir = env.Dir(os.environ['QT_DIR'])

if qtDir:
  env.Append(CPPPATH = [os.path.join(qtDir.abspath, 'include')])
  env.Append(LIBPATH = [os.path.join(qtDir.abspath, 'lib')])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

if FABRIC_BUILD_OS == 'Linux':
  env.Append(LIBS=['boost_filesystem', 'boost_system'])
  exportsFile = env.File('Linux.exports').srcnode()
  env.Append(SHLINKFLAGS = ['-Wl,--version-script='+str(exportsFile)])
elif FABRIC_BUILD_OS == 'Windows':
  env.Append(LIBS = ['OpenGL32.lib'])
  env.Append(LIBS = ['user32.lib'])

if len(commandsFlags.keys()) > 0:
  env.MergeFlags(commandsFlags)
  env.MergeFlags(astWrapperFlags)
  env.MergeFlags(legacyBoostFlags)
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

qtIncludeDir = '/usr/include/Qt'
qtMocBin = '/usr/lib64/qt4/bin/moc'
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
    env.Append(LIBS = ['QtCore4'])
    env.Append(LIBS = ['QtGui4'])
    env.Append(LIBS = ['QtOpenGL4'])
  else:
    env.Append(LIBS = ['QtCored4'])
    env.Append(LIBS = ['QtGuid4'])
    env.Append(LIBS = ['QtOpenGLd4'])
else:
  env.Append(LIBS = ['QtCore'])
  env.Append(LIBS = ['QtGui'])
  env.Append(LIBS = ['QtOpenGL'])

target = 'FabricSpliceSoftimage'

if FABRIC_BUILD_OS == 'Linux':
  env[ '_LIBFLAGS' ] = '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'

softimageModule = env.SharedLibrary(target = target, source = Glob('*.cpp'), SHLIBPREFIX='')

if FABRIC_BUILD_OS == 'Linux':
  env.Depends(softimageModule, exportsFile)

softimageFiles = []

installedModule = env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'), softimageModule)
softimageFiles.append(installedModule)

softimageFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'Application', 'UI'), os.path.join('UI', 'FE_logo.bmp')))

softimageFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

# also install the FabricCore dynamic library
if FABRIC_BUILD_OS == 'Linux':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../../../lib/')])
if FABRIC_BUILD_OS == 'Windows':
  FABRIC_CORE_VERSION = FABRIC_SPLICE_VERSION.rpartition('.')[0]
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      env.Glob(os.path.join(FABRIC_DIR, 'bin', '*.dll'))
      )
    )
  softimageFiles.append(
    env.Install(
      os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
      os.path.join(FABRIC_DIR, 'bin', 'FabricCore-' + FABRIC_CORE_VERSION + '.pdb')
      )
    )

softimageFiles.append(
  env.Install(
    os.path.join(STAGE_DIR.abspath, 'Application', 'Plugins'),
    env.File('FabricSplice_Python.py')
    )
  )

# todo: install the python client

alias = env.Alias('splicesoftimage', softimageFiles)
spliceData = (alias, softimageFiles)
Return('spliceData')
