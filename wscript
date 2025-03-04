#
# Example Waf build script for RTEMS
#
# To configure, build and run:
#
# $ waf configure --rtems=$HOME/development/rtems/build/5 \
#                 --rtems-tools=$HOME/development/rtems/5 \
#                 --rtems-bsps=sparc/erc32
# $ waf
# $ $HOME/development/rtems/5/bin/sparc-rtems5-run \
#                             ./build/sparc-rtems5-erc32/hello.exe
#
# You can use '--rtems-archs=sparc,i386' or
# '--rtems-bsps=sparc/erc32,i386/pc586' to build for more than one BSP at a
# time.
#
rtems_version = '6'
import sys

try:
    import rtems_waf.rtems as rtems
except:
    print('error: no rtems_waf git submodule; see README.waf', file = sys.stderr)
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version = rtems_version, long_commands = True)

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf)

def build(bld):
    rtems.build(bld)
    bld.env.CFLAGS += ['-O2','-g']
    bld(features = 'c cprogram',
        defines = ['RTEMS_NETWORK_CONFIG_CLUSTER_SPACE=5120', 'RTEMS_NETWORK_CONFIG_MBUF_SPACE=2048'],
        target = 'rtems.exe',
        source = [
            'src/rtems_config.c',
            'src/init.c'
        ],
        lib='rtemsbsp rtemscpu bsd rtemscxx ntp debugger tftpfs c m')
