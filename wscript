# Waf build script for rtems-init
rtems_version = "7"

import rtems_waf.rtems as rtems
from waflib import Task
from tools.waftools import (
    get_includes, get_install_prefix, get_lib_paths,
    build_module, install_headers, install_libs,
    check_headers, has_c_header, has_lib, add_rootfs)
import os

ROOT = os.getcwd()

def _get_text_seg_size(ctx) -> str|None:
    """
    Returns the text segment size. If none, do not define it!
    """
    if ctx.options.ENABLE_TEXT_SEGMENT == 'default':
        if ctx.env.RTEMS_ARCH in ['powerpc', 'arm']:
            ctx.options.ENABLE_TEXT_SEGMENT = '0'
        else:
            ctx.options.ENABLE_TEXT_SEGMENT = 'no'
    if ctx.options.ENABLE_TEXT_SEGMENT == 'no':
        return None
    if ctx.options.ENABLE_TEXT_SEGMENT == 'yes':
        return '0x800000'
    return ctx.options.ENABLE_TEXT_SEGMENT

def _bsp_configure(conf, bsp, lib_confs : list = []):
    """
    Configures the build for the specified BSP

    Parameters
    ----------
    conf :
        Configuration context
    bsp : str
        arch + bsp
    """

    # Pretty ugly, but replicates what Till did in his m4 utils package. Throws these into config.h
    conf.undefine('RTEMS_VERSION_LATER_THAN')
    conf.define('RTEMS_VERSION_LATER_THAN(ma,mi,re)',
	'(    __RTEMS_MAJOR__  > (ma)	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ > (re)) \
    )', quote=False)
    conf.undefine('RTEMS_VERSION_ATLEAST')
    conf.define('RTEMS_VERSION_ATLEAST(ma,mi,re)',
    '(    __RTEMS_MAJOR__  > (ma)	\
	|| (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	|| (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ >= (re)) \
	)', quote=False)

    # Check for common headers
    check_headers(conf, {
        'sys/mman.h': 'HAVE_SYS_MMAN_H',
        'strings.h': 'HAVE_STRINGS_H',
        'sys/select.h': 'HAVE_SYS_SELECT_H',
        'sys/termios.h': 'HAVE_SYS_TERMIOS_H',
        'termios.h': 'HAVE_TERMIOS_H',
        'ncurses/term.h': 'HAVE_NCURSES_TERM_H',
        'ncurses/curses.h': 'HAVE_NCURSES_CURSES_H',
        'sys/features.h': 'HAVE_SYS_FEATURES_H',
        'link.h': 'HAVE_LINK_H',
        'pthread.h': 'HAVE_PTHREADS',
        'rtems/rtems/cache.h': 'HAVE_RTEMS_CACHE_H',
        'rtems.h': 'HAVE_RTEMS_H',
    })

    # TODO: Tecla-specific things
    conf.define('HAVE_TYPE_UINT32_T', 1)
    conf.define('HAVE_TECLA', 1)

    conf.write_config_header(f'{conf.env.RTEMS_ARCH_BSP}/config.h')

    # Determine networking stack
    conf.env.NETWORKING_STACK = 'none'
    if has_c_header(conf, 'rtems/bsd/bsd.h'):
        conf.env.CPPFLAGS += ['-DRTEMS_LIBBSD_STACK=1']
        conf.env.NETWORKING_STACK = 'bsd'
    elif has_c_header(conf, 'rtems/rtems_bsdnet_internal.h'):
        conf.env.CPPFLAGS += ['-DRTEMS_LEGACY_STACK=1']
        conf.env.NETWORKING_STACK = 'legacy'

    # Determine if libdebugger is available
    conf.env.HAVE_DEBUGGER = False
    if has_lib(conf, ['debugger']):
        conf.env.CPPFLAGS += ['-DHAVE_DEBUGGER=1']
        conf.env.HAVE_DEBUGGER=True

    # Now configure all other libraries
    for c in lib_confs:
        c(conf, bsp)

