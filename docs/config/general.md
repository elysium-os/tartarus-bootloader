# Location

Tartarus will search every FAT12/16/32 partition of every disk at the following locations:

- `/tartarus.cfg`

# Format

The configuration format is documented in the [format.md](./format.md) file.

# Paths

Paths consist of file/directory names separated by slashes (`/`). The names should only contains alphanumeric characters (`a-zA-Z0-9`). Example: `/sys/kernel.elf`. Currently this path is relative to the partition the config is found on.

# Options

The accepted options are documented in the [options.md](./options.md) file.

# Example

```
framebuffer = {
    strict_rgb = true
    width = 1200
    height = 600
}

boot_entries = {
    cronus = {
        protocol = "tartarus"
        kernel = "kernel.elf"
        modules = ["initramfs.tar.gz"]
    }
}
```
