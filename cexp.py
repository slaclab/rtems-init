# ----------------------------------------------------------------------------
# Company    : SLAC National Accelerator Laboratory
# ----------------------------------------------------------------------------
# Description : WAF build script for Cexpsh
# ----------------------------------------------------------------------------
# This file is part of the 'rtems-init' package. It is subject to
# the license terms in the LICENSE.txt file found in the top-level directory
# of this distribution and at:
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
# No part of the 'rtems-init' package, including this file, may be
# copied, modified, propagated, or distributed except according to the terms
# contained in the LICENSE.txt file.
# ----------------------------------------------------------------------------

def build_spencer_regexp(bld):
    bld(
        features = 'c',
        target   = 'regexp',
        includes = ['modules/cexp'],
        cflags   = ['-std=c99'], # Spencer regexp uses old-style C decls still.
        defines  = [],
        source   = [
            "modules/cexp/regexp/regexp.c",
            "modules/cexp/regexp/regerror.c",
            "modules/cexp/regexp/regsub.c",
        ]
    )

def build_tecla(bld):
    bld(
        features = 'c cstlib',
        target   = 'tecla',
        includes = ['modules/cexp'],
        defines  = ['_POSIX_C_SOURCE=199506L', 'PREFER_REENTRANT=1'],
        source   = [
            "modules/cexp/libtecla/chrqueue.c",
            "modules/cexp/libtecla/cplfile.c",
            "modules/cexp/libtecla/cplmatch.c",
            "modules/cexp/libtecla/direader.c",
            "modules/cexp/libtecla/errmsg.c",
            "modules/cexp/libtecla/expand.c",
            "modules/cexp/libtecla/freelist.c",
            "modules/cexp/libtecla/getline.c",
            "modules/cexp/libtecla/hash.c",
            "modules/cexp/libtecla/history.c",
            "modules/cexp/libtecla/homedir.c",
            "modules/cexp/libtecla/ioutil.c",
            "modules/cexp/libtecla/keytab.c",
            "modules/cexp/libtecla/pathutil.c",
            "modules/cexp/libtecla/pcache.c",
            "modules/cexp/libtecla/stringrp.c",
            "modules/cexp/libtecla/strngmem.c",
            "modules/cexp/libtecla/version.c",
        ]
    )

def build_pmelf(bld):
    bld(
        features = 'c cstlib',
        target   = 'pmelf',
        includes = ['modules/cexp/pmbfd'],
        defines  = [],
        source   = [
            "modules/cexp/pmbfd/symname.c",
            "modules/cexp/pmbfd/secname.c",
            "modules/cexp/pmbfd/putdat.c",
            "modules/cexp/pmbfd/dmpgrps.c",
            "modules/cexp/pmbfd/strm.c",
            "modules/cexp/pmbfd/fstrm.c",
            "modules/cexp/pmbfd/mstrm.c",
            "modules/cexp/pmbfd/dmpsym.c",
            "modules/cexp/pmbfd/dmpsymtab.c",
            "modules/cexp/pmbfd/dmpshdr.c",
            "modules/cexp/pmbfd/dmpshtab.c",
            "modules/cexp/pmbfd/dmpehdr.c",
            "modules/cexp/pmbfd/dmprels.c",
            "modules/cexp/pmbfd/dmpphdr.c",
            "modules/cexp/pmbfd/symtab.c",
            "modules/cexp/pmbfd/findsymhdrs.c",
            "modules/cexp/pmbfd/shtab.c",
            "modules/cexp/pmbfd/getgrp.c",
            "modules/cexp/pmbfd/getrel.c",
            "modules/cexp/pmbfd/getscn.c",
            "modules/cexp/pmbfd/getsym.c",
            "modules/cexp/pmbfd/putsym.c",
            "modules/cexp/pmbfd/getshdr.c",
            "modules/cexp/pmbfd/getphdr.c",
            "modules/cexp/pmbfd/putshdr.c",
            "modules/cexp/pmbfd/getehdr.c",
            "modules/cexp/pmbfd/putehdr.c",
            "modules/cexp/pmbfd/attpbfasmatch.c",
            "modules/cexp/pmbfd/attpbfasdestroy.c",
            "modules/cexp/pmbfd/attpbfasread.c",
            "modules/cexp/pmbfd/attpbfasprint.c",
            "modules/cexp/pmbfd/attpbfaprint.c",
            "modules/cexp/pmbfd/attpbprinttag.c",
            "modules/cexp/pmbfd/attset.c",
            "modules/cexp/pmbfd/attprint.c",
            "modules/cexp/pmbfd/attvendfind.c",
            "modules/cexp/pmbfd/guleb128.c",
            "modules/cexp/pmbfd/getwrd.c",
            "modules/cexp/pmbfd/att-gnu-powerpc.c",
            "modules/cexp/pmbfd/noelf64.c" # TODO: NO_64BIT,
        ]
    )

