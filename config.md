# Tartarus Config File (TCFG)

## Format

The TCFG should be encoded in ASCII and follow the format described below:

- The file consists of key-value pairs on every line.
- Each key-value pair should be separated by an equal sign ( `=` ).
- Both spaces and tabs are considered whitespace.
- Certain keys can appear many times for multiple values.
- Valid values are:
    - Strings (ex. `"hello world"`)
    - Numbers (ex. `420`)
    - Booleans (ex. `false`)

Example:

```
a_key="somestring"
somekey = "A string separated by spaces"
anotherkey = 432141
awesomebool = true
```

## Location

Tartarus will search every FAT12/16/32 partition of every disk at the following locations:

- `/tartarus.cfg`

## Options

| PROTOCOL   | Key           | Value                   | Required | Default  | Description                                                                                                              |
| ---------- | ------------- | ----------------------- | -------- | -------- | ------------------------------------------------------------------------------------------------------------------------ |
|            | kernel        | string                  | Yes      |          | The path of the kernel file.                                                                                             |
|            | protocol      | `"tartarus"`, `"linux"` | Yes      |          | Which boot protocol tartarus should use to boot the kernel.                                                              |
|            | fb            | boolean                 | No       | `true`   | Whether to retrieve a framebuffer.                                                                                       |
|            | fb_width      | number                  | No       | `1920`   | Preferred framebuffer width.                                                                                             |
|            | fb_height     | number                  | No       | `1080`   | Preferred framebuffer height.                                                                                            |
|            | fb_strict_rgb | boolean                 | No       | `false`  | Only retrieve a framebuffer with RGBX8 format.                                                                           |
| `linux`    | cmd           | string                  | No       | `"auto"` | Commandline which is passed to the linux kernel.                                                                         |
| `linux`    | initrd        | string                  | Yes      |          | Path of the initial ramdisk to load.                                                                                     |
| `tartarus` | module        | string                  | No       |          | Path to a file which will be loaded as a module. It is possible to define this key multiple times for different modules. |
| `tartarus` | find_rsdp     | string                  | No       | `true`   | Whether to retrieve the RSDP.                                                                                            |
| `tartarus` | smp           | boolean                 | No       | `true`   | Initialize appliocation processors.                                                                                      |

### Path

Paths consist of file/directory names separated by slashes (`/`). The names should only contains alphanumeric characters (`a-zA-Z0-9`). Example: `/sys/kernel.elf`. Currently this path is relative to the partition the config is found on.
