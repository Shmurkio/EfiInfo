# EfiInfo

Small PoC for locating `EFI_SYSTEM_TABLE` with a kernel driver.

## How It Works

The driver scans the first 4 GiB of physical memory for a potential `EFI_SYSTEM_TABLE`, validates candidates using their signature, header fields, and checksum, then prints basic information about the table through kernel debug output.

## Note

Some EFI structures and configuration table entries may no longer be usable after Windows takes control, as firmware memory can be reclaimed or remapped.
