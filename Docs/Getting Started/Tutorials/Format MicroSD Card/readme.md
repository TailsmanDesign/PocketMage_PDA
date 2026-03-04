# PocketMage SD Card Formatting Guide

**WARNING**  
These steps will ERASE the SD card. Back up your data first.

This guide ensures SD cards are formatted in a way that is compatible with the PocketMage device.

---

## Supported SD Card Requirements

- **Card size:** 2 GB – 32 GB microSD
- **Filesystem:** FAT32
- **Partition scheme:** MBR (Master Boot Record)
- **Partitions:** Single primary partition
- **Cluster (allocation unit) size:** 32 KB
- **Sector size:** 512 bytes (default)

X exFAT is NOT supported  
X GPT partition tables are NOT supported  
X 64 GB+ cards are NOT recommended unless you explicitly force FAT32 + MBR

---

## Windows (Recommended)

### Step 1 — Open diskpart

1. Insert the SD card  
2. Open **Command Prompt as Administrator**  
3. Run:
diskpart

### Step 2 — Identify the SD card
list disk

Find the disk number that matches your SD card size.

**Make sure you select the correct disk.**

### Step 3 — Format the card (MBR + FAT32)
select disk X
clean
convert mbr
create partition primary
format fs=fat32 unit=32k quick
assign
exit

### Step 4 — Verify (optional)

- Open **Disk Management**
- Right-click the disk → Properties → Volumes
- Partition style must say **Master Boot Record (MBR)**

---

## macOS (Terminal — NOT Disk Utility)

macOS Disk Utility often creates GPT layouts that do not work.

### Step 1 — Identify the SD card
diskutil list

Find the disk number (example: disk2).

### Step 2 — Erase and create MBR + FAT32
diskutil eraseDisk FAT32 PM_SD MBRFormat /dev/disk2

### Step 3 — Force 32 KB cluster size
sudo newfs_msdos -F 32 -c 64 /dev/disk2s1

Explanation:  
-c 64 = 64 × 512-byte sectors = 32 KB clusters

### Step 4 — Verify
diskutil info /dev/disk2 | grep "Partition Map"

Must say:
Partition Map Scheme: Master Boot Record

---

## Linux

### Step 1 — Identify the SD card
lsblk

Example device: /dev/sdb

### Step 2 — Remove existing partition data
sudo wipefs -a /dev/sdb

### Step 3 — Create MBR partition table
sudo parted /dev/sdb --script mklabel msdos
sudo parted /dev/sdb --script mkpart primary fat32 1MiB 100%

### Step 4 — Format FAT32 with 32 KB clusters
sudo mkfs.fat -F 32 -s 64 /dev/sdb1

### Step 5 — Verify
lsblk -o NAME,SIZE,PTTYPE

PTTYPE must be dos.

---

## Final Checklist

Before using the card in PocketMage, confirm:

- FAT32 filesystem
- MBR partition table (not GPT)
- One primary partition
- 32 KB cluster size
- Card size ≤32 GB

If any of these are incorrect, the card may not be detected.

---

If issues persist after following this guide, the SD card itself may be incompatible.