def build_miscUtils(bld):
    """
    Builds the miscUtils module
    """
    source=[
        'miscUtils/icmpping.c',
        'miscUtils/memUtils.c',
        'miscUtils/traceroute.c',
        'miscUtils/ttyconfi.c',
        'miscUtils/sockstats.c',
        'miscUtils/task_uptime.c',
        'miscUtils/exectime.c',
        'miscUtils/loop.c',
    ]

    build_module(bld, 'miscUtils.obj', sources=source, includes=['miscUtils'])

def build_telnetd(bld):
    build_module(bld, 'telnetd.obj', ldflags=['-Wl,-u,rtems_telnetd_initialize'], libs=['-ltelnetd'])

def build_libbspExt(bld):
    """
    Builds the libbspExt module
    """
    sources = [
        'libbspExt/bspExt.c',
        'libbspExt/dabrBpnt.c',
        'libbspExt/isrWrap.c',
        'libbspExt/memProbe.c'
    ]
    build_module(bld, target='bspExt.obj', sources=sources)

    bld(
        target='bspExt',
        features='c cstlib',
        source=sources,
        includes=get_includes(bld)
    )

    bld.install_files(f'{get_install_prefix(bld)}/lib', 'libbspExt.a')
    bld.install_files(f'{get_install_prefix(bld)}/include/bsp', ['libbspExt/bspExt.h'])

def build_monitor(bld):
    """
    Builds the monitor module
    """
    sources = [
        'monitor/call.cc',
        'monitor/memusage.c',
        'monitor/stack.c'
    ]

    build_module(bld, 'monitor', sources=sources)

def build_regexp(bld):
    """
    Builds the spencer regexp library
    """

    dir = 'cexp/regexp'
    sources = [
        f'{dir}/regexp.c',
        f'{dir}/regerror.c',
        f'{dir}/regsub.c'
    ]

    bld(
        target='spencer_regexp',
        features='c cstlib',
        source=sources,
        includes=get_includes(bld)
    )

    install_libs(bld, ['libspencer_regexp.a'])
    install_headers(bld, [f'{dir}/spencer_regexp.h'])


def conf_pmbfd(conf, bsp: str):
    pass

def build_pmbfd(bld):
    """
    Builds the pmbfd library from Cexp
    """

    dir = 'cexp/pmbfd'
    pmelf_sources = [
        f'{dir}/symname.c',
        f'{dir}/secname.c',
        f'{dir}/putdat.c',
        f'{dir}/dmpgrps.c',
        f'{dir}/strm.c',
        f'{dir}/fstrm.c',
        f'{dir}/mstrm.c',
        f'{dir}/dmpsym.c',
        f'{dir}/dmpsymtab.c',
        f'{dir}/dmpshdr.c',
        f'{dir}/dmpshtab.c',
        f'{dir}/dmpehdr.c',
        f'{dir}/dmprels.c',
        f'{dir}/dmpphdr.c',
        f'{dir}/symtab.c',
        f'{dir}/findsymhdrs.c',
        f'{dir}/shtab.c',
        f'{dir}/getgrp.c',
        f'{dir}/getrel.c',
        f'{dir}/getscn.c',
        f'{dir}/getsym.c',
        f'{dir}/putsym.c',
        f'{dir}/getshdr.c',
        f'{dir}/getphdr.c',
        f'{dir}/putshdr.c',
        f'{dir}/getehdr.c',
        f'{dir}/putehdr.c',
        f'{dir}/attpbfasmatch.c',
        f'{dir}/attpbfasdestroy.c',
        f'{dir}/attpbfasread.c',
        f'{dir}/attpbfasprint.c',
        f'{dir}/attpbfaprint.c',
        f'{dir}/attpbprinttag.c',
        f'{dir}/attset.c',
        f'{dir}/attprint.c',
        f'{dir}/attvendfind.c',
        f'{dir}/guleb128.c',
        f'{dir}/getwrd.c',
        f'{dir}/att-gnu-powerpc.c',
    ]

    # TODO: if NO_64BIT
    pmelf_sources += [f'{dir}/noelf64.c']

    bld(
        target='pmelf',
        features='c cstlib',
        source=pmelf_sources,
        includes=get_includes(bld) + ['cexp/pmbfd']
    )

    install_headers(bld, [f'{dir}/pmelf.h'])

    pmbfd_sources = [
        f'{dir}/bfd.c',
        f'{dir}/opcodesup.c',
        f'{dir}/bfd-reloc-arm.c',
        f'{dir}/bfd-reloc-m68k.c',
        f'{dir}/bfd-reloc-i386.c',
        f'{dir}/bfd-reloc-powerpc.c',
        f'{dir}/bfd-reloc-sparc.c',
        f'{dir}/bfd-reloc-x86_64.c',
    ]

    bld(
        target='pmbfd',
        features='c cstlib',
        source=pmbfd_sources,
        includes=get_includes(bld)
    )

    install_libs(bld, ['libpmbfd.a', 'libpmelf.a'])


