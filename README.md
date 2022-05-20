# flu-va-drivers

Drivers for _libva_ that act as a translation layer to VAAPI from:

 - VDPAU (default)
 - ...

In a future more drivers could be added.

# Requirements

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

# How to compile

```sh
mkdir builddir
meson builddir
ninja -C builddir
# or: meson compile -C builddir
```

# How to install

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

# How to use 

Refer to [data/doc/flu-va-drivers/README.md](data/doc/flu-va-drivers/README.md).

# Testing

## VDPAU

The project is on develop and there is no way to do a full decoding test. So by
now the way we're testing consist on try to decode using `ffmpeg` with this
driver and checking the libva trace and/or debugging it with `gdb`. Here is an
example script to do it:

```sh
#gdb="gdb --args"
url='https://www.itu.int/wftp3/av-arch/jvt-site/draft_conformance/AVCv1/CVWP1_TOSHIBA_E.zip'
test_file="CVWP1_TOSHIBA_E.264"

[ -f "$test_file" ] || wget "$url" -qO - | zcat > "$test_file"
rm -fv ./deleteme.out
rm -fv ./libva_ffmpeg*

DISPLAY=:0 \
LIBVA_TRACE=./libva_ffmpeg \
LIBVA_DRIVER_NAME=flu_va_drivers_vdpau \
LIBVA_DRIVERS_PATH=$PWD/builddir/src/ \
$gdb ffmpeg \
	-hwaccel vaapi \
	-threads 1 \
	-init_hw_device "vaapi=vaapi0:,connection_type=x11" \
	-i "$test_file" \
	-vf format=pix_fmts=yuv420p \
	-f rawvideo \
	./deleteme.out
```
