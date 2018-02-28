# bhyve-multiboot

[![Build Status](https://travis-ci.org/fubarnetes/bhyve-multiboot.svg?branch=master)](https://travis-ci.org/fubarnetes/bhyve-multiboot)
[![codecov](https://codecov.io/gh/fubarnetes/bhyve-multiboot/branch/master/graph/badge.svg)](https://codecov.io/gh/fubarnetes/bhyve-multiboot)
 [![Gitter chat](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/fubarnetes/Lobby)

bhyveload bootloader that aims to be multiboot compliant.

## project status
This project is under heavy development and nowhere near ready to use.

## usage

```
bhyveload -h <hostbase> -m <memsize> -l /path/to/multiboot.so [[-e <key>=<value> ]] <vmname>
```

With the following environment variables specified with -e:

| name      | optional | description                                                                     |
|-----------|----------|---------------------------------------------------------------------------------|
| `kernel`  | no       | Path to the kernel image relative to `hostbase`. Must start with a leading `/`. |
| `cmdline` | yes      | command line to pass to the kernel                                              |
| `module`  | yes      | Path to a module relative to `hostbase` and optional string to be passed in the format `module=/<filename>[:string]`<br/>Must start with a leading `/`. May be specified multiple times to load multiple modules. |