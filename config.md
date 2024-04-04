# Tartarus Config File (TCF)

## Format
The TCF should be encoded in ASCII and follow the format described below:
- The file consists of key-value pairs on every line.
- Each key-value pair should be separated by an equal sign ( `=` ).
- Both spaces and tabs are allowed. If they are present at the beginning or end of a token, they will be stripped.

Example:
```
a_key=somestring
somekey = A string separated by spaces
anotherkey = 432141
awesomebool = true
```

## Location
Tartarus will search every FAT12/16/32 partition of every disk at the following locations:
- `/tartarus.cfg`

## Options
|PROTOCOL|Key|Value|Required|Description|
|-|-|-|-|-|
||KERNEL|PATH|Yes|The path of the kernel file.|
||PROTOCOL|`tartarus`, `linux`|Yes|Which boot protocol tartarus should use to boot the kernel.|
||SCRW|NUMBER|No|Preferred framebuffer width.|
||SCRH|NUMBER|No|Preferred framebuffer height.|
|`tartarus`|MODULE|PATH|No|Path to a file which will be loaded as a module. It is possible to define this key multiple times for different modules.|
|`tartarus`|SMP|BOOL|No|Initialize APs (Appliocation Processor)|
|`linux`|CMD|STRING|Yes|Commandline which is passed to the linux kernel|
|`linux`|INITRD|PATH|Yes|Initial ramdisk to load|

### Path
Paths consist of file/directory names separated by slashes (`/`). The names should only contains alphanumeric characters (`a-zA-Z0-9`). Example: `/sys/kernel.elf`. Currently this path is relative to the partition the config is found on.