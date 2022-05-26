project(
  'flu-va-drivers',
  'c',
  version : '0.0.2',
  meson_version : '>= 0.46.0'
)

python = import('python').find_installation()
script_tmpl = 'import shutil; shutil.copy("@0@", "@1@")'
run_command(python, '-c',
            script_tmpl.format('common/hooks/pre-commit.hook', '.git/hooks/pre-commit'))

fs = import('fs')
if not fs.is_file('common/.git')
  error('The submodule "common" is missing. Try "git submodule update --init"')
endif

libva_version = '>= 1.10.0'
libva_dep = dependency('libva', version : libva_version)

libva_driver_dir = get_option('driverdir')
if libva_driver_dir == ''
  libva_driver_dir = libva_dep.get_pkgconfig_variable('driverdir')
endif

libva_version_array = libva_dep.version().split('.')
libva_major_version = libva_version_array[0]
libva_minor_version = libva_version_array[1]

datadir = get_option('datadir')

subdir('src')
subdir('data')
