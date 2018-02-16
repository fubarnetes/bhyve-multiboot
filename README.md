# bhyve-multiboot

[![Build Status](https://travis-ci.org/fubarnetes/bhyve-multiboot.svg?branch=master)](https://travis-ci.org/fubarnetes/bhyve-multiboot)

bhyveload bootloader that aims to be multiboot compliant.

## project status
This project is under heavy development and nowhere near ready to use.

## usage

```
bhyveload -h <hostbase> -m <memsize> -l /path/to/multiboot.so [[-e <key>=<value> ]] <vmname>
```

With the following environment variables specified with -e:

| name     | optional | description                                                                     |
|----------|----------|---------------------------------------------------------------------------------|
| `kernel` | no       | Path to the kernel image relative to `hostbase`. Must start with a leading `/`. |