#most of this stuff is copied shamelessly from the enlightenment meson.build
project('expose', 'c',
     version: '0.1',
     license: 'BSD 2 clause',
     default_options: [ 'c_std=gnu99', 'warning_level=2' ],
     meson_version: '>= 0.40.0')

cc = meson.get_compiler('c')

host_os = host_machine.system()
 if host_os == 'linux'
     if cc.has_header_symbol('features.h', '__UCLIBC__')
         host_os = 'linux-uclibc'
     elif cc.has_header_symbol('features.h', '__dietlibc__')
         host_os = 'linux-dietlibc'
     else
         host_os = 'linux-gnu'
     endif
 endif

module_arch = '@0@-@1@-@2@'.format(host_os, host_machine.cpu_family(), 'ver-0.22')

dep_e = dependency('enlightenment')


dir_module_e = '/lib/enlightenment/modules'

dir_mod=get_option('prefix') + dir_module_e

message('installing to ' + dir_mod)





src = [
  'e_mod_main.c',
  'e_int_config_winlist.c',
  'expose_winlist.c',
  'e_mod_main.h',
  'expose_winlist.h'
]


icon = [
  'e-module-expose.edj',
  'module.desktop'
]


install_dir = join_paths( dir_mod, 'expose' )
dir_mod_bin = join_paths( install_dir, module_arch)

#if get_option(opt) == true

  install_data(icon, install_dir: install_dir)

  shared_module('module', src,
                #include_directories: include_directories(module_includes),
                name_prefix        : '',
                #dependencies       : module_deps,
				dependencies		: dep_e,
                install_dir        : dir_mod_bin,
                install            : true
               )
#endif
