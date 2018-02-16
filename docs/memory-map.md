# bhyve memory mapping

from [libvmmapi.c](https://github.com/freebsd/freebsd/blob/260ba0bff18bb32b01216d6870c8273cf22246a7/lib/libvmmapi/vmmapi.c#L360):
~~~c
/*
 * If 'memsize' cannot fit entirely in the 'lowmem' segment then
 * create another 'highmem' segment above 4GB for the remainder.
 */
if (memsize > ctx->lowmem_limit) {
    ctx->lowmem = ctx->lowmem_limit;
    ctx->highmem = memsize - ctx->lowmem_limit;
    objsize = 4*GB + ctx->highmem;
} else {
    ctx->lowmem = memsize;
    ctx->highmem = 0;
    objsize = ctx->lowmem;
}
~~~

The `lowmem` segment [ends at most at 3 GiB](https://github.com/freebsd/freebsd/blob/260ba0bff18bb32b01216d6870c8273cf22246a7/lib/libvmmapi/vmmapi.c#L374)
unless specified otherwise using [`vm_set_lowmem_limit`](https://github.com/freebsd/freebsd/blob/260ba0bff18bb32b01216d6870c8273cf22246a7/lib/libvmmapi/vmmapi.c#L177-L181),
which `bhyveload` does not do.

The resulting memory map on boot looks like this (see also [pci_emul.c](https://github.com/freebsd/freebsd/blob/35326d3159b53afb3e64a9926a953b32e27852c9/usr.sbin/bhyve/pci_emul.c#L1151-L1157):

| at    | end               | usable                                                              |
| ----- | ----------------- | --------------------------------------------------------------------|
| 0     | 640 KiB           | usable                                                              |
| 640k  | 1 MiB             | VGA hole                                                            |
| 1 MiB | `lowmem`          | usable                                                              |
| 3 GiB | 4 GiB             | PCI hole, PCI extended config window, LAPIC, IOAPIC, HPET, firmware |
| 4 GiB | 4 GiB + `highmem` | usable (absent if < 3 GiB are mapped.)                              |

`lowmem` and `highmem` can be queried using the [get_mem callback](https://github.com/freebsd/freebsd/blob/260ba0bff18bb32b01216d6870c8273cf22246a7/usr.sbin/bhyveload/bhyveload.c#L505-L511):
~~~c
static void
cb_getmem(void *arg, uint64_t *ret_lowmem, uint64_t *ret_highmem)
{

	*ret_lowmem = vm_get_lowmem_size(ctx);
	*ret_highmem = vm_get_highmem_size(ctx);
}
~~~