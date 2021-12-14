config = configuration_data()
init_func_name = '__vaDriverInit_@0@_@1@'.format(libva_major_version,
                                                 libva_minor_version)
config.set('FLU_VA_DRIVERS_DRIVER_INIT', init_func_name)
config.set('FLU_VA_DRIVERS_MAJOR_VERSION', flu_va_drivers_major_version)
config.set('FLU_VA_DRIVERS_MINOR_VERSION', flu_va_drivers_minor_version)
config.set('FLU_VA_DRIVERS_MICRO_VERSION', flu_va_drivers_micro_version)
config.set_quoted('FLU_VA_DRIVERS_VENDOR', 'Fluendo S.A.')
config.set_quoted('FLU_VA_DRIVERS_COMMERCIAL_NAME', meson.project_name())
config.set('VA_MAJOR_VERSION', libva_major_version)
config.set('VA_MINOR_VERSION', libva_minor_version)
config_file = configure_file(output: 'config.h', configuration: config)

vdpau_dep = dependency('vdpau', version : '>= 1.1.1')

if get_option('vdpau').enabled() and vdpau_dep.found()
  sources = [
    'flu_va_drivers_vdpau.c',
    'flu_va_drivers_vdpau_vdp_device_impl.c',
    'flu_va_drivers_utils.c',
    'flu_va_drivers_vdpau_utils.c',
    '../ext/intel/intel-vaapi-drivers/object_heap.c'
  ]

  headers = [
    'flu_va_drivers_vdpau.h',
    'flu_va_drivers_vdpau_vdp_device_impl.h',
    'flu_va_drivers_utils.h',
    'flu_va_drivers_vdpau_utils.h',
    '../ext/intel/intel-vaapi-drivers/object_heap.h',
    '../ext/intel/intel-vaapi-drivers/i965_mutext.h',
    '../ext/intel/intel-vaapi-drivers/intel_compiler.h'
  ]

  flu_va_drivers_vdpau_drv_video = shared_module(
    'flu_va_drivers_vdpau_drv_video',
    name_prefix : '',
    install : true,
    install_dir : libva_driver_dir,
    sources: [sources, headers, config_file],
    c_args: ['-DHAVE_CONFIG_H'],
    dependencies : [libva_dep, vdpau_dep]
  )
endif
