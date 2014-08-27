import os, platform

# check environment variables
for var in ['FABRIC_CAPI_DIR', 'FABRIC_SPLICE_VERSION', 'FABRIC_BUILD_OS', 'FABRIC_BUILD_ARCH', 'FABRIC_BUILD_TYPE', 'BOOST_DIR', 'SOFTIMAGE_INCLUDE_DIR', 'SOFTIMAGE_LIB_DIR', 'SOFTIMAGE_VERSION']:
  if not os.environ.has_key(var):
    print 'The environment variable %s is not defined.' % var
    exit(0)
  if var.lower().endswith('_dir'):
    if not os.path.exists(os.environ[var]):
      print 'The path for environment variable %s does not exist.' % var
      exit(0)

spliceEnv = Environment()

if not os.path.exists(spliceEnv.Dir('.stage').abspath):
  os.makedirs(spliceEnv.Dir('.stage').abspath)

# determine if we have the SpliceAPI two levels up
spliceApiDir = spliceEnv.Dir('..').Dir('..').Dir('SpliceAPI')

# try to get the Splice API from there
if os.path.exists(spliceApiDir.abspath):

  (spliceAPIAlias, spliceAPIFiles) = SConscript(
    dirs = [spliceApiDir],
    exports = {
      'parentEnv': spliceEnv,
      'FABRIC_CAPI_DIR': os.environ['FABRIC_CAPI_DIR'],
      'FABRIC_SPLICE_VERSION': os.environ['FABRIC_SPLICE_VERSION'],
      'FABRIC_BUILD_TYPE': os.environ['FABRIC_BUILD_TYPE'],
      'FABRIC_BUILD_OS': os.environ['FABRIC_BUILD_OS'],
      'FABRIC_BUILD_ARCH': os.environ['FABRIC_BUILD_ARCH'],
      'STAGE_DIR': spliceEnv.Dir('.build').Dir('SpliceAPI').Dir('.stage'),
      'BOOST_DIR': os.environ['BOOST_DIR']
    },
    variant_dir = spliceEnv.Dir('.build').Dir('SpliceAPI')
  )
  
  spliceApiDir = spliceEnv.Dir('.build').Dir('SpliceAPI').Dir('.stage').abspath
  
else:

  print 'The folder "'+spliceApiDir.abspath+'" does not exist. Please see the README.md for build instructions.'
  exit(0)

(softimageSpliceAlias, softimageSpliceFiles) = SConscript(
  os.path.join('SConscript'),
  exports = {
    'parentEnv': spliceEnv,
    'FABRIC_CAPI_DIR': os.environ['FABRIC_CAPI_DIR'],
    'FABRIC_SPLICE_VERSION': os.environ['FABRIC_SPLICE_VERSION'],
    'FABRIC_BUILD_TYPE': os.environ['FABRIC_BUILD_TYPE'],
    'FABRIC_BUILD_OS': os.environ['FABRIC_BUILD_OS'],
    'FABRIC_BUILD_ARCH': os.environ['FABRIC_BUILD_ARCH'],
    'STAGE_DIR': spliceEnv.Dir('.stage').Dir('Applications').Dir('FabricSpliceSoftimage'+os.environ['SOFTIMAGE_VERSION']),
    'BOOST_DIR': os.environ['BOOST_DIR'],
    'SOFTIMAGE_INCLUDE_DIR': os.environ['SOFTIMAGE_INCLUDE_DIR'],
    'SOFTIMAGE_LIB_DIR': os.environ['SOFTIMAGE_LIB_DIR'],
    'SOFTIMAGE_VERSION': os.environ['SOFTIMAGE_VERSION']
  },
  variant_dir = spliceEnv.Dir('.build').Dir(os.environ['SOFTIMAGE_VERSION'])
)

allAliases = [softimageSpliceAlias]
spliceEnv.Alias('all', allAliases)
spliceEnv.Default(allAliases)
