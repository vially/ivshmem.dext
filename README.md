# ivshmem.dext

[IVSHMEM](https://github.com/qemu/qemu/blob/master/docs/specs/ivshmem-spec.rst) DriverKit extension for macOS.

:construction: This driver is still a work in progress. Use at your own risk. :construction:

### Known issues and limitations

This driver only supports memory sharing (using BAR2). Interrupts are not currently implemented (see [IVSHMEM spec](https://github.com/qemu/qemu/blob/master/docs/specs/ivshmem-spec.rst) for details).
