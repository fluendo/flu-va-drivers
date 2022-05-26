# flu-va-drivers

This package includes drivers for _libva_ that act as a translation layer from
VA-API to VDPAU.

# Requirements

The following dependencies are needed:

  - libva >= 2.10.0 (VA-API >= 1.10)

# How to install

## Tarballs

Copy the generated *flu_va_drivers_vdpau_drv_video.so* file (in *builddir/src*)
to your libva driver directory (given by `pkg-config --variable=driverdir libva`)

## Deb packages

```sh
sudo apt install flu-va-drivers-vdpau_x.y.z-1_amd64.deb
```

# How to use

Ensure to set the environment variable `LIBVA_DRIVER_NAME=flu_va_drivers_vdpau`
and `LIBVA_DRIVERS_PATH` to point to the path of where the
*flu_va_drivers_vdpau_drv_video.so* file is located.

## Google Chrome (Chromium)

In order to get Google Chrome using this project, you have to run Google Chrome
as following:
```sh
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$(pkg-config --variable=prefix libva) \
google-chrome --enable-features=VaapiVideoDecoder --ignore-gpu-blocklist --use-gl=desktop
```

Note, in latest versions of Google Chrome, also add:
`--disable-features=UseChromeOSDirectVideoDecoder`

In the case of Chromium, the same arguments have to be provided, but also you
have to ensure to have compiled Chromium with vaapi support, adding the
following to your `gn.args` Chromium configuration file:

```
use_vaapi=true
proprietary_codecs=true
ffmpeg_branding="Chrome"
```

For more information, see [the official documentation for VA-API support in
Chromium](https://chromium.googlesource.com/chromium/src/+/master/docs/gpu/vaapi.md)

