# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME='NDNEM'

from waflib import Build, Logs, Utils, Task, TaskGen, Configure

def options(opt):
    opt.add_option('--debug', action='store_true', default=False,
                   dest='debug', help='''debugging mode''')
    opt.add_option('--test', action='store_true', default=False,
                   dest='_test', help='''build unit tests''')

    opt.load('compiler_c compiler_cxx')
    opt.load('boost', tooldir=['waf-tools'])

def configure(conf):
    conf.load("compiler_c compiler_cxx")

    if conf.options.debug:
        conf.define('NDNEM_DEBUG', 1)
        conf.add_supported_cxxflags(cxxflags=['-O0',
                                              '-Wall',
                                              '-Wno-unused-variable',
                                              '-g3',
                                              '-Wno-unused-private-field', # only clang supports
                                              '-fcolor-diagnostics',       # only clang supports
                                              '-Qunused-arguments'         # only clang supports
                                              ])
    else:
        conf.add_supported_cxxflags(cxxflags=['-O3',
                                              '-Wall',
                                              '-g'])

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    # conf.write_config_header('config.h')

    if conf.options._test:
        conf.define('_TESTS', 1)
        conf.env.TEST = 1

    conf.load('boost')
    conf.check_boost(lib='system filesystem random')

def build (bld):
    bld(target="ndnem",
        features=["cxx", "cxxprogram"],
        source=bld.path.ant_glob(['core/*.cc']),
        use='NDN_CXX BOOST BOOST_SYSTEM BOOST_FILESYSTEM BOOST_RANDOM',
        includes='. core'
        )

    for app in bld.path.ant_glob('apps/*.cc'):
        bld(features=['cxx', 'cxxprogram'],
            target='%s' % (str(app.change_ext('','.cc'))),
            source=app,
            use='NDN_CXX BOOST BOOST_SYSTEM BOOST_FILESYSTEM BOOST_RANDOM',
            includes='. apps',
            )

@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    self.start_msg('Checking allowed flags for c++ compiler')

    supportedFlags = []
    for flag in cxxflags:
        if self.check_cxx(cxxflags=[flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg(' '.join(supportedFlags))
    self.env.CXXFLAGS += supportedFlags