def build_pmbfd(bld):
    bld(
        features = 'c cstlib',
        target   = 'pmbfd',
        includes = ['modules/cexp'],
        defines  = [],
        source   = [
            "modules/cexp/pmbfd/bfd.c",
            "modules/cexp/pmbfd/opcodesup.c",
            f"modules/cexp/pmbfd/bfd-reloc-{bld.env.RTEMS_ARCH}.c",
        ]
    )


def build_cexp(bld):
    # build dependencies first
    build_spencer_regexp(bld)
    build_pmelf(bld)
    build_pmbfd(bld)

    bld(
        name    = 'gentab',
        target  = 'gentab',
        source  = ['modules/cexp/gentab.c'],
        rule    = 'gcc -o ${TGT} ${SRC}'
    )

    bld(
        name    = 'jumptab',
        target  = f'jumptab.c',
        source  = ['gentab'],
        rule    = f'{bld.out_dir}/{bld.env.RTEMS_ARCH_BSP}/gentab -o ${{TGT}}'
    )
    
    bld(
        name    = 'cexp.tab.c',
        target  = 'cexp.tab.c cexp.tab.h',
        source  = ['modules/cexp/cexp.y'],
        rule    = 'bison -v -d -p cexp -o ${TGT[0].abspath()} --defines=${TGT[1].abspath()} ${SRC}'
    )
    
    bld.add_group()

    cexp_src = [
        'cexp.tab.c',
        'modules/cexp/ctyps.c',
        'modules/cexp/cexp.c',
        'modules/cexp/cexpsyms.c',
        'modules/cexp/vars.c',
        'modules/cexp/rshload.c',
        'modules/cexp/cexplock.c',
        'modules/cexp/cexpmod.c',
        'modules/cexp/cexpveneer.c',
        'modules/cexp/getopt/mygetopt_r.c',
        'modules/cexp/help.c',
        'modules/cexp/cexpsegs.c',
        'modules/cexp/cexpsegs-alloc.c',
        'modules/cexp/wrap.c',
    ]

    cexp_defines = ['HAVE_BESTLINE=1', 'PACKAGE_VERSION="7_dev"']
    cexp_libs = ['regexp']

    cexp_inc = [
        'modules/cexp',
        f'{bld.out_dir}/{bld.env.RTEMS_ARCH_BSP}',
        'modules/cexp/pmbfd',
        'modules/cexp/regexp',
    ]

    cexp_src += [
        'modules/cexp/bestline/bestline.c',
    ]

    # Configuration for the text segment
    if bld.options.cexp_text_section == 'auto':
        if bld.env.RTEMS_ARCH in ['powerpc', 'arm']:
            cexp_defines += [f'CEXP_TEXT_REGION_SIZE=0']
    else:
        cexp_defines += [
            f'CEXP_TEXT_REGION_SIZE={hex(int(bld.options.cexp_text_section))}'
        ]

    if bld.options.cexp_loader != 'none':
        cexp_defines += [
            "USELOADER=1",
            "USE_LOADER=1",
        ]

    # Determine the loader to use
    if bld.options.cexp_loader == 'bfd':
        cexp_defines += [
            "USEPMBFD=1",
            "USE_PMBFD=1",
        ]
        cexp_src += [
            "modules/cexp/bfdstuff.c"
        ]
        cexp_libs += ['pmbfd', 'pmelf']
        cexp_inc += ['modules/cexp/pmbfd']
    elif bld.options.cexp_loader == 'rtl':
        cexp_defines += [
            'USE_RTL=1',
        ]
        cexp_src += [
            'modules/cexp/cexprtl.c'
        ]

    cexp = bld(
        features   = 'c cstlib',
        target     = 'cexp',
        includes   = cexp_inc,
        defines    = cexp_defines,
        cflags     = ['-std=gnu17'], # Forcing C17 due to compile problems in jumptab.c
        depends_on = ['jumptab.c'],
        source     = cexp_src,
        use        = cexp_libs,
    )