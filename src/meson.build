sources = [
  'flu_va_drivers_vdpau.c',
]

headers = [
  'flu_va_drivers_vdpau.h',
]

config = configuration_data()
init_func_name = '__vaDriverInit_@0@_@1@'.format(libva_major_version,
                                                 libva_minor_version)
config.set('FLU_VA_DRIVERS_DRIVER_INIT', init_func_name)
config_file = configure_file(output: 'config.h', configuration: config)

flu_va_drivers_vdpau_drv_video = shared_module(
  'flu_va_drivers_vdpau_drv_video',
  name_prefix : '',
  install : true,
  install_dir : libva_driver_dir,
  sources: [sources, headers, config_file],
  c_args: ['-DHAVE_CONFIG_H'],
  dependencies : [libva_dep]
)
