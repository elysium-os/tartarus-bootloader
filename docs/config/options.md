# Tartarus Configuration Options

All configuration problems are fatal and will cause a runtime panic. Option paths below use dots to show nesting (e.g. `framebuffer.width`).

## Top-level options

### `boot_entries`

- **Type:** object
- **Required:** yes

The set of bootable entries. Each field is one entry: the field name is the entry's name and the value is an object of entry options (see [Boot entry options](#boot-entry-options)).

At least one entry must be defined. Entries are kept in the order they are written, and the first one is selected initially.

### `framebuffer`

- **Type:** object
- **Required:** no

Framebuffer settings. If omitted, all framebuffer options take their defaults.

## Framebuffer options

### `framebuffer.width`

- **Type:** integer
- **Required:** no
- **Default:** `1920`
- **Constraints:** must be greater than 0

Framebuffer width in pixels.

### `framebuffer.height`

- **Type:** integer
- **Required:** no
- **Default:** `1080`
- **Constraints:** must be greater than 0

Framebuffer height in pixels.

### `framebuffer.strict_rgb`

- **Type:** boolean
- **Required:** no
- **Default:** `false`

Whether the framebuffer must use an RGB pixel format.

## Boot entry options

These go inside each entry in `boot_entries`, e.g. `boot_entries.xyz.kernel`.

### `protocol`

- **Type:** string
- **Required:** yes
- **Allowed values:** `"tartarus"` (case-insensitive)

The boot protocol used to hand control to the kernel. It determines which protocol-specific options are available. Any other value is an error.

### `kernel`

- **Type:** string
- **Required:** yes

Path to the kernel image to boot (e.g. `"kernel.elf"`).

## Tartarus protocol options

Available when `protocol = "tartarus"`.

### `smp`

- **Type:** boolean
- **Required:** no
- **Default:** `true`

Whether to enable SMP, i.e. bring up the additional CPU cores. Set to `false` to boot on a single core.

### `modules`

- **Type:** array of strings
- **Required:** no
- **Default:** empty (no modules)

Paths of additional files to load alongside the kernel and pass to it, in the order listed (e.g. `"initramfs.tar.gz"`). Every item must be a string.