def build_tecla(bld):
    """
    Builds libtecla, which is needed for cexp
    """
    dir = 'cexp/libtecla'
    sources = [
        f'{dir}/chrqueue.c',
        f'{dir}/cplfile.c',
        f'{dir}/cplmatch.c',
        f'{dir}/direader.c',
        f'{dir}/errmsg.c',
        f'{dir}/expand.c',
        f'{dir}/freelist.c',
        f'{dir}/getline.c',
        f'{dir}/hash.c',
        f'{dir}/history.c',
        f'{dir}/homedir.c',
        f'{dir}/ioutil.c',
        f'{dir}/keytab.c',
        f'{dir}/pathutil.c',
        f'{dir}/pcache.c',
        f'{dir}/stringrp.c',
        f'{dir}/strngmem.c',
        f'{dir}/version.c',
    ]
    
    bld(
        target='tecla',
        features='c cstlib',
        source=sources,
        includes=get_includes(bld) + ['cexp']
    )
    
    bld(
        target='tecla_r',
        features='c cstlib',
        source=sources,
        includes=get_includes(bld) + ['cexp'],
        defines=['_POSIX_C_SOURCE=199506L', 'PREFER_REENTRANT']
    )

    install_headers(bld, [f'{dir}/libtecla.h'])

def build_cexp(bld):
    """
    Builds cexpsh
    """
    dir = f'cexp'
    defines = []
    libs = []
    sources = []
    includes = [dir, f'{dir}/regexp', 'cexp']
    
    # TODO: FIXME: implement this
    defines += ['PACKAGE_VERSION="6_dev"']

    # Generate the jump table generator
    bld(
        name='build_gentab',
        rule=f'cc -o ${{TGT}} ${{SRC}}',
        source=f'cexp/gentab.c',
        target=f'gentab',
    )

    # Generate the jump table
    bld(
        name='generate_jumptab',
        rule=f'./${{SRC}} -o ${{TGT}}',
        target=f'{bld.out_dir}/{bld.env.RTEMS_ARCH_BSP}/cexp/jumptab.c',
        source='gentab',
        depends_on='build_gentab',
    )
    
    defines += [
        f'CEXP_TEXT_REGION_SIZE={_get_text_seg_size(bld)}'
    ]
    
    # TODO: if use_tecla:
    if True:
        sources += [f'{dir}/teclastuff.c']
        libs += ['tecla']
        includes += [f'{dir}/libtecla']
        defines += ['USETECLA=1']
    
    # TODO: if enable_loader:
    if True:
        sources += [f'{dir}/bfdstuff.c']
        defines += ['USELOADER=1']
    elif False: # TODO: use_elfsyms:
        sources += [f'{dir}/elfsyms.c', f'{dir}/elfdlmap.c']
    else:
        sources += [f'{dir}/noloader.c']
    
    # TODO: if use_pmbfd:
    if True:
        libs += ['pmbfd']
        defines += ['USEPMBFD=1', 'USE_PMBFD=1']
        includes += [f'{dir}/pmbfd']
    libs += ['pmelf']
    
    bld(
        rule=f'bison -v -d -p cexp -o cexp/cexp.tab.c --header=cexp/cexp.tab.h ${{SRC}}',
        target=f'cexp/cexp.tab.c cexp/cexp.tab.h',
        source=f'{dir}/cexp.y'
    )

    sources += [
        f'{dir}/cexp.c',
        f'{dir}/ctyps.c',
        f'{dir}/cexpsyms.c',
        f'{dir}/vars.c',
        f'{dir}/rshload.c',
        f'{dir}/cexplock.c',
        f'{dir}/cexpmod.c',
        f'{dir}/cexp.tab.c',
        f'{dir}/cexpveneer.c',
        f'{dir}/getopt/mygetopt_r.c',
        f'{dir}/help.c',
        f'{dir}/cexpsegs.c',
        f'{dir}/cexpsegs-alloc.c',
        f'{dir}/wrap.c',
    ]
    
    bld(
        target='cexp',
        features='c cstlib',
        source=sources,
        use=libs,
        includes=includes + get_includes(bld),
        defines=defines,
        depends_on='generate_jumptab',
    )
    
    install_headers(bld, [
        f'{dir}/cexp.h',
        f'{dir}/cexpHelp.h',
        f'{dir}/ctyps.h',
        f'{dir}/cexpsyms.h'])

    install_libs(bld, ['libcexp.a'])

def build_rtems_init(bld):
    """
    Build the rtems-init package (GeSys reimplementation)
    """
    dir = f'{bld.top_dir}'
    defines = []
    includes = ['.']
    libs = ['dl']

    add_rootfs(bld, f'{dir}/rootfs')

    sources = [
        'src/getopt_s.c',
        'src/hack.c',
        'src/init.c',
        'src/net.c',
        'src/nvram.c',
        'src/rtems-config.c',
        'src/util.c',
        'src/shell.c',
        f'{bld.out_dir}/rootfs.S'
    ]
    
    if bld.env.NETWORKING_STACK == 'bsd':
        sources += ['src/net_bsd.c']
        libs += ['bsd']
    elif bld.env.NETWORKING_STACK == 'legacy':
        sources += ['src/net_legacy.c']
        libs += ['networking']

    # Other core RTEMS components
    libs += ['rtemscxx', 'rtemscpu', 'rtemsbsp']

    # rtems-net-services components
    libs += ['ntp', 'telnetd', 'tftpfs']

    libs += ['c', 'm']

    print(libs)

    bld(
        target='rtems-init',
        features='asm c cprogram',
        source=sources,
        use=libs,
        includes=includes + get_includes(bld),
        defines=defines,
        deep_inputs=True,
    )


#####################################################################
# Waf methods; must always be defined
#####################################################################

def build(bld):
    rtems.build(bld)

    # Not suppporting for now...
    if bld.env.RTEMS_BSP == 'psim':
        return

    bld.env.CFLAGS += ['-O2', '-g']
    bld.env.CPPFLAGS += ['-DHAVE_CONFIG_H=1']

    #build_miscUtils(bld)
    #build_telnetd(bld)

    build_regexp(bld)
    build_pmbfd(bld)
    build_tecla(bld)
    build_cexp(bld)

    # TODO: Needs tecla from cexp!
    #build_monitor(bld)

    # Only available for PPC
    if bld.env.RTEMS_ARCH == 'powerpc':
        build_libbspExt(bld)

    build_rtems_init(bld)

def configure(conf):
    """
    Configure the build

    Parameters
    ----------
    conf :
        Configuration context
    """
    conf.env.ROOT = ROOT

    # These will be invoked per-BSP
    lib_confs = [
        conf_pmbfd
    ]

    rtems.configure(conf, bsp_configure=lambda c, b : _bsp_configure(c, b, lib_confs))

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)

def options(opt):
    rtems.options(opt)
    opt.add_option('--ssrl-version', dest='SSRLAPPS_VER', type=str, default='ssrlApps',
                   help='ssrlApps patch level string (i.e. ssrlApps_p4)')
    opt.add_option('--enable-text-segment', dest='ENABLE_TEXT_SEGMENT', type=str, default='default',
                   help="""
        Reserve <size> space in the CEXP executable for the .text
        sections of loadable modules. This is required on powerpc
        platforms with more than 32M of memory so that no far jumps
        are needed.
        <size> may be 'no' (disable feature), 'yes' (reserve default
        [[8MB]]) or a number. If <size> is zero then the application
        must provide the following two global variables:

            unsigned long cexpTextRegionSize=<desired_size>;

            unsigned char cexpTextRegion[<desired_size>];

        defining the memory region used.
		Alternatively, the application (or the linker script) may
		provide symbols _cexpTextRegionStart/_cexpTextRegionEnd.
        This option is ignored on platforms other than powerpc.
		The default is 0.
        """)

