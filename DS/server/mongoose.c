// Copyright (c) 2004-2013 Sergey Lyubka
// Copyright (c) 2013-2024 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see http://www.gnu.org/licenses/
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in https://www.mongoose.ws/licensing/
//
// SPDX-License-Identifier: GPL-2.0-only or commercial

#include "mongoose.h"

#ifdef MG_ENABLE_LINES
#  line 1 "src/base64.c"
#endif

static int
mg_base64_encode_single (int c)
{
  if (c < 26)
    {
      return c + 'A';
    }
  else if (c < 52)
    {
      return c - 26 + 'a';
    }
  else if (c < 62)
    {
      return c - 52 + '0';
    }
  else
    {
      return c == 62 ? '+' : '/';
    }
}

static int
mg_base64_decode_single (int c)
{
  if (c >= 'A' && c <= 'Z')
    {
      return c - 'A';
    }
  else if (c >= 'a' && c <= 'z')
    {
      return c + 26 - 'a';
    }
  else if (c >= '0' && c <= '9')
    {
      return c + 52 - '0';
    }
  else if (c == '+')
    {
      return 62;
    }
  else if (c == '/')
    {
      return 63;
    }
  else if (c == '=')
    {
      return 64;
    }
  else
    {
      return -1;
    }
}

size_t
mg_base64_update (unsigned char ch, char *to, size_t n)
{
  unsigned long rem = (n & 3) % 3;
  if (rem == 0)
    {
      to[n] = (char) mg_base64_encode_single (ch >> 2);
      to[++n] = (char) ((ch & 3) << 4);
    }
  else if (rem == 1)
    {
      to[n] = (char) mg_base64_encode_single (to[n] | (ch >> 4));
      to[++n] = (char) ((ch & 15) << 2);
    }
  else
    {
      to[n] = (char) mg_base64_encode_single (to[n] | (ch >> 6));
      to[++n] = (char) mg_base64_encode_single (ch & 63);
      n++;
    }
  return n;
}

size_t
mg_base64_final (char *to, size_t n)
{
  size_t saved = n;
  // printf("---[%.*s]\n", n, to);
  if (n & 3)
    n = mg_base64_update (0, to, n);
  if ((saved & 3) == 2)
    n--;
  // printf("    %d[%.*s]\n", n, n, to);
  while (n & 3)
    to[n++] = '=';
  to[n] = '\0';
  return n;
}

size_t
mg_base64_encode (const unsigned char *p, size_t n, char *to, size_t dl)
{
  size_t i, len = 0;
  if (dl > 0)
    to[0] = '\0';
  if (dl < ((n / 3) + (n % 3 ? 1 : 0)) * 4 + 1)
    return 0;
  for (i = 0; i < n; i++)
    len = mg_base64_update (p[i], to, len);
  len = mg_base64_final (to, len);
  return len;
}

size_t
mg_base64_decode (const char *src, size_t n, char *dst, size_t dl)
{
  const char *end = src == NULL ? NULL : src + n; // Cannot add to NULL
  size_t len = 0;
  if (dl < n / 4 * 3 + 1)
    goto fail;
  while (src != NULL && src + 3 < end)
    {
      int a = mg_base64_decode_single (src[0]),
	  b = mg_base64_decode_single (src[1]),
	  c = mg_base64_decode_single (src[2]),
	  d = mg_base64_decode_single (src[3]);
      if (a == 64 || a < 0 || b == 64 || b < 0 || c < 0 || d < 0)
	{
	  goto fail;
	}
      dst[len++] = (char) ((a << 2) | (b >> 4));
      if (src[2] != '=')
	{
	  dst[len++] = (char) ((b << 4) | (c >> 2));
	  if (src[3] != '=')
	    dst[len++] = (char) ((c << 6) | d);
	}
      src += 4;
    }
  dst[len] = '\0';
  return len;
fail:
  if (dl > 0)
    dst[0] = '\0';
  return 0;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/device_ch32v307.c"
#endif

#if MG_DEVICE == MG_DEVICE_CH32V307
// RM: https://www.wch-ic.com/downloads/CH32FV2x_V3xRM_PDF.html

#  define FLASH_BASE 0x40022000
#  define FLASH_ACTLR (FLASH_BASE + 0)
#  define FLASH_KEYR (FLASH_BASE + 4)
#  define FLASH_OBKEYR (FLASH_BASE + 8)
#  define FLASH_STATR (FLASH_BASE + 12)
#  define FLASH_CTLR (FLASH_BASE + 16)
#  define FLASH_ADDR (FLASH_BASE + 20)
#  define FLASH_OBR (FLASH_BASE + 28)
#  define FLASH_WPR (FLASH_BASE + 32)

void *
mg_flash_start (void)
{
  return (void *) 0x08000000;
}
size_t
mg_flash_size (void)
{
  return 480 * 1024; // First 320k is 0-wait
}
size_t
mg_flash_sector_size (void)
{
  return 4096;
}
size_t
mg_flash_write_align (void)
{
  return 4;
}
int
mg_flash_bank (void)
{
  return 0;
}
void
mg_device_reset (void)
{
  *((volatile uint32_t *) 0xbeef0000) |= 1U << 7; // NVIC_SystemReset()
}
static void
flash_unlock (void)
{
  static bool unlocked;
  if (unlocked == false)
    {
      MG_REG (FLASH_KEYR) = 0x45670123;
      MG_REG (FLASH_KEYR) = 0xcdef89ab;
      unlocked = true;
    }
}
static void
flash_wait (void)
{
  while (MG_REG (FLASH_STATR) & MG_BIT (0))
    (void) 0;
}

bool
mg_flash_erase (void *addr)
{
  // MG_INFO(("%p", addr));
  flash_unlock ();
  flash_wait ();
  MG_REG (FLASH_ADDR) = (uint32_t) addr;
  MG_REG (FLASH_CTLR) |= MG_BIT (1) | MG_BIT (6); // PER | STRT;
  flash_wait ();
  return true;
}

static bool
is_page_boundary (const void *addr)
{
  uint32_t val = (uint32_t) addr;
  return (val & (mg_flash_sector_size () - 1)) == 0;
}

bool
mg_flash_write (void *addr, const void *buf, size_t len)
{
  // MG_INFO(("%p %p %lu", addr, buf, len));
  // mg_hexdump(buf, len);
  flash_unlock ();
  const uint16_t *src = (uint16_t *) buf, *end = &src[len / 2];
  uint16_t *dst = (uint16_t *) addr;
  MG_REG (FLASH_CTLR) |= MG_BIT (0); // Set PG
  // MG_INFO(("CTLR: %#lx", MG_REG(FLASH_CTLR)));
  while (src < end)
    {
      if (is_page_boundary (dst))
	mg_flash_erase (dst);
      *dst++ = *src++;
      flash_wait ();
    }
  MG_REG (FLASH_CTLR) &= ~MG_BIT (0); // Clear PG
  return true;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/device_dummy.c"
#endif

#if MG_DEVICE == MG_DEVICE_NONE
void *
mg_flash_start (void)
{
  return NULL;
}
size_t
mg_flash_size (void)
{
  return 0;
}
size_t
mg_flash_sector_size (void)
{
  return 0;
}
size_t
mg_flash_write_align (void)
{
  return 0;
}
int
mg_flash_bank (void)
{
  return 0;
}
bool
mg_flash_erase (void *location)
{
  (void) location;
  return false;
}
bool
mg_flash_swap_bank (void)
{
  return true;
}
bool
mg_flash_write (void *addr, const void *buf, size_t len)
{
  (void) addr, (void) buf, (void) len;
  return false;
}
void
mg_device_reset (void){}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/device_flash.c"
#endif

#if MG_DEVICE == MG_DEVICE_STM32H7 || MG_DEVICE == MG_DEVICE_STM32H5          \
    || MG_DEVICE == MG_DEVICE_RT1020 || MG_DEVICE == MG_DEVICE_RT1060
// Flash can be written only if it is erased. Erased flash is 0xff (all bits 1)
// Writes must be mg_flash_write_align() - aligned. Thus if we want to save an
// object, we pad it at the end for alignment.
//
// Objects in the flash sector are stored sequentially:
// | 32-bit size | 32-bit KEY | ..data.. | ..pad.. | 32-bit size | ......
//
// In order to get to the next object, read its size, then align up.

// Traverse the list of saved objects
size_t mg_flash_next (char *p, char *end, uint32_t *key, size_t *size)
{
  size_t aligned_size = 0, align = mg_flash_write_align (), left = end - p;
  uint32_t *p32 = (uint32_t *) p, min_size = sizeof (uint32_t) * 2;
  if (p32[0] != 0xffffffff && left > MG_ROUND_UP (min_size, align))
    {
      if (size)
	*size = (size_t) p32[0];
      if (key)
	*key = p32[1];
      aligned_size = MG_ROUND_UP (p32[0] + sizeof (uint32_t) * 2, align);
      if (left < aligned_size)
	aligned_size = 0; // Out of bounds, fail
    }
  return aligned_size;
}

// Return the last sector of Bank 2
static char *
flash_last_sector (void)
{
  size_t ss = mg_flash_sector_size (), size = mg_flash_size ();
  char *base = (char *) mg_flash_start (), *last = base + size - ss;
  if (mg_flash_bank () == 2)
    last -= size / 2;
  return last;
}

// Find a saved object with a given key
bool
mg_flash_load (void *sector, uint32_t key, void *buf, size_t len)
{
  char *base = (char *) mg_flash_start (), *s = (char *) sector, *res = NULL;
  size_t ss = mg_flash_sector_size (), ofs = 0, n, sz;
  bool ok = false;
  if (s == NULL)
    s = flash_last_sector ();
  if (s < base || s >= base + mg_flash_size ())
    {
      MG_ERROR (("%p is outsize of flash", sector));
    }
  else if (((s - base) % ss) != 0)
    {
      MG_ERROR (("%p is not a sector boundary", sector));
    }
  else
    {
      uint32_t k, scanned = 0;
      while ((n = mg_flash_next (s + ofs, s + ss, &k, &sz)) > 0)
	{
	  // MG_DEBUG((" > obj %lu, ofs %lu, key %x/%x", scanned, ofs, k,
	  // key)); mg_hexdump(s + ofs, n);
	  if (k == key && sz == len)
	    {
	      res = s + ofs + sizeof (uint32_t) * 2;
	      memcpy (buf, res, len); // Copy object
	      ok = true; // Keep scanning for the newer versions of it
	    }
	  ofs += n, scanned++;
	}
      MG_DEBUG (("Scanned %u objects, key %x is @ %p", scanned, key, res));
    }
  return ok;
}

// For all saved objects in the sector, delete old versions of objects
static void
mg_flash_sector_cleanup (char *sector)
{
  // Buffer all saved objects into an IO buffer (backed by RAM)
  // erase sector, and re-save them.
  struct mg_iobuf io = { 0, 0, 0, 2048 };
  size_t ss = mg_flash_sector_size ();
  size_t n, size, size2, ofs = 0, hs = sizeof (uint32_t) * 2;
  uint32_t key;
  // Traverse all objects
  MG_DEBUG (("Cleaning up sector %p", sector));
  while ((n = mg_flash_next (sector + ofs, sector + ss, &key, &size)) > 0)
    {
      // Delete an old copy of this object in the cache
      for (size_t o = 0; o < io.len; o += size2 + hs)
	{
	  uint32_t k = *(uint32_t *) (io.buf + o + sizeof (uint32_t));
	  size2 = *(uint32_t *) (io.buf + o);
	  if (k == key)
	    {
	      mg_iobuf_del (&io, o, size2 + hs);
	      break;
	    }
	}
      // And add the new copy
      mg_iobuf_add (&io, io.len, sector + ofs, size + hs);
      ofs += n;
    }
  // All objects are cached in RAM now
  if (mg_flash_erase (sector))
    { // Erase sector. If successful,
      for (ofs = 0; ofs < io.len; ofs += size + hs)
	{ // Traverse cached objects
	  size = *(uint32_t *) (io.buf + ofs);
	  key = *(uint32_t *) (io.buf + ofs + sizeof (uint32_t));
	  mg_flash_save (sector, key, io.buf + ofs + hs,
			 size); // Save to flash
	}
    }
  mg_iobuf_free (&io);
}

// Save an object with a given key - append to the end of an object list
bool
mg_flash_save (void *sector, uint32_t key, const void *buf, size_t len)
{
  char *base = (char *) mg_flash_start (), *s = (char *) sector;
  size_t ss = mg_flash_sector_size (), ofs = 0, n;
  bool ok = false;
  if (s == NULL)
    s = flash_last_sector ();
  if (s < base || s >= base + mg_flash_size ())
    {
      MG_ERROR (("%p is outsize of flash", sector));
    }
  else if (((s - base) % ss) != 0)
    {
      MG_ERROR (("%p is not a sector boundary", sector));
    }
  else
    {
      char ab[mg_flash_write_align ()]; // Aligned write block
      uint32_t hdr[2] = { (uint32_t) len, key };
      size_t needed = sizeof (hdr) + len;
      size_t needed_aligned = MG_ROUND_UP (needed, sizeof (ab));
      while ((n = mg_flash_next (s + ofs, s + ss, NULL, NULL)) > 0)
	ofs += n;

      // If there is not enough space left, cleanup sector and re-eval ofs
      if (ofs + needed_aligned >= ss)
	{
	  mg_flash_sector_cleanup (s);
	  ofs = 0;
	  while ((n = mg_flash_next (s + ofs, s + ss, NULL, NULL)) > 0)
	    ofs += n;
	}

      if (ofs + needed_aligned <= ss)
	{
	  // Enough space to save this object
	  if (sizeof (ab) < sizeof (hdr))
	    {
	      // Flash write granularity is 32 bit or less, write with no
	      // buffering
	      ok = mg_flash_write (s + ofs, hdr, sizeof (hdr));
	      if (ok)
		mg_flash_write (s + ofs + sizeof (hdr), buf, len);
	    }
	  else
	    {
	      // Flash granularity is sizeof(hdr) or more. We need to save in
	      // 3 chunks: initial block, bulk, rest. This is because we have
	      // two memory chunks to write: hdr and buf, on aligned
	      // boundaries.
	      n = sizeof (ab) - sizeof (hdr); // Initial chunk that we write
	      if (n > len)
		n = len;		      // is
	      memset (ab, 0xff, sizeof (ab)); // initialized to all-one
	      memcpy (ab, hdr,
		      sizeof (hdr)); // contains the header (key + size)
	      memcpy (ab + sizeof (hdr), buf, n); // and an initial part of buf
	      MG_INFO (("saving initial block of %lu", sizeof (ab)));
	      ok = mg_flash_write (s + ofs, ab, sizeof (ab));
	      if (ok && len > n)
		{
		  size_t n2 = MG_ROUND_DOWN (len - n, sizeof (ab));
		  if (n2 > 0)
		    {
		      MG_INFO (("saving bulk, %lu", n2));
		      ok = mg_flash_write (s + ofs + sizeof (ab),
					   (char *) buf + n, n2);
		    }
		  if (ok && len > n)
		    {
		      size_t n3 = len - n - n2;
		      if (n3 > sizeof (ab))
			n3 = sizeof (ab);
		      memset (ab, 0xff, sizeof (ab));
		      memcpy (ab, (char *) buf + n + n2, n3);
		      MG_INFO (("saving rest, %lu", n3));
		      ok = mg_flash_write (s + ofs + sizeof (ab) + n2, ab,
					   sizeof (ab));
		    }
		}
	    }
	  MG_DEBUG (("Saved %lu/%lu bytes @ %p, key %x: %d", len,
		     needed_aligned, s + ofs, key, ok));
	  MG_DEBUG (
	      ("Sector space left: %lu bytes", ss - ofs - needed_aligned));
	}
      else
	{
	  MG_ERROR (("Sector is full"));
	}
    }
  return ok;
}
#else
bool
mg_flash_save (void *sector, uint32_t key, const void *buf, size_t len)
{
  (void) sector, (void) key, (void) buf, (void) len;
  return false;
}
bool
mg_flash_load (void *sector, uint32_t key, void *buf, size_t len)
{
  (void) sector, (void) key, (void) buf, (void) len;
  return false;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/device_imxrt.c"
#endif

#if MG_DEVICE == MG_DEVICE_RT1020 || MG_DEVICE == MG_DEVICE_RT1060

struct mg_flexspi_lut_seq
{
  uint8_t seqNum;
  uint8_t seqId;
  uint16_t reserved;
};

struct mg_flexspi_mem_config
{
  uint32_t tag;
  uint32_t version;
  uint32_t reserved0;
  uint8_t readSampleClkSrc;
  uint8_t csHoldTime;
  uint8_t csSetupTime;
  uint8_t columnAddressWidth;
  uint8_t deviceModeCfgEnable;
  uint8_t deviceModeType;
  uint16_t waitTimeCfgCommands;
  struct mg_flexspi_lut_seq deviceModeSeq;
  uint32_t deviceModeArg;
  uint8_t configCmdEnable;
  uint8_t configModeType[3];
  struct mg_flexspi_lut_seq configCmdSeqs[3];
  uint32_t reserved1;
  uint32_t configCmdArgs[3];
  uint32_t reserved2;
  uint32_t controllerMiscOption;
  uint8_t deviceType;
  uint8_t sflashPadType;
  uint8_t serialClkFreq;
  uint8_t lutCustomSeqEnable;
  uint32_t reserved3[2];
  uint32_t sflashA1Size;
  uint32_t sflashA2Size;
  uint32_t sflashB1Size;
  uint32_t sflashB2Size;
  uint32_t csPadSettingOverride;
  uint32_t sclkPadSettingOverride;
  uint32_t dataPadSettingOverride;
  uint32_t dqsPadSettingOverride;
  uint32_t timeoutInMs;
  uint32_t commandInterval;
  uint16_t dataValidTime[2];
  uint16_t busyOffset;
  uint16_t busyBitPolarity;
  uint32_t lookupTable[64];
  struct mg_flexspi_lut_seq lutCustomSeq[12];
  uint32_t reserved4[4];
};

struct mg_flexspi_nor_config
{
  struct mg_flexspi_mem_config memConfig;
  uint32_t pageSize;
  uint32_t sectorSize;
  uint8_t ipcmdSerialClkFreq;
  uint8_t isUniformBlockSize;
  uint8_t reserved0[2];
  uint8_t serialNorType;
  uint8_t needExitNoCmdMode;
  uint8_t halfClkForNonReadCmd;
  uint8_t needRestoreNoCmdMode;
  uint32_t blockSize;
  uint32_t reserve2[11];
};

/* FLEXSPI memory config block related defintions */
#  define MG_FLEXSPI_CFG_BLK_TAG (0x42464346UL)	    // ascii "FCFB" Big Endian
#  define MG_FLEXSPI_CFG_BLK_VERSION (0x56010400UL) // V1.4.0

#  define MG_FLEXSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1)                \
    (MG_FLEXSPI_LUT_OPERAND0 (op0) | MG_FLEXSPI_LUT_NUM_PADS0 (pad0)          \
     | MG_FLEXSPI_LUT_OPCODE0 (cmd0) | MG_FLEXSPI_LUT_OPERAND1 (op1)          \
     | MG_FLEXSPI_LUT_NUM_PADS1 (pad1) | MG_FLEXSPI_LUT_OPCODE1 (cmd1))

#  define MG_CMD_SDR 0x01
#  define MG_CMD_DDR 0x21
#  define MG_DUMMY_SDR 0x0C
#  define MG_DUMMY_DDR 0x2C
#  define MG_RADDR_SDR 0x02
#  define MG_RADDR_DDR 0x22
#  define MG_READ_SDR 0x09
#  define MG_READ_DDR 0x29
#  define MG_WRITE_SDR 0x08
#  define MG_WRITE_DDR 0x28
#  define MG_STOP 0

#  define MG_FLEXSPI_1PAD 0
#  define MG_FLEXSPI_2PAD 1
#  define MG_FLEXSPI_4PAD 2
#  define MG_FLEXSPI_8PAD 3

#  define MG_FLEXSPI_QSPI_LUT                                                 \
    {                                                                         \
      [0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0xEB,            \
				MG_RADDR_SDR, MG_FLEXSPI_4PAD, 0x18),         \
      [1] = MG_FLEXSPI_LUT_SEQ (MG_DUMMY_SDR, MG_FLEXSPI_4PAD, 0x06,          \
				MG_READ_SDR, MG_FLEXSPI_4PAD, 0x04),          \
      [4 * 1 + 0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0x05,    \
					MG_READ_SDR, MG_FLEXSPI_1PAD, 0x04),  \
      [4 * 3 + 0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0x06,    \
					MG_STOP, MG_FLEXSPI_1PAD, 0x0),       \
      [4 * 5 + 0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0x20,    \
					MG_RADDR_SDR, MG_FLEXSPI_1PAD, 0x18), \
      [4 * 8 + 0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0xD8,    \
					MG_RADDR_SDR, MG_FLEXSPI_1PAD, 0x18), \
      [4 * 9 + 0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0x02,    \
					MG_RADDR_SDR, MG_FLEXSPI_1PAD, 0x18), \
      [4 * 9 + 1] = MG_FLEXSPI_LUT_SEQ (MG_WRITE_SDR, MG_FLEXSPI_1PAD, 0x04,  \
					MG_STOP, MG_FLEXSPI_1PAD, 0x0),       \
      [4 * 11 + 0] = MG_FLEXSPI_LUT_SEQ (MG_CMD_SDR, MG_FLEXSPI_1PAD, 0x60,   \
					 MG_STOP, MG_FLEXSPI_1PAD, 0x0),      \
    }

#  define MG_FLEXSPI_LUT_OPERAND0(x) (((uint32_t) (((uint32_t) (x)))) & 0xFFU)
#  define MG_FLEXSPI_LUT_NUM_PADS0(x)                                         \
    (((uint32_t) (((uint32_t) (x)) << 8U)) & 0x300U)
#  define MG_FLEXSPI_LUT_OPCODE0(x)                                           \
    (((uint32_t) (((uint32_t) (x)) << 10U)) & 0xFC00U)
#  define MG_FLEXSPI_LUT_OPERAND1(x)                                          \
    (((uint32_t) (((uint32_t) (x)) << 16U)) & 0xFF0000U)
#  define MG_FLEXSPI_LUT_NUM_PADS1(x)                                         \
    (((uint32_t) (((uint32_t) (x)) << 24U)) & 0x3000000U)
#  define MG_FLEXSPI_LUT_OPCODE1(x)                                           \
    (((uint32_t) (((uint32_t) (x)) << 26U)) & 0xFC000000U)

#  define FLEXSPI_NOR_INSTANCE 0

#  if MG_DEVICE == MG_DEVICE_RT1020
struct mg_flexspi_nor_driver_interface
{
  uint32_t version;
  int (*init) (uint32_t instance, struct mg_flexspi_nor_config *config);
  int (*program) (uint32_t instance, struct mg_flexspi_nor_config *config,
		  uint32_t dst_addr, const uint32_t *src);
  uint32_t reserved;
  int (*erase) (uint32_t instance, struct mg_flexspi_nor_config *config,
		uint32_t start, uint32_t lengthInBytes);
  uint32_t reserved2;
  int (*update_lut) (uint32_t instance, uint32_t seqIndex,
		     const uint32_t *lutBase, uint32_t seqNumber);
  int (*xfer) (uint32_t instance, char *xfer);
  void (*clear_cache) (uint32_t instance);
};
#  elif MG_DEVICE == MG_DEVICE_RT1060
struct mg_flexspi_nor_driver_interface
{
  uint32_t version;
  int (*init) (uint32_t instance, struct mg_flexspi_nor_config *config);
  int (*program) (uint32_t instance, struct mg_flexspi_nor_config *config,
		  uint32_t dst_addr, const uint32_t *src);
  int (*erase_all) (uint32_t instance, struct mg_flexspi_nor_config *config);
  int (*erase) (uint32_t instance, struct mg_flexspi_nor_config *config,
		uint32_t start, uint32_t lengthInBytes);
  int (*read) (uint32_t instance, struct mg_flexspi_nor_config *config,
	       uint32_t *dst, uint32_t addr, uint32_t lengthInBytes);
  void (*clear_cache) (uint32_t instance);
  int (*xfer) (uint32_t instance, char *xfer);
  int (*update_lut) (uint32_t instance, uint32_t seqIndex,
		     const uint32_t *lutBase, uint32_t seqNumber);
  int (*get_config) (uint32_t instance, struct mg_flexspi_nor_config *config,
		     uint32_t *option);
};
#  endif

#  define flexspi_nor                                                         \
    (*((struct mg_flexspi_nor_driver_interface **) (*(uint32_t *) 0x0020001c  \
						    + 16)))

static bool s_flash_irq_disabled;

MG_IRAM void *
mg_flash_start (void)
{
  return (void *) 0x60000000;
}
MG_IRAM size_t
mg_flash_size (void)
{
  return 8 * 1024 * 1024;
}
MG_IRAM size_t
mg_flash_sector_size (void)
{
  return 4 * 1024; // 4k
}
MG_IRAM size_t
mg_flash_write_align (void)
{
  return 256;
}
MG_IRAM int
mg_flash_bank (void)
{
  return 0;
}

MG_IRAM static bool
flash_page_start (volatile uint32_t *dst)
{
  char *base = (char *) mg_flash_start (), *end = base + mg_flash_size ();
  volatile char *p = (char *) dst;
  return p >= base && p < end && ((p - base) % mg_flash_sector_size ()) == 0;
}

// Note: the get_config function below works both for RT1020 and 1060
#  if MG_DEVICE == MG_DEVICE_RT1020
MG_IRAM static int
flexspi_nor_get_config (struct mg_flexspi_nor_config *config)
{
  struct mg_flexspi_nor_config default_config
      = { .memConfig
	  = { .tag = MG_FLEXSPI_CFG_BLK_TAG,
	      .version = MG_FLEXSPI_CFG_BLK_VERSION,
	      .readSampleClkSrc = 1, // ReadSampleClk_LoopbackFromDqsPad
	      .csHoldTime = 3,
	      .csSetupTime = 3,
	      .controllerMiscOption = MG_BIT (4),
	      .deviceType = 1, // serial NOR
	      .sflashPadType = 4,
	      .serialClkFreq = 7, // 133MHz
	      .sflashA1Size = 8 * 1024 * 1024,
	      .lookupTable = MG_FLEXSPI_QSPI_LUT },
	  .pageSize = 256,
	  .sectorSize = 4 * 1024,
	  .ipcmdSerialClkFreq = 1,
	  .blockSize = 64 * 1024,
	  .isUniformBlockSize = false };

  *config = default_config;
  return 0;
}
#  else
MG_IRAM static int
flexspi_nor_get_config (struct mg_flexspi_nor_config *config)
{
  uint32_t options[] = { 0xc0000000, 0x00 };

  MG_ARM_DISABLE_IRQ ();
  uint32_t status
      = flexspi_nor->get_config (FLEXSPI_NOR_INSTANCE, config, options);
  if (!s_flash_irq_disabled)
    {
      MG_ARM_ENABLE_IRQ ();
    }
  if (status)
    {
      MG_ERROR (("Failed to extract flash configuration: status %u", status));
    }
  return status;
}
#  endif

MG_IRAM bool
mg_flash_erase (void *addr)
{
  struct mg_flexspi_nor_config config;
  if (flexspi_nor_get_config (&config) != 0)
    {
      return false;
    }
  if (flash_page_start (addr) == false)
    {
      MG_ERROR (("%p is not on a sector boundary", addr));
      return false;
    }

  void *dst = (void *) ((char *) addr - (char *) mg_flash_start ());

  // Note: Interrupts must be disabled before any call to the ROM API on RT1020
  // and 1060
  MG_ARM_DISABLE_IRQ ();
  bool ok = (flexspi_nor->erase (FLEXSPI_NOR_INSTANCE, &config, (uint32_t) dst,
				 mg_flash_sector_size ())
	     == 0);
  if (!s_flash_irq_disabled)
    {
      MG_ARM_ENABLE_IRQ (); // Reenable them after the call
    }
  MG_DEBUG (("Sector starting at %p erasure: %s", addr, ok ? "ok" : "fail"));
  return ok;
}

MG_IRAM bool
mg_flash_swap_bank (void)
{
  return true;
}

static inline void
spin (volatile uint32_t count)
{
  while (count--)
    (void) 0;
}

static inline void
flash_wait (void)
{
  while ((*((volatile uint32_t *) (0x402A8000 + 0xE0)) & MG_BIT (1)) == 0)
    spin (1);
}

MG_IRAM static void *
flash_code_location (void)
{
  return (void *) ((char *) mg_flash_start () + 0x2000);
}

MG_IRAM bool
mg_flash_write (void *addr, const void *buf, size_t len)
{
  struct mg_flexspi_nor_config config;
  if (flexspi_nor_get_config (&config) != 0)
    {
      return false;
    }
  if ((len % mg_flash_write_align ()) != 0)
    {
      MG_ERROR (("%lu is not aligned to %lu", len, mg_flash_write_align ()));
      return false;
    }

  if ((char *) addr < (char *) mg_flash_start ())
    {
      MG_ERROR (("Invalid flash write address: %p", addr));
      return false;
    }

  uint32_t *dst = (uint32_t *) addr;
  uint32_t *src = (uint32_t *) buf;
  uint32_t *end = (uint32_t *) ((char *) buf + len);
  bool ok = true;

  // Note: If we overwrite the flash irq section of the image, we must also
  // make sure interrupts are disabled and are not reenabled until we write
  // this sector with another irq table.
  if ((char *) addr == (char *) flash_code_location ())
    {
      s_flash_irq_disabled = true;
      MG_ARM_DISABLE_IRQ ();
    }

  while (ok && src < end)
    {
      if (flash_page_start (dst) && mg_flash_erase (dst) == false)
	{
	  break;
	}
      uint32_t status;
      uint32_t dst_ofs = (uint32_t) dst - (uint32_t) mg_flash_start ();
      if ((char *) buf >= (char *) mg_flash_start ())
	{
	  // If we copy from FLASH to FLASH, then we first need to copy the
	  // source to RAM
	  size_t tmp_buf_size = mg_flash_write_align () / sizeof (uint32_t);
	  uint32_t tmp[tmp_buf_size];

	  for (size_t i = 0; i < tmp_buf_size; i++)
	    {
	      flash_wait ();
	      tmp[i] = src[i];
	    }
	  MG_ARM_DISABLE_IRQ ();
	  status = flexspi_nor->program (FLEXSPI_NOR_INSTANCE, &config,
					 (uint32_t) dst_ofs, tmp);
	}
      else
	{
	  MG_ARM_DISABLE_IRQ ();
	  status = flexspi_nor->program (FLEXSPI_NOR_INSTANCE, &config,
					 (uint32_t) dst_ofs, src);
	}
      if (!s_flash_irq_disabled)
	{
	  MG_ARM_ENABLE_IRQ ();
	}
      src = (uint32_t *) ((char *) src + mg_flash_write_align ());
      dst = (uint32_t *) ((char *) dst + mg_flash_write_align ());
      if (status != 0)
	{
	  ok = false;
	}
    }
  MG_DEBUG (("Flash write %lu bytes @ %p: %s.", len, dst, ok ? "ok" : "fail"));
  return ok;
}

MG_IRAM void
mg_device_reset (void)
{
  MG_DEBUG (("Resetting device..."));
  *(volatile unsigned long *) 0xe000ed0c = 0x5fa0004;
}

#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/device_stm32h5.c"
#endif

#if MG_DEVICE == MG_DEVICE_STM32H5

#  define FLASH_BASE 0x40022000		// Base address of the flash controller
#  define FLASH_KEYR (FLASH_BASE + 0x4) // See RM0481 7.11
#  define FLASH_OPTKEYR (FLASH_BASE + 0xc)
#  define FLASH_OPTCR (FLASH_BASE + 0x1c)
#  define FLASH_NSSR (FLASH_BASE + 0x20)
#  define FLASH_NSCR (FLASH_BASE + 0x28)
#  define FLASH_NSCCR (FLASH_BASE + 0x30)
#  define FLASH_OPTSR_CUR (FLASH_BASE + 0x50)
#  define FLASH_OPTSR_PRG (FLASH_BASE + 0x54)

void *
mg_flash_start (void)
{
  return (void *) 0x08000000;
}
size_t
mg_flash_size (void)
{
  return 2 * 1024 * 1024; // 2Mb
}
size_t
mg_flash_sector_size (void)
{
  return 8 * 1024; // 8k
}
size_t
mg_flash_write_align (void)
{
  return 16; // 128 bit
}
int
mg_flash_bank (void)
{
  return MG_REG (FLASH_OPTCR) & MG_BIT (31) ? 2 : 1;
}

static void
flash_unlock (void)
{
  static bool unlocked = false;
  if (unlocked == false)
    {
      MG_REG (FLASH_KEYR) = 0x45670123;
      MG_REG (FLASH_KEYR) = 0Xcdef89ab;
      MG_REG (FLASH_OPTKEYR) = 0x08192a3b;
      MG_REG (FLASH_OPTKEYR) = 0x4c5d6e7f;
      unlocked = true;
    }
}

static int
flash_page_start (volatile uint32_t *dst)
{
  char *base = (char *) mg_flash_start (), *end = base + mg_flash_size ();
  volatile char *p = (char *) dst;
  return p >= base && p < end && ((p - base) % mg_flash_sector_size ()) == 0;
}

static bool
flash_is_err (void)
{
  return MG_REG (FLASH_NSSR) & ((MG_BIT (8) - 1) << 17); // RM0481 7.11.9
}

static void
flash_wait (void)
{
  while ((MG_REG (FLASH_NSSR) & MG_BIT (0))
	 && (MG_REG (FLASH_NSSR) & MG_BIT (16)) == 0)
    {
      (void) 0;
    }
}

static void
flash_clear_err (void)
{
  flash_wait ();				    // Wait until ready
  MG_REG (FLASH_NSCCR) = ((MG_BIT (9) - 1) << 16U); // Clear all errors
}

static bool
flash_bank_is_swapped (void)
{
  return MG_REG (FLASH_OPTCR) & MG_BIT (31); // RM0481 7.11.8
}

bool
mg_flash_erase (void *location)
{
  bool ok = false;
  if (flash_page_start (location) == false)
    {
      MG_ERROR (("%p is not on a sector boundary"));
    }
  else
    {
      uintptr_t diff = (char *) location - (char *) mg_flash_start ();
      uint32_t sector = diff / mg_flash_sector_size ();
      uint32_t saved_cr = MG_REG (FLASH_NSCR); // Save CR value
      flash_unlock ();
      flash_clear_err ();
      MG_REG (FLASH_NSCR) = 0;
      if ((sector < 128 && flash_bank_is_swapped ())
	  || (sector > 127 && !flash_bank_is_swapped ()))
	{
	  MG_REG (FLASH_NSCR) |= MG_BIT (31); // Set FLASH_CR_BKSEL
	}
      if (sector > 127)
	sector -= 128;
      MG_REG (FLASH_NSCR) |= MG_BIT (2) | (sector << 6); // Erase | sector_num
      MG_REG (FLASH_NSCR) |= MG_BIT (5);		 // Start erasing
      flash_wait ();
      ok = !flash_is_err ();
      MG_DEBUG (("Erase sector %lu @ %p: %s. CR %#lx SR %#lx", sector,
		 location, ok ? "ok" : "fail", MG_REG (FLASH_NSCR),
		 MG_REG (FLASH_NSSR)));
      // mg_hexdump(location, 32);
      MG_REG (FLASH_NSCR) = saved_cr; // Restore saved CR
    }
  return ok;
}

bool
mg_flash_swap_bank (void)
{
  uint32_t desired = flash_bank_is_swapped () ? 0 : MG_BIT (31);
  flash_unlock ();
  flash_clear_err ();
  // printf("OPTSR_PRG 1 %#lx\n", FLASH->OPTSR_PRG);
  MG_SET_BITS (MG_REG (FLASH_OPTSR_PRG), MG_BIT (31), desired);
  // printf("OPTSR_PRG 2 %#lx\n", FLASH->OPTSR_PRG);
  MG_REG (FLASH_OPTCR) |= MG_BIT (1); // OPTSTART
  while ((MG_REG (FLASH_OPTSR_CUR) & MG_BIT (31)) != desired)
    (void) 0;
  return true;
}

bool
mg_flash_write (void *addr, const void *buf, size_t len)
{
  if ((len % mg_flash_write_align ()) != 0)
    {
      MG_ERROR (("%lu is not aligned to %lu", len, mg_flash_write_align ()));
      return false;
    }
  uint32_t *dst = (uint32_t *) addr;
  uint32_t *src = (uint32_t *) buf;
  uint32_t *end = (uint32_t *) ((char *) buf + len);
  bool ok = true;
  flash_unlock ();
  flash_clear_err ();
  MG_ARM_DISABLE_IRQ ();
  // MG_DEBUG(("Starting flash write %lu bytes @ %p", len, addr));
  MG_REG (FLASH_NSCR) = MG_BIT (1); // Set programming flag
  while (ok && src < end)
    {
      if (flash_page_start (dst) && mg_flash_erase (dst) == false)
	break;
      *(volatile uint32_t *) dst++ = *src++;
      flash_wait ();
      if (flash_is_err ())
	ok = false;
    }
  MG_ARM_ENABLE_IRQ ();
  MG_DEBUG (("Flash write %lu bytes @ %p: %s. CR %#lx SR %#lx", len, dst,
	     flash_is_err () ? "fail" : "ok", MG_REG (FLASH_NSCR),
	     MG_REG (FLASH_NSSR)));
  MG_REG (FLASH_NSCR) = 0; // Clear flags
  return ok;
}

void
mg_device_reset (void)
{
  // SCB->AIRCR = ((0x5fa << SCB_AIRCR_VECTKEY_Pos)|SCB_AIRCR_SYSRESETREQ_Msk);
  *(volatile unsigned long *) 0xe000ed0c = 0x5fa0004;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/device_stm32h7.c"
#endif

#if MG_DEVICE == MG_DEVICE_STM32H7

#  define FLASH_BASE1 0x52002000 // Base address for bank1
#  define FLASH_BASE2 0x52002100 // Base address for bank2
#  define FLASH_KEYR 0x04	 // See RM0433 4.9.2
#  define FLASH_OPTKEYR 0x08
#  define FLASH_OPTCR 0x18
#  define FLASH_SR 0x10
#  define FLASH_CR 0x0c
#  define FLASH_CCR 0x14
#  define FLASH_OPTSR_CUR 0x1c
#  define FLASH_OPTSR_PRG 0x20
#  define FLASH_SIZE_REG 0x1ff1e880

MG_IRAM void *
mg_flash_start (void)
{
  return (void *) 0x08000000;
}
MG_IRAM size_t
mg_flash_size (void)
{
  return MG_REG (FLASH_SIZE_REG) * 1024;
}
MG_IRAM size_t
mg_flash_sector_size (void)
{
  return 128 * 1024; // 128k
}
MG_IRAM size_t
mg_flash_write_align (void)
{
  return 32; // 256 bit
}
MG_IRAM int
mg_flash_bank (void)
{
  if (mg_flash_size () < 2 * 1024 * 1024)
    return 0; // No dual bank support
  return MG_REG (FLASH_BASE1 + FLASH_OPTCR) & MG_BIT (31) ? 2 : 1;
}

MG_IRAM static void
flash_unlock (void)
{
  static bool unlocked = false;
  if (unlocked == false)
    {
      MG_REG (FLASH_BASE1 + FLASH_KEYR) = 0x45670123;
      MG_REG (FLASH_BASE1 + FLASH_KEYR) = 0xcdef89ab;
      if (mg_flash_bank () > 0)
	{
	  MG_REG (FLASH_BASE2 + FLASH_KEYR) = 0x45670123;
	  MG_REG (FLASH_BASE2 + FLASH_KEYR) = 0xcdef89ab;
	}
      MG_REG (FLASH_BASE1 + FLASH_OPTKEYR) = 0x08192a3b; // opt reg is "shared"
      MG_REG (FLASH_BASE1 + FLASH_OPTKEYR) = 0x4c5d6e7f; // thus unlock once
      unlocked = true;
    }
}

MG_IRAM static bool
flash_page_start (volatile uint32_t *dst)
{
  char *base = (char *) mg_flash_start (), *end = base + mg_flash_size ();
  volatile char *p = (char *) dst;
  return p >= base && p < end && ((p - base) % mg_flash_sector_size ()) == 0;
}

MG_IRAM static bool
flash_is_err (uint32_t bank)
{
  return MG_REG (bank + FLASH_SR) & ((MG_BIT (11) - 1) << 17); // RM0433 4.9.5
}

MG_IRAM static void
flash_wait (uint32_t bank)
{
  while (MG_REG (bank + FLASH_SR) & (MG_BIT (0) | MG_BIT (2)))
    (void) 0;
}

MG_IRAM static void
flash_clear_err (uint32_t bank)
{
  flash_wait (bank);					  // Wait until ready
  MG_REG (bank + FLASH_CCR) = ((MG_BIT (11) - 1) << 16U); // Clear all errors
}

MG_IRAM static bool
flash_bank_is_swapped (uint32_t bank)
{
  return MG_REG (bank + FLASH_OPTCR) & MG_BIT (31); // RM0433 4.9.7
}

// Figure out flash bank based on the address
MG_IRAM static uint32_t
flash_bank (void *addr)
{
  size_t ofs = (char *) addr - (char *) mg_flash_start ();
  if (mg_flash_bank () == 0)
    return FLASH_BASE1;
  return ofs < mg_flash_size () / 2 ? FLASH_BASE1 : FLASH_BASE2;
}

MG_IRAM bool
mg_flash_erase (void *addr)
{
  bool ok = false;
  if (flash_page_start (addr) == false)
    {
      MG_ERROR (("%p is not on a sector boundary", addr));
    }
  else
    {
      uintptr_t diff = (char *) addr - (char *) mg_flash_start ();
      uint32_t sector = diff / mg_flash_sector_size ();
      uint32_t bank = flash_bank (addr);
      uint32_t saved_cr = MG_REG (bank + FLASH_CR); // Save CR value

      flash_unlock ();
      if (sector > 7)
	sector -= 8;

      flash_clear_err (bank);
      MG_REG (bank + FLASH_CR) = MG_BIT (5); // 32-bit write parallelism
      MG_REG (bank + FLASH_CR) |= (sector & 7U) << 8U; // Sector to erase
      MG_REG (bank + FLASH_CR) |= MG_BIT (2);	       // Sector erase bit
      MG_REG (bank + FLASH_CR) |= MG_BIT (7);	       // Start erasing
      ok = !flash_is_err (bank);
      MG_DEBUG (("Erase sector %lu @ %p %s. CR %#lx SR %#lx", sector, addr,
		 ok ? "ok" : "fail", MG_REG (bank + FLASH_CR),
		 MG_REG (bank + FLASH_SR)));
      MG_REG (bank + FLASH_CR) = saved_cr; // Restore CR
    }
  return ok;
}

MG_IRAM bool
mg_flash_swap_bank (void)
{
  if (mg_flash_bank () == 0)
    return true;
  uint32_t bank = FLASH_BASE1;
  uint32_t desired = flash_bank_is_swapped (bank) ? 0 : MG_BIT (31);
  flash_unlock ();
  flash_clear_err (bank);
  // printf("OPTSR_PRG 1 %#lx\n", FLASH->OPTSR_PRG);
  MG_SET_BITS (MG_REG (bank + FLASH_OPTSR_PRG), MG_BIT (31), desired);
  // printf("OPTSR_PRG 2 %#lx\n", FLASH->OPTSR_PRG);
  MG_REG (bank + FLASH_OPTCR) |= MG_BIT (1); // OPTSTART
  while ((MG_REG (bank + FLASH_OPTSR_CUR) & MG_BIT (31)) != desired)
    (void) 0;
  return true;
}

MG_IRAM bool
mg_flash_write (void *addr, const void *buf, size_t len)
{
  if ((len % mg_flash_write_align ()) != 0)
    {
      MG_ERROR (("%lu is not aligned to %lu", len, mg_flash_write_align ()));
      return false;
    }
  uint32_t bank = flash_bank (addr);
  uint32_t *dst = (uint32_t *) addr;
  uint32_t *src = (uint32_t *) buf;
  uint32_t *end = (uint32_t *) ((char *) buf + len);
  bool ok = true;
  flash_unlock ();
  flash_clear_err (bank);
  MG_REG (bank + FLASH_CR) = MG_BIT (1);  // Set programming flag
  MG_REG (bank + FLASH_CR) |= MG_BIT (5); // 32-bit write parallelism
  MG_DEBUG (("Writing flash @ %p, %lu bytes", addr, len));
  MG_ARM_DISABLE_IRQ ();
  while (ok && src < end)
    {
      if (flash_page_start (dst) && mg_flash_erase (dst) == false)
	break;
      *(volatile uint32_t *) dst++ = *src++;
      flash_wait (bank);
      if (flash_is_err (bank))
	ok = false;
    }
  MG_ARM_ENABLE_IRQ ();
  MG_DEBUG (("Flash write %lu bytes @ %p: %s. CR %#lx SR %#lx", len, dst,
	     ok ? "ok" : "fail", MG_REG (bank + FLASH_CR),
	     MG_REG (bank + FLASH_SR)));
  MG_REG (bank + FLASH_CR) &= ~MG_BIT (1); // Clear programming flag
  return ok;
}

MG_IRAM void
mg_device_reset (void)
{
  // SCB->AIRCR = ((0x5fa << SCB_AIRCR_VECTKEY_Pos)|SCB_AIRCR_SYSRESETREQ_Msk);
  *(volatile unsigned long *) 0xe000ed0c = 0x5fa0004;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/dns.c"
#endif

struct dns_data
{
  struct dns_data *next;
  struct mg_connection *c;
  uint64_t expire;
  uint16_t txnid;
};

static void mg_sendnsreq (struct mg_connection *, struct mg_str *, int,
			  struct mg_dns *, bool);

static void
mg_dns_free (struct dns_data **head, struct dns_data *d)
{
  LIST_DELETE (struct dns_data, head, d);
  free (d);
}

void
mg_resolve_cancel (struct mg_connection *c)
{
  struct dns_data *tmp, *d;
  struct dns_data **head = (struct dns_data **) &c->mgr->active_dns_requests;
  for (d = *head; d != NULL; d = tmp)
    {
      tmp = d->next;
      if (d->c == c)
	mg_dns_free (head, d);
    }
}

static size_t
mg_dns_parse_name_depth (const uint8_t *s, size_t len, size_t ofs, char *to,
			 size_t tolen, size_t j, int depth)
{
  size_t i = 0;
  if (tolen > 0 && depth == 0)
    to[0] = '\0';
  if (depth > 5)
    return 0;
  // MG_INFO(("ofs %lx %x %x", (unsigned long) ofs, s[ofs], s[ofs + 1]));
  while (ofs + i + 1 < len)
    {
      size_t n = s[ofs + i];
      if (n == 0)
	{
	  i++;
	  break;
	}
      if (n & 0xc0)
	{
	  size_t ptr = (((n & 0x3f) << 8) | s[ofs + i + 1]); // 12 is hdr len
	  // MG_INFO(("PTR %lx", (unsigned long) ptr));
	  if (ptr + 1 < len && (s[ptr] & 0xc0) == 0
	      && mg_dns_parse_name_depth (s, len, ptr, to, tolen, j, depth + 1)
		     == 0)
	    return 0;
	  i += 2;
	  break;
	}
      if (ofs + i + n + 1 >= len)
	return 0;
      if (j > 0)
	{
	  if (j < tolen)
	    to[j] = '.';
	  j++;
	}
      if (j + n < tolen)
	memcpy (&to[j], &s[ofs + i + 1], n);
      j += n;
      i += n + 1;
      if (j < tolen)
	to[j] = '\0'; // Zero-terminate this chunk
      // MG_INFO(("--> [%s]", to));
    }
  if (tolen > 0)
    to[tolen - 1] = '\0'; // Make sure make sure it is nul-term
  return i;
}

static size_t
mg_dns_parse_name (const uint8_t *s, size_t n, size_t ofs, char *dst,
		   size_t dstlen)
{
  return mg_dns_parse_name_depth (s, n, ofs, dst, dstlen, 0, 0);
}

size_t
mg_dns_parse_rr (const uint8_t *buf, size_t len, size_t ofs, bool is_question,
		 struct mg_dns_rr *rr)
{
  const uint8_t *s = buf + ofs, *e = &buf[len];

  memset (rr, 0, sizeof (*rr));
  if (len < sizeof (struct mg_dns_header))
    return 0; // Too small
  if (len > 512)
    return 0; //  Too large, we don't expect that
  if (s >= e)
    return 0; //  Overflow

  if ((rr->nlen = (uint16_t) mg_dns_parse_name (buf, len, ofs, NULL, 0)) == 0)
    return 0;
  s += rr->nlen + 4;
  if (s > e)
    return 0;
  rr->atype = (uint16_t) (((uint16_t) s[-4] << 8) | s[-3]);
  rr->aclass = (uint16_t) (((uint16_t) s[-2] << 8) | s[-1]);
  if (is_question)
    return (size_t) (rr->nlen + 4);

  s += 6;
  if (s > e)
    return 0;
  rr->alen = (uint16_t) (((uint16_t) s[-2] << 8) | s[-1]);
  if (s + rr->alen > e)
    return 0;
  return (size_t) (rr->nlen + rr->alen + 10);
}

bool
mg_dns_parse (const uint8_t *buf, size_t len, struct mg_dns_message *dm)
{
  const struct mg_dns_header *h = (struct mg_dns_header *) buf;
  struct mg_dns_rr rr;
  size_t i, n, num_answers, ofs = sizeof (*h);
  memset (dm, 0, sizeof (*dm));

  if (len < sizeof (*h))
    return 0; // Too small, headers dont fit
  if (mg_ntohs (h->num_questions) > 1)
    return 0; // Sanity
  num_answers = mg_ntohs (h->num_answers);
  if (num_answers > 10)
    {
      MG_DEBUG (("Got %u answers, ignoring beyond 10th one", num_answers));
      num_answers = 10; // Sanity cap
    }
  dm->txnid = mg_ntohs (h->txnid);

  for (i = 0; i < mg_ntohs (h->num_questions); i++)
    {
      if ((n = mg_dns_parse_rr (buf, len, ofs, true, &rr)) == 0)
	return false;
      // MG_INFO(("Q %lu %lu %hu/%hu", ofs, n, rr.atype, rr.aclass));
      ofs += n;
    }
  for (i = 0; i < num_answers; i++)
    {
      if ((n = mg_dns_parse_rr (buf, len, ofs, false, &rr)) == 0)
	return false;
      // MG_INFO(("A -- %lu %lu %hu/%hu %s", ofs, n, rr.atype, rr.aclass,
      // dm->name));
      mg_dns_parse_name (buf, len, ofs, dm->name, sizeof (dm->name));
      ofs += n;

      if (rr.alen == 4 && rr.atype == 1 && rr.aclass == 1)
	{
	  dm->addr.is_ip6 = false;
	  memcpy (&dm->addr.ip, &buf[ofs - 4], 4);
	  dm->resolved = true;
	  break; // Return success
	}
      else if (rr.alen == 16 && rr.atype == 28 && rr.aclass == 1)
	{
	  dm->addr.is_ip6 = true;
	  memcpy (&dm->addr.ip, &buf[ofs - 16], 16);
	  dm->resolved = true;
	  break; // Return success
	}
    }
  return true;
}

static void
dns_cb (struct mg_connection *c, int ev, void *ev_data)
{
  struct dns_data *d, *tmp;
  struct dns_data **head = (struct dns_data **) &c->mgr->active_dns_requests;
  if (ev == MG_EV_POLL)
    {
      uint64_t now = *(uint64_t *) ev_data;
      for (d = *head; d != NULL; d = tmp)
	{
	  tmp = d->next;
	  // MG_DEBUG ("%lu %lu dns poll", d->expire, now));
	  if (now > d->expire)
	    mg_error (d->c, "DNS timeout");
	}
    }
  else if (ev == MG_EV_READ)
    {
      struct mg_dns_message dm;
      int resolved = 0;
      if (mg_dns_parse (c->recv.buf, c->recv.len, &dm) == false)
	{
	  MG_ERROR (("Unexpected DNS response:"));
	  mg_hexdump (c->recv.buf, c->recv.len);
	}
      else
	{
	  // MG_VERBOSE(("%s %d", dm.name, dm.resolved));
	  for (d = *head; d != NULL; d = tmp)
	    {
	      tmp = d->next;
	      // MG_INFO(("d %p %hu %hu", d, d->txnid, dm.txnid));
	      if (dm.txnid != d->txnid)
		continue;
	      if (d->c->is_resolving)
		{
		  if (dm.resolved)
		    {
		      dm.addr.port = d->c->rem.port; // Save port
		      d->c->rem = dm.addr;	     // Copy resolved address
		      MG_DEBUG (("%lu %s is %M", d->c->id, dm.name,
				 mg_print_ip, &d->c->rem));
		      mg_connect_resolved (d->c);
#if MG_ENABLE_IPV6
		    }
		  else if (dm.addr.is_ip6 == false && dm.name[0] != '\0'
			   && c->mgr->use_dns6 == false)
		    {
		      struct mg_str x = mg_str (dm.name);
		      mg_sendnsreq (d->c, &x, c->mgr->dnstimeout,
				    &c->mgr->dns6, true);
#endif
		    }
		  else
		    {
		      mg_error (d->c, "%s DNS lookup failed", dm.name);
		    }
		}
	      else
		{
		  MG_ERROR (("%lu already resolved", d->c->id));
		}
	      mg_dns_free (head, d);
	      resolved = 1;
	    }
	}
      if (!resolved)
	MG_ERROR (("stray DNS reply"));
      c->recv.len = 0;
    }
  else if (ev == MG_EV_CLOSE)
    {
      for (d = *head; d != NULL; d = tmp)
	{
	  tmp = d->next;
	  mg_error (d->c, "DNS error");
	  mg_dns_free (head, d);
	}
    }
}

static bool
mg_dns_send (struct mg_connection *c, const struct mg_str *name,
	     uint16_t txnid, bool ipv6)
{
  struct
  {
    struct mg_dns_header header;
    uint8_t data[256];
  } pkt;
  size_t i, n;
  memset (&pkt, 0, sizeof (pkt));
  pkt.header.txnid = mg_htons (txnid);
  pkt.header.flags = mg_htons (0x100);
  pkt.header.num_questions = mg_htons (1);
  for (i = n = 0; i < sizeof (pkt.data) - 5; i++)
    {
      if (name->buf[i] == '.' || i >= name->len)
	{
	  pkt.data[n] = (uint8_t) (i - n);
	  memcpy (&pkt.data[n + 1], name->buf + n, i - n);
	  n = i + 1;
	}
      if (i >= name->len)
	break;
    }
  memcpy (&pkt.data[n], "\x00\x00\x01\x00\x01", 5); // A query
  n += 5;
  if (ipv6)
    pkt.data[n - 3] = 0x1c; // AAAA query
  // memcpy(&pkt.data[n], "\xc0\x0c\x00\x1c\x00\x01", 6);  // AAAA query
  // n += 6;
  return mg_send (c, &pkt, sizeof (pkt.header) + n);
}

static void
mg_sendnsreq (struct mg_connection *c, struct mg_str *name, int ms,
	      struct mg_dns *dnsc, bool ipv6)
{
  struct dns_data *d = NULL;
  if (dnsc->url == NULL)
    {
      mg_error (c, "DNS server URL is NULL. Call mg_mgr_init()");
    }
  else if (dnsc->c == NULL)
    {
      dnsc->c = mg_connect (c->mgr, dnsc->url, NULL, NULL);
      if (dnsc->c != NULL)
	{
	  dnsc->c->pfn = dns_cb;
	  // dnsc->c->is_hexdumping = 1;
	}
    }
  if (dnsc->c == NULL)
    {
      mg_error (c, "resolver");
    }
  else if ((d = (struct dns_data *) calloc (1, sizeof (*d))) == NULL)
    {
      mg_error (c, "resolve OOM");
    }
  else
    {
      struct dns_data *reqs = (struct dns_data *) c->mgr->active_dns_requests;
      d->txnid = reqs ? (uint16_t) (reqs->txnid + 1) : 1;
      d->next = (struct dns_data *) c->mgr->active_dns_requests;
      c->mgr->active_dns_requests = d;
      d->expire = mg_millis () + (uint64_t) ms;
      d->c = c;
      c->is_resolving = 1;
      MG_VERBOSE (("%lu resolving %.*s @ %s, txnid %hu", c->id,
		   (int) name->len, name->buf, dnsc->url, d->txnid));
      if (!mg_dns_send (dnsc->c, name, d->txnid, ipv6))
	{
	  mg_error (dnsc->c, "DNS send");
	}
    }
}

void
mg_resolve (struct mg_connection *c, const char *url)
{
  struct mg_str host = mg_url_host (url);
  c->rem.port = mg_htons (mg_url_port (url));
  if (mg_aton (host, &c->rem))
    {
      // host is an IP address, do not fire name resolution
      mg_connect_resolved (c);
    }
  else
    {
      // host is not an IP, send DNS resolution request
      struct mg_dns *dns = c->mgr->use_dns6 ? &c->mgr->dns6 : &c->mgr->dns4;
      mg_sendnsreq (c, &host, c->mgr->dnstimeout, dns, c->mgr->use_dns6);
    }
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/event.c"
#endif

void
mg_call (struct mg_connection *c, int ev, void *ev_data)
{
#if MG_ENABLE_PROFILE
  const char *names[]
      = { "EV_ERROR",	 "EV_OPEN",	 "EV_POLL",	 "EV_RESOLVE",
	  "EV_CONNECT",	 "EV_ACCEPT",	 "EV_TLS_HS",	 "EV_READ",
	  "EV_WRITE",	 "EV_CLOSE",	 "EV_HTTP_MSG",	 "EV_HTTP_CHUNK",
	  "EV_WS_OPEN",	 "EV_WS_MSG",	 "EV_WS_CTL",	 "EV_MQTT_CMD",
	  "EV_MQTT_MSG", "EV_MQTT_OPEN", "EV_SNTP_TIME", "EV_USER" };
  if (ev != MG_EV_POLL && ev < (int) (sizeof (names) / sizeof (names[0])))
    {
      MG_PROF_ADD (c, names[ev]);
    }
#endif
  // Fire protocol handler first, user handler second. See #2559
  if (c->pfn != NULL)
    c->pfn (c, ev, ev_data);
  if (c->fn != NULL)
    c->fn (c, ev, ev_data);
}

void
mg_error (struct mg_connection *c, const char *fmt, ...)
{
  char buf[64];
  va_list ap;
  va_start (ap, fmt);
  mg_vsnprintf (buf, sizeof (buf), fmt, &ap);
  va_end (ap);
  MG_ERROR (("%lu %ld %s", c->id, c->fd, buf));
  c->is_closing = 1;		 // Set is_closing before sending MG_EV_CALL
  mg_call (c, MG_EV_ERROR, buf); // Let user handler override it
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/fmt.c"
#endif

static bool
is_digit (int c)
{
  return c >= '0' && c <= '9';
}

static int
addexp (char *buf, int e, int sign)
{
  int n = 0;
  buf[n++] = 'e';
  buf[n++] = (char) sign;
  if (e > 400)
    return 0;
  if (e < 10)
    buf[n++] = '0';
  if (e >= 100)
    buf[n++] = (char) (e / 100 + '0'), e -= 100 * (e / 100);
  if (e >= 10)
    buf[n++] = (char) (e / 10 + '0'), e -= 10 * (e / 10);
  buf[n++] = (char) (e + '0');
  return n;
}

static int
xisinf (double x)
{
  union
  {
    double f;
    uint64_t u;
  } ieee754 = { x };
  return ((unsigned) (ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000
	 && ((unsigned) ieee754.u == 0);
}

static int
xisnan (double x)
{
  union
  {
    double f;
    uint64_t u;
  } ieee754 = { x };
  return ((unsigned) (ieee754.u >> 32) & 0x7fffffff)
	     + ((unsigned) ieee754.u != 0)
	 > 0x7ff00000;
}

static size_t
mg_dtoa (char *dst, size_t dstlen, double d, int width, bool tz)
{
  char buf[40];
  int i, s = 0, n = 0, e = 0;
  double t, mul, saved;
  if (d == 0.0)
    return mg_snprintf (dst, dstlen, "%s", "0");
  if (xisinf (d))
    return mg_snprintf (dst, dstlen, "%s", d > 0 ? "inf" : "-inf");
  if (xisnan (d))
    return mg_snprintf (dst, dstlen, "%s", "nan");
  if (d < 0.0)
    d = -d, buf[s++] = '-';

  // Round
  saved = d;
  mul = 1.0;
  while (d >= 10.0 && d / mul >= 10.0)
    mul *= 10.0;
  while (d <= 1.0 && d / mul <= 1.0)
    mul /= 10.0;
  for (i = 0, t = mul * 5; i < width; i++)
    t /= 10.0;
  d += t;
  // Calculate exponent, and 'mul' for scientific representation
  mul = 1.0;
  while (d >= 10.0 && d / mul >= 10.0)
    mul *= 10.0, e++;
  while (d < 1.0 && d / mul < 1.0)
    mul /= 10.0, e--;
  // printf(" --> %g %d %g %g\n", saved, e, t, mul);

  if (e >= width && width > 1)
    {
      n = (int) mg_dtoa (buf, sizeof (buf), saved / mul, width, tz);
      // printf(" --> %.*g %d [%.*s]\n", 10, d / t, e, n, buf);
      n += addexp (buf + s + n, e, '+');
      return mg_snprintf (dst, dstlen, "%.*s", n, buf);
    }
  else if (e <= -width && width > 1)
    {
      n = (int) mg_dtoa (buf, sizeof (buf), saved / mul, width, tz);
      // printf(" --> %.*g %d [%.*s]\n", 10, d / mul, e, n, buf);
      n += addexp (buf + s + n, -e, '-');
      return mg_snprintf (dst, dstlen, "%.*s", n, buf);
    }
  else
    {
      for (i = 0, t = mul; t >= 1.0 && s + n < (int) sizeof (buf); i++)
	{
	  int ch = (int) (d / t);
	  if (n > 0 || ch > 0)
	    buf[s + n++] = (char) (ch + '0');
	  d -= ch * t;
	  t /= 10.0;
	}
      // printf(" --> [%g] -> %g %g (%d) [%.*s]\n", saved, d, t, n, s + n,
      // buf);
      if (n == 0)
	buf[s++] = '0';
      while (t >= 1.0 && n + s < (int) sizeof (buf))
	buf[n++] = '0', t /= 10.0;
      if (s + n < (int) sizeof (buf))
	buf[n + s++] = '.';
      // printf(" 1--> [%g] -> [%.*s]\n", saved, s + n, buf);
      for (i = 0, t = 0.1; s + n < (int) sizeof (buf) && n < width; i++)
	{
	  int ch = (int) (d / t);
	  buf[s + n++] = (char) (ch + '0');
	  d -= ch * t;
	  t /= 10.0;
	}
    }
  while (tz && n > 0 && buf[s + n - 1] == '0')
    n--; // Trim trailing zeroes
  if (n > 0 && buf[s + n - 1] == '.')
    n--; // Trim trailing dot
  n += s;
  if (n >= (int) sizeof (buf))
    n = (int) sizeof (buf) - 1;
  buf[n] = '\0';
  return mg_snprintf (dst, dstlen, "%s", buf);
}

static size_t
mg_lld (char *buf, int64_t val, bool is_signed, bool is_hex)
{
  const char *letters = "0123456789abcdef";
  uint64_t v = (uint64_t) val;
  size_t s = 0, n, i;
  if (is_signed && val < 0)
    buf[s++] = '-', v = (uint64_t) (-val);
  // This loop prints a number in reverse order. I guess this is because we
  // write numbers from right to left: least significant digit comes last.
  // Maybe because we use Arabic numbers, and Arabs write RTL?
  if (is_hex)
    {
      for (n = 0; v; v >>= 4)
	buf[s + n++] = letters[v & 15];
    }
  else
    {
      for (n = 0; v; v /= 10)
	buf[s + n++] = letters[v % 10];
    }
  // Reverse a string
  for (i = 0; i < n / 2; i++)
    {
      char t = buf[s + i];
      buf[s + i] = buf[s + n - i - 1], buf[s + n - i - 1] = t;
    }
  if (val == 0)
    buf[n++] = '0'; // Handle special case
  return n + s;
}

static size_t
scpy (void (*out) (char, void *), void *ptr, char *buf, size_t len)
{
  size_t i = 0;
  while (i < len && buf[i] != '\0')
    out (buf[i++], ptr);
  return i;
}

size_t
mg_xprintf (void (*out) (char, void *), void *ptr, const char *fmt, ...)
{
  size_t len = 0;
  va_list ap;
  va_start (ap, fmt);
  len = mg_vxprintf (out, ptr, fmt, &ap);
  va_end (ap);
  return len;
}

size_t
mg_vxprintf (void (*out) (char, void *), void *param, const char *fmt,
	     va_list *ap)
{
  size_t i = 0, n = 0;
  while (fmt[i] != '\0')
    {
      if (fmt[i] == '%')
	{
	  size_t j, k, x = 0, is_long = 0, w = 0 /* width */,
		       pr = ~0U /* prec */;
	  char pad = ' ', minus = 0, c = fmt[++i];
	  if (c == '#')
	    x++, c = fmt[++i];
	  if (c == '-')
	    minus++, c = fmt[++i];
	  if (c == '0')
	    pad = '0', c = fmt[++i];
	  while (is_digit (c))
	    w *= 10, w += (size_t) (c - '0'), c = fmt[++i];
	  if (c == '.')
	    {
	      c = fmt[++i];
	      if (c == '*')
		{
		  pr = (size_t) va_arg (*ap, int);
		  c = fmt[++i];
		}
	      else
		{
		  pr = 0;
		  while (is_digit (c))
		    pr *= 10, pr += (size_t) (c - '0'), c = fmt[++i];
		}
	    }
	  while (c == 'h')
	    c = fmt[++i]; // Treat h and hh as int
	  if (c == 'l')
	    {
	      is_long++, c = fmt[++i];
	      if (c == 'l')
		is_long++, c = fmt[++i];
	    }
	  if (c == 'p')
	    x = 1, is_long = 1;
	  if (c == 'd' || c == 'u' || c == 'x' || c == 'X' || c == 'p'
	      || c == 'g' || c == 'f')
	    {
	      bool s = (c == 'd'), h = (c == 'x' || c == 'X' || c == 'p');
	      char tmp[40];
	      size_t xl = x ? 2 : 0;
	      if (c == 'g' || c == 'f')
		{
		  double v = va_arg (*ap, double);
		  if (pr == ~0U)
		    pr = 6;
		  k = mg_dtoa (tmp, sizeof (tmp), v, (int) pr, c == 'g');
		}
	      else if (is_long == 2)
		{
		  int64_t v = va_arg (*ap, int64_t);
		  k = mg_lld (tmp, v, s, h);
		}
	      else if (is_long == 1)
		{
		  long v = va_arg (*ap, long);
		  k = mg_lld (tmp,
			      s ? (int64_t) v : (int64_t) (unsigned long) v, s,
			      h);
		}
	      else
		{
		  int v = va_arg (*ap, int);
		  k = mg_lld (tmp, s ? (int64_t) v : (int64_t) (unsigned) v, s,
			      h);
		}
	      for (j = 0; j < xl && w > 0; j++)
		w--;
	      for (j = 0; pad == ' ' && !minus && k < w && j + k < w; j++)
		n += scpy (out, param, &pad, 1);
	      n += scpy (out, param, (char *) "0x", xl);
	      for (j = 0; pad == '0' && k < w && j + k < w; j++)
		n += scpy (out, param, &pad, 1);
	      n += scpy (out, param, tmp, k);
	      for (j = 0; pad == ' ' && minus && k < w && j + k < w; j++)
		n += scpy (out, param, &pad, 1);
	    }
	  else if (c == 'm' || c == 'M')
	    {
	      mg_pm_t f = va_arg (*ap, mg_pm_t);
	      if (c == 'm')
		out ('"', param);
	      n += f (out, param, ap);
	      if (c == 'm')
		n += 2, out ('"', param);
	    }
	  else if (c == 'c')
	    {
	      int ch = va_arg (*ap, int);
	      out ((char) ch, param);
	      n++;
	    }
	  else if (c == 's')
	    {
	      char *p = va_arg (*ap, char *);
	      if (pr == ~0U)
		pr = p == NULL ? 0 : strlen (p);
	      for (j = 0; !minus && pr < w && j + pr < w; j++)
		n += scpy (out, param, &pad, 1);
	      n += scpy (out, param, p, pr);
	      for (j = 0; minus && pr < w && j + pr < w; j++)
		n += scpy (out, param, &pad, 1);
	    }
	  else if (c == '%')
	    {
	      out ('%', param);
	      n++;
	    }
	  else
	    {
	      out ('%', param);
	      out (c, param);
	      n += 2;
	    }
	  i++;
	}
      else
	{
	  out (fmt[i], param), n++, i++;
	}
    }
  return n;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/fs.c"
#endif

struct mg_fd *
mg_fs_open (struct mg_fs *fs, const char *path, int flags)
{
  struct mg_fd *fd = (struct mg_fd *) calloc (1, sizeof (*fd));
  if (fd != NULL)
    {
      fd->fd = fs->op (path, flags);
      fd->fs = fs;
      if (fd->fd == NULL)
	{
	  free (fd);
	  fd = NULL;
	}
    }
  return fd;
}

void
mg_fs_close (struct mg_fd *fd)
{
  if (fd != NULL)
    {
      fd->fs->cl (fd->fd);
      free (fd);
    }
}

struct mg_str
mg_file_read (struct mg_fs *fs, const char *path)
{
  struct mg_str result = { NULL, 0 };
  void *fp;
  fs->st (path, &result.len, NULL);
  if ((fp = fs->op (path, MG_FS_READ)) != NULL)
    {
      result.buf = (char *) calloc (1, result.len + 1);
      if (result.buf != NULL
	  && fs->rd (fp, (void *) result.buf, result.len) != result.len)
	{
	  free ((void *) result.buf);
	  result.buf = NULL;
	}
      fs->cl (fp);
    }
  if (result.buf == NULL)
    result.len = 0;
  return result;
}

bool
mg_file_write (struct mg_fs *fs, const char *path, const void *buf, size_t len)
{
  bool result = false;
  struct mg_fd *fd;
  char tmp[MG_PATH_MAX];
  mg_snprintf (tmp, sizeof (tmp), "%s..%d", path, rand ());
  if ((fd = mg_fs_open (fs, tmp, MG_FS_WRITE)) != NULL)
    {
      result = fs->wr (fd->fd, buf, len) == len;
      mg_fs_close (fd);
      if (result)
	{
	  fs->rm (path);
	  fs->mv (tmp, path);
	}
      else
	{
	  fs->rm (tmp);
	}
    }
  return result;
}

bool
mg_file_printf (struct mg_fs *fs, const char *path, const char *fmt, ...)
{
  va_list ap;
  char *data;
  bool result = false;
  va_start (ap, fmt);
  data = mg_vmprintf (fmt, &ap);
  va_end (ap);
  result = mg_file_write (fs, path, data, strlen (data));
  free (data);
  return result;
}

// This helper function allows to scan a filesystem in a sequential way,
// without using callback function:
//      char buf[100] = "";
//      while (mg_fs_ls(&mg_fs_posix, "./", buf, sizeof(buf))) {
//        ...
static void
mg_fs_ls_fn (const char *filename, void *param)
{
  struct mg_str *s = (struct mg_str *) param;
  if (s->buf[0] == '\0')
    {
      mg_snprintf ((char *) s->buf, s->len, "%s", filename);
    }
  else if (strcmp (s->buf, filename) == 0)
    {
      ((char *) s->buf)[0] = '\0'; // Fetch next file
    }
}

bool
mg_fs_ls (struct mg_fs *fs, const char *path, char *buf, size_t len)
{
  struct mg_str s = { buf, len };
  fs->ls (path, mg_fs_ls_fn, &s);
  return buf[0] != '\0';
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/fs_fat.c"
#endif

#if MG_ENABLE_FATFS
#  include <ff.h>

static int
mg_days_from_epoch (int y, int m, int d)
{
  y -= m <= 2;
  int era = y / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

static time_t
mg_timegm (const struct tm *t)
{
  int year = t->tm_year + 1900;
  int month = t->tm_mon; // 0-11
  if (month > 11)
    {
      year += month / 12;
      month %= 12;
    }
  else if (month < 0)
    {
      int years_diff = (11 - month) / 12;
      year -= years_diff;
      month += 12 * years_diff;
    }
  int x = mg_days_from_epoch (year, month + 1, t->tm_mday);
  return 60 * (60 * (24L * x + t->tm_hour) + t->tm_min) + t->tm_sec;
}

static time_t
ff_time_to_epoch (uint16_t fdate, uint16_t ftime)
{
  struct tm tm;
  memset (&tm, 0, sizeof (struct tm));
  tm.tm_sec = (ftime << 1) & 0x3e;
  tm.tm_min = ((ftime >> 5) & 0x3f);
  tm.tm_hour = ((ftime >> 11) & 0x1f);
  tm.tm_mday = (fdate & 0x1f);
  tm.tm_mon = ((fdate >> 5) & 0x0f) - 1;
  tm.tm_year = ((fdate >> 9) & 0x7f) + 80;
  return mg_timegm (&tm);
}

static int
ff_stat (const char *path, size_t *size, time_t *mtime)
{
  FILINFO fi;
  if (path[0] == '\0')
    {
      if (size)
	*size = 0;
      if (mtime)
	*mtime = 0;
      return MG_FS_DIR;
    }
  else if (f_stat (path, &fi) == 0)
    {
      if (size)
	*size = (size_t) fi.fsize;
      if (mtime)
	*mtime = ff_time_to_epoch (fi.fdate, fi.ftime);
      return MG_FS_READ | MG_FS_WRITE
	     | ((fi.fattrib & AM_DIR) ? MG_FS_DIR : 0);
    }
  else
    {
      return 0;
    }
}

static void
ff_list (const char *dir, void (*fn) (const char *, void *), void *userdata)
{
  DIR d;
  FILINFO fi;
  if (f_opendir (&d, dir) == FR_OK)
    {
      while (f_readdir (&d, &fi) == FR_OK && fi.fname[0] != '\0')
	{
	  if (!strcmp (fi.fname, ".") || !strcmp (fi.fname, ".."))
	    continue;
	  fn (fi.fname, userdata);
	}
      f_closedir (&d);
    }
}

static void *
ff_open (const char *path, int flags)
{
  FIL f;
  unsigned char mode = FA_READ;
  if (flags & MG_FS_WRITE)
    mode |= FA_WRITE | FA_OPEN_ALWAYS | FA_OPEN_APPEND;
  if (f_open (&f, path, mode) == 0)
    {
      FIL *fp;
      if ((fp = calloc (1, sizeof (*fp))) != NULL)
	{
	  memcpy (fp, &f, sizeof (*fp));
	  return fp;
	}
    }
  return NULL;
}

static void
ff_close (void *fp)
{
  if (fp != NULL)
    {
      f_close ((FIL *) fp);
      free (fp);
    }
}

static size_t
ff_read (void *fp, void *buf, size_t len)
{
  UINT n = 0, misalign = ((size_t) buf) & 3;
  if (misalign)
    {
      char aligned[4];
      f_read ((FIL *) fp, aligned, len > misalign ? misalign : len, &n);
      memcpy (buf, aligned, n);
    }
  else
    {
      f_read ((FIL *) fp, buf, len, &n);
    }
  return n;
}

static size_t
ff_write (void *fp, const void *buf, size_t len)
{
  UINT n = 0;
  return f_write ((FIL *) fp, (char *) buf, len, &n) == FR_OK ? n : 0;
}

static size_t
ff_seek (void *fp, size_t offset)
{
  f_lseek ((FIL *) fp, offset);
  return offset;
}

static bool
ff_rename (const char *from, const char *to)
{
  return f_rename (from, to) == FR_OK;
}

static bool
ff_remove (const char *path)
{
  return f_unlink (path) == FR_OK;
}

static bool
ff_mkdir (const char *path)
{
  return f_mkdir (path) == FR_OK;
}

struct mg_fs mg_fs_fat = { ff_stat,  ff_list, ff_open,	 ff_close,  ff_read,
			   ff_write, ff_seek, ff_rename, ff_remove, ff_mkdir };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/fs_packed.c"
#endif

struct packed_file
{
  const char *data;
  size_t size;
  size_t pos;
};

#if MG_ENABLE_PACKED_FS
#else
const char *
mg_unpack (const char *path, size_t *size, time_t *mtime)
{
  *size = 0, *mtime = 0;
  (void) path;
  return NULL;
}
const char *
mg_unlist (size_t no)
{
  (void) no;
  return NULL;
}
#endif

struct mg_str
mg_unpacked (const char *path)
{
  size_t len = 0;
  const char *buf = mg_unpack (path, &len, NULL);
  return mg_str_n (buf, len);
}

static int
is_dir_prefix (const char *prefix, size_t n, const char *path)
{
  // MG_INFO(("[%.*s] [%s] %c", (int) n, prefix, path, path[n]));
  return n < strlen (path) && strncmp (prefix, path, n) == 0
	 && (n == 0 || path[n] == '/' || path[n - 1] == '/');
}

static int
packed_stat (const char *path, size_t *size, time_t *mtime)
{
  const char *p;
  size_t i, n = strlen (path);
  if (mg_unpack (path, size, mtime))
    return MG_FS_READ; // Regular file
  // Scan all files. If `path` is a dir prefix for any of them, it's a dir
  for (i = 0; (p = mg_unlist (i)) != NULL; i++)
    {
      if (is_dir_prefix (path, n, p))
	return MG_FS_DIR;
    }
  return 0;
}

static void
packed_list (const char *dir, void (*fn) (const char *, void *),
	     void *userdata)
{
  char buf[MG_PATH_MAX], tmp[sizeof (buf)];
  const char *path, *begin, *end;
  size_t i, n = strlen (dir);
  tmp[0] = '\0'; // Previously listed entry
  for (i = 0; (path = mg_unlist (i)) != NULL; i++)
    {
      if (!is_dir_prefix (dir, n, path))
	continue;
      begin = &path[n + 1];
      end = strchr (begin, '/');
      if (end == NULL)
	end = begin + strlen (begin);
      mg_snprintf (buf, sizeof (buf), "%.*s", (int) (end - begin), begin);
      buf[sizeof (buf) - 1] = '\0';
      // If this entry has been already listed, skip
      // NOTE: we're assuming that file list is sorted alphabetically
      if (strcmp (buf, tmp) == 0)
	continue;
      fn (buf, userdata); // Not yet listed, call user function
      strcpy (tmp, buf);  // And save this entry as listed
    }
}

static void *
packed_open (const char *path, int flags)
{
  size_t size = 0;
  const char *data = mg_unpack (path, &size, NULL);
  struct packed_file *fp = NULL;
  if (data == NULL)
    return NULL;
  if (flags & MG_FS_WRITE)
    return NULL;
  if ((fp = (struct packed_file *) calloc (1, sizeof (*fp))) != NULL)
    {
      fp->size = size;
      fp->data = data;
    }
  return (void *) fp;
}

static void
packed_close (void *fp)
{
  if (fp != NULL)
    free (fp);
}

static size_t
packed_read (void *fd, void *buf, size_t len)
{
  struct packed_file *fp = (struct packed_file *) fd;
  if (fp->pos + len > fp->size)
    len = fp->size - fp->pos;
  memcpy (buf, &fp->data[fp->pos], len);
  fp->pos += len;
  return len;
}

static size_t
packed_write (void *fd, const void *buf, size_t len)
{
  (void) fd, (void) buf, (void) len;
  return 0;
}

static size_t
packed_seek (void *fd, size_t offset)
{
  struct packed_file *fp = (struct packed_file *) fd;
  fp->pos = offset;
  if (fp->pos > fp->size)
    fp->pos = fp->size;
  return fp->pos;
}

static bool
packed_rename (const char *from, const char *to)
{
  (void) from, (void) to;
  return false;
}

static bool
packed_remove (const char *path)
{
  (void) path;
  return false;
}

static bool
packed_mkdir (const char *path)
{
  (void) path;
  return false;
}

struct mg_fs mg_fs_packed = { packed_stat,  packed_list,   packed_open,
			      packed_close, packed_read,   packed_write,
			      packed_seek,  packed_rename, packed_remove,
			      packed_mkdir };

#ifdef MG_ENABLE_LINES
#  line 1 "src/fs_posix.c"
#endif

#if MG_ENABLE_POSIX_FS

#  ifndef MG_STAT_STRUCT
#    define MG_STAT_STRUCT stat
#  endif

#  ifndef MG_STAT_FUNC
#    define MG_STAT_FUNC stat
#  endif

static int
p_stat (const char *path, size_t *size, time_t *mtime)
{
#  if !defined(S_ISDIR)
  MG_ERROR (("stat() API is not supported. %p %p %p", path, size, mtime));
  return 0;
#  else
#    if MG_ARCH == MG_ARCH_WIN32
  struct _stati64 st;
  wchar_t tmp[MG_PATH_MAX];
  MultiByteToWideChar (CP_UTF8, 0, path, -1, tmp,
		       sizeof (tmp) / sizeof (tmp[0]));
  if (_wstati64 (tmp, &st) != 0)
    return 0;
  // If path is a symlink, windows reports 0 in st.st_size.
  // Get a real file size by opening it and jumping to the end
  if (st.st_size == 0 && (st.st_mode & _S_IFREG))
    {
      FILE *fp = _wfopen (tmp, L"rb");
      if (fp != NULL)
	{
	  fseek (fp, 0, SEEK_END);
	  if (ftell (fp) > 0)
	    st.st_size = ftell (fp); // Use _ftelli64 on win10+
	  fclose (fp);
	}
    }
#    else
  struct MG_STAT_STRUCT st;
  if (MG_STAT_FUNC (path, &st) != 0)
    return 0;
#    endif
  if (size)
    *size = (size_t) st.st_size;
  if (mtime)
    *mtime = st.st_mtime;
  return MG_FS_READ | MG_FS_WRITE | (S_ISDIR (st.st_mode) ? MG_FS_DIR : 0);
#  endif
}

#  if MG_ARCH == MG_ARCH_WIN32
struct dirent
{
  char d_name[MAX_PATH];
};

typedef struct win32_dir
{
  HANDLE handle;
  WIN32_FIND_DATAW info;
  struct dirent result;
} DIR;

#    if 0
int gettimeofday(struct timeval *tv, void *tz) {
  FILETIME ft;
  unsigned __int64 tmpres = 0;

  if (tv != NULL) {
    GetSystemTimeAsFileTime(&ft);
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
    tmpres /= 10;  // convert into microseconds
    tmpres -= (int64_t) 11644473600000000;
    tv->tv_sec = (long) (tmpres / 1000000UL);
    tv->tv_usec = (long) (tmpres % 1000000UL);
  }
  (void) tz;
  return 0;
}
#    endif

static int
to_wchar (const char *path, wchar_t *wbuf, size_t wbuf_len)
{
  int ret;
  char buf[MAX_PATH * 2], buf2[MAX_PATH * 2], *p;
  strncpy (buf, path, sizeof (buf));
  buf[sizeof (buf) - 1] = '\0';
  // Trim trailing slashes. Leave backslash for paths like "X:\"
  p = buf + strlen (buf) - 1;
  while (p > buf && p[-1] != ':' && (p[0] == '\\' || p[0] == '/'))
    *p-- = '\0';
  memset (wbuf, 0, wbuf_len * sizeof (wchar_t));
  ret = MultiByteToWideChar (CP_UTF8, 0, buf, -1, wbuf, (int) wbuf_len);
  // Convert back to Unicode. If doubly-converted string does not match the
  // original, something is fishy, reject.
  WideCharToMultiByte (CP_UTF8, 0, wbuf, (int) wbuf_len, buf2, sizeof (buf2),
		       NULL, NULL);
  if (strcmp (buf, buf2) != 0)
    {
      wbuf[0] = L'\0';
      ret = 0;
    }
  return ret;
}

DIR *
opendir (const char *name)
{
  DIR *d = NULL;
  wchar_t wpath[MAX_PATH];
  DWORD attrs;

  if (name == NULL)
    {
      SetLastError (ERROR_BAD_ARGUMENTS);
    }
  else if ((d = (DIR *) calloc (1, sizeof (*d))) == NULL)
    {
      SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    }
  else
    {
      to_wchar (name, wpath, sizeof (wpath) / sizeof (wpath[0]));
      attrs = GetFileAttributesW (wpath);
      if (attrs != 0Xffffffff && (attrs & FILE_ATTRIBUTE_DIRECTORY))
	{
	  (void) wcscat (wpath, L"\\*");
	  d->handle = FindFirstFileW (wpath, &d->info);
	  d->result.d_name[0] = '\0';
	}
      else
	{
	  free (d);
	  d = NULL;
	}
    }
  return d;
}

int
closedir (DIR *d)
{
  int result = 0;
  if (d != NULL)
    {
      if (d->handle != INVALID_HANDLE_VALUE)
	result = FindClose (d->handle) ? 0 : -1;
      free (d);
    }
  else
    {
      result = -1;
      SetLastError (ERROR_BAD_ARGUMENTS);
    }
  return result;
}

struct dirent *
readdir (DIR *d)
{
  struct dirent *result = NULL;
  if (d != NULL)
    {
      memset (&d->result, 0, sizeof (d->result));
      if (d->handle != INVALID_HANDLE_VALUE)
	{
	  result = &d->result;
	  WideCharToMultiByte (CP_UTF8, 0, d->info.cFileName, -1,
			       result->d_name, sizeof (result->d_name), NULL,
			       NULL);
	  if (!FindNextFileW (d->handle, &d->info))
	    {
	      FindClose (d->handle);
	      d->handle = INVALID_HANDLE_VALUE;
	    }
	}
      else
	{
	  SetLastError (ERROR_FILE_NOT_FOUND);
	}
    }
  else
    {
      SetLastError (ERROR_BAD_ARGUMENTS);
    }
  return result;
}
#  endif

static void
p_list (const char *dir, void (*fn) (const char *, void *), void *userdata)
{
#  if MG_ENABLE_DIRLIST
  struct dirent *dp;
  DIR *dirp;
  if ((dirp = (opendir (dir))) == NULL)
    return;
  while ((dp = readdir (dirp)) != NULL)
    {
      if (!strcmp (dp->d_name, ".") || !strcmp (dp->d_name, ".."))
	continue;
      fn (dp->d_name, userdata);
    }
  closedir (dirp);
#  else
  (void) dir, (void) fn, (void) userdata;
#  endif
}

static void *
p_open (const char *path, int flags)
{
#  if MG_ARCH == MG_ARCH_WIN32
  const char *mode = flags == MG_FS_READ ? "rb" : "a+b";
  wchar_t b1[MG_PATH_MAX], b2[10];
  MultiByteToWideChar (CP_UTF8, 0, path, -1, b1, sizeof (b1) / sizeof (b1[0]));
  MultiByteToWideChar (CP_UTF8, 0, mode, -1, b2, sizeof (b2) / sizeof (b2[0]));
  return (void *) _wfopen (b1, b2);
#  else
  const char *mode = flags == MG_FS_READ ? "rbe" : "a+be"; // e for CLOEXEC
  return (void *) fopen (path, mode);
#  endif
}

static void
p_close (void *fp)
{
  fclose ((FILE *) fp);
}

static size_t
p_read (void *fp, void *buf, size_t len)
{
  return fread (buf, 1, len, (FILE *) fp);
}

static size_t
p_write (void *fp, const void *buf, size_t len)
{
  return fwrite (buf, 1, len, (FILE *) fp);
}

static size_t
p_seek (void *fp, size_t offset)
{
#  if (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64)                 \
      || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)             \
      || (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)
  if (fseeko ((FILE *) fp, (off_t) offset, SEEK_SET) != 0)
    (void) 0;
#  else
  if (fseek ((FILE *) fp, (long) offset, SEEK_SET) != 0)
    (void) 0;
#  endif
  return (size_t) ftell ((FILE *) fp);
}

static bool
p_rename (const char *from, const char *to)
{
  return rename (from, to) == 0;
}

static bool
p_remove (const char *path)
{
  return remove (path) == 0;
}

static bool
p_mkdir (const char *path)
{
  return mkdir (path, 0775) == 0;
}

#else

static int
p_stat (const char *path, size_t *size, time_t *mtime)
{
  (void) path, (void) size, (void) mtime;
  return 0;
}
static void
p_list (const char *path, void (*fn) (const char *, void *), void *userdata)
{
  (void) path, (void) fn, (void) userdata;
}
static void *
p_open (const char *path, int flags)
{
  (void) path, (void) flags;
  return NULL;
}
static void
p_close (void *fp)
{
  (void) fp;
}
static size_t
p_read (void *fd, void *buf, size_t len)
{
  (void) fd, (void) buf, (void) len;
  return 0;
}
static size_t
p_write (void *fd, const void *buf, size_t len)
{
  (void) fd, (void) buf, (void) len;
  return 0;
}
static size_t
p_seek (void *fd, size_t offset)
{
  (void) fd, (void) offset;
  return (size_t) ~0;
}
static bool
p_rename (const char *from, const char *to)
{
  (void) from, (void) to;
  return false;
}
static bool
p_remove (const char *path)
{
  (void) path;
  return false;
}
static bool
p_mkdir (const char *path)
{
  (void) path;
  return false;
}
#endif

struct mg_fs mg_fs_posix = { p_stat,  p_list, p_open,	p_close,  p_read,
			     p_write, p_seek, p_rename, p_remove, p_mkdir };

#ifdef MG_ENABLE_LINES
#  line 1 "src/http.c"
#endif

static int
mg_ncasecmp (const char *s1, const char *s2, size_t len)
{
  int diff = 0;
  if (len > 0)
    do
      {
	int c = *s1++, d = *s2++;
	if (c >= 'A' && c <= 'Z')
	  c += 'a' - 'A';
	if (d >= 'A' && d <= 'Z')
	  d += 'a' - 'A';
	diff = c - d;
      }
    while (diff == 0 && s1[-1] != '\0' && --len > 0);
  return diff;
}

bool mg_to_size_t (struct mg_str str, size_t *val);
bool
mg_to_size_t (struct mg_str str, size_t *val)
{
  size_t i = 0, max = (size_t) -1, max2 = max / 10, result = 0, ndigits = 0;
  while (i < str.len && (str.buf[i] == ' ' || str.buf[i] == '\t'))
    i++;
  if (i < str.len && str.buf[i] == '-')
    return false;
  while (i < str.len && str.buf[i] >= '0' && str.buf[i] <= '9')
    {
      size_t digit = (size_t) (str.buf[i] - '0');
      if (result > max2)
	return false; // Overflow
      result *= 10;
      if (result > max - digit)
	return false; // Overflow
      result += digit;
      i++, ndigits++;
    }
  while (i < str.len && (str.buf[i] == ' ' || str.buf[i] == '\t'))
    i++;
  if (ndigits == 0)
    return false; // #2322: Content-Length = 1 * DIGIT
  if (i != str.len)
    return false; // Ditto
  *val = (size_t) result;
  return true;
}

// Chunk deletion marker is the MSB in the "processed" counter
#define MG_DMARK ((size_t) 1 << (sizeof (size_t) * 8 - 1))

// Multipart POST example:
// --xyz
// Content-Disposition: form-data; name="val"
//
// abcdef
// --xyz
// Content-Disposition: form-data; name="foo"; filename="a.txt"
// Content-Type: text/plain
//
// hello world
//
// --xyz--
size_t
mg_http_next_multipart (struct mg_str body, size_t ofs,
			struct mg_http_part *part)
{
  struct mg_str cd = mg_str_n ("Content-Disposition", 19);
  const char *s = body.buf;
  size_t b = ofs, h1, h2, b1, b2, max = body.len;

  // Init part params
  if (part != NULL)
    part->name = part->filename = part->body = mg_str_n (0, 0);

  // Skip boundary
  while (b + 2 < max && s[b] != '\r' && s[b + 1] != '\n')
    b++;
  if (b <= ofs || b + 2 >= max)
    return 0;
  // MG_INFO(("B: %zu %zu [%.*s]", ofs, b - ofs, (int) (b - ofs), s));

  // Skip headers
  h1 = h2 = b + 2;
  for (;;)
    {
      while (h2 + 2 < max && s[h2] != '\r' && s[h2 + 1] != '\n')
	h2++;
      if (h2 == h1)
	break;
      if (h2 + 2 >= max)
	return 0;
      // MG_INFO(("Header: [%.*s]", (int) (h2 - h1), &s[h1]));
      if (part != NULL && h1 + cd.len + 2 < h2 && s[h1 + cd.len] == ':'
	  && mg_ncasecmp (&s[h1], cd.buf, cd.len) == 0)
	{
	  struct mg_str v
	      = mg_str_n (&s[h1 + cd.len + 2], h2 - (h1 + cd.len + 2));
	  part->name = mg_http_get_header_var (v, mg_str_n ("name", 4));
	  part->filename
	      = mg_http_get_header_var (v, mg_str_n ("filename", 8));
	}
      h1 = h2 = h2 + 2;
    }
  b1 = b2 = h2 + 2;
  while (b2 + 2 + (b - ofs) + 2 < max
	 && !(s[b2] == '\r' && s[b2 + 1] == '\n'
	      && memcmp (&s[b2 + 2], s, b - ofs) == 0))
    b2++;

  if (b2 + 2 >= max)
    return 0;
  if (part != NULL)
    part->body = mg_str_n (&s[b1], b2 - b1);
  // MG_INFO(("Body: [%.*s]", (int) (b2 - b1), &s[b1]));
  return b2 + 2;
}

void
mg_http_bauth (struct mg_connection *c, const char *user, const char *pass)
{
  struct mg_str u = mg_str (user), p = mg_str (pass);
  size_t need = c->send.len + 36 + (u.len + p.len) * 2;
  if (c->send.size < need)
    mg_iobuf_resize (&c->send, need);
  if (c->send.size >= need)
    {
      size_t i, n = 0;
      char *buf = (char *) &c->send.buf[c->send.len];
      memcpy (buf, "Authorization: Basic ", 21); // DON'T use mg_send!
      for (i = 0; i < u.len; i++)
	{
	  n = mg_base64_update (((unsigned char *) u.buf)[i], buf + 21, n);
	}
      if (p.len > 0)
	{
	  n = mg_base64_update (':', buf + 21, n);
	  for (i = 0; i < p.len; i++)
	    {
	      n = mg_base64_update (((unsigned char *) p.buf)[i], buf + 21, n);
	    }
	}
      n = mg_base64_final (buf + 21, n);
      c->send.len += 21 + (size_t) n + 2;
      memcpy (&c->send.buf[c->send.len - 2], "\r\n", 2);
    }
  else
    {
      MG_ERROR (("%lu oom %d->%d ", c->id, (int) c->send.size, (int) need));
    }
}

struct mg_str
mg_http_var (struct mg_str buf, struct mg_str name)
{
  struct mg_str entry, k, v, result = mg_str_n (NULL, 0);
  while (mg_span (buf, &entry, &buf, '&'))
    {
      if (mg_span (entry, &k, &v, '=') && name.len == k.len
	  && mg_ncasecmp (name.buf, k.buf, k.len) == 0)
	{
	  result = v;
	  break;
	}
    }
  return result;
}

int
mg_http_get_var (const struct mg_str *buf, const char *name, char *dst,
		 size_t dst_len)
{
  int len;
  if (dst != NULL && dst_len > 0)
    {
      dst[0] = '\0'; // If destination buffer is valid, always nul-terminate it
    }
  if (dst == NULL || dst_len == 0)
    {
      len = -2; // Bad destination
    }
  else if (buf->buf == NULL || name == NULL || buf->len == 0)
    {
      len = -1; // Bad source
    }
  else
    {
      struct mg_str v = mg_http_var (*buf, mg_str (name));
      if (v.buf == NULL)
	{
	  len = -4; // Name does not exist
	}
      else
	{
	  len = mg_url_decode (v.buf, v.len, dst, dst_len, 1);
	  if (len < 0)
	    len = -3; // Failed to decode
	}
    }
  return len;
}

static bool
isx (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
	 || (c >= 'A' && c <= 'F');
}

int
mg_url_decode (const char *src, size_t src_len, char *dst, size_t dst_len,
	       int is_form_url_encoded)
{
  size_t i, j;
  for (i = j = 0; i < src_len && j + 1 < dst_len; i++, j++)
    {
      if (src[i] == '%')
	{
	  // Use `i + 2 < src_len`, not `i < src_len - 2`, note small src_len
	  if (i + 2 < src_len && isx (src[i + 1]) && isx (src[i + 2]))
	    {
	      mg_str_to_num (mg_str_n (src + i + 1, 2), 16, &dst[j],
			     sizeof (uint8_t));
	      i += 2;
	    }
	  else
	    {
	      return -1;
	    }
	}
      else if (is_form_url_encoded && src[i] == '+')
	{
	  dst[j] = ' ';
	}
      else
	{
	  dst[j] = src[i];
	}
    }
  if (j < dst_len)
    dst[j] = '\0'; // Null-terminate the destination
  return i >= src_len && j < dst_len ? (int) j : -1;
}

static bool
isok (uint8_t c)
{
  return c == '\n' || c == '\r' || c >= ' ';
}

int
mg_http_get_request_len (const unsigned char *buf, size_t buf_len)
{
  size_t i;
  for (i = 0; i < buf_len; i++)
    {
      if (!isok (buf[i]))
	return -1;
      if ((i > 0 && buf[i] == '\n' && buf[i - 1] == '\n')
	  || (i > 3 && buf[i] == '\n' && buf[i - 1] == '\r'
	      && buf[i - 2] == '\n'))
	return (int) i + 1;
    }
  return 0;
}
struct mg_str *
mg_http_get_header (struct mg_http_message *h, const char *name)
{
  size_t i, n = strlen (name),
	    max = sizeof (h->headers) / sizeof (h->headers[0]);
  for (i = 0; i < max && h->headers[i].name.len > 0; i++)
    {
      struct mg_str *k = &h->headers[i].name, *v = &h->headers[i].value;
      if (n == k->len && mg_ncasecmp (k->buf, name, n) == 0)
	return v;
    }
  return NULL;
}

// Is it a valid utf-8 continuation byte
static bool
vcb (uint8_t c)
{
  return (c & 0xc0) == 0x80;
}

// Get character length (valid utf-8). Used to parse method, URI, headers
static size_t
clen (const char *s, const char *end)
{
  const unsigned char *u = (unsigned char *) s, c = *u;
  long n = (long) (end - s);
  if (c > ' ' && c < '~')
    return 1; // Usual ascii printed char
  if ((c & 0xe0) == 0xc0 && n > 1 && vcb (u[1]))
    return 2; // 2-byte UTF8
  if ((c & 0xf0) == 0xe0 && n > 2 && vcb (u[1]) && vcb (u[2]))
    return 3;
  if ((c & 0xf8) == 0xf0 && n > 3 && vcb (u[1]) && vcb (u[2]) && vcb (u[3]))
    return 4;
  return 0;
}

// Skip until the newline. Return advanced `s`, or NULL on error
static const char *
skiptorn (const char *s, const char *end, struct mg_str *v)
{
  v->buf = (char *) s;
  while (s < end && s[0] != '\n' && s[0] != '\r')
    s++, v->len++; // To newline
  if (s >= end || (s[0] == '\r' && s[1] != '\n'))
    return NULL; // Stray \r
  if (s < end && s[0] == '\r')
    s++; // Skip \r
  if (s >= end || *s++ != '\n')
    return NULL; // Skip \n
  return s;
}

static bool
mg_http_parse_headers (const char *s, const char *end,
		       struct mg_http_header *h, size_t max_hdrs)
{
  size_t i, n;
  for (i = 0; i < max_hdrs; i++)
    {
      struct mg_str k = { NULL, 0 }, v = { NULL, 0 };
      if (s >= end)
	return false;
      if (s[0] == '\n' || (s[0] == '\r' && s[1] == '\n'))
	break;
      k.buf = (char *) s;
      while (s < end && s[0] != ':' && (n = clen (s, end)) > 0)
	s += n, k.len += n;
      if (k.len == 0)
	return false; // Empty name
      if (s >= end || clen (s, end) == 0)
	return false; // Invalid UTF-8
      if (*s++ != ':')
	return false; // Invalid, not followed by :
      // if (clen(s, end) == 0) return false;        // Invalid UTF-8
      while (s < end && s[0] == ' ')
	s++; // Skip spaces
      if ((s = skiptorn (s, end, &v)) == NULL)
	return false;
      while (v.len > 0 && v.buf[v.len - 1] == ' ')
	v.len--; // Trim spaces
      // MG_INFO(("--HH [%.*s] [%.*s]", (int) k.len, k.buf, (int) v.len,
      // v.buf));
      h[i].name = k, h[i].value = v; // Success. Assign values
    }
  return true;
}

int
mg_http_parse (const char *s, size_t len, struct mg_http_message *hm)
{
  int is_response,
      req_len = mg_http_get_request_len ((unsigned char *) s, len);
  const char *end = s == NULL ? NULL : s + req_len, *qs; // Cannot add to NULL
  const struct mg_str *cl;
  size_t n;

  memset (hm, 0, sizeof (*hm));
  if (req_len <= 0)
    return req_len;

  hm->message.buf = hm->head.buf = (char *) s;
  hm->body.buf = (char *) end;
  hm->head.len = (size_t) req_len;
  hm->message.len = hm->body.len = (size_t) -1; // Set body length to infinite

  // Parse request line
  hm->method.buf = (char *) s;
  while (s < end && (n = clen (s, end)) > 0)
    s += n, hm->method.len += n;
  while (s < end && s[0] == ' ')
    s++; // Skip spaces
  hm->uri.buf = (char *) s;
  while (s < end && (n = clen (s, end)) > 0)
    s += n, hm->uri.len += n;
  while (s < end && s[0] == ' ')
    s++; // Skip spaces
  if ((s = skiptorn (s, end, &hm->proto)) == NULL)
    return false;

  // If URI contains '?' character, setup query string
  if ((qs = (const char *) memchr (hm->uri.buf, '?', hm->uri.len)) != NULL)
    {
      hm->query.buf = (char *) qs + 1;
      hm->query.len = (size_t) (&hm->uri.buf[hm->uri.len] - (qs + 1));
      hm->uri.len = (size_t) (qs - hm->uri.buf);
    }

  // Sanity check. Allow protocol/reason to be empty
  // Do this check after hm->method.len and hm->uri.len are finalised
  if (hm->method.len == 0 || hm->uri.len == 0)
    return -1;

  if (!mg_http_parse_headers (s, end, hm->headers,
			      sizeof (hm->headers) / sizeof (hm->headers[0])))
    return -1; // error when parsing
  if ((cl = mg_http_get_header (hm, "Content-Length")) != NULL)
    {
      if (mg_to_size_t (*cl, &hm->body.len) == false)
	return -1;
      hm->message.len = (size_t) req_len + hm->body.len;
    }

  // mg_http_parse() is used to parse both HTTP requests and HTTP
  // responses. If HTTP response does not have Content-Length set, then
  // body is read until socket is closed, i.e. body.len is infinite (~0).
  //
  // For HTTP requests though, according to
  // http://tools.ietf.org/html/rfc7231#section-8.1.3,
  // only POST and PUT methods have defined body semantics.
  // Therefore, if Content-Length is not specified and methods are
  // not one of PUT or POST, set body length to 0.
  //
  // So, if it is HTTP request, and Content-Length is not set,
  // and method is not (PUT or POST) then reset body length to zero.
  is_response = mg_ncasecmp (hm->method.buf, "HTTP/", 5) == 0;
  if (hm->body.len == (size_t) ~0 && !is_response
      && mg_strcasecmp (hm->method, mg_str ("PUT")) != 0
      && mg_strcasecmp (hm->method, mg_str ("POST")) != 0)
    {
      hm->body.len = 0;
      hm->message.len = (size_t) req_len;
    }

  // The 204 (No content) responses also have 0 body length
  if (hm->body.len == (size_t) ~0 && is_response
      && mg_strcasecmp (hm->uri, mg_str ("204")) == 0)
    {
      hm->body.len = 0;
      hm->message.len = (size_t) req_len;
    }
  if (hm->message.len < (size_t) req_len)
    return -1; // Overflow protection

  return req_len;
}

static void
mg_http_vprintf_chunk (struct mg_connection *c, const char *fmt, va_list *ap)
{
  size_t len = c->send.len;
  mg_send (c, "        \r\n", 10);
  mg_vxprintf (mg_pfn_iobuf, &c->send, fmt, ap);
  if (c->send.len >= len + 10)
    {
      mg_snprintf ((char *) c->send.buf + len, 9, "%08lx",
		   c->send.len - len - 10);
      c->send.buf[len + 8] = '\r';
      if (c->send.len == len + 10)
	c->is_resp = 0; // Last chunk, reset marker
    }
  mg_send (c, "\r\n", 2);
}

void
mg_http_printf_chunk (struct mg_connection *c, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  mg_http_vprintf_chunk (c, fmt, &ap);
  va_end (ap);
}

void
mg_http_write_chunk (struct mg_connection *c, const char *buf, size_t len)
{
  mg_printf (c, "%lx\r\n", (unsigned long) len);
  mg_send (c, buf, len);
  mg_send (c, "\r\n", 2);
  if (len == 0)
    c->is_resp = 0;
}

// clang-format off
static const char *mg_http_status_code_str(int status_code) {
  switch (status_code) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 207: return "Multi-Status";
    case 208: return "Already Reported";
    case 226: return "IM Used";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Payload Too Large";
    case 414: return "Request-URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 418: return "I'm a teapot";
    case 421: return "Misdirected Request";
    case 422: return "Unprocessable Entity";
    case 423: return "Locked";
    case 424: return "Failed Dependency";
    case 426: return "Upgrade Required";
    case 428: return "Precondition Required";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 444: return "Connection Closed Without Response";
    case 451: return "Unavailable For Legal Reasons";
    case 499: return "Client Closed Request";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    case 506: return "Variant Also Negotiates";
    case 507: return "Insufficient Storage";
    case 508: return "Loop Detected";
    case 510: return "Not Extended";
    case 511: return "Network Authentication Required";
    case 599: return "Network Connect Timeout Error";
    default: return "";
  }
}
// clang-format on

void
mg_http_reply (struct mg_connection *c, int code, const char *headers,
	       const char *fmt, ...)
{
  va_list ap;
  size_t len;
  mg_printf (c, "HTTP/1.1 %d %s\r\n%sContent-Length:            \r\n\r\n",
	     code, mg_http_status_code_str (code),
	     headers == NULL ? "" : headers);
  len = c->send.len;
  va_start (ap, fmt);
  mg_vxprintf (mg_pfn_iobuf, &c->send, fmt, &ap);
  va_end (ap);
  if (c->send.len > 16)
    {
      size_t n = mg_snprintf ((char *) &c->send.buf[len - 15], 11, "%-10lu",
			      (unsigned long) (c->send.len - len));
      c->send.buf[len - 15 + n] = ' '; // Change ending 0 to space
    }
  c->is_resp = 0;
}

static void http_cb (struct mg_connection *, int, void *);
static void
restore_http_cb (struct mg_connection *c)
{
  mg_fs_close ((struct mg_fd *) c->pfn_data);
  c->pfn_data = NULL;
  c->pfn = http_cb;
  c->is_resp = 0;
}

char *mg_http_etag (char *buf, size_t len, size_t size, time_t mtime);
char *
mg_http_etag (char *buf, size_t len, size_t size, time_t mtime)
{
  mg_snprintf (buf, len, "\"%lld.%lld\"", (int64_t) mtime, (int64_t) size);
  return buf;
}

static void
static_cb (struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_WRITE || ev == MG_EV_POLL)
    {
      struct mg_fd *fd = (struct mg_fd *) c->pfn_data;
      // Read to send IO buffer directly, avoid extra on-stack buffer
      size_t n, max = MG_IO_SIZE, space;
      size_t *cl = (size_t *) &c->data[(sizeof (c->data) - sizeof (size_t))
				       / sizeof (size_t) * sizeof (size_t)];
      if (c->send.size < max)
	mg_iobuf_resize (&c->send, max);
      if (c->send.len >= c->send.size)
	return; // Rate limit
      if ((space = c->send.size - c->send.len) > *cl)
	space = *cl;
      n = fd->fs->rd (fd->fd, c->send.buf + c->send.len, space);
      c->send.len += n;
      *cl -= n;
      if (n == 0)
	restore_http_cb (c);
    }
  else if (ev == MG_EV_CLOSE)
    {
      restore_http_cb (c);
    }
  (void) ev_data;
}

// Known mime types. Keep it outside guess_content_type() function, since
// some environments don't like it defined there.
// clang-format off
#define MG_C_STR(a) { (char *) (a), sizeof(a) - 1 }
static struct mg_str s_known_types[] = {
    MG_C_STR("html"), MG_C_STR("text/html; charset=utf-8"),
    MG_C_STR("htm"), MG_C_STR("text/html; charset=utf-8"),
    MG_C_STR("css"), MG_C_STR("text/css; charset=utf-8"),
    MG_C_STR("js"), MG_C_STR("text/javascript; charset=utf-8"),
    MG_C_STR("gif"), MG_C_STR("image/gif"),
    MG_C_STR("png"), MG_C_STR("image/png"),
    MG_C_STR("jpg"), MG_C_STR("image/jpeg"),
    MG_C_STR("jpeg"), MG_C_STR("image/jpeg"),
    MG_C_STR("woff"), MG_C_STR("font/woff"),
    MG_C_STR("ttf"), MG_C_STR("font/ttf"),
    MG_C_STR("svg"), MG_C_STR("image/svg+xml"),
    MG_C_STR("txt"), MG_C_STR("text/plain; charset=utf-8"),
    MG_C_STR("avi"), MG_C_STR("video/x-msvideo"),
    MG_C_STR("csv"), MG_C_STR("text/csv"),
    MG_C_STR("doc"), MG_C_STR("application/msword"),
    MG_C_STR("exe"), MG_C_STR("application/octet-stream"),
    MG_C_STR("gz"), MG_C_STR("application/gzip"),
    MG_C_STR("ico"), MG_C_STR("image/x-icon"),
    MG_C_STR("json"), MG_C_STR("application/json"),
    MG_C_STR("mov"), MG_C_STR("video/quicktime"),
    MG_C_STR("mp3"), MG_C_STR("audio/mpeg"),
    MG_C_STR("mp4"), MG_C_STR("video/mp4"),
    MG_C_STR("mpeg"), MG_C_STR("video/mpeg"),
    MG_C_STR("pdf"), MG_C_STR("application/pdf"),
    MG_C_STR("shtml"), MG_C_STR("text/html; charset=utf-8"),
    MG_C_STR("tgz"), MG_C_STR("application/tar-gz"),
    MG_C_STR("wav"), MG_C_STR("audio/wav"),
    MG_C_STR("webp"), MG_C_STR("image/webp"),
    MG_C_STR("zip"), MG_C_STR("application/zip"),
    MG_C_STR("3gp"), MG_C_STR("video/3gpp"),
    {0, 0},
};
// clang-format on

static struct mg_str
guess_content_type (struct mg_str path, const char *extra)
{
  struct mg_str entry, k, v, s = mg_str (extra);
  size_t i = 0;

  // Shrink path to its extension only
  while (i < path.len && path.buf[path.len - i - 1] != '.')
    i++;
  path.buf += path.len - i;
  path.len = i;

  // Process user-provided mime type overrides, if any
  while (mg_span (s, &entry, &s, ','))
    {
      if (mg_span (entry, &k, &v, '=') && mg_strcmp (path, k) == 0)
	return v;
    }

  // Process built-in mime types
  for (i = 0; s_known_types[i].buf != NULL; i += 2)
    {
      if (mg_strcmp (path, s_known_types[i]) == 0)
	return s_known_types[i + 1];
    }

  return mg_str ("text/plain; charset=utf-8");
}

static int
getrange (struct mg_str *s, size_t *a, size_t *b)
{
  size_t i, numparsed = 0;
  for (i = 0; i + 6 < s->len; i++)
    {
      struct mg_str k, v = mg_str_n (s->buf + i + 6, s->len - i - 6);
      if (memcmp (&s->buf[i], "bytes=", 6) != 0)
	continue;
      if (mg_span (v, &k, &v, '-'))
	{
	  if (mg_to_size_t (k, a))
	    numparsed++;
	  if (v.len > 0 && mg_to_size_t (v, b))
	    numparsed++;
	}
      else
	{
	  if (mg_to_size_t (v, a))
	    numparsed++;
	}
      break;
    }
  return (int) numparsed;
}

void
mg_http_serve_file (struct mg_connection *c, struct mg_http_message *hm,
		    const char *path, const struct mg_http_serve_opts *opts)
{
  char etag[64], tmp[MG_PATH_MAX];
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct mg_fd *fd = NULL;
  size_t size = 0;
  time_t mtime = 0;
  struct mg_str *inm = NULL;
  struct mg_str mime = guess_content_type (mg_str (path), opts->mime_types);
  bool gzip = false;

  if (path != NULL)
    {
      // If a browser sends us "Accept-Encoding: gzip", try to open .gz first
      struct mg_str *ae = mg_http_get_header (hm, "Accept-Encoding");
      if (ae != NULL)
	{
	  char *ae_ = mg_mprintf ("%.*s", ae->len, ae->buf);
	  if (ae_ != NULL && strstr (ae_, "gzip") != NULL)
	    {
	      mg_snprintf (tmp, sizeof (tmp), "%s.gz", path);
	      fd = mg_fs_open (fs, tmp, MG_FS_READ);
	      if (fd != NULL)
		gzip = true, path = tmp;
	    }
	  free (ae_);
	}
      // No luck opening .gz? Open what we've told to open
      if (fd == NULL)
	fd = mg_fs_open (fs, path, MG_FS_READ);
    }

  // Failed to open, and page404 is configured? Open it, then
  if (fd == NULL && opts->page404 != NULL)
    {
      fd = mg_fs_open (fs, opts->page404, MG_FS_READ);
      path = opts->page404;
      mime = guess_content_type (mg_str (path), opts->mime_types);
    }

  if (fd == NULL || fs->st (path, &size, &mtime) == 0)
    {
      mg_http_reply (c, 404, opts->extra_headers, "Not found\n");
      mg_fs_close (fd);
      // NOTE: mg_http_etag() call should go first!
    }
  else if (mg_http_etag (etag, sizeof (etag), size, mtime) != NULL
	   && (inm = mg_http_get_header (hm, "If-None-Match")) != NULL
	   && mg_strcasecmp (*inm, mg_str (etag)) == 0)
    {
      mg_fs_close (fd);
      mg_http_reply (c, 304, opts->extra_headers, "");
    }
  else
    {
      int n, status = 200;
      char range[100];
      size_t r1 = 0, r2 = 0, cl = size;

      // Handle Range header
      struct mg_str *rh = mg_http_get_header (hm, "Range");
      range[0] = '\0';
      if (rh != NULL && (n = getrange (rh, &r1, &r2)) > 0)
	{
	  // If range is specified like "400-", set second limit to content len
	  if (n == 1)
	    r2 = cl - 1;
	  if (r1 > r2 || r2 >= cl)
	    {
	      status = 416;
	      cl = 0;
	      mg_snprintf (range, sizeof (range),
			   "Content-Range: bytes */%lld\r\n", (int64_t) size);
	    }
	  else
	    {
	      status = 206;
	      cl = r2 - r1 + 1;
	      mg_snprintf (range, sizeof (range),
			   "Content-Range: bytes %llu-%llu/%llu\r\n",
			   (uint64_t) r1, (uint64_t) (r1 + cl - 1),
			   (uint64_t) size);
	      fs->sk (fd->fd, r1);
	    }
	}
      mg_printf (c,
		 "HTTP/1.1 %d %s\r\n"
		 "Content-Type: %.*s\r\n"
		 "Etag: %s\r\n"
		 "Content-Length: %llu\r\n"
		 "%s%s%s\r\n",
		 status, mg_http_status_code_str (status), (int) mime.len,
		 mime.buf, etag, (uint64_t) cl,
		 gzip ? "Content-Encoding: gzip\r\n" : "", range,
		 opts->extra_headers ? opts->extra_headers : "");
      if (mg_strcasecmp (hm->method, mg_str ("HEAD")) == 0)
	{
	  c->is_draining = 1;
	  c->is_resp = 0;
	  mg_fs_close (fd);
	}
      else
	{
	  // Track to-be-sent content length at the end of c->data, aligned
	  size_t *clp
	      = (size_t *) &c->data[(sizeof (c->data) - sizeof (size_t))
				    / sizeof (size_t) * sizeof (size_t)];
	  c->pfn = static_cb;
	  c->pfn_data = fd;
	  *clp = cl;
	}
    }
}

struct printdirentrydata
{
  struct mg_connection *c;
  struct mg_http_message *hm;
  const struct mg_http_serve_opts *opts;
  const char *dir;
};

#if MG_ENABLE_DIRLIST
static void
printdirentry (const char *name, void *userdata)
{
  struct printdirentrydata *d = (struct printdirentrydata *) userdata;
  struct mg_fs *fs = d->opts->fs == NULL ? &mg_fs_posix : d->opts->fs;
  size_t size = 0;
  time_t t = 0;
  char path[MG_PATH_MAX], sz[40], mod[40];
  int flags, n = 0;

  // MG_DEBUG(("[%s] [%s]", d->dir, name));
  if (mg_snprintf (path, sizeof (path), "%s%c%s", d->dir, '/', name)
      > sizeof (path))
    {
      MG_ERROR (("%s truncated", name));
    }
  else if ((flags = fs->st (path, &size, &t)) == 0)
    {
      MG_ERROR (("%lu stat(%s): %d", d->c->id, path, errno));
    }
  else
    {
      const char *slash = flags & MG_FS_DIR ? "/" : "";
      if (flags & MG_FS_DIR)
	{
	  mg_snprintf (sz, sizeof (sz), "%s", "[DIR]");
	}
      else
	{
	  mg_snprintf (sz, sizeof (sz), "%lld", (uint64_t) size);
	}
#  if defined(MG_HTTP_DIRLIST_TIME_FMT)
      {
	char time_str[40];
	struct tm *time_info = localtime (&t);
	strftime (time_str, sizeof time_str, "%Y/%m/%d %H:%M:%S", time_info);
	mg_snprintf (mod, sizeof (mod), "%s", time_str);
      }
#  else
      mg_snprintf (mod, sizeof (mod), "%lu", (unsigned long) t);
#  endif
      n = (int) mg_url_encode (name, strlen (name), path, sizeof (path));
      mg_printf (d->c,
		 "  <tr><td><a href=\"%.*s%s\">%s%s</a></td>"
		 "<td name=%lu>%s</td><td name=%lld>%s</td></tr>\n",
		 n, path, slash, name, slash, (unsigned long) t, mod,
		 flags & MG_FS_DIR ? (int64_t) -1 : (int64_t) size, sz);
    }
}

static void
listdir (struct mg_connection *c, struct mg_http_message *hm,
	 const struct mg_http_serve_opts *opts, char *dir)
{
  const char *sort_js_code
      = "<script>function srt(tb, sc, so, d) {"
	"var tr = Array.prototype.slice.call(tb.rows, 0),"
	"tr = tr.sort(function (a, b) { var c1 = a.cells[sc], c2 = "
	"b.cells[sc],"
	"n1 = c1.getAttribute('name'), n2 = c2.getAttribute('name'), "
	"t1 = a.cells[2].getAttribute('name'), "
	"t2 = b.cells[2].getAttribute('name'); "
	"return so * (t1 < 0 && t2 >= 0 ? -1 : t2 < 0 && t1 >= 0 ? 1 : "
	"n1 ? parseInt(n2) - parseInt(n1) : "
	"c1.textContent.trim().localeCompare(c2.textContent.trim())); });";
  const char *sort_js_code2
      = "for (var i = 0; i < tr.length; i++) tb.appendChild(tr[i]); "
	"if (!d) window.location.hash = ('sc=' + sc + '&so=' + so); "
	"};"
	"window.onload = function() {"
	"var tb = document.getElementById('tb');"
	"var m = /sc=([012]).so=(1|-1)/.exec(window.location.hash) || [0, 2, "
	"1];"
	"var sc = m[1], so = m[2]; document.onclick = function(ev) { "
	"var c = ev.target.rel; if (c) {if (c == sc) so *= -1; srt(tb, c, "
	"so); "
	"sc = c; ev.preventDefault();}};"
	"srt(tb, sc, so, true);"
	"}"
	"</script>";
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct printdirentrydata d = { c, hm, opts, dir };
  char tmp[10], buf[MG_PATH_MAX];
  size_t off, n;
  int len = mg_url_decode (hm->uri.buf, hm->uri.len, buf, sizeof (buf), 0);
  struct mg_str uri = len > 0 ? mg_str_n (buf, (size_t) len) : hm->uri;

  mg_printf (c,
	     "HTTP/1.1 200 OK\r\n"
	     "Content-Type: text/html; charset=utf-8\r\n"
	     "%s"
	     "Content-Length:         \r\n\r\n",
	     opts->extra_headers == NULL ? "" : opts->extra_headers);
  off = c->send.len; // Start of body
  mg_printf (c,
	     "<!DOCTYPE html><html><head><title>Index of %.*s</title>%s%s"
	     "<style>th,td {text-align: left; padding-right: 1em; "
	     "font-family: monospace; }</style></head>"
	     "<body><h1>Index of %.*s</h1><table cellpadding=\"0\"><thead>"
	     "<tr><th><a href=\"#\" rel=\"0\">Name</a></th><th>"
	     "<a href=\"#\" rel=\"1\">Modified</a></th>"
	     "<th><a href=\"#\" rel=\"2\">Size</a></th></tr>"
	     "<tr><td colspan=\"3\"><hr></td></tr>"
	     "</thead>"
	     "<tbody id=\"tb\">\n",
	     (int) uri.len, uri.buf, sort_js_code, sort_js_code2,
	     (int) uri.len, uri.buf);
  mg_printf (c, "%s",
	     "  <tr><td><a href=\"..\">..</a></td>"
	     "<td name=-1></td><td name=-1>[DIR]</td></tr>\n");

  fs->ls (dir, printdirentry, &d);
  mg_printf (c,
	     "</tbody><tfoot><tr><td colspan=\"3\"><hr></td></tr></tfoot>"
	     "</table><address>Mongoose v.%s</address></body></html>\n",
	     MG_VERSION);
  n = mg_snprintf (tmp, sizeof (tmp), "%lu",
		   (unsigned long) (c->send.len - off));
  if (n > sizeof (tmp))
    n = 0;
  memcpy (c->send.buf + off - 12, tmp, n); // Set content length
  c->is_resp = 0;			   // Mark response end
}
#endif

// Resolve requested file into `path` and return its fs->st() result
static int
uri_to_path2 (struct mg_connection *c, struct mg_http_message *hm,
	      struct mg_fs *fs, struct mg_str url, struct mg_str dir,
	      char *path, size_t path_size)
{
  int flags, tmp;
  // Append URI to the root_dir, and sanitize it
  size_t n = mg_snprintf (path, path_size, "%.*s", (int) dir.len, dir.buf);
  if (n + 2 >= path_size)
    {
      mg_http_reply (c, 400, "", "Exceeded path size");
      return -1;
    }
  path[path_size - 1] = '\0';
  // Terminate root dir with slash
  if (n > 0 && path[n - 1] != '/')
    path[n++] = '/', path[n] = '\0';
  if (url.len < hm->uri.len)
    {
      mg_url_decode (hm->uri.buf + url.len, hm->uri.len - url.len, path + n,
		     path_size - n, 0);
    }
  path[path_size - 1] = '\0'; // Double-check
  if (!mg_path_is_sane (mg_str_n (path, path_size)))
    {
      mg_http_reply (c, 400, "", "Invalid path");
      return -1;
    }
  n = strlen (path);
  while (n > 1 && path[n - 1] == '/')
    path[--n] = 0; // Trim trailing slashes
  flags = mg_strcmp (hm->uri, mg_str ("/")) == 0 ? MG_FS_DIR
						 : fs->st (path, NULL, NULL);
  MG_VERBOSE (("%lu %.*s -> %s %d", c->id, (int) hm->uri.len, hm->uri.buf,
	       path, flags));
  if (flags == 0)
    {
      // Do nothing - let's caller decide
    }
  else if ((flags & MG_FS_DIR) && hm->uri.len > 0
	   && hm->uri.buf[hm->uri.len - 1] != '/')
    {
      mg_printf (c,
		 "HTTP/1.1 301 Moved\r\n"
		 "Location: %.*s/\r\n"
		 "Content-Length: 0\r\n"
		 "\r\n",
		 (int) hm->uri.len, hm->uri.buf);
      c->is_resp = 0;
      flags = -1;
    }
  else if (flags & MG_FS_DIR)
    {
      if (((mg_snprintf (path + n, path_size - n, "/" MG_HTTP_INDEX) > 0
	    && (tmp = fs->st (path, NULL, NULL)) != 0)
	   || (mg_snprintf (path + n, path_size - n, "/index.shtml") > 0
	       && (tmp = fs->st (path, NULL, NULL)) != 0)))
	{
	  flags = tmp;
	}
      else if ((mg_snprintf (path + n, path_size - n, "/" MG_HTTP_INDEX ".gz")
		    > 0
		&& (tmp = fs->st (path, NULL, NULL)) != 0))
	{ // check for gzipped index
	  flags = tmp;
	  path[n + 1 + strlen (MG_HTTP_INDEX)]
	      = '\0'; // Remove appended .gz in index file name
	}
      else
	{
	  path[n] = '\0'; // Remove appended index file name
	}
    }
  return flags;
}

static int
uri_to_path (struct mg_connection *c, struct mg_http_message *hm,
	     const struct mg_http_serve_opts *opts, char *path,
	     size_t path_size)
{
  struct mg_fs *fs = opts->fs == NULL ? &mg_fs_posix : opts->fs;
  struct mg_str k, v, part, s = mg_str (opts->root_dir), u = { NULL, 0 },
			    p = u;
  while (mg_span (s, &part, &s, ','))
    {
      if (!mg_span (part, &k, &v, '='))
	k = part, v = mg_str_n (NULL, 0);
      if (v.len == 0)
	v = k, k = mg_str ("/"), u = k, p = v;
      if (hm->uri.len < k.len)
	continue;
      if (mg_strcmp (k, mg_str_n (hm->uri.buf, k.len)) != 0)
	continue;
      u = k, p = v;
    }
  return uri_to_path2 (c, hm, fs, u, p, path, path_size);
}

void
mg_http_serve_dir (struct mg_connection *c, struct mg_http_message *hm,
		   const struct mg_http_serve_opts *opts)
{
  char path[MG_PATH_MAX];
  const char *sp = opts->ssi_pattern;
  int flags = uri_to_path (c, hm, opts, path, sizeof (path));
  if (flags < 0)
    {
      // Do nothing: the response has already been sent by uri_to_path()
    }
  else if (flags & MG_FS_DIR)
    {
#if MG_ENABLE_DIRLIST
      listdir (c, hm, opts, path);
#else
      mg_http_reply (c, 403, "", "Forbidden\n");
#endif
    }
  else if (flags && sp != NULL && mg_match (mg_str (path), mg_str (sp), NULL))
    {
      mg_http_serve_ssi (c, opts->root_dir, path);
    }
  else
    {
      mg_http_serve_file (c, hm, path, opts);
    }
}

static bool
mg_is_url_safe (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z')
	 || (c >= 'A' && c <= 'Z') || c == '.' || c == '_' || c == '-'
	 || c == '~';
}

size_t
mg_url_encode (const char *s, size_t sl, char *buf, size_t len)
{
  size_t i, n = 0;
  for (i = 0; i < sl; i++)
    {
      int c = *(unsigned char *) &s[i];
      if (n + 4 >= len)
	return 0;
      if (mg_is_url_safe (c))
	{
	  buf[n++] = s[i];
	}
      else
	{
	  mg_snprintf (&buf[n], 4, "%%%M", mg_print_hex, 1, &s[i]);
	  n += 3;
	}
    }
  if (len > 0 && n < len - 1)
    buf[n] = '\0'; // Null-terminate the destination
  if (len > 0)
    buf[len - 1] = '\0'; // Always.
  return n;
}

void
mg_http_creds (struct mg_http_message *hm, char *user, size_t userlen,
	       char *pass, size_t passlen)
{
  struct mg_str *v = mg_http_get_header (hm, "Authorization");
  user[0] = pass[0] = '\0';
  if (v != NULL && v->len > 6 && memcmp (v->buf, "Basic ", 6) == 0)
    {
      char buf[256];
      size_t n = mg_base64_decode (v->buf + 6, v->len - 6, buf, sizeof (buf));
      const char *p = (const char *) memchr (buf, ':', n > 0 ? n : 0);
      if (p != NULL)
	{
	  mg_snprintf (user, userlen, "%.*s", p - buf, buf);
	  mg_snprintf (pass, passlen, "%.*s", n - (size_t) (p - buf) - 1,
		       p + 1);
	}
    }
  else if (v != NULL && v->len > 7 && memcmp (v->buf, "Bearer ", 7) == 0)
    {
      mg_snprintf (pass, passlen, "%.*s", (int) v->len - 7, v->buf + 7);
    }
  else if ((v = mg_http_get_header (hm, "Cookie")) != NULL)
    {
      struct mg_str t
	  = mg_http_get_header_var (*v, mg_str_n ("access_token", 12));
      if (t.len > 0)
	mg_snprintf (pass, passlen, "%.*s", (int) t.len, t.buf);
    }
  else
    {
      mg_http_get_var (&hm->query, "access_token", pass, passlen);
    }
}

static struct mg_str
stripquotes (struct mg_str s)
{
  return s.len > 1 && s.buf[0] == '"' && s.buf[s.len - 1] == '"'
	     ? mg_str_n (s.buf + 1, s.len - 2)
	     : s;
}

struct mg_str
mg_http_get_header_var (struct mg_str s, struct mg_str v)
{
  size_t i;
  for (i = 0; v.len > 0 && i + v.len + 2 < s.len; i++)
    {
      if (s.buf[i + v.len] == '=' && memcmp (&s.buf[i], v.buf, v.len) == 0)
	{
	  const char *p = &s.buf[i + v.len + 1], *b = p, *x = &s.buf[s.len];
	  int q = p < x && *p == '"' ? 1 : 0;
	  while (p < x
		 && (q ? p == b || *p != '"'
		       : *p != ';' && *p != ' ' && *p != ','))
	    p++;
	  // MG_INFO(("[%.*s] [%.*s] [%.*s]", (int) s.len, s.buf, (int) v.len,
	  // v.buf, (int) (p - b), b));
	  return stripquotes (mg_str_n (b, (size_t) (p - b + q)));
	}
    }
  return mg_str_n (NULL, 0);
}

long
mg_http_upload (struct mg_connection *c, struct mg_http_message *hm,
		struct mg_fs *fs, const char *dir, size_t max_size)
{
  char buf[20] = "0", file[MG_PATH_MAX], path[MG_PATH_MAX];
  long res = 0, offset;
  mg_http_get_var (&hm->query, "offset", buf, sizeof (buf));
  mg_http_get_var (&hm->query, "file", file, sizeof (file));
  offset = strtol (buf, NULL, 0);
  mg_snprintf (path, sizeof (path), "%s%c%s", dir, MG_DIRSEP, file);
  if (hm->body.len == 0)
    {
      mg_http_reply (c, 200, "", "%ld", res); // Nothing to write
    }
  else if (file[0] == '\0')
    {
      mg_http_reply (c, 400, "", "file required");
      res = -1;
    }
  else if (mg_path_is_sane (mg_str (file)) == false)
    {
      mg_http_reply (c, 400, "", "%s: invalid file", file);
      res = -2;
    }
  else if (offset < 0)
    {
      mg_http_reply (c, 400, "", "offset required");
      res = -3;
    }
  else if ((size_t) offset + hm->body.len > max_size)
    {
      mg_http_reply (c, 400, "", "%s: over max size of %lu", path,
		     (unsigned long) max_size);
      res = -4;
    }
  else
    {
      struct mg_fd *fd;
      size_t current_size = 0;
      MG_DEBUG (("%s -> %lu bytes @ %ld", path, hm->body.len, offset));
      if (offset == 0)
	fs->rm (path); // If offset if 0, truncate file
      fs->st (path, &current_size, NULL);
      if (offset > 0 && current_size != (size_t) offset)
	{
	  mg_http_reply (c, 400, "", "%s: offset mismatch", path);
	  res = -5;
	}
      else if ((fd = mg_fs_open (fs, path, MG_FS_WRITE)) == NULL)
	{
	  mg_http_reply (c, 400, "", "open(%s): %d", path, errno);
	  res = -6;
	}
      else
	{
	  res = offset + (long) fs->wr (fd->fd, hm->body.buf, hm->body.len);
	  mg_fs_close (fd);
	  mg_http_reply (c, 200, "", "%ld", res);
	}
    }
  return res;
}

int
mg_http_status (const struct mg_http_message *hm)
{
  return atoi (hm->uri.buf);
}

static bool
is_hex_digit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
	 || (c >= 'A' && c <= 'F');
}

static int
skip_chunk (const char *buf, int len, int *pl, int *dl)
{
  int i = 0, n = 0;
  if (len < 3)
    return 0;
  while (i < len && is_hex_digit (buf[i]))
    i++;
  if (i == 0)
    return -1; // Error, no length specified
  if (i > (int) sizeof (int) * 2)
    return -1; // Chunk length is too big
  if (len < i + 1 || buf[i] != '\r' || buf[i + 1] != '\n')
    return -1; // Error
  if (mg_str_to_num (mg_str_n (buf, (size_t) i), 16, &n, sizeof (int))
      == false)
    return -1; // Decode chunk length, overflow
  if (n < 0)
    return -1; // Error. TODO(): some checks now redundant
  if (n > len - i - 4)
    return 0; // Chunk not yet fully buffered
  if (buf[i + n + 2] != '\r' || buf[i + n + 3] != '\n')
    return -1; // Error
  *pl = i + 2, *dl = n;
  return i + 2 + n + 2;
}

static void
http_cb (struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_READ || ev == MG_EV_CLOSE)
    {
      struct mg_http_message hm;
      size_t ofs = 0; // Parsing offset
      while (c->is_resp == 0 && ofs < c->recv.len)
	{
	  const char *buf = (char *) c->recv.buf + ofs;
	  int n = mg_http_parse (buf, c->recv.len - ofs, &hm);
	  struct mg_str *te; // Transfer - encoding header
	  bool is_chunked = false;
	  if (n < 0)
	    {
	      // We don't use mg_error() here, to avoid closing pipelined
	      // requests prematurely, see #2592
	      MG_ERROR (("HTTP parse, %lu bytes", c->recv.len));
	      c->is_draining = 1;
	      mg_hexdump (buf,
			  c->recv.len - ofs > 16 ? 16 : c->recv.len - ofs);
	      c->recv.len = 0;
	      return;
	    }
	  if (n == 0)
	    break;			     // Request is not buffered yet
	  mg_call (c, MG_EV_HTTP_HDRS, &hm); // Got all HTTP headers
	  if (ev == MG_EV_CLOSE)
	    { // If client did not set Content-Length
	      hm.message.len
		  = c->recv.len - ofs; // and closes now, deliver MSG
	      hm.body.len
		  = hm.message.len - (size_t) (hm.body.buf - hm.message.buf);
	    }
	  if ((te = mg_http_get_header (&hm, "Transfer-Encoding")) != NULL)
	    {
	      if (mg_strcasecmp (*te, mg_str ("chunked")) == 0)
		{
		  is_chunked = true;
		}
	      else
		{
		  mg_error (c, "Invalid Transfer-Encoding"); // See #2460
		  return;
		}
	    }
	  else if (mg_http_get_header (&hm, "Content-length") == NULL)
	    {
	      // #2593: HTTP packets must contain either Transfer-Encoding or
	      // Content-length
	      bool is_response = mg_ncasecmp (hm.method.buf, "HTTP/", 5) == 0;
	      bool require_content_len = false;
	      if (!is_response
		  && (mg_strcasecmp (hm.method, mg_str ("POST")) == 0
		      || mg_strcasecmp (hm.method, mg_str ("PUT")) == 0))
		{
		  // POST and PUT should include an entity body. Therefore,
		  // they should contain a Content-length header. Other
		  // requests can also contain a body, but their content has no
		  // defined semantics (RFC 7231)
		  require_content_len = true;
		}
	      else if (is_response)
		{
		  // HTTP spec 7.2 Entity body: All other responses must
		  // include a body or Content-Length header field defined with
		  // a value of 0.
		  int status = mg_http_status (&hm);
		  require_content_len
		      = status >= 200 && status != 204 && status != 304;
		}
	      if (require_content_len)
		{
		  mg_http_reply (c, 411, "", "");
		  MG_ERROR (("%s", "Content length missing from request"));
		}
	    }

	  if (is_chunked)
	    {
	      // For chunked data, strip off prefixes and suffixes from chunks
	      // and relocate them right after the headers, then report a
	      // message
	      char *s = (char *) c->recv.buf + ofs + n;
	      int o = 0, pl, dl, cl,
		  len = (int) (c->recv.len - ofs - (size_t) n);

	      // Find zero-length chunk (the end of the body)
	      while ((cl = skip_chunk (s + o, len - o, &pl, &dl)) > 0 && dl)
		o += cl;
	      if (cl == 0)
		break; // No zero-len chunk, buffer more data
	      if (cl < 0)
		{
		  mg_error (c, "Invalid chunk");
		  break;
		}

	      // Zero chunk found. Second pass: strip + relocate
	      o = 0, hm.body.len = 0, hm.message.len = (size_t) n;
	      while ((cl = skip_chunk (s + o, len - o, &pl, &dl)) > 0)
		{
		  memmove (s + hm.body.len, s + o + pl, (size_t) dl);
		  o += cl, hm.body.len += (size_t) dl,
		      hm.message.len += (size_t) dl;
		  if (dl == 0)
		    break;
		}
	      ofs += (size_t) (n + o);
	    }
	  else
	    { // Normal, non-chunked data
	      size_t len = c->recv.len - ofs - (size_t) n;
	      if (hm.body.len > len)
		break; // Buffer more data
	      ofs += (size_t) n + hm.body.len;
	    }

	  if (c->is_accepted)
	    c->is_resp = 1;		    // Start generating response
	  mg_call (c, MG_EV_HTTP_MSG, &hm); // User handler can clear is_resp
	}
      if (ofs > 0)
	mg_iobuf_del (&c->recv, 0, ofs); // Delete processed data
    }
  (void) ev_data;
}

static void
mg_hfn (struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_HTTP_MSG)
    {
      struct mg_http_message *hm = (struct mg_http_message *) ev_data;
      if (mg_match (hm->uri, mg_str ("/quit"), NULL))
	{
	  mg_http_reply (c, 200, "", "ok\n");
	  c->is_draining = 1;
	  c->data[0] = 'X';
	}
      else if (mg_match (hm->uri, mg_str ("/debug"), NULL))
	{
	  int level
	      = (int) mg_json_get_long (hm->body, "$.level", MG_LL_DEBUG);
	  mg_log_set (level);
	  mg_http_reply (c, 200, "", "Debug level set to %d\n", level);
	}
      else
	{
	  mg_http_reply (c, 200, "", "hi\n");
	}
    }
  else if (ev == MG_EV_CLOSE)
    {
      if (c->data[0] == 'X')
	*(bool *) c->fn_data = true;
    }
}

void
mg_hello (const char *url)
{
  struct mg_mgr mgr;
  bool done = false;
  mg_mgr_init (&mgr);
  if (mg_http_listen (&mgr, url, mg_hfn, &done) == NULL)
    done = true;
  while (done == false)
    mg_mgr_poll (&mgr, 100);
  mg_mgr_free (&mgr);
}

struct mg_connection *
mg_http_connect (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
		 void *fn_data)
{
  struct mg_connection *c = mg_connect (mgr, url, fn, fn_data);
  if (c != NULL)
    c->pfn = http_cb;
  return c;
}

struct mg_connection *
mg_http_listen (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
		void *fn_data)
{
  struct mg_connection *c = mg_listen (mgr, url, fn, fn_data);
  if (c != NULL)
    c->pfn = http_cb;
  return c;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/iobuf.c"
#endif

static size_t
roundup (size_t size, size_t align)
{
  return align == 0 ? size : (size + align - 1) / align * align;
}

int
mg_iobuf_resize (struct mg_iobuf *io, size_t new_size)
{
  int ok = 1;
  new_size = roundup (new_size, io->align);
  if (new_size == 0)
    {
      mg_bzero (io->buf, io->size);
      free (io->buf);
      io->buf = NULL;
      io->len = io->size = 0;
    }
  else if (new_size != io->size)
    {
      // NOTE(lsm): do not use realloc here. Use calloc/free only, to ease the
      // porting to some obscure platforms like FreeRTOS
      void *p = calloc (1, new_size);
      if (p != NULL)
	{
	  size_t len = new_size < io->len ? new_size : io->len;
	  if (len > 0 && io->buf != NULL)
	    memmove (p, io->buf, len);
	  mg_bzero (io->buf, io->size);
	  free (io->buf);
	  io->buf = (unsigned char *) p;
	  io->size = new_size;
	}
      else
	{
	  ok = 0;
	  MG_ERROR (("%lld->%lld", (uint64_t) io->size, (uint64_t) new_size));
	}
    }
  return ok;
}

int
mg_iobuf_init (struct mg_iobuf *io, size_t size, size_t align)
{
  io->buf = NULL;
  io->align = align;
  io->size = io->len = 0;
  return mg_iobuf_resize (io, size);
}

size_t
mg_iobuf_add (struct mg_iobuf *io, size_t ofs, const void *buf, size_t len)
{
  size_t new_size = roundup (io->len + len, io->align);
  mg_iobuf_resize (io, new_size); // Attempt to resize
  if (new_size != io->size)
    len = 0; // Resize failure, append nothing
  if (ofs < io->len)
    memmove (io->buf + ofs + len, io->buf + ofs, io->len - ofs);
  if (buf != NULL)
    memmove (io->buf + ofs, buf, len);
  if (ofs > io->len)
    io->len += ofs - io->len;
  io->len += len;
  return len;
}

size_t
mg_iobuf_del (struct mg_iobuf *io, size_t ofs, size_t len)
{
  if (ofs > io->len)
    ofs = io->len;
  if (ofs + len > io->len)
    len = io->len - ofs;
  if (io->buf)
    memmove (io->buf + ofs, io->buf + ofs + len, io->len - ofs - len);
  if (io->buf)
    mg_bzero (io->buf + io->len - len, len);
  io->len -= len;
  return len;
}

void
mg_iobuf_free (struct mg_iobuf *io)
{
  mg_iobuf_resize (io, 0);
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/json.c"
#endif

static const char *
escapeseq (int esc)
{
  return esc ? "\b\f\n\r\t\\\"" : "bfnrt\\\"";
}

static char
json_esc (int c, int esc)
{
  const char *p, *esc1 = escapeseq (esc), *esc2 = escapeseq (!esc);
  for (p = esc1; *p != '\0'; p++)
    {
      if (*p == c)
	return esc2[p - esc1];
    }
  return 0;
}

static int
mg_pass_string (const char *s, int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      if (s[i] == '\\' && i + 1 < len && json_esc (s[i + 1], 1))
	{
	  i++;
	}
      else if (s[i] == '\0')
	{
	  return MG_JSON_INVALID;
	}
      else if (s[i] == '"')
	{
	  return i;
	}
    }
  return MG_JSON_INVALID;
}

static double
mg_atod (const char *p, int len, int *numlen)
{
  double d = 0.0;
  int i = 0, sign = 1;

  // Sign
  if (i < len && *p == '-')
    {
      sign = -1, i++;
    }
  else if (i < len && *p == '+')
    {
      i++;
    }

  // Decimal
  for (; i < len && p[i] >= '0' && p[i] <= '9'; i++)
    {
      d *= 10.0;
      d += p[i] - '0';
    }
  d *= sign;

  // Fractional
  if (i < len && p[i] == '.')
    {
      double frac = 0.0, base = 0.1;
      i++;
      for (; i < len && p[i] >= '0' && p[i] <= '9'; i++)
	{
	  frac += base * (p[i] - '0');
	  base /= 10.0;
	}
      d += frac * sign;
    }

  // Exponential
  if (i < len && (p[i] == 'e' || p[i] == 'E'))
    {
      int j, exp = 0, minus = 0;
      i++;
      if (i < len && p[i] == '-')
	minus = 1, i++;
      if (i < len && p[i] == '+')
	i++;
      while (i < len && p[i] >= '0' && p[i] <= '9' && exp < 308)
	exp = exp * 10 + (p[i++] - '0');
      if (minus)
	exp = -exp;
      for (j = 0; j < exp; j++)
	d *= 10.0;
      for (j = 0; j < -exp; j++)
	d /= 10.0;
    }

  if (numlen != NULL)
    *numlen = i;
  return d;
}

// Iterate over object or array elements
size_t
mg_json_next (struct mg_str obj, size_t ofs, struct mg_str *key,
	      struct mg_str *val)
{
  if (ofs >= obj.len)
    {
      ofs = 0; // Out of boundaries, stop scanning
    }
  else if (obj.len < 2 || (*obj.buf != '{' && *obj.buf != '['))
    {
      ofs = 0; // Not an array or object, stop
    }
  else
    {
      struct mg_str sub = mg_str_n (obj.buf + ofs, obj.len - ofs);
      if (ofs == 0)
	ofs++, sub.buf++, sub.len--;
      if (*obj.buf == '[')
	{ // Iterate over an array
	  int n = 0, o = mg_json_get (sub, "$", &n);
	  if (n < 0 || o < 0 || (size_t) (o + n) > sub.len)
	    {
	      ofs = 0; // Error parsing key, stop scanning
	    }
	  else
	    {
	      if (key)
		*key = mg_str_n (NULL, 0);
	      if (val)
		*val = mg_str_n (sub.buf + o, (size_t) n);
	      ofs = (size_t) (&sub.buf[o + n] - obj.buf);
	    }
	}
      else
	{ // Iterate over an object
	  int n = 0, o = mg_json_get (sub, "$", &n);
	  if (n < 0 || o < 0 || (size_t) (o + n) > sub.len)
	    {
	      ofs = 0; // Error parsing key, stop scanning
	    }
	  else
	    {
	      if (key)
		*key = mg_str_n (sub.buf + o, (size_t) n);
	      sub.buf += o + n, sub.len -= (size_t) (o + n);
	      while (sub.len > 0 && *sub.buf != ':')
		sub.len--, sub.buf++;
	      if (sub.len > 0 && *sub.buf == ':')
		sub.len--, sub.buf++;
	      n = 0, o = mg_json_get (sub, "$", &n);
	      if (n < 0 || o < 0 || (size_t) (o + n) > sub.len)
		{
		  ofs = 0; // Error parsing value, stop scanning
		}
	      else
		{
		  if (val)
		    *val = mg_str_n (sub.buf + o, (size_t) n);
		  ofs = (size_t) (&sub.buf[o + n] - obj.buf);
		}
	    }
	}
      // MG_INFO(("SUB ofs %u %.*s", ofs, sub.len, sub.buf));
      while (ofs && ofs < obj.len
	     && (obj.buf[ofs] == ' ' || obj.buf[ofs] == '\t'
		 || obj.buf[ofs] == '\n' || obj.buf[ofs] == '\r'))
	{
	  ofs++;
	}
      if (ofs && ofs < obj.len && obj.buf[ofs] == ',')
	ofs++;
      if (ofs > obj.len)
	ofs = 0;
    }
  return ofs;
}

int
mg_json_get (struct mg_str json, const char *path, int *toklen)
{
  const char *s = json.buf;
  int len = (int) json.len;
  enum
  {
    S_VALUE,
    S_KEY,
    S_COLON,
    S_COMMA_OR_EOO
  } expecting
      = S_VALUE;
  unsigned char nesting[MG_JSON_MAX_DEPTH];
  int i = 0;		// Current offset in `s`
  int j = 0;		// Offset in `s` we're looking for (return value)
  int depth = 0;	// Current depth (nesting level)
  int ed = 0;		// Expected depth
  int pos = 1;		// Current position in `path`
  int ci = -1, ei = -1; // Current and expected index in array

  if (toklen)
    *toklen = 0;
  if (path[0] != '$')
    return MG_JSON_INVALID;

#define MG_CHECKRET(x)                                                        \
  do                                                                          \
    {                                                                         \
      if (depth == ed && path[pos] == '\0' && ci == ei)                       \
	{                                                                     \
	  if (toklen)                                                         \
	    *toklen = i - j + 1;                                              \
	  return j;                                                           \
	}                                                                     \
    }                                                                         \
  while (0)

// In the ascii table, the distance between `[` and `]` is 2.
// Ditto for `{` and `}`. Hence +2 in the code below.
#define MG_EOO(x)                                                             \
  do                                                                          \
    {                                                                         \
      if (depth == ed && ci != ei)                                            \
	return MG_JSON_NOT_FOUND;                                             \
      if (c != nesting[depth - 1] + 2)                                        \
	return MG_JSON_INVALID;                                               \
      depth--;                                                                \
      MG_CHECKRET (x);                                                        \
    }                                                                         \
  while (0)

  for (i = 0; i < len; i++)
    {
      unsigned char c = ((unsigned char *) s)[i];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
	continue;
      switch (expecting)
	{
	case S_VALUE:
	  // p("V %s [%.*s] %d %d %d %d\n", path, pos, path, depth, ed, ci,
	  // ei);
	  if (depth == ed)
	    j = i;
	  if (c == '{')
	    {
	      if (depth >= (int) sizeof (nesting))
		return MG_JSON_TOO_DEEP;
	      if (depth == ed && path[pos] == '.' && ci == ei)
		{
		  // If we start the object, reset array indices
		  ed++, pos++, ci = ei = -1;
		}
	      nesting[depth++] = c;
	      expecting = S_KEY;
	      break;
	    }
	  else if (c == '[')
	    {
	      if (depth >= (int) sizeof (nesting))
		return MG_JSON_TOO_DEEP;
	      if (depth == ed && path[pos] == '[' && ei == ci)
		{
		  ed++, pos++, ci = 0;
		  for (ei = 0; path[pos] != ']' && path[pos] != '\0'; pos++)
		    {
		      ei *= 10;
		      ei += path[pos] - '0';
		    }
		  if (path[pos] != 0)
		    pos++;
		}
	      nesting[depth++] = c;
	      break;
	    }
	  else if (c == ']' && depth > 0)
	    { // Empty array
	      MG_EOO (']');
	    }
	  else if (c == 't' && i + 3 < len && memcmp (&s[i], "true", 4) == 0)
	    {
	      i += 3;
	    }
	  else if (c == 'n' && i + 3 < len && memcmp (&s[i], "null", 4) == 0)
	    {
	      i += 3;
	    }
	  else if (c == 'f' && i + 4 < len && memcmp (&s[i], "false", 5) == 0)
	    {
	      i += 4;
	    }
	  else if (c == '-' || ((c >= '0' && c <= '9')))
	    {
	      int numlen = 0;
	      mg_atod (&s[i], len - i, &numlen);
	      i += numlen - 1;
	    }
	  else if (c == '"')
	    {
	      int n = mg_pass_string (&s[i + 1], len - i - 1);
	      if (n < 0)
		return n;
	      i += n + 1;
	    }
	  else
	    {
	      return MG_JSON_INVALID;
	    }
	  MG_CHECKRET ('V');
	  if (depth == ed && ei >= 0)
	    ci++;
	  expecting = S_COMMA_OR_EOO;
	  break;

	case S_KEY:
	  if (c == '"')
	    {
	      int n = mg_pass_string (&s[i + 1], len - i - 1);
	      if (n < 0)
		return n;
	      if (i + 1 + n >= len)
		return MG_JSON_NOT_FOUND;
	      if (depth < ed)
		return MG_JSON_NOT_FOUND;
	      if (depth == ed && path[pos - 1] != '.')
		return MG_JSON_NOT_FOUND;
	      // printf("K %s [%.*s] [%.*s] %d %d %d %d %d\n", path, pos, path,
	      // n,
	      //        &s[i + 1], n, depth, ed, ci, ei);
	      //  NOTE(cpq): in the check sequence below is important.
	      //  strncmp() must go first: it fails fast if the remaining
	      //  length of the path is smaller than `n`.
	      if (depth == ed && path[pos - 1] == '.'
		  && strncmp (&s[i + 1], &path[pos], (size_t) n) == 0
		  && (path[pos + n] == '\0' || path[pos + n] == '.'
		      || path[pos + n] == '['))
		{
		  pos += n;
		}
	      i += n + 1;
	      expecting = S_COLON;
	    }
	  else if (c == '}')
	    { // Empty object
	      MG_EOO ('}');
	      expecting = S_COMMA_OR_EOO;
	      if (depth == ed && ei >= 0)
		ci++;
	    }
	  else
	    {
	      return MG_JSON_INVALID;
	    }
	  break;

	case S_COLON:
	  if (c == ':')
	    {
	      expecting = S_VALUE;
	    }
	  else
	    {
	      return MG_JSON_INVALID;
	    }
	  break;

	case S_COMMA_OR_EOO:
	  if (depth <= 0)
	    {
	      return MG_JSON_INVALID;
	    }
	  else if (c == ',')
	    {
	      expecting = (nesting[depth - 1] == '{') ? S_KEY : S_VALUE;
	    }
	  else if (c == ']' || c == '}')
	    {
	      if (depth == ed && c == '}' && path[pos - 1] == '.')
		return MG_JSON_NOT_FOUND;
	      if (depth == ed && c == ']' && path[pos - 1] == ',')
		return MG_JSON_NOT_FOUND;
	      MG_EOO ('O');
	      if (depth == ed && ei >= 0)
		ci++;
	    }
	  else
	    {
	      return MG_JSON_INVALID;
	    }
	  break;
	}
    }
  return MG_JSON_NOT_FOUND;
}

struct mg_str
mg_json_get_tok (struct mg_str json, const char *path)
{
  int len = 0, ofs = mg_json_get (json, path, &len);
  return mg_str_n (ofs < 0 ? NULL : json.buf + ofs,
		   (size_t) (len < 0 ? 0 : len));
}

bool
mg_json_get_num (struct mg_str json, const char *path, double *v)
{
  int n, toklen, found = 0;
  if ((n = mg_json_get (json, path, &toklen)) >= 0
      && (json.buf[n] == '-' || (json.buf[n] >= '0' && json.buf[n] <= '9')))
    {
      if (v != NULL)
	*v = mg_atod (json.buf + n, toklen, NULL);
      found = 1;
    }
  return found;
}

bool
mg_json_get_bool (struct mg_str json, const char *path, bool *v)
{
  int found = 0, off = mg_json_get (json, path, NULL);
  if (off >= 0 && (json.buf[off] == 't' || json.buf[off] == 'f'))
    {
      if (v != NULL)
	*v = json.buf[off] == 't';
      found = 1;
    }
  return found;
}

bool
mg_json_unescape (struct mg_str s, char *to, size_t n)
{
  size_t i, j;
  for (i = 0, j = 0; i < s.len && j < n; i++, j++)
    {
      if (s.buf[i] == '\\' && i + 5 < s.len && s.buf[i + 1] == 'u')
	{
	  //  \uXXXX escape. We process simple one-byte chars \u00xx within
	  //  ASCII range. More complex chars would require dragging in a UTF8
	  //  library, which is too much for us
	  if (mg_str_to_num (mg_str_n (s.buf + i + 2, 4), 16, &to[j],
			     sizeof (uint8_t))
	      == false)
	    return false;
	  i += 5;
	}
      else if (s.buf[i] == '\\' && i + 1 < s.len)
	{
	  char c = json_esc (s.buf[i + 1], 0);
	  if (c == 0)
	    return false;
	  to[j] = c;
	  i++;
	}
      else
	{
	  to[j] = s.buf[i];
	}
    }
  if (j >= n)
    return false;
  if (n > 0)
    to[j] = '\0';
  return true;
}

char *
mg_json_get_str (struct mg_str json, const char *path)
{
  char *result = NULL;
  int len = 0, off = mg_json_get (json, path, &len);
  if (off >= 0 && len > 1 && json.buf[off] == '"')
    {
      if ((result = (char *) calloc (1, (size_t) len)) != NULL
	  && !mg_json_unescape (
	      mg_str_n (json.buf + off + 1, (size_t) (len - 2)), result,
	      (size_t) len))
	{
	  free (result);
	  result = NULL;
	}
    }
  return result;
}

char *
mg_json_get_b64 (struct mg_str json, const char *path, int *slen)
{
  char *result = NULL;
  int len = 0, off = mg_json_get (json, path, &len);
  if (off >= 0 && json.buf[off] == '"' && len > 1
      && (result = (char *) calloc (1, (size_t) len)) != NULL)
    {
      size_t k = mg_base64_decode (json.buf + off + 1, (size_t) (len - 2),
				   result, (size_t) len);
      if (slen != NULL)
	*slen = (int) k;
    }
  return result;
}

char *
mg_json_get_hex (struct mg_str json, const char *path, int *slen)
{
  char *result = NULL;
  int len = 0, off = mg_json_get (json, path, &len);
  if (off >= 0 && json.buf[off] == '"' && len > 1
      && (result = (char *) calloc (1, (size_t) len / 2)) != NULL)
    {
      int i;
      for (i = 0; i < len - 2; i += 2)
	{
	  mg_str_to_num (mg_str_n (json.buf + off + 1 + i, 2), 16,
			 &result[i >> 1], sizeof (uint8_t));
	}
      result[len / 2 - 1] = '\0';
      if (slen != NULL)
	*slen = len / 2 - 1;
    }
  return result;
}

long
mg_json_get_long (struct mg_str json, const char *path, long dflt)
{
  double dv;
  long result = dflt;
  if (mg_json_get_num (json, path, &dv))
    result = (long) dv;
  return result;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/log.c"
#endif

int mg_log_level = MG_LL_INFO;
static mg_pfn_t s_log_func = mg_pfn_stdout;
static void *s_log_func_param = NULL;

void
mg_log_set_fn (mg_pfn_t fn, void *param)
{
  s_log_func = fn;
  s_log_func_param = param;
}

static void
logc (unsigned char c)
{
  s_log_func ((char) c, s_log_func_param);
}

static void
logs (const char *buf, size_t len)
{
  size_t i;
  for (i = 0; i < len; i++)
    logc (((unsigned char *) buf)[i]);
}

#if MG_ENABLE_CUSTOM_LOG
// Let user define their own mg_log_prefix() and mg_log()
#else
void
mg_log_prefix (int level, const char *file, int line, const char *fname)
{
  const char *p = strrchr (file, '/');
  char buf[41];
  size_t n;
  if (p == NULL)
    p = strrchr (file, '\\');
  n = mg_snprintf (buf, sizeof (buf), "%-6llx %d %s:%d:%s", mg_millis (),
		   level, p == NULL ? file : p + 1, line, fname);
  if (n > sizeof (buf) - 2)
    n = sizeof (buf) - 2;
  while (n < sizeof (buf))
    buf[n++] = ' ';
  logs (buf, n - 1);
}

void
mg_log (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  mg_vxprintf (s_log_func, s_log_func_param, fmt, &ap);
  va_end (ap);
  logs ("\r\n", 2);
}
#endif

static unsigned char
nibble (unsigned c)
{
  return (unsigned char) (c < 10 ? c + '0' : c + 'W');
}

#define ISPRINT(x) ((x) >= ' ' && (x) <= '~')
void
mg_hexdump (const void *buf, size_t len)
{
  const unsigned char *p = (const unsigned char *) buf;
  unsigned char ascii[16], alen = 0;
  size_t i;
  for (i = 0; i < len; i++)
    {
      if ((i % 16) == 0)
	{
	  // Print buffered ascii chars
	  if (i > 0)
	    logs ("  ", 2), logs ((char *) ascii, 16), logc ('\n'), alen = 0;
	  // Print hex address, then \t
	  logc (nibble ((i >> 12) & 15)), logc (nibble ((i >> 8) & 15)),
	      logc (nibble ((i >> 4) & 15)), logc ('0'), logs ("   ", 3);
	}
      logc (nibble (p[i] >> 4)),
	  logc (nibble (p[i] & 15));		   // Two nibbles, e.g. c5
      logc (' ');				   // Space after hex number
      ascii[alen++] = ISPRINT (p[i]) ? p[i] : '.'; // Add to the ascii buf
    }
  while (alen < 16)
    logs ("   ", 3), ascii[alen++] = ' ';
  logs ("  ", 2), logs ((char *) ascii, 16), logc ('\n');
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/md5.c"
#endif

//  This code implements the MD5 message-digest algorithm.
//  The algorithm is due to Ron Rivest.  This code was
//  written by Colin Plumb in 1993, no copyright is claimed.
//  This code is in the public domain; do with it what you wish.
//
//  Equivalent code is available from RSA Data Security, Inc.
//  This code has been tested against that, and is equivalent,
//  except that you don't need to include two pages of legalese
//  with every copy.
//
//  To compute the message digest of a chunk of bytes, declare an
//  MD5Context structure, pass it to MD5Init, call MD5Update as
//  needed on buffers full of bytes, and then call MD5Final, which
//  will fill a supplied 16-byte array with the digest.

#if defined(MG_ENABLE_MD5) && MG_ENABLE_MD5

static void
mg_byte_reverse (unsigned char *buf, unsigned longs)
{
  if (MG_BIG_ENDIAN)
    {
      do
	{
	  uint32_t t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16
		       | ((unsigned) buf[1] << 8 | buf[0]);
	  *(uint32_t *) buf = t;
	  buf += 4;
	}
      while (--longs);
    }
  else
    {
      (void) buf, (void) longs; // Little endian. Do nothing
    }
}

#  define F1(x, y, z) (z ^ (x & (y ^ z)))
#  define F2(x, y, z) F1 (z, x, y)
#  define F3(x, y, z) (x ^ y ^ z)
#  define F4(x, y, z) (y ^ (x | ~z))

#  define MD5STEP(f, w, x, y, z, data, s)                                     \
    (w += f (x, y, z) + data, w = w << s | w >> (32 - s), w += x)

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void
mg_md5_init (mg_md5_ctx *ctx)
{
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

static void
mg_md5_transform (uint32_t buf[4], uint32_t const in[16])
{
  uint32_t a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP (F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP (F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP (F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP (F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP (F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP (F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP (F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP (F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP (F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP (F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP (F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP (F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP (F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP (F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP (F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP (F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP (F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP (F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP (F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP (F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP (F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP (F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP (F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP (F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP (F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP (F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP (F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP (F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP (F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP (F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP (F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP (F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP (F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP (F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP (F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP (F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP (F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP (F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP (F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP (F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP (F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP (F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP (F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP (F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP (F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP (F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP (F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP (F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP (F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP (F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP (F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP (F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP (F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP (F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP (F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP (F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP (F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP (F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP (F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP (F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP (F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP (F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP (F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP (F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

void
mg_md5_update (mg_md5_ctx *ctx, const unsigned char *buf, size_t len)
{
  uint32_t t;

  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
    ctx->bits[1]++;
  ctx->bits[1] += (uint32_t) len >> 29;

  t = (t >> 3) & 0x3f;

  if (t)
    {
      unsigned char *p = (unsigned char *) ctx->in + t;

      t = 64 - t;
      if (len < t)
	{
	  memcpy (p, buf, len);
	  return;
	}
      memcpy (p, buf, t);
      mg_byte_reverse (ctx->in, 16);
      mg_md5_transform (ctx->buf, (uint32_t *) ctx->in);
      buf += t;
      len -= t;
    }

  while (len >= 64)
    {
      memcpy (ctx->in, buf, 64);
      mg_byte_reverse (ctx->in, 16);
      mg_md5_transform (ctx->buf, (uint32_t *) ctx->in);
      buf += 64;
      len -= 64;
    }

  memcpy (ctx->in, buf, len);
}

void
mg_md5_final (mg_md5_ctx *ctx, unsigned char digest[16])
{
  unsigned count;
  unsigned char *p;
  uint32_t *a;

  count = (ctx->bits[0] >> 3) & 0x3F;

  p = ctx->in + count;
  *p++ = 0x80;
  count = 64 - 1 - count;
  if (count < 8)
    {
      memset (p, 0, count);
      mg_byte_reverse (ctx->in, 16);
      mg_md5_transform (ctx->buf, (uint32_t *) ctx->in);
      memset (ctx->in, 0, 56);
    }
  else
    {
      memset (p, 0, count - 8);
    }
  mg_byte_reverse (ctx->in, 14);

  a = (uint32_t *) ctx->in;
  a[14] = ctx->bits[0];
  a[15] = ctx->bits[1];

  mg_md5_transform (ctx->buf, (uint32_t *) ctx->in);
  mg_byte_reverse ((unsigned char *) ctx->buf, 4);
  memcpy (digest, ctx->buf, 16);
  memset ((char *) ctx, 0, sizeof (*ctx));
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/mqtt.c"
#endif

#define MQTT_CLEAN_SESSION 0x02
#define MQTT_HAS_WILL 0x04
#define MQTT_WILL_RETAIN 0x20
#define MQTT_HAS_PASSWORD 0x40
#define MQTT_HAS_USER_NAME 0x80

struct mg_mqtt_pmap
{
  uint8_t id;
  uint8_t type;
};

static const struct mg_mqtt_pmap s_prop_map[]
    = { { MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, MQTT_PROP_TYPE_INT },
	{ MQTT_PROP_CONTENT_TYPE, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_RESPONSE_TOPIC, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_CORRELATION_DATA, MQTT_PROP_TYPE_BINARY_DATA },
	{ MQTT_PROP_SUBSCRIPTION_IDENTIFIER, MQTT_PROP_TYPE_VARIABLE_INT },
	{ MQTT_PROP_SESSION_EXPIRY_INTERVAL, MQTT_PROP_TYPE_INT },
	{ MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_SERVER_KEEP_ALIVE, MQTT_PROP_TYPE_SHORT },
	{ MQTT_PROP_AUTHENTICATION_METHOD, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_AUTHENTICATION_DATA, MQTT_PROP_TYPE_BINARY_DATA },
	{ MQTT_PROP_REQUEST_PROBLEM_INFORMATION, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_WILL_DELAY_INTERVAL, MQTT_PROP_TYPE_INT },
	{ MQTT_PROP_REQUEST_RESPONSE_INFORMATION, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_RESPONSE_INFORMATION, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_SERVER_REFERENCE, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_REASON_STRING, MQTT_PROP_TYPE_STRING },
	{ MQTT_PROP_RECEIVE_MAXIMUM, MQTT_PROP_TYPE_SHORT },
	{ MQTT_PROP_TOPIC_ALIAS_MAXIMUM, MQTT_PROP_TYPE_SHORT },
	{ MQTT_PROP_TOPIC_ALIAS, MQTT_PROP_TYPE_SHORT },
	{ MQTT_PROP_MAXIMUM_QOS, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_RETAIN_AVAILABLE, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_USER_PROPERTY, MQTT_PROP_TYPE_STRING_PAIR },
	{ MQTT_PROP_MAXIMUM_PACKET_SIZE, MQTT_PROP_TYPE_INT },
	{ MQTT_PROP_WILDCARD_SUBSCRIPTION_AVAILABLE, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_SUBSCRIPTION_IDENTIFIER_AVAILABLE, MQTT_PROP_TYPE_BYTE },
	{ MQTT_PROP_SHARED_SUBSCRIPTION_AVAILABLE, MQTT_PROP_TYPE_BYTE } };

void
mg_mqtt_send_header (struct mg_connection *c, uint8_t cmd, uint8_t flags,
		     uint32_t len)
{
  uint8_t buf[1 + sizeof (len)], *vlen = &buf[1];
  buf[0] = (uint8_t) ((cmd << 4) | flags);
  do
    {
      *vlen = len % 0x80;
      len /= 0x80;
      if (len > 0)
	*vlen |= 0x80;
      vlen++;
    }
  while (len > 0 && vlen < &buf[sizeof (buf)]);
  mg_send (c, buf, (size_t) (vlen - buf));
}

static void
mg_send_u16 (struct mg_connection *c, uint16_t value)
{
  mg_send (c, &value, sizeof (value));
}

static void
mg_send_u32 (struct mg_connection *c, uint32_t value)
{
  mg_send (c, &value, sizeof (value));
}

static uint8_t
varint_size (size_t length)
{
  uint8_t bytes_needed = 0;
  do
    {
      bytes_needed++;
      length /= 0x80;
    }
  while (length > 0);
  return bytes_needed;
}

static size_t
encode_varint (uint8_t *buf, size_t value)
{
  size_t len = 0;

  do
    {
      uint8_t b = (uint8_t) (value % 128);
      value /= 128;
      if (value > 0)
	b |= 0x80;
      buf[len++] = b;
    }
  while (value > 0);

  return len;
}

static size_t
decode_varint (const uint8_t *buf, size_t len, size_t *value)
{
  size_t multiplier = 1, offset;
  *value = 0;

  for (offset = 0; offset < 4 && offset < len; offset++)
    {
      uint8_t encoded_byte = buf[offset];
      *value += (encoded_byte & 0x7f) * multiplier;
      multiplier *= 128;

      if ((encoded_byte & 0x80) == 0)
	return offset + 1;
    }

  return 0;
}

static int
mqtt_prop_type_by_id (uint8_t prop_id)
{
  size_t i, num_properties = sizeof (s_prop_map) / sizeof (s_prop_map[0]);
  for (i = 0; i < num_properties; ++i)
    {
      if (s_prop_map[i].id == prop_id)
	return s_prop_map[i].type;
    }
  return -1; // Property ID not found
}

// Returns the size of the properties section, without the
// size of the content's length
static size_t
get_properties_length (struct mg_mqtt_prop *props, size_t count)
{
  size_t i, size = 0;
  for (i = 0; i < count; i++)
    {
      size++; // identifier
      switch (mqtt_prop_type_by_id (props[i].id))
	{
	case MQTT_PROP_TYPE_STRING_PAIR:
	  size += (uint32_t) (props[i].val.len + props[i].key.len
			      + 2 * sizeof (uint16_t));
	  break;
	case MQTT_PROP_TYPE_STRING:
	  size += (uint32_t) (props[i].val.len + sizeof (uint16_t));
	  break;
	case MQTT_PROP_TYPE_BINARY_DATA:
	  size += (uint32_t) (props[i].val.len + sizeof (uint16_t));
	  break;
	case MQTT_PROP_TYPE_VARIABLE_INT:
	  size += varint_size ((uint32_t) props[i].iv);
	  break;
	case MQTT_PROP_TYPE_INT:
	  size += (uint32_t) sizeof (uint32_t);
	  break;
	case MQTT_PROP_TYPE_SHORT:
	  size += (uint32_t) sizeof (uint16_t);
	  break;
	case MQTT_PROP_TYPE_BYTE:
	  size += (uint32_t) sizeof (uint8_t);
	  break;
	default:
	  return size; // cannot parse further down
	}
    }

  return size;
}

// returns the entire size of the properties section, including the
// size of the variable length of the content
static size_t
get_props_size (struct mg_mqtt_prop *props, size_t count)
{
  size_t size = get_properties_length (props, count);
  size += varint_size (size);
  return size;
}

static void
mg_send_mqtt_properties (struct mg_connection *c, struct mg_mqtt_prop *props,
			 size_t nprops)
{
  size_t total_size = get_properties_length (props, nprops);
  uint8_t buf_v[4] = { 0, 0, 0, 0 };
  uint8_t buf[4] = { 0, 0, 0, 0 };
  size_t i, len = encode_varint (buf, total_size);

  mg_send (c, buf, (size_t) len);
  for (i = 0; i < nprops; i++)
    {
      mg_send (c, &props[i].id, sizeof (props[i].id));
      switch (mqtt_prop_type_by_id (props[i].id))
	{
	case MQTT_PROP_TYPE_STRING_PAIR:
	  mg_send_u16 (c, mg_htons ((uint16_t) props[i].key.len));
	  mg_send (c, props[i].key.buf, props[i].key.len);
	  mg_send_u16 (c, mg_htons ((uint16_t) props[i].val.len));
	  mg_send (c, props[i].val.buf, props[i].val.len);
	  break;
	case MQTT_PROP_TYPE_BYTE:
	  mg_send (c, &props[i].iv, sizeof (uint8_t));
	  break;
	case MQTT_PROP_TYPE_SHORT:
	  mg_send_u16 (c, mg_htons ((uint16_t) props[i].iv));
	  break;
	case MQTT_PROP_TYPE_INT:
	  mg_send_u32 (c, mg_htonl ((uint32_t) props[i].iv));
	  break;
	case MQTT_PROP_TYPE_STRING:
	  mg_send_u16 (c, mg_htons ((uint16_t) props[i].val.len));
	  mg_send (c, props[i].val.buf, props[i].val.len);
	  break;
	case MQTT_PROP_TYPE_BINARY_DATA:
	  mg_send_u16 (c, mg_htons ((uint16_t) props[i].val.len));
	  mg_send (c, props[i].val.buf, props[i].val.len);
	  break;
	case MQTT_PROP_TYPE_VARIABLE_INT:
	  len = encode_varint (buf_v, props[i].iv);
	  mg_send (c, buf_v, (size_t) len);
	  break;
	}
    }
}

size_t
mg_mqtt_next_prop (struct mg_mqtt_message *msg, struct mg_mqtt_prop *prop,
		   size_t ofs)
{
  uint8_t *i = (uint8_t *) msg->dgram.buf + msg->props_start + ofs;
  uint8_t *end = (uint8_t *) msg->dgram.buf + msg->dgram.len;
  size_t new_pos = ofs, len;
  prop->id = i[0];

  if (ofs >= msg->dgram.len || ofs >= msg->props_start + msg->props_size)
    return 0;
  i++, new_pos++;

  switch (mqtt_prop_type_by_id (prop->id))
    {
    case MQTT_PROP_TYPE_STRING_PAIR:
      prop->key.len = (uint16_t) ((((uint16_t) i[0]) << 8) | i[1]);
      prop->key.buf = (char *) i + 2;
      i += 2 + prop->key.len;
      prop->val.len = (uint16_t) ((((uint16_t) i[0]) << 8) | i[1]);
      prop->val.buf = (char *) i + 2;
      new_pos += 2 * sizeof (uint16_t) + prop->val.len + prop->key.len;
      break;
    case MQTT_PROP_TYPE_BYTE:
      prop->iv = (uint8_t) i[0];
      new_pos++;
      break;
    case MQTT_PROP_TYPE_SHORT:
      prop->iv = (uint16_t) ((((uint16_t) i[0]) << 8) | i[1]);
      new_pos += sizeof (uint16_t);
      break;
    case MQTT_PROP_TYPE_INT:
      prop->iv = ((uint32_t) i[0] << 24) | ((uint32_t) i[1] << 16)
		 | ((uint32_t) i[2] << 8) | i[3];
      new_pos += sizeof (uint32_t);
      break;
    case MQTT_PROP_TYPE_STRING:
      prop->val.len = (uint16_t) ((((uint16_t) i[0]) << 8) | i[1]);
      prop->val.buf = (char *) i + 2;
      new_pos += 2 + prop->val.len;
      break;
    case MQTT_PROP_TYPE_BINARY_DATA:
      prop->val.len = (uint16_t) ((((uint16_t) i[0]) << 8) | i[1]);
      prop->val.buf = (char *) i + 2;
      new_pos += 2 + prop->val.len;
      break;
    case MQTT_PROP_TYPE_VARIABLE_INT:
      len = decode_varint (i, (size_t) (end - i), (size_t *) &prop->iv);
      new_pos = (!len) ? 0 : new_pos + len;
      break;
    default:
      new_pos = 0;
    }

  return new_pos;
}

void
mg_mqtt_login (struct mg_connection *c, const struct mg_mqtt_opts *opts)
{
  char client_id[21];
  struct mg_str cid = opts->client_id;
  size_t total_len = 7 + 1 + 2 + 2;
  uint8_t hdr[8] = { 0, 4, 'M', 'Q', 'T', 'T', opts->version, 0 };

  if (cid.len == 0)
    {
      mg_random_str (client_id, sizeof (client_id) - 1);
      client_id[sizeof (client_id) - 1] = '\0';
      cid = mg_str (client_id);
    }

  if (hdr[6] == 0)
    hdr[6] = 4;		     // If version is not set, use 4 (3.1.1)
  c->is_mqtt5 = hdr[6] == 5; // Set version 5 flag
  hdr[7] = (uint8_t) ((opts->qos & 3) << 3); // Connection flags
  if (opts->user.len > 0)
    {
      total_len += 2 + (uint32_t) opts->user.len;
      hdr[7] |= MQTT_HAS_USER_NAME;
    }
  if (opts->pass.len > 0)
    {
      total_len += 2 + (uint32_t) opts->pass.len;
      hdr[7] |= MQTT_HAS_PASSWORD;
    }
  if (opts->topic.len > 0)
    { // allow zero-length msgs, message.len is size_t
      total_len
	  += 4 + (uint32_t) opts->topic.len + (uint32_t) opts->message.len;
      hdr[7] |= MQTT_HAS_WILL;
    }
  if (opts->clean || cid.len == 0)
    hdr[7] |= MQTT_CLEAN_SESSION;
  if (opts->retain)
    hdr[7] |= MQTT_WILL_RETAIN;
  total_len += (uint32_t) cid.len;
  if (c->is_mqtt5)
    {
      total_len += get_props_size (opts->props, opts->num_props);
      if (hdr[7] & MQTT_HAS_WILL)
	total_len += get_props_size (opts->will_props, opts->num_will_props);
    }

  mg_mqtt_send_header (c, MQTT_CMD_CONNECT, 0, (uint32_t) total_len);
  mg_send (c, hdr, sizeof (hdr));
  // keepalive == 0 means "do not disconnect us!"
  mg_send_u16 (c, mg_htons ((uint16_t) opts->keepalive));

  if (c->is_mqtt5)
    mg_send_mqtt_properties (c, opts->props, opts->num_props);

  mg_send_u16 (c, mg_htons ((uint16_t) cid.len));
  mg_send (c, cid.buf, cid.len);

  if (hdr[7] & MQTT_HAS_WILL)
    {
      if (c->is_mqtt5)
	mg_send_mqtt_properties (c, opts->will_props, opts->num_will_props);

      mg_send_u16 (c, mg_htons ((uint16_t) opts->topic.len));
      mg_send (c, opts->topic.buf, opts->topic.len);
      mg_send_u16 (c, mg_htons ((uint16_t) opts->message.len));
      mg_send (c, opts->message.buf, opts->message.len);
    }
  if (opts->user.len > 0)
    {
      mg_send_u16 (c, mg_htons ((uint16_t) opts->user.len));
      mg_send (c, opts->user.buf, opts->user.len);
    }
  if (opts->pass.len > 0)
    {
      mg_send_u16 (c, mg_htons ((uint16_t) opts->pass.len));
      mg_send (c, opts->pass.buf, opts->pass.len);
    }
}

uint16_t
mg_mqtt_pub (struct mg_connection *c, const struct mg_mqtt_opts *opts)
{
  uint16_t id = opts->retransmit_id;
  uint8_t flags = (uint8_t) (((opts->qos & 3) << 1) | (opts->retain ? 1 : 0));
  size_t len = 2 + opts->topic.len + opts->message.len;
  MG_DEBUG (("%lu [%.*s] -> [%.*s]", c->id, (int) opts->topic.len,
	     (char *) opts->topic.buf, (int) opts->message.len,
	     (char *) opts->message.buf));
  if (opts->qos > 0)
    len += 2;
  if (c->is_mqtt5)
    len += get_props_size (opts->props, opts->num_props);

  if (opts->qos > 0 && id != 0)
    flags |= 1 << 3;
  mg_mqtt_send_header (c, MQTT_CMD_PUBLISH, flags, (uint32_t) len);
  mg_send_u16 (c, mg_htons ((uint16_t) opts->topic.len));
  mg_send (c, opts->topic.buf, opts->topic.len);
  if (opts->qos > 0)
    { // need to send 'id' field
      if (id == 0)
	{ // generate new one if not resending
	  if (++c->mgr->mqtt_id == 0)
	    ++c->mgr->mqtt_id;
	  id = c->mgr->mqtt_id;
	}
      mg_send_u16 (c, mg_htons (id));
    }

  if (c->is_mqtt5)
    mg_send_mqtt_properties (c, opts->props, opts->num_props);

  if (opts->message.len > 0)
    mg_send (c, opts->message.buf, opts->message.len);
  return id;
}

void
mg_mqtt_sub (struct mg_connection *c, const struct mg_mqtt_opts *opts)
{
  uint8_t qos_ = opts->qos & 3;
  size_t plen
      = c->is_mqtt5 ? get_props_size (opts->props, opts->num_props) : 0;
  size_t len = 2 + opts->topic.len + 2 + 1 + plen;

  mg_mqtt_send_header (c, MQTT_CMD_SUBSCRIBE, 2, (uint32_t) len);
  if (++c->mgr->mqtt_id == 0)
    ++c->mgr->mqtt_id;
  mg_send_u16 (c, mg_htons (c->mgr->mqtt_id));
  if (c->is_mqtt5)
    mg_send_mqtt_properties (c, opts->props, opts->num_props);

  mg_send_u16 (c, mg_htons ((uint16_t) opts->topic.len));
  mg_send (c, opts->topic.buf, opts->topic.len);
  mg_send (c, &qos_, sizeof (qos_));
}

int
mg_mqtt_parse (const uint8_t *buf, size_t len, uint8_t version,
	       struct mg_mqtt_message *m)
{
  uint8_t lc = 0, *p, *end;
  uint32_t n = 0, len_len = 0;

  memset (m, 0, sizeof (*m));
  m->dgram.buf = (char *) buf;
  if (len < 2)
    return MQTT_INCOMPLETE;
  m->cmd = (uint8_t) (buf[0] >> 4);
  m->qos = (buf[0] >> 1) & 3;

  n = len_len = 0;
  p = (uint8_t *) buf + 1;
  while ((size_t) (p - buf) < len)
    {
      lc = *((uint8_t *) p++);
      n += (uint32_t) ((lc & 0x7f) << 7 * len_len);
      len_len++;
      if (!(lc & 0x80))
	break;
      if (len_len >= 4)
	return MQTT_MALFORMED;
    }
  end = p + n;
  if ((lc & 0x80) || (end > buf + len))
    return MQTT_INCOMPLETE;
  m->dgram.len = (size_t) (end - buf);

  switch (m->cmd)
    {
    case MQTT_CMD_CONNACK:
      if (end - p < 2)
	return MQTT_MALFORMED;
      m->ack = p[1];
      break;
    case MQTT_CMD_PUBACK:
    case MQTT_CMD_PUBREC:
    case MQTT_CMD_PUBREL:
    case MQTT_CMD_PUBCOMP:
    case MQTT_CMD_SUBSCRIBE:
    case MQTT_CMD_SUBACK:
    case MQTT_CMD_UNSUBSCRIBE:
    case MQTT_CMD_UNSUBACK:
      if (p + 2 > end)
	return MQTT_MALFORMED;
      m->id = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
      p += 2;
      break;
    case MQTT_CMD_PUBLISH:
      {
	if (p + 2 > end)
	  return MQTT_MALFORMED;
	m->topic.len = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
	m->topic.buf = (char *) p + 2;
	p += 2 + m->topic.len;
	if (p > end)
	  return MQTT_MALFORMED;
	if (m->qos > 0)
	  {
	    if (p + 2 > end)
	      return MQTT_MALFORMED;
	    m->id = (uint16_t) ((((uint16_t) p[0]) << 8) | p[1]);
	    p += 2;
	  }
	if (p > end)
	  return MQTT_MALFORMED;
	if (version == 5 && p + 2 < end)
	  {
	    len_len = (uint32_t) decode_varint (p, (size_t) (end - p),
						&m->props_size);
	    if (!len_len)
	      return MQTT_MALFORMED;
	    m->props_start = (size_t) (p + len_len - buf);
	    p += len_len + m->props_size;
	  }
	if (p > end)
	  return MQTT_MALFORMED;
	m->data.buf = (char *) p;
	m->data.len = (size_t) (end - p);
	break;
      }
    default:
      break;
    }
  return MQTT_OK;
}

static void
mqtt_cb (struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_READ)
    {
      for (;;)
	{
	  uint8_t version = c->is_mqtt5 ? 5 : 4;
	  struct mg_mqtt_message mm;
	  int rc = mg_mqtt_parse (c->recv.buf, c->recv.len, version, &mm);
	  if (rc == MQTT_MALFORMED)
	    {
	      MG_ERROR (("%lu MQTT malformed message", c->id));
	      c->is_closing = 1;
	      break;
	    }
	  else if (rc == MQTT_OK)
	    {
	      MG_VERBOSE (("%lu MQTT CMD %d len %d [%.*s]", c->id, mm.cmd,
			   (int) mm.dgram.len, (int) mm.data.len,
			   mm.data.buf));
	      switch (mm.cmd)
		{
		case MQTT_CMD_CONNACK:
		  mg_call (c, MG_EV_MQTT_OPEN, &mm.ack);
		  if (mm.ack == 0)
		    {
		      MG_DEBUG (("%lu Connected", c->id));
		    }
		  else
		    {
		      MG_ERROR (
			  ("%lu MQTT auth failed, code %d", c->id, mm.ack));
		      c->is_closing = 1;
		    }
		  break;
		case MQTT_CMD_PUBLISH:
		  {
		    /*MG_DEBUG(("%lu [%.*s] -> [%.*s]", c->id, (int)
		       mm.topic.len, mm.topic.buf, (int) mm.data.len,
		       mm.data.buf));*/
		    if (mm.qos > 0)
		      {
			uint16_t id = mg_ntohs (mm.id);
			uint32_t remaining_len = sizeof (id);
			if (c->is_mqtt5)
			  remaining_len += 2; // 3.4.2

			mg_mqtt_send_header (c,
					     (uint8_t) (mm.qos == 2
							    ? MQTT_CMD_PUBREC
							    : MQTT_CMD_PUBACK),
					     0, remaining_len);
			mg_send (c, &id, sizeof (id));

			if (c->is_mqtt5)
			  {
			    uint16_t zero = 0;
			    mg_send (c, &zero, sizeof (zero));
			  }
		      }
		    mg_call (c, MG_EV_MQTT_MSG,
			     &mm); // let the app handle qos stuff
		    break;
		  }
		case MQTT_CMD_PUBREC:
		  { // MQTT5: 3.5.2-1 TODO(): variable header rc
		    uint16_t id = mg_ntohs (mm.id);
		    uint32_t remaining_len = sizeof (id); // MQTT5 3.6.2-1
		    mg_mqtt_send_header (c, MQTT_CMD_PUBREL, 2, remaining_len);
		    mg_send (c, &id, sizeof (id)); // MQTT5 3.6.1-1, flags = 2
		    break;
		  }
		case MQTT_CMD_PUBREL:
		  { // MQTT5: 3.6.2-1 TODO(): variable header rc
		    uint16_t id = mg_ntohs (mm.id);
		    uint32_t remaining_len = sizeof (id); // MQTT5 3.7.2-1
		    mg_mqtt_send_header (c, MQTT_CMD_PUBCOMP, 0,
					 remaining_len);
		    mg_send (c, &id, sizeof (id));
		    break;
		  }
		}
	      mg_call (c, MG_EV_MQTT_CMD, &mm);
	      mg_iobuf_del (&c->recv, 0, mm.dgram.len);
	    }
	  else
	    {
	      break;
	    }
	}
    }
  (void) ev_data;
}

void
mg_mqtt_ping (struct mg_connection *nc)
{
  mg_mqtt_send_header (nc, MQTT_CMD_PINGREQ, 0, 0);
}

void
mg_mqtt_pong (struct mg_connection *nc)
{
  mg_mqtt_send_header (nc, MQTT_CMD_PINGRESP, 0, 0);
}

void
mg_mqtt_disconnect (struct mg_connection *c, const struct mg_mqtt_opts *opts)
{
  size_t len = 0;
  if (c->is_mqtt5)
    len = 1 + get_props_size (opts->props, opts->num_props);
  mg_mqtt_send_header (c, MQTT_CMD_DISCONNECT, 0, (uint32_t) len);

  if (c->is_mqtt5)
    {
      uint8_t zero = 0;
      mg_send (c, &zero, sizeof (zero)); // reason code
      mg_send_mqtt_properties (c, opts->props, opts->num_props);
    }
}

struct mg_connection *
mg_mqtt_connect (struct mg_mgr *mgr, const char *url,
		 const struct mg_mqtt_opts *opts, mg_event_handler_t fn,
		 void *fn_data)
{
  struct mg_connection *c = mg_connect (mgr, url, fn, fn_data);
  if (c != NULL)
    {
      struct mg_mqtt_opts empty;
      memset (&empty, 0, sizeof (empty));
      mg_mqtt_login (c, opts == NULL ? &empty : opts);
      c->pfn = mqtt_cb;
    }
  return c;
}

struct mg_connection *
mg_mqtt_listen (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
		void *fn_data)
{
  struct mg_connection *c = mg_listen (mgr, url, fn, fn_data);
  if (c != NULL)
    c->pfn = mqtt_cb, c->pfn_data = mgr;
  return c;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/net.c"
#endif

size_t
mg_vprintf (struct mg_connection *c, const char *fmt, va_list *ap)
{
  size_t old = c->send.len;
  mg_vxprintf (mg_pfn_iobuf, &c->send, fmt, ap);
  return c->send.len - old;
}

size_t
mg_printf (struct mg_connection *c, const char *fmt, ...)
{
  size_t len = 0;
  va_list ap;
  va_start (ap, fmt);
  len = mg_vprintf (c, fmt, &ap);
  va_end (ap);
  return len;
}

static bool
mg_atonl (struct mg_str str, struct mg_addr *addr)
{
  uint32_t localhost = mg_htonl (0x7f000001);
  if (mg_strcasecmp (str, mg_str ("localhost")) != 0)
    return false;
  memcpy (addr->ip, &localhost, sizeof (uint32_t));
  addr->is_ip6 = false;
  return true;
}

static bool
mg_atone (struct mg_str str, struct mg_addr *addr)
{
  if (str.len > 0)
    return false;
  memset (addr->ip, 0, sizeof (addr->ip));
  addr->is_ip6 = false;
  return true;
}

static bool
mg_aton4 (struct mg_str str, struct mg_addr *addr)
{
  uint8_t data[4] = { 0, 0, 0, 0 };
  size_t i, num_dots = 0;
  for (i = 0; i < str.len; i++)
    {
      if (str.buf[i] >= '0' && str.buf[i] <= '9')
	{
	  int octet = data[num_dots] * 10 + (str.buf[i] - '0');
	  if (octet > 255)
	    return false;
	  data[num_dots] = (uint8_t) octet;
	}
      else if (str.buf[i] == '.')
	{
	  if (num_dots >= 3 || i == 0 || str.buf[i - 1] == '.')
	    return false;
	  num_dots++;
	}
      else
	{
	  return false;
	}
    }
  if (num_dots != 3 || str.buf[i - 1] == '.')
    return false;
  memcpy (&addr->ip, data, sizeof (data));
  addr->is_ip6 = false;
  return true;
}

static bool
mg_v4mapped (struct mg_str str, struct mg_addr *addr)
{
  int i;
  uint32_t ipv4;
  if (str.len < 14)
    return false;
  if (str.buf[0] != ':' || str.buf[1] != ':' || str.buf[6] != ':')
    return false;
  for (i = 2; i < 6; i++)
    {
      if (str.buf[i] != 'f' && str.buf[i] != 'F')
	return false;
    }
  // struct mg_str s = mg_str_n(&str.buf[7], str.len - 7);
  if (!mg_aton4 (mg_str_n (&str.buf[7], str.len - 7), addr))
    return false;
  memcpy (&ipv4, addr->ip, sizeof (ipv4));
  memset (addr->ip, 0, sizeof (addr->ip));
  addr->ip[10] = addr->ip[11] = 255;
  memcpy (&addr->ip[12], &ipv4, 4);
  addr->is_ip6 = true;
  return true;
}

static bool
mg_aton6 (struct mg_str str, struct mg_addr *addr)
{
  size_t i, j = 0, n = 0, dc = 42;
  addr->scope_id = 0;
  if (str.len > 2 && str.buf[0] == '[')
    str.buf++, str.len -= 2;
  if (mg_v4mapped (str, addr))
    return true;
  for (i = 0; i < str.len; i++)
    {
      if ((str.buf[i] >= '0' && str.buf[i] <= '9')
	  || (str.buf[i] >= 'a' && str.buf[i] <= 'f')
	  || (str.buf[i] >= 'A' && str.buf[i] <= 'F'))
	{
	  unsigned long val; // TODO(): This loops on chars, refactor
	  if (i > j + 3)
	    return false;
	  // MG_DEBUG(("%lu %lu [%.*s]", i, j, (int) (i - j + 1),
	  // &str.buf[j]));
	  mg_str_to_num (mg_str_n (&str.buf[j], i - j + 1), 16, &val,
			 sizeof (val));
	  addr->ip[n] = (uint8_t) ((val >> 8) & 255);
	  addr->ip[n + 1] = (uint8_t) (val & 255);
	}
      else if (str.buf[i] == ':')
	{
	  j = i + 1;
	  if (i > 0 && str.buf[i - 1] == ':')
	    {
	      dc = n; // Double colon
	      if (i > 1 && str.buf[i - 2] == ':')
		return false;
	    }
	  else if (i > 0)
	    {
	      n += 2;
	    }
	  if (n > 14)
	    return false;
	  addr->ip[n] = addr->ip[n + 1] = 0; // For trailing ::
	}
      else if (str.buf[i] == '%')
	{ // Scope ID, last in string
	  return mg_str_to_num (mg_str_n (&str.buf[i + 1], str.len - i - 1),
				10, &addr->scope_id, sizeof (uint8_t));
	}
      else
	{
	  return false;
	}
    }
  if (n < 14 && dc == 42)
    return false;
  if (n < 14)
    {
      memmove (&addr->ip[dc + (14 - n)], &addr->ip[dc], n - dc + 2);
      memset (&addr->ip[dc], 0, 14 - n);
    }

  addr->is_ip6 = true;
  return true;
}

bool
mg_aton (struct mg_str str, struct mg_addr *addr)
{
  // MG_INFO(("[%.*s]", (int) str.len, str.buf));
  return mg_atone (str, addr) || mg_atonl (str, addr) || mg_aton4 (str, addr)
	 || mg_aton6 (str, addr);
}

struct mg_connection *
mg_alloc_conn (struct mg_mgr *mgr)
{
  struct mg_connection *c
      = (struct mg_connection *) calloc (1, sizeof (*c) + mgr->extraconnsize);
  if (c != NULL)
    {
      c->mgr = mgr;
      c->send.align = c->recv.align = c->rtls.align = MG_IO_SIZE;
      c->id = ++mgr->nextid;
      MG_PROF_INIT (c);
    }
  return c;
}

void
mg_close_conn (struct mg_connection *c)
{
  mg_resolve_cancel (c); // Close any pending DNS query
  LIST_DELETE (struct mg_connection, &c->mgr->conns, c);
  if (c == c->mgr->dns4.c)
    c->mgr->dns4.c = NULL;
  if (c == c->mgr->dns6.c)
    c->mgr->dns6.c = NULL;
  // Order of operations is important. `MG_EV_CLOSE` event must be fired
  // before we deallocate received data, see #1331
  mg_call (c, MG_EV_CLOSE, NULL);
  MG_DEBUG (("%lu %ld closed", c->id, c->fd));
  MG_PROF_DUMP (c);
  MG_PROF_FREE (c);

  mg_tls_free (c);
  mg_iobuf_free (&c->recv);
  mg_iobuf_free (&c->send);
  mg_iobuf_free (&c->rtls);
  mg_bzero ((unsigned char *) c, sizeof (*c));
  free (c);
}

struct mg_connection *
mg_connect (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
	    void *fn_data)
{
  struct mg_connection *c = NULL;
  if (url == NULL || url[0] == '\0')
    {
      MG_ERROR (("null url"));
    }
  else if ((c = mg_alloc_conn (mgr)) == NULL)
    {
      MG_ERROR (("OOM"));
    }
  else
    {
      LIST_ADD_HEAD (struct mg_connection, &mgr->conns, c);
      c->is_udp = (strncmp (url, "udp:", 4) == 0);
      c->fd = (void *) (size_t) MG_INVALID_SOCKET;
      c->fn = fn;
      c->is_client = true;
      c->fn_data = fn_data;
      MG_DEBUG (("%lu %ld %s", c->id, c->fd, url));
      mg_call (c, MG_EV_OPEN, (void *) url);
      mg_resolve (c, url);
    }
  return c;
}

struct mg_connection *
mg_listen (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
	   void *fn_data)
{
  struct mg_connection *c = NULL;
  if ((c = mg_alloc_conn (mgr)) == NULL)
    {
      MG_ERROR (("OOM %s", url));
    }
  else if (!mg_open_listener (c, url))
    {
      MG_ERROR (("Failed: %s, errno %d", url, errno));
      MG_PROF_FREE (c);
      free (c);
      c = NULL;
    }
  else
    {
      c->is_listening = 1;
      c->is_udp = strncmp (url, "udp:", 4) == 0;
      LIST_ADD_HEAD (struct mg_connection, &mgr->conns, c);
      c->fn = fn;
      c->fn_data = fn_data;
      mg_call (c, MG_EV_OPEN, NULL);
      if (mg_url_is_ssl (url))
	c->is_tls = 1; // Accepted connection must
      MG_DEBUG (("%lu %ld %s", c->id, c->fd, url));
    }
  return c;
}

struct mg_connection *
mg_wrapfd (struct mg_mgr *mgr, int fd, mg_event_handler_t fn, void *fn_data)
{
  struct mg_connection *c = mg_alloc_conn (mgr);
  if (c != NULL)
    {
      c->fd = (void *) (size_t) fd;
      c->fn = fn;
      c->fn_data = fn_data;
      MG_EPOLL_ADD (c);
      mg_call (c, MG_EV_OPEN, NULL);
      LIST_ADD_HEAD (struct mg_connection, &mgr->conns, c);
    }
  return c;
}

struct mg_timer *
mg_timer_add (struct mg_mgr *mgr, uint64_t milliseconds, unsigned flags,
	      void (*fn) (void *), void *arg)
{
  struct mg_timer *t = (struct mg_timer *) calloc (1, sizeof (*t));
  if (t != NULL)
    {
      mg_timer_init (&mgr->timers, t, milliseconds, flags, fn, arg);
      t->id = mgr->timerid++;
    }
  return t;
}

long
mg_io_recv (struct mg_connection *c, void *buf, size_t len)
{
  if (c->rtls.len == 0)
    return MG_IO_WAIT;
  if (len > c->rtls.len)
    len = c->rtls.len;
  memcpy (buf, c->rtls.buf, len);
  mg_iobuf_del (&c->rtls, 0, len);
  return (long) len;
}

void
mg_mgr_free (struct mg_mgr *mgr)
{
  struct mg_connection *c;
  struct mg_timer *tmp, *t = mgr->timers;
  while (t != NULL)
    tmp = t->next, free (t), t = tmp;
  mgr->timers = NULL; // Important. Next call to poll won't touch timers
  for (c = mgr->conns; c != NULL; c = c->next)
    c->is_closing = 1;
  mg_mgr_poll (mgr, 0);
#if MG_ENABLE_FREERTOS_TCP
  FreeRTOS_DeleteSocketSet (mgr->ss);
#endif
  MG_DEBUG (("All connections closed"));
#if MG_ENABLE_EPOLL
  if (mgr->epoll_fd >= 0)
    close (mgr->epoll_fd), mgr->epoll_fd = -1;
#endif
  mg_tls_ctx_free (mgr);
}

void
mg_mgr_init (struct mg_mgr *mgr)
{
  memset (mgr, 0, sizeof (*mgr));
#if MG_ENABLE_EPOLL
  if ((mgr->epoll_fd = epoll_create1 (EPOLL_CLOEXEC)) < 0)
    MG_ERROR (("epoll_create1 errno %d", errno));
#else
  mgr->epoll_fd = -1;
#endif
#if MG_ARCH == MG_ARCH_WIN32 && MG_ENABLE_WINSOCK
  // clang-format off
  { WSADATA data; WSAStartup(MAKEWORD(2, 2), &data); }
  // clang-format on
#elif MG_ENABLE_FREERTOS_TCP
  mgr->ss = FreeRTOS_CreateSocketSet ();
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
  // Ignore SIGPIPE signal, so if client cancels the request, it
  // won't kill the whole process.
  signal (SIGPIPE, SIG_IGN);
#elif MG_ENABLE_TCPIP_DRIVER_INIT && defined(MG_TCPIP_DRIVER_INIT)
  MG_TCPIP_DRIVER_INIT (mgr);
#endif
  mgr->pipe = MG_INVALID_SOCKET;
  mgr->dnstimeout = 3000;
  mgr->dns4.url = "udp://8.8.8.8:53";
  mgr->dns6.url = "udp://[2001:4860:4860::8888]:53";
  mg_tls_ctx_init (mgr);
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/net_builtin.c"
#endif

#if defined(MG_ENABLE_TCPIP) && MG_ENABLE_TCPIP
#  define MG_EPHEMERAL_PORT_BASE 32768
#  define PDIFF(a, b) ((size_t) (((char *) (b)) - ((char *) (a))))

#  ifndef MIP_TCP_KEEPALIVE_MS
#    define MIP_TCP_KEEPALIVE_MS 45000 // TCP keep-alive period, ms
#  endif

#  define MIP_TCP_ACK_MS 150   // Timeout for ACKing
#  define MIP_TCP_ARP_MS 100   // Timeout for ARP response
#  define MIP_TCP_SYN_MS 15000 // Timeout for connection establishment
#  define MIP_TCP_FIN_MS 1000  // Timeout for closing connection
#  define MIP_TCP_WIN 6000     // TCP window size

struct connstate
{
  uint32_t seq, ack;		// TCP seq/ack counters
  uint64_t timer;		// TCP keep-alive / ACK timer
  uint32_t acked;		// Last ACK-ed number
  size_t unacked;		// Not acked bytes
  uint8_t mac[6];		// Peer MAC address
  uint8_t ttype;		// Timer type. 0: ack, 1: keep-alive
#  define MIP_TTYPE_KEEPALIVE 0 // Connection is idle for long, send keepalive
#  define MIP_TTYPE_ACK 1	// Peer sent us data, we have to ack it soon
#  define MIP_TTYPE_ARP 2	// ARP resolve sent, waiting for response
#  define MIP_TTYPE_SYN 3	// SYN sent, waiting for response
#  define MIP_TTYPE_FIN 4 // FIN sent, waiting until terminating the connection
  uint8_t tmiss;	  // Number of keep-alive misses
  struct mg_iobuf raw;	  // For TLS only. Incoming raw data
};

#  pragma pack(push, 1)

struct lcp
{
  uint8_t addr, ctrl, proto[2], code, id, len[2];
};

struct eth
{
  uint8_t dst[6]; // Destination MAC address
  uint8_t src[6]; // Source MAC address
  uint16_t type;  // Ethernet type
};

struct ip
{
  uint8_t ver;	 // Version
  uint8_t tos;	 // Unused
  uint16_t len;	 // Length
  uint16_t id;	 // Unused
  uint16_t frag; // Fragmentation
#  define IP_FRAG_OFFSET_MSK 0xFF1F
#  define IP_MORE_FRAGS_MSK 0x20
  uint8_t ttl;	 // Time to live
  uint8_t proto; // Upper level protocol
  uint16_t csum; // Checksum
  uint32_t src;	 // Source IP
  uint32_t dst;	 // Destination IP
};

struct ip6
{
  uint8_t ver;	   // Version
  uint8_t opts[3]; // Options
  uint16_t len;	   // Length
  uint8_t proto;   // Upper level protocol
  uint8_t ttl;	   // Time to live
  uint8_t src[16]; // Source IP
  uint8_t dst[16]; // Destination IP
};

struct icmp
{
  uint8_t type;
  uint8_t code;
  uint16_t csum;
};

struct arp
{
  uint16_t fmt;	  // Format of hardware address
  uint16_t pro;	  // Format of protocol address
  uint8_t hlen;	  // Length of hardware address
  uint8_t plen;	  // Length of protocol address
  uint16_t op;	  // Operation
  uint8_t sha[6]; // Sender hardware address
  uint32_t spa;	  // Sender protocol address
  uint8_t tha[6]; // Target hardware address
  uint32_t tpa;	  // Target protocol address
};

struct tcp
{
  uint16_t sport; // Source port
  uint16_t dport; // Destination port
  uint32_t seq;	  // Sequence number
  uint32_t ack;	  // Acknowledgement number
  uint8_t off;	  // Data offset
  uint8_t flags;  // TCP flags
#  define TH_FIN 0x01
#  define TH_SYN 0x02
#  define TH_RST 0x04
#  define TH_PUSH 0x08
#  define TH_ACK 0x10
#  define TH_URG 0x20
#  define TH_ECE 0x40
#  define TH_CWR 0x80
  uint16_t win;	 // Window
  uint16_t csum; // Checksum
  uint16_t urp;	 // Urgent pointer
};

struct udp
{
  uint16_t sport; // Source port
  uint16_t dport; // Destination port
  uint16_t len;	  // UDP length
  uint16_t csum;  // UDP checksum
};

struct dhcp
{
  uint8_t op, htype, hlen, hops;
  uint32_t xid;
  uint16_t secs, flags;
  uint32_t ciaddr, yiaddr, siaddr, giaddr;
  uint8_t hwaddr[208];
  uint32_t magic;
  uint8_t options[32];
};

#  pragma pack(pop)

struct pkt
{
  struct mg_str raw; // Raw packet data
  struct mg_str pay; // Payload data
  struct eth *eth;
  struct llc *llc;
  struct arp *arp;
  struct ip *ip;
  struct ip6 *ip6;
  struct icmp *icmp;
  struct tcp *tcp;
  struct udp *udp;
  struct dhcp *dhcp;
};

static void send_syn (struct mg_connection *c);

static void
mkpay (struct pkt *pkt, void *p)
{
  pkt->pay = mg_str_n ((char *) p,
		       (size_t) (&pkt->raw.buf[pkt->raw.len] - (char *) p));
}

static uint32_t
csumup (uint32_t sum, const void *buf, size_t len)
{
  size_t i;
  const uint8_t *p = (const uint8_t *) buf;
  for (i = 0; i < len; i++)
    sum += i & 1 ? p[i] : (uint32_t) (p[i] << 8);
  return sum;
}

static uint16_t
csumfin (uint32_t sum)
{
  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);
  return mg_htons (~sum & 0xffff);
}

static uint16_t
ipcsum (const void *buf, size_t len)
{
  uint32_t sum = csumup (0, buf, len);
  return csumfin (sum);
}

static void
settmout (struct mg_connection *c, uint8_t type)
{
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) c->mgr->priv;
  struct connstate *s = (struct connstate *) (c + 1);
  unsigned n = type == MIP_TTYPE_ACK   ? MIP_TCP_ACK_MS
	       : type == MIP_TTYPE_ARP ? MIP_TCP_ARP_MS
	       : type == MIP_TTYPE_SYN ? MIP_TCP_SYN_MS
	       : type == MIP_TTYPE_FIN ? MIP_TCP_FIN_MS
				       : MIP_TCP_KEEPALIVE_MS;
  s->timer = ifp->now + n;
  s->ttype = type;
  MG_VERBOSE (("%lu %d -> %llx", c->id, type, s->timer));
}

static size_t
ether_output (struct mg_tcpip_if *ifp, size_t len)
{
  size_t n = ifp->driver->tx (ifp->tx.buf, len, ifp);
  if (n == len)
    ifp->nsent++;
  return n;
}

static void
arp_ask (struct mg_tcpip_if *ifp, uint32_t ip)
{
  struct eth *eth = (struct eth *) ifp->tx.buf;
  struct arp *arp = (struct arp *) (eth + 1);
  memset (eth->dst, 255, sizeof (eth->dst));
  memcpy (eth->src, ifp->mac, sizeof (eth->src));
  eth->type = mg_htons (0x806);
  memset (arp, 0, sizeof (*arp));
  arp->fmt = mg_htons (1), arp->pro = mg_htons (0x800), arp->hlen = 6,
  arp->plen = 4;
  arp->op = mg_htons (1), arp->tpa = ip, arp->spa = ifp->ip;
  memcpy (arp->sha, ifp->mac, sizeof (arp->sha));
  ether_output (ifp, PDIFF (eth, arp + 1));
}

static void
onstatechange (struct mg_tcpip_if *ifp)
{
  if (ifp->state == MG_TCPIP_STATE_READY)
    {
      MG_INFO (("READY, IP: %M", mg_print_ip4, &ifp->ip));
      MG_INFO (("       GW: %M", mg_print_ip4, &ifp->gw));
      MG_INFO (("      MAC: %M", mg_print_mac, &ifp->mac));
      arp_ask (ifp, ifp->gw);
    }
  else if (ifp->state == MG_TCPIP_STATE_UP)
    {
      MG_ERROR (("Link up"));
      srand ((unsigned int) mg_millis ());
    }
  else if (ifp->state == MG_TCPIP_STATE_DOWN)
    {
      MG_ERROR (("Link down"));
    }
}

static struct ip *
tx_ip (struct mg_tcpip_if *ifp, uint8_t *mac_dst, uint8_t proto,
       uint32_t ip_src, uint32_t ip_dst, size_t plen)
{
  struct eth *eth = (struct eth *) ifp->tx.buf;
  struct ip *ip = (struct ip *) (eth + 1);
  memcpy (eth->dst, mac_dst, sizeof (eth->dst));
  memcpy (eth->src, ifp->mac, sizeof (eth->src)); // Use our MAC
  eth->type = mg_htons (0x800);
  memset (ip, 0, sizeof (*ip));
  ip->ver = 0x45;  // Version 4, header length 5 words
  ip->frag = 0x40; // Don't fragment
  ip->len = mg_htons ((uint16_t) (sizeof (*ip) + plen));
  ip->ttl = 64;
  ip->proto = proto;
  ip->src = ip_src;
  ip->dst = ip_dst;
  ip->csum = ipcsum (ip, sizeof (*ip));
  return ip;
}

static void
tx_udp (struct mg_tcpip_if *ifp, uint8_t *mac_dst, uint32_t ip_src,
	uint16_t sport, uint32_t ip_dst, uint16_t dport, const void *buf,
	size_t len)
{
  struct ip *ip
      = tx_ip (ifp, mac_dst, 17, ip_src, ip_dst, len + sizeof (struct udp));
  struct udp *udp = (struct udp *) (ip + 1);
  // MG_DEBUG(("UDP XX LEN %d %d", (int) len, (int) ifp->tx.len));
  udp->sport = sport;
  udp->dport = dport;
  udp->len = mg_htons ((uint16_t) (sizeof (*udp) + len));
  udp->csum = 0;
  uint32_t cs = csumup (0, udp, sizeof (*udp));
  cs = csumup (cs, buf, len);
  cs = csumup (cs, &ip->src, sizeof (ip->src));
  cs = csumup (cs, &ip->dst, sizeof (ip->dst));
  cs += (uint32_t) (ip->proto + sizeof (*udp) + len);
  udp->csum = csumfin (cs);
  memmove (udp + 1, buf, len);
  // MG_DEBUG(("UDP LEN %d %d", (int) len, (int) ifp->frame_len));
  ether_output (ifp, sizeof (struct eth) + sizeof (*ip) + sizeof (*udp) + len);
}

static void
tx_dhcp (struct mg_tcpip_if *ifp, uint8_t *mac_dst, uint32_t ip_src,
	 uint32_t ip_dst, uint8_t *opts, size_t optslen, bool ciaddr)
{
  // https://datatracker.ietf.org/doc/html/rfc2132#section-9.6
  struct dhcp dhcp = { 1, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, 0, { 0 } };
  dhcp.magic = mg_htonl (0x63825363);
  memcpy (&dhcp.hwaddr, ifp->mac, sizeof (ifp->mac));
  memcpy (&dhcp.xid, ifp->mac + 2, sizeof (dhcp.xid));
  memcpy (&dhcp.options, opts, optslen);
  if (ciaddr)
    dhcp.ciaddr = ip_src;
  tx_udp (ifp, mac_dst, ip_src, mg_htons (68), ip_dst, mg_htons (67), &dhcp,
	  sizeof (dhcp));
}

static const uint8_t broadcast[] = { 255, 255, 255, 255, 255, 255 };

// RFC-2131 #4.3.6, #4.4.1
static void
tx_dhcp_request_sel (struct mg_tcpip_if *ifp, uint32_t ip_req, uint32_t ip_srv)
{
  uint8_t opts[] = {
    53, 1, 3,		     // Type: DHCP request
    55, 2, 1,	3,	     // GW and mask
    12, 3, 'm', 'i', 'p',    // Host name: "mip"
    54, 4, 0,	0,   0,	  0, // DHCP server ID
    50, 4, 0,	0,   0,	  0, // Requested IP
    255			     // End of options
  };
  memcpy (opts + 14, &ip_srv, sizeof (ip_srv));
  memcpy (opts + 20, &ip_req, sizeof (ip_req));
  tx_dhcp (ifp, (uint8_t *) broadcast, 0, 0xffffffff, opts, sizeof (opts),
	   false);
  MG_DEBUG (("DHCP req sent"));
}

// RFC-2131 #4.3.6, #4.4.5 (renewing: unicast, rebinding: bcast)
static void
tx_dhcp_request_re (struct mg_tcpip_if *ifp, uint8_t *mac_dst, uint32_t ip_src,
		    uint32_t ip_dst)
{
  uint8_t opts[] = {
    53, 1, 3, // Type: DHCP request
    255	      // End of options
  };
  tx_dhcp (ifp, mac_dst, ip_src, ip_dst, opts, sizeof (opts), true);
  MG_DEBUG (("DHCP req sent"));
}

static void
tx_dhcp_discover (struct mg_tcpip_if *ifp)
{
  uint8_t opts[] = {
    53, 1, 1,	 // Type: DHCP discover
    55, 2, 1, 3, // Parameters: ip, mask
    255		 // End of options
  };
  tx_dhcp (ifp, (uint8_t *) broadcast, 0, 0xffffffff, opts, sizeof (opts),
	   false);
  MG_DEBUG (("DHCP discover sent. Our MAC: %M", mg_print_mac, ifp->mac));
}

static struct mg_connection *
getpeer (struct mg_mgr *mgr, struct pkt *pkt, bool lsn)
{
  struct mg_connection *c = NULL;
  for (c = mgr->conns; c != NULL; c = c->next)
    {
      if (c->is_arplooking && pkt->arp
	  && memcmp (&pkt->arp->spa, c->rem.ip, sizeof (pkt->arp->spa)) == 0)
	break;
      if (c->is_udp && pkt->udp && c->loc.port == pkt->udp->dport)
	break;
      if (!c->is_udp && pkt->tcp && c->loc.port == pkt->tcp->dport
	  && lsn == c->is_listening && (lsn || c->rem.port == pkt->tcp->sport))
	break;
    }
  return c;
}

static void
rx_arp (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  if (pkt->arp->op == mg_htons (1) && pkt->arp->tpa == ifp->ip)
    {
      // ARP request. Make a response, then send
      // MG_DEBUG(("ARP op %d %M: %M", mg_ntohs(pkt->arp->op), mg_print_ip4,
      //          &pkt->arp->spa, mg_print_ip4, &pkt->arp->tpa));
      struct eth *eth = (struct eth *) ifp->tx.buf;
      struct arp *arp = (struct arp *) (eth + 1);
      memcpy (eth->dst, pkt->eth->src, sizeof (eth->dst));
      memcpy (eth->src, ifp->mac, sizeof (eth->src));
      eth->type = mg_htons (0x806);
      *arp = *pkt->arp;
      arp->op = mg_htons (2);
      memcpy (arp->tha, pkt->arp->sha, sizeof (pkt->arp->tha));
      memcpy (arp->sha, ifp->mac, sizeof (pkt->arp->sha));
      arp->tpa = pkt->arp->spa;
      arp->spa = ifp->ip;
      MG_DEBUG (("ARP: tell %M we're %M", mg_print_ip4, &arp->tpa,
		 mg_print_mac, &ifp->mac));
      ether_output (ifp, PDIFF (eth, arp + 1));
    }
  else if (pkt->arp->op == mg_htons (2))
    {
      if (memcmp (pkt->arp->tha, ifp->mac, sizeof (pkt->arp->tha)) != 0)
	return;
      if (pkt->arp->spa == ifp->gw)
	{
	  // Got response for the GW ARP request. Set ifp->gwmac
	  memcpy (ifp->gwmac, pkt->arp->sha, sizeof (ifp->gwmac));
	}
      else
	{
	  struct mg_connection *c = getpeer (ifp->mgr, pkt, false);
	  if (c != NULL && c->is_arplooking)
	    {
	      struct connstate *s = (struct connstate *) (c + 1);
	      memcpy (s->mac, pkt->arp->sha, sizeof (s->mac));
	      MG_DEBUG (("%lu ARP resolved %M -> %M", c->id, mg_print_ip4,
			 c->rem.ip, mg_print_mac, s->mac));
	      c->is_arplooking = 0;
	      send_syn (c);
	      settmout (c, MIP_TTYPE_SYN);
	    }
	}
    }
}

static void
rx_icmp (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  // MG_DEBUG(("ICMP %d", (int) len));
  if (pkt->icmp->type == 8 && pkt->ip != NULL && pkt->ip->dst == ifp->ip)
    {
      size_t hlen
	  = sizeof (struct eth) + sizeof (struct ip) + sizeof (struct icmp);
      size_t space = ifp->tx.len - hlen, plen = pkt->pay.len;
      if (plen > space)
	plen = space;
      struct ip *ip = tx_ip (ifp, pkt->eth->src, 1, ifp->ip, pkt->ip->src,
			     sizeof (struct icmp) + plen);
      struct icmp *icmp = (struct icmp *) (ip + 1);
      memset (icmp, 0, sizeof (*icmp));	     // Set csum to 0
      memcpy (icmp + 1, pkt->pay.buf, plen); // Copy RX payload to TX
      icmp->csum = ipcsum (icmp, sizeof (*icmp) + plen);
      ether_output (ifp, hlen + plen);
    }
}

static void
rx_dhcp_client (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  uint32_t ip = 0, gw = 0, mask = 0, lease = 0;
  uint8_t msgtype = 0, state = ifp->state;
  // perform size check first, then access fields
  uint8_t *p = pkt->dhcp->options,
	  *end = (uint8_t *) &pkt->raw.buf[pkt->raw.len];
  if (end < (uint8_t *) (pkt->dhcp + 1))
    return;
  if (memcmp (&pkt->dhcp->xid, ifp->mac + 2, sizeof (pkt->dhcp->xid)))
    return;
  while (p + 1 < end && p[0] != 255)
    { // Parse options RFC-1533 #9
      if (p[0] == 1 && p[1] == sizeof (ifp->mask) && p + 6 < end)
	{ // Mask
	  memcpy (&mask, p + 2, sizeof (mask));
	}
      else if (p[0] == 3 && p[1] == sizeof (ifp->gw) && p + 6 < end)
	{ // GW
	  memcpy (&gw, p + 2, sizeof (gw));
	  ip = pkt->dhcp->yiaddr;
	}
      else if (p[0] == 51 && p[1] == 4 && p + 6 < end)
	{ // Lease
	  memcpy (&lease, p + 2, sizeof (lease));
	  lease = mg_ntohl (lease);
	}
      else if (p[0] == 53 && p[1] == 1 && p + 6 < end)
	{ // Msg Type
	  msgtype = p[2];
	}
      p += p[1] + 2;
    }
  // Process message type, RFC-1533 (9.4); RFC-2131 (3.1, 4)
  if (msgtype == 6 && ifp->ip == ip)
    { // DHCPNACK, release IP
      ifp->state = MG_TCPIP_STATE_UP, ifp->ip = 0;
    }
  else if (msgtype == 2 && ifp->state == MG_TCPIP_STATE_UP && ip && gw
	   && lease)
    { // DHCPOFFER
      // select IP, (4.4.1) (fallback to IP source addr on foul play)
      tx_dhcp_request_sel (
	  ifp, ip, pkt->dhcp->siaddr ? pkt->dhcp->siaddr : pkt->ip->src);
      ifp->state = MG_TCPIP_STATE_REQ; // REQUESTING state
    }
  else if (msgtype == 5)
    { // DHCPACK
      if (ifp->state == MG_TCPIP_STATE_REQ && ip && gw && lease)
	{ // got an IP
	  ifp->lease_expire = ifp->now + lease * 1000;
	  MG_INFO (("Lease: %u sec (%lld)", lease, ifp->lease_expire / 1000));
	  // assume DHCP server = router until ARP resolves
	  memcpy (ifp->gwmac, pkt->eth->src, sizeof (ifp->gwmac));
	  ifp->ip = ip, ifp->gw = gw, ifp->mask = mask;
	  ifp->state = MG_TCPIP_STATE_READY; // BOUND state
	  uint64_t rand;
	  mg_random (&rand, sizeof (rand));
	  srand ((unsigned int) (rand + mg_millis ()));
	}
      else if (ifp->state == MG_TCPIP_STATE_READY && ifp->ip == ip)
	{ // renew
	  ifp->lease_expire = ifp->now + lease * 1000;
	  MG_INFO (("Lease: %u sec (%lld)", lease, ifp->lease_expire / 1000));
	} // TODO(): accept provided T1/T2 and store server IP for renewal
	  // (4.4)
    }
  if (ifp->state != state)
    onstatechange (ifp);
}

// Simple DHCP server that assigns a next IP address: ifp->ip + 1
static void
rx_dhcp_server (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  uint8_t op = 0, *p = pkt->dhcp->options,
	  *end = (uint8_t *) &pkt->raw.buf[pkt->raw.len];
  if (end < (uint8_t *) (pkt->dhcp + 1))
    return;
  // struct dhcp *req = pkt->dhcp;
  struct dhcp res = { 2, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, 0, { 0 } };
  res.yiaddr = ifp->ip;
  ((uint8_t *) (&res.yiaddr))[3]++; // Offer our IP + 1
  while (p + 1 < end && p[0] != 255)
    { // Parse options
      if (p[0] == 53 && p[1] == 1 && p + 2 < end)
	{ // Message type
	  op = p[2];
	}
      p += p[1] + 2;
    }
  if (op == 1 || op == 3)
    {				     // DHCP Discover or DHCP Request
      uint8_t msg = op == 1 ? 2 : 5; // Message type: DHCP OFFER or DHCP ACK
      uint8_t opts[] = {
	53, 1, msg,		   // Message type
	1,  4, 0,   0,	 0,   0,   // Subnet mask
	54, 4, 0,   0,	 0,   0,   // Server ID
	12, 3, 'm', 'i', 'p',	   // Host name: "mip"
	51, 4, 255, 255, 255, 255, // Lease time
	255			   // End of options
      };
      memcpy (&res.hwaddr, pkt->dhcp->hwaddr, 6);
      memcpy (opts + 5, &ifp->mask, sizeof (ifp->mask));
      memcpy (opts + 11, &ifp->ip, sizeof (ifp->ip));
      memcpy (&res.options, opts, sizeof (opts));
      res.magic = pkt->dhcp->magic;
      res.xid = pkt->dhcp->xid;
      if (ifp->enable_get_gateway)
	{
	  ifp->gw = res.yiaddr;
	  memcpy (ifp->gwmac, pkt->eth->src, sizeof (ifp->gwmac));
	}
      tx_udp (ifp, pkt->eth->src, ifp->ip, mg_htons (67),
	      op == 1 ? ~0U : res.yiaddr, mg_htons (68), &res, sizeof (res));
    }
}

static void
rx_udp (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  struct mg_connection *c = getpeer (ifp->mgr, pkt, true);
  if (c == NULL)
    {
      // No UDP listener on this port. Should send ICMP, but keep silent.
    }
  else
    {
      c->rem.port = pkt->udp->sport;
      memcpy (c->rem.ip, &pkt->ip->src, sizeof (uint32_t));
      struct connstate *s = (struct connstate *) (c + 1);
      memcpy (s->mac, pkt->eth->src, sizeof (s->mac));
      if (c->recv.len >= MG_MAX_RECV_SIZE)
	{
	  mg_error (c, "max_recv_buf_size reached");
	}
      else if (c->recv.size - c->recv.len < pkt->pay.len
	       && !mg_iobuf_resize (&c->recv, c->recv.len + pkt->pay.len))
	{
	  mg_error (c, "oom");
	}
      else
	{
	  memcpy (&c->recv.buf[c->recv.len], pkt->pay.buf, pkt->pay.len);
	  c->recv.len += pkt->pay.len;
	  mg_call (c, MG_EV_READ, &pkt->pay.len);
	}
    }
}

static size_t
tx_tcp (struct mg_tcpip_if *ifp, uint8_t *dst_mac, uint32_t dst_ip,
	uint8_t flags, uint16_t sport, uint16_t dport, uint32_t seq,
	uint32_t ack, const void *buf, size_t len)
{
#  if 0
  uint8_t opts[] = {2, 4, 5, 0xb4, 4, 2, 0, 0};  // MSS = 1460, SACK permitted
  if (flags & TH_SYN) {
    // Handshake? Set MSS
    buf = opts;
    len = sizeof(opts);
  }
#  endif
  struct ip *ip
      = tx_ip (ifp, dst_mac, 6, ifp->ip, dst_ip, sizeof (struct tcp) + len);
  struct tcp *tcp = (struct tcp *) (ip + 1);
  memset (tcp, 0, sizeof (*tcp));
  if (buf != NULL && len)
    memmove (tcp + 1, buf, len);
  tcp->sport = sport;
  tcp->dport = dport;
  tcp->seq = seq;
  tcp->ack = ack;
  tcp->flags = flags;
  tcp->win = mg_htons (MIP_TCP_WIN);
  tcp->off = (uint8_t) (sizeof (*tcp) / 4 << 4);
  // if (flags & TH_SYN) tcp->off = 0x70;  // Handshake? header size 28 bytes

  uint32_t cs = 0;
  uint16_t n = (uint16_t) (sizeof (*tcp) + len);
  uint8_t pseudo[] = { 0, ip->proto, (uint8_t) (n >> 8), (uint8_t) (n & 255) };
  cs = csumup (cs, tcp, n);
  cs = csumup (cs, &ip->src, sizeof (ip->src));
  cs = csumup (cs, &ip->dst, sizeof (ip->dst));
  cs = csumup (cs, pseudo, sizeof (pseudo));
  tcp->csum = csumfin (cs);
  MG_VERBOSE (("TCP %M:%hu -> %M:%hu fl %x len %u", mg_print_ip4, &ip->src,
	       mg_ntohs (tcp->sport), mg_print_ip4, &ip->dst,
	       mg_ntohs (tcp->dport), tcp->flags, len));
  // mg_hexdump(ifp->tx.buf, PDIFF(ifp->tx.buf, tcp + 1) + len);
  return ether_output (ifp, PDIFF (ifp->tx.buf, tcp + 1) + len);
}

static size_t
tx_tcp_pkt (struct mg_tcpip_if *ifp, struct pkt *pkt, uint8_t flags,
	    uint32_t seq, const void *buf, size_t len)
{
  uint32_t delta = (pkt->tcp->flags & (TH_SYN | TH_FIN)) ? 1 : 0;
  return tx_tcp (ifp, pkt->eth->src, pkt->ip->src, flags, pkt->tcp->dport,
		 pkt->tcp->sport, seq,
		 mg_htonl (mg_ntohl (pkt->tcp->seq) + delta), buf, len);
}

static struct mg_connection *
accept_conn (struct mg_connection *lsn, struct pkt *pkt)
{
  struct mg_connection *c = mg_alloc_conn (lsn->mgr);
  if (c == NULL)
    {
      MG_ERROR (("OOM"));
      return NULL;
    }
  struct connstate *s = (struct connstate *) (c + 1);
  s->seq = mg_ntohl (pkt->tcp->ack), s->ack = mg_ntohl (pkt->tcp->seq);
  memcpy (s->mac, pkt->eth->src, sizeof (s->mac));
  settmout (c, MIP_TTYPE_KEEPALIVE);
  memcpy (c->rem.ip, &pkt->ip->src, sizeof (uint32_t));
  c->rem.port = pkt->tcp->sport;
  MG_DEBUG (("%lu accepted %M", c->id, mg_print_ip_port, &c->rem));
  LIST_ADD_HEAD (struct mg_connection, &lsn->mgr->conns, c);
  c->is_accepted = 1;
  c->is_hexdumping = lsn->is_hexdumping;
  c->pfn = lsn->pfn;
  c->loc = lsn->loc;
  c->pfn_data = lsn->pfn_data;
  c->fn = lsn->fn;
  c->fn_data = lsn->fn_data;
  mg_call (c, MG_EV_OPEN, NULL);
  mg_call (c, MG_EV_ACCEPT, NULL);
  return c;
}

static size_t
trim_len (struct mg_connection *c, size_t len)
{
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) c->mgr->priv;
  size_t eth_h_len = 14, ip_max_h_len = 24, tcp_max_h_len = 60, udp_h_len = 8;
  size_t max_headers_len
      = eth_h_len + ip_max_h_len + (c->is_udp ? udp_h_len : tcp_max_h_len);
  size_t min_mtu = c->is_udp ? 68 /* RFC-791 */ : max_headers_len - eth_h_len;

  // If the frame exceeds the available buffer, trim the length
  if (len + max_headers_len > ifp->tx.len)
    {
      len = ifp->tx.len - max_headers_len;
    }
  // Ensure the MTU isn't lower than the minimum allowed value
  if (ifp->mtu < min_mtu)
    {
      MG_ERROR (("MTU is lower than minimum, capping to %lu", min_mtu));
      ifp->mtu = (uint16_t) min_mtu;
    }
  // If the total packet size exceeds the MTU, trim the length
  if (len + max_headers_len - eth_h_len > ifp->mtu)
    {
      len = ifp->mtu - max_headers_len + eth_h_len;
      if (c->is_udp)
	{
	  MG_ERROR (("UDP datagram exceeds MTU. Truncating it."));
	}
    }

  return len;
}

long
mg_io_send (struct mg_connection *c, const void *buf, size_t len)
{
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) c->mgr->priv;
  struct connstate *s = (struct connstate *) (c + 1);
  uint32_t dst_ip = *(uint32_t *) c->rem.ip;
  len = trim_len (c, len);
  if (c->is_udp)
    {
      tx_udp (ifp, s->mac, ifp->ip, c->loc.port, dst_ip, c->rem.port, buf,
	      len);
    }
  else
    {
      size_t sent = tx_tcp (ifp, s->mac, dst_ip, TH_PUSH | TH_ACK, c->loc.port,
			    c->rem.port, mg_htonl (s->seq), mg_htonl (s->ack),
			    buf, len);
      if (sent == 0)
	{
	  return MG_IO_WAIT;
	}
      else if (sent == (size_t) -1)
	{
	  return MG_IO_ERR;
	}
      else
	{
	  s->seq += (uint32_t) len;
	  if (s->ttype == MIP_TTYPE_ACK)
	    settmout (c, MIP_TTYPE_KEEPALIVE);
	}
    }
  return (long) len;
}

static void
handle_tls_recv (struct mg_connection *c, struct mg_iobuf *io)
{
  long n = mg_tls_recv (c, &io->buf[io->len], io->size - io->len);
  if (n == MG_IO_ERR)
    {
      mg_error (c, "TLS recv error");
    }
  else if (n > 0)
    {
      // Decrypted successfully - trigger MG_EV_READ
      io->len += (size_t) n;
      mg_call (c, MG_EV_READ, &n);
    }
}

static void
read_conn (struct mg_connection *c, struct pkt *pkt)
{
  struct connstate *s = (struct connstate *) (c + 1);
  struct mg_iobuf *io = c->is_tls ? &c->rtls : &c->recv;
  uint32_t seq = mg_ntohl (pkt->tcp->seq);
  uint32_t rem_ip;
  memcpy (&rem_ip, c->rem.ip, sizeof (uint32_t));
  if (pkt->tcp->flags & TH_FIN)
    {
      // If we initiated the closure, we reply with ACK upon receiving FIN
      // If we didn't initiate it, we reply with FIN as part of the normal TCP
      // closure process
      uint8_t flags = TH_ACK;
      s->ack = (uint32_t) (mg_htonl (pkt->tcp->seq) + pkt->pay.len + 1);
      if (c->is_draining && s->ttype == MIP_TTYPE_FIN)
	{
	  if (s->seq == mg_htonl (pkt->tcp->ack))
	    {		// Simultaneous closure ?
	      s->seq++; // Yes. Increment our SEQ
	    }
	  else
	    {					 // Otherwise,
	      s->seq = mg_htonl (pkt->tcp->ack); // Set to peer's ACK
	    }
	}
      else
	{
	  flags |= TH_FIN;
	  c->is_draining = 1;
	  settmout (c, MIP_TTYPE_FIN);
	}
      tx_tcp ((struct mg_tcpip_if *) c->mgr->priv, s->mac, rem_ip, flags,
	      c->loc.port, c->rem.port, mg_htonl (s->seq), mg_htonl (s->ack),
	      "", 0);
    }
  else if (pkt->pay.len == 0)
    {
      // TODO(cpq): handle this peer's ACK
    }
  else if (seq != s->ack)
    {
      uint32_t ack = (uint32_t) (mg_htonl (pkt->tcp->seq) + pkt->pay.len);
      if (s->ack == ack)
	{
	  MG_VERBOSE (("ignoring duplicate pkt"));
	}
      else
	{
	  MG_VERBOSE (("SEQ != ACK: %x %x %x", seq, s->ack, ack));
	  tx_tcp ((struct mg_tcpip_if *) c->mgr->priv, s->mac, rem_ip, TH_ACK,
		  c->loc.port, c->rem.port, mg_htonl (s->seq),
		  mg_htonl (s->ack), "", 0);
	}
    }
  else if (io->size - io->len < pkt->pay.len
	   && !mg_iobuf_resize (io, io->len + pkt->pay.len))
    {
      mg_error (c, "oom");
    }
  else
    {
      // Copy TCP payload into the IO buffer. If the connection is plain text,
      // we copy to c->recv. If the connection is TLS, this data is encrypted,
      // therefore we copy that encrypted data to the c->rtls iobuffer instead,
      // and then call mg_tls_recv() to decrypt it. NOTE: mg_tls_recv() will
      // call back mg_io_recv() which grabs raw data from c->rtls
      memcpy (&io->buf[io->len], pkt->pay.buf, pkt->pay.len);
      io->len += pkt->pay.len;

      MG_VERBOSE (
	  ("%lu SEQ %x -> %x", c->id, mg_htonl (pkt->tcp->seq), s->ack));
      // Advance ACK counter
      s->ack = (uint32_t) (mg_htonl (pkt->tcp->seq) + pkt->pay.len);
      s->unacked += pkt->pay.len;
      // size_t diff = s->acked <= s->ack ? s->ack - s->acked : s->ack;
      if (s->unacked > MIP_TCP_WIN / 2 && s->acked != s->ack)
	{
	  // Send ACK immediately
	  MG_VERBOSE (("%lu imm ACK %lu", c->id, s->acked));
	  tx_tcp ((struct mg_tcpip_if *) c->mgr->priv, s->mac, rem_ip, TH_ACK,
		  c->loc.port, c->rem.port, mg_htonl (s->seq),
		  mg_htonl (s->ack), NULL, 0);
	  s->unacked = 0;
	  s->acked = s->ack;
	  if (s->ttype != MIP_TTYPE_KEEPALIVE)
	    settmout (c, MIP_TTYPE_KEEPALIVE);
	}
      else
	{
	  // if not already running, setup a timer to send an ACK later
	  if (s->ttype != MIP_TTYPE_ACK)
	    settmout (c, MIP_TTYPE_ACK);
	}

      if (c->is_tls && c->is_tls_hs)
	{
	  mg_tls_handshake (c);
	}
      else if (c->is_tls)
	{
	  // TLS connection. Make room for decrypted data in c->recv
	  io = &c->recv;
	  if (io->size - io->len < pkt->pay.len
	      && !mg_iobuf_resize (io, io->len + pkt->pay.len))
	    {
	      mg_error (c, "oom");
	    }
	  else
	    {
	      // Decrypt data directly into c->recv
	      handle_tls_recv (c, io);
	    }
	}
      else
	{
	  // Plain text connection, data is already in c->recv, trigger
	  // MG_EV_READ
	  mg_call (c, MG_EV_READ, &pkt->pay.len);
	}
    }
}

static void
rx_tcp (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  struct mg_connection *c = getpeer (ifp->mgr, pkt, false);
  struct connstate *s = c == NULL ? NULL : (struct connstate *) (c + 1);
#  if 0
  MG_INFO(("%lu %hhu %d", c ? c->id : 0, pkt->tcp->flags, (int) pkt->pay.len));
#  endif
  if (c != NULL && c->is_connecting && pkt->tcp->flags == (TH_SYN | TH_ACK))
    {
      s->seq = mg_ntohl (pkt->tcp->ack), s->ack = mg_ntohl (pkt->tcp->seq) + 1;
      tx_tcp_pkt (ifp, pkt, TH_ACK, pkt->tcp->ack, NULL, 0);
      c->is_connecting = 0; // Client connected
      settmout (c, MIP_TTYPE_KEEPALIVE);
      mg_call (c, MG_EV_CONNECT, NULL); // Let user know
    }
  else if (c != NULL && c->is_connecting && pkt->tcp->flags != TH_ACK)
    {
      // mg_hexdump(pkt->raw.buf, pkt->raw.len);
      tx_tcp_pkt (ifp, pkt, TH_RST | TH_ACK, pkt->tcp->ack, NULL, 0);
    }
  else if (c != NULL && pkt->tcp->flags & TH_RST)
    {
      mg_error (c, "peer RST"); // RFC-1122 4.2.2.13
    }
  else if (c != NULL)
    {
#  if 0
    MG_DEBUG(("%lu %d %M:%hu -> %M:%hu", c->id, (int) pkt->raw.len,
              mg_print_ip4, &pkt->ip->src, mg_ntohs(pkt->tcp->sport),
              mg_print_ip4, &pkt->ip->dst, mg_ntohs(pkt->tcp->dport)));
    mg_hexdump(pkt->pay.buf, pkt->pay.len);
#  endif
      s->tmiss = 0;			   // Reset missed keep-alive counter
      if (s->ttype == MIP_TTYPE_KEEPALIVE) // Advance keep-alive timer
	settmout (
	    c,
	    MIP_TTYPE_KEEPALIVE); // unless a former ACK timeout is pending
      read_conn (c, pkt);	  // Override timer with ACK timeout if needed
    }
  else if ((c = getpeer (ifp->mgr, pkt, true)) == NULL)
    {
      tx_tcp_pkt (ifp, pkt, TH_RST | TH_ACK, pkt->tcp->ack, NULL, 0);
    }
  else if (pkt->tcp->flags & TH_RST)
    {
      if (c->is_accepted)
	mg_error (c, "peer RST"); // RFC-1122 4.2.2.13
      // ignore RST if not connected
    }
  else if (pkt->tcp->flags & TH_SYN)
    {
      // Use peer's source port as ISN, in order to recognise the handshake
      uint32_t isn = mg_htonl ((uint32_t) mg_ntohs (pkt->tcp->sport));
      tx_tcp_pkt (ifp, pkt, TH_SYN | TH_ACK, isn, NULL, 0);
    }
  else if (pkt->tcp->flags & TH_FIN)
    {
      tx_tcp_pkt (ifp, pkt, TH_FIN | TH_ACK, pkt->tcp->ack, NULL, 0);
    }
  else if (mg_htonl (pkt->tcp->ack) == mg_htons (pkt->tcp->sport) + 1U)
    {
      accept_conn (c, pkt);
    }
  else if (!c->is_accepted)
    { // no peer
      tx_tcp_pkt (ifp, pkt, TH_RST | TH_ACK, pkt->tcp->ack, NULL, 0);
    }
  else
    {
      // MG_VERBOSE(("dropped silently.."));
    }
}

static void
rx_ip (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  if (pkt->ip->frag & IP_MORE_FRAGS_MSK || pkt->ip->frag & IP_FRAG_OFFSET_MSK)
    {
      if (pkt->ip->proto == 17)
	pkt->udp = (struct udp *) (pkt->ip + 1);
      if (pkt->ip->proto == 6)
	pkt->tcp = (struct tcp *) (pkt->ip + 1);
      struct mg_connection *c = getpeer (ifp->mgr, pkt, false);
      if (c)
	mg_error (c, "Received fragmented packet");
    }
  else if (pkt->ip->proto == 1)
    {
      pkt->icmp = (struct icmp *) (pkt->ip + 1);
      if (pkt->pay.len < sizeof (*pkt->icmp))
	return;
      mkpay (pkt, pkt->icmp + 1);
      rx_icmp (ifp, pkt);
    }
  else if (pkt->ip->proto == 17)
    {
      pkt->udp = (struct udp *) (pkt->ip + 1);
      if (pkt->pay.len < sizeof (*pkt->udp))
	return;
      mkpay (pkt, pkt->udp + 1);
      MG_VERBOSE (("UDP %M:%hu -> %M:%hu len %u", mg_print_ip4, &pkt->ip->src,
		   mg_ntohs (pkt->udp->sport), mg_print_ip4, &pkt->ip->dst,
		   mg_ntohs (pkt->udp->dport), (int) pkt->pay.len));
      if (ifp->enable_dhcp_client && pkt->udp->dport == mg_htons (68))
	{
	  pkt->dhcp = (struct dhcp *) (pkt->udp + 1);
	  mkpay (pkt, pkt->dhcp + 1);
	  rx_dhcp_client (ifp, pkt);
	}
      else if (ifp->enable_dhcp_server && pkt->udp->dport == mg_htons (67))
	{
	  pkt->dhcp = (struct dhcp *) (pkt->udp + 1);
	  mkpay (pkt, pkt->dhcp + 1);
	  rx_dhcp_server (ifp, pkt);
	}
      else
	{
	  rx_udp (ifp, pkt);
	}
    }
  else if (pkt->ip->proto == 6)
    {
      pkt->tcp = (struct tcp *) (pkt->ip + 1);
      if (pkt->pay.len < sizeof (*pkt->tcp))
	return;
      mkpay (pkt, pkt->tcp + 1);
      uint16_t iplen = mg_ntohs (pkt->ip->len);
      uint16_t off
	  = (uint16_t) (sizeof (*pkt->ip) + ((pkt->tcp->off >> 4) * 4U));
      if (iplen >= off)
	pkt->pay.len = (size_t) (iplen - off);
      MG_VERBOSE (("TCP %M:%hu -> %M:%hu len %u", mg_print_ip4, &pkt->ip->src,
		   mg_ntohs (pkt->tcp->sport), mg_print_ip4, &pkt->ip->dst,
		   mg_ntohs (pkt->tcp->dport), (int) pkt->pay.len));
      rx_tcp (ifp, pkt);
    }
}

static void
rx_ip6 (struct mg_tcpip_if *ifp, struct pkt *pkt)
{
  // MG_DEBUG(("IP %d", (int) len));
  if (pkt->ip6->proto == 1 || pkt->ip6->proto == 58)
    {
      pkt->icmp = (struct icmp *) (pkt->ip6 + 1);
      if (pkt->pay.len < sizeof (*pkt->icmp))
	return;
      mkpay (pkt, pkt->icmp + 1);
      rx_icmp (ifp, pkt);
    }
  else if (pkt->ip6->proto == 17)
    {
      pkt->udp = (struct udp *) (pkt->ip6 + 1);
      if (pkt->pay.len < sizeof (*pkt->udp))
	return;
      // MG_DEBUG(("  UDP %u %u -> %u", len, mg_htons(udp->sport),
      // mg_htons(udp->dport)));
      mkpay (pkt, pkt->udp + 1);
    }
}

static void
mg_tcpip_rx (struct mg_tcpip_if *ifp, void *buf, size_t len)
{
  struct pkt pkt;
  memset (&pkt, 0, sizeof (pkt));
  pkt.raw.buf = (char *) buf;
  pkt.raw.len = len;
  pkt.eth = (struct eth *) buf;
  // mg_hexdump(buf, len > 16 ? 16: len);
  if (pkt.raw.len < sizeof (*pkt.eth))
    return; // Truncated - runt?
  if (ifp->enable_mac_check
      && memcmp (pkt.eth->dst, ifp->mac, sizeof (pkt.eth->dst)) != 0
      && memcmp (pkt.eth->dst, broadcast, sizeof (pkt.eth->dst)) != 0)
    return;
  if (ifp->enable_crc32_check && len > 4)
    {
      len -= 4; // TODO(scaprile): check on bigendian
      uint32_t crc = mg_crc32 (0, (const char *) buf, len);
      if (memcmp ((void *) ((size_t) buf + len), &crc, sizeof (crc)))
	return;
    }
  if (pkt.eth->type == mg_htons (0x806))
    {
      pkt.arp = (struct arp *) (pkt.eth + 1);
      if (sizeof (*pkt.eth) + sizeof (*pkt.arp) > pkt.raw.len)
	return; // Truncated
      rx_arp (ifp, &pkt);
    }
  else if (pkt.eth->type == mg_htons (0x86dd))
    {
      pkt.ip6 = (struct ip6 *) (pkt.eth + 1);
      if (pkt.raw.len < sizeof (*pkt.eth) + sizeof (*pkt.ip6))
	return; // Truncated
      if ((pkt.ip6->ver >> 4) != 0x6)
	return; // Not IP
      mkpay (&pkt, pkt.ip6 + 1);
      rx_ip6 (ifp, &pkt);
    }
  else if (pkt.eth->type == mg_htons (0x800))
    {
      pkt.ip = (struct ip *) (pkt.eth + 1);
      if (pkt.raw.len < sizeof (*pkt.eth) + sizeof (*pkt.ip))
	return; // Truncated
      // Truncate frame to what IP header tells us
      if ((size_t) mg_ntohs (pkt.ip->len) + sizeof (struct eth) < pkt.raw.len)
	{
	  pkt.raw.len = (size_t) mg_ntohs (pkt.ip->len) + sizeof (struct eth);
	}
      if (pkt.raw.len < sizeof (*pkt.eth) + sizeof (*pkt.ip))
	return; // Truncated
      if ((pkt.ip->ver >> 4) != 4)
	return; // Not IP
      mkpay (&pkt, pkt.ip + 1);
      rx_ip (ifp, &pkt);
    }
  else
    {
      MG_DEBUG (("Unknown eth type %x", mg_htons (pkt.eth->type)));
      if (mg_log_level >= MG_LL_VERBOSE)
	mg_hexdump (buf, len >= 32 ? 32 : len);
    }
}

static void
mg_tcpip_poll (struct mg_tcpip_if *ifp, uint64_t now)
{
  struct mg_connection *c;
  bool expired_1000ms = mg_timer_expired (&ifp->timer_1000ms, 1000, now);
  ifp->now = now;

#  if MG_ENABLE_TCPIP_PRINT_DEBUG_STATS
  if (expired_1000ms)
    {
      const char *names[] = { "down", "up", "req", "ready" };
      MG_INFO (("Status: %s, IP: %M, rx:%u, tx:%u, dr:%u, er:%u",
		names[ifp->state], mg_print_ip4, &ifp->ip, ifp->nrecv,
		ifp->nsent, ifp->ndrop, ifp->nerr));
    }
#  endif
  // Handle physical interface up/down status
  if (expired_1000ms && ifp->driver->up)
    {
      bool up = ifp->driver->up (ifp);
      bool current = ifp->state != MG_TCPIP_STATE_DOWN;
      if (up != current)
	{
	  ifp->state = up == false		 ? MG_TCPIP_STATE_DOWN
		       : ifp->enable_dhcp_client ? MG_TCPIP_STATE_UP
						 : MG_TCPIP_STATE_READY;
	  if (!up && ifp->enable_dhcp_client)
	    ifp->ip = 0;
	  onstatechange (ifp);
	}
      if (ifp->state == MG_TCPIP_STATE_DOWN)
	MG_ERROR (("Network is down"));
    }
  if (ifp->state == MG_TCPIP_STATE_DOWN)
    return;

  // DHCP RFC-2131 (4.4)
  if (ifp->state == MG_TCPIP_STATE_UP && expired_1000ms)
    {
      tx_dhcp_discover (ifp); // INIT (4.4.1)
    }
  else if (expired_1000ms && ifp->state == MG_TCPIP_STATE_READY
	   && ifp->lease_expire > 0)
    { // BOUND / RENEWING / REBINDING
      if (ifp->now >= ifp->lease_expire)
	{
	  ifp->state = MG_TCPIP_STATE_UP, ifp->ip = 0; // expired, release IP
	  onstatechange (ifp);
	}
      else if (ifp->now + 30UL * 60UL * 1000UL > ifp->lease_expire
	       && ((ifp->now / 1000) % 60) == 0)
	{
	  // hack: 30 min before deadline, try to rebind (4.3.6) every min
	  tx_dhcp_request_re (ifp, (uint8_t *) broadcast, ifp->ip, 0xffffffff);
	} // TODO(): Handle T1 (RENEWING) and T2 (REBINDING) (4.4.5)
    }

  // Read data from the network
  if (ifp->driver->rx != NULL)
    { // Polling driver. We must call it
      size_t len
	  = ifp->driver->rx (ifp->recv_queue.buf, ifp->recv_queue.size, ifp);
      if (len > 0)
	{
	  ifp->nrecv++;
	  mg_tcpip_rx (ifp, ifp->recv_queue.buf, len);
	}
    }
  else
    { // Interrupt-based driver. Fills recv queue itself
      char *buf;
      size_t len = mg_queue_next (&ifp->recv_queue, &buf);
      if (len > 0)
	{
	  mg_tcpip_rx (ifp, buf, len);
	  mg_queue_del (&ifp->recv_queue, len);
	}
    }

  // Process timeouts
  for (c = ifp->mgr->conns; c != NULL; c = c->next)
    {
      if (c->is_udp || c->is_listening || c->is_resolving)
	continue;
      struct connstate *s = (struct connstate *) (c + 1);
      uint32_t rem_ip;
      memcpy (&rem_ip, c->rem.ip, sizeof (uint32_t));
      if (now > s->timer)
	{
	  if (s->ttype == MIP_TTYPE_ACK && s->acked != s->ack)
	    {
	      MG_VERBOSE (("%lu ack %x %x", c->id, s->seq, s->ack));
	      tx_tcp (ifp, s->mac, rem_ip, TH_ACK, c->loc.port, c->rem.port,
		      mg_htonl (s->seq), mg_htonl (s->ack), NULL, 0);
	      s->acked = s->ack;
	    }
	  else if (s->ttype == MIP_TTYPE_ARP)
	    {
	      mg_error (c, "ARP timeout");
	    }
	  else if (s->ttype == MIP_TTYPE_SYN)
	    {
	      mg_error (c, "Connection timeout");
	    }
	  else if (s->ttype == MIP_TTYPE_FIN)
	    {
	      c->is_closing = 1;
	      continue;
	    }
	  else
	    {
	      if (s->tmiss++ > 2)
		{
		  mg_error (c, "keepalive");
		}
	      else
		{
		  MG_VERBOSE (("%lu keepalive", c->id));
		  tx_tcp (ifp, s->mac, rem_ip, TH_ACK, c->loc.port,
			  c->rem.port, mg_htonl (s->seq - 1),
			  mg_htonl (s->ack), NULL, 0);
		}
	    }

	  settmout (c, MIP_TTYPE_KEEPALIVE);
	}
    }
}

// This function executes in interrupt context, thus it should copy data
// somewhere fast. Note that newlib's malloc is not thread safe, thus use
// our lock-free queue with preallocated buffer to copy data and return asap
void
mg_tcpip_qwrite (void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  char *p;
  if (mg_queue_book (&ifp->recv_queue, &p, len) >= len)
    {
      memcpy (p, buf, len);
      mg_queue_add (&ifp->recv_queue, len);
      ifp->nrecv++;
    }
  else
    {
      ifp->ndrop++;
    }
}

void
mg_tcpip_init (struct mg_mgr *mgr, struct mg_tcpip_if *ifp)
{
  // If MAC address is not set, make a random one
  if (ifp->mac[0] == 0 && ifp->mac[1] == 0 && ifp->mac[2] == 0
      && ifp->mac[3] == 0 && ifp->mac[4] == 0 && ifp->mac[5] == 0)
    {
      ifp->mac[0] = 0x02; // Locally administered, unicast
      mg_random (&ifp->mac[1], sizeof (ifp->mac) - 1);
      MG_INFO (("MAC not set. Generated random: %M", mg_print_mac, ifp->mac));
    }

  if (ifp->driver->init && !ifp->driver->init (ifp))
    {
      MG_ERROR (("driver init failed"));
    }
  else
    {
      size_t framesize = 1540;
      ifp->tx.buf = (char *) calloc (1, framesize), ifp->tx.len = framesize;
      if (ifp->recv_queue.size == 0)
	ifp->recv_queue.size = ifp->driver->rx ? framesize : 8192;
      ifp->recv_queue.buf = (char *) calloc (1, ifp->recv_queue.size);
      ifp->timer_1000ms = mg_millis ();
      mgr->priv = ifp;
      ifp->mgr = mgr;
      ifp->mtu = MG_TCPIP_MTU_DEFAULT;
      mgr->extraconnsize = sizeof (struct connstate);
      if (ifp->ip == 0)
	ifp->enable_dhcp_client = true;
      memset (ifp->gwmac, 255, sizeof (ifp->gwmac)); // Set to broadcast
      mg_random (&ifp->eport, sizeof (ifp->eport));  // Random from 0 to 65535
      ifp->eport |= MG_EPHEMERAL_PORT_BASE;	     // Random from
					    // MG_EPHEMERAL_PORT_BASE to 65535
      if (ifp->tx.buf == NULL || ifp->recv_queue.buf == NULL)
	MG_ERROR (("OOM"));
    }
}

void
mg_tcpip_free (struct mg_tcpip_if *ifp)
{
  free (ifp->recv_queue.buf);
  free (ifp->tx.buf);
}

static void
send_syn (struct mg_connection *c)
{
  struct connstate *s = (struct connstate *) (c + 1);
  uint32_t isn = mg_htonl ((uint32_t) mg_ntohs (c->loc.port));
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) c->mgr->priv;
  uint32_t rem_ip;
  memcpy (&rem_ip, c->rem.ip, sizeof (uint32_t));
  tx_tcp (ifp, s->mac, rem_ip, TH_SYN, c->loc.port, c->rem.port, isn, 0, NULL,
	  0);
}

void
mg_connect_resolved (struct mg_connection *c)
{
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) c->mgr->priv;
  uint32_t rem_ip;
  memcpy (&rem_ip, c->rem.ip, sizeof (uint32_t));
  c->is_resolving = 0;
  if (ifp->eport < MG_EPHEMERAL_PORT_BASE)
    ifp->eport = MG_EPHEMERAL_PORT_BASE;
  memcpy (c->loc.ip, &ifp->ip, sizeof (uint32_t));
  c->loc.port = mg_htons (ifp->eport++);
  MG_DEBUG (("%lu %M -> %M", c->id, mg_print_ip_port, &c->loc,
	     mg_print_ip_port, &c->rem));
  mg_call (c, MG_EV_RESOLVE, NULL);
  if (c->is_udp && (rem_ip == 0xffffffff || rem_ip == (ifp->ip | ~ifp->mask)))
    {
      struct connstate *s = (struct connstate *) (c + 1);
      memset (s->mac, 0xFF, sizeof (s->mac)); // global or local broadcast
    }
  else if (ifp->ip && ((rem_ip & ifp->mask) == (ifp->ip & ifp->mask)))
    {
      // If we're in the same LAN, fire an ARP lookup.
      MG_DEBUG (("%lu ARP lookup...", c->id));
      arp_ask (ifp, rem_ip);
      settmout (c, MIP_TTYPE_ARP);
      c->is_arplooking = 1;
      c->is_connecting = 1;
    }
  else if ((*((uint8_t *) &rem_ip) & 0xE0) == 0xE0)
    {
      struct connstate *s
	  = (struct connstate *) (c + 1);	// 224 to 239, E0 to EF
      uint8_t mcastp[3] = { 0x01, 0x00, 0x5E }; // multicast group
      memcpy (s->mac, mcastp, 3);
      memcpy (s->mac + 3, ((uint8_t *) &rem_ip) + 1, 3); // 23 LSb
      s->mac[3] &= 0x7F;
    }
  else
    {
      struct connstate *s = (struct connstate *) (c + 1);
      memcpy (s->mac, ifp->gwmac, sizeof (ifp->gwmac));
      if (c->is_udp)
	{
	  mg_call (c, MG_EV_CONNECT, NULL);
	}
      else
	{
	  send_syn (c);
	  settmout (c, MIP_TTYPE_SYN);
	  c->is_connecting = 1;
	}
    }
}

bool
mg_open_listener (struct mg_connection *c, const char *url)
{
  c->loc.port = mg_htons (mg_url_port (url));
  return true;
}

static void
write_conn (struct mg_connection *c)
{
  long len = c->is_tls ? mg_tls_send (c, c->send.buf, c->send.len)
		       : mg_io_send (c, c->send.buf, c->send.len);
  if (len == MG_IO_ERR)
    {
      mg_error (c, "tx err");
    }
  else if (len > 0)
    {
      mg_iobuf_del (&c->send, 0, (size_t) len);
      mg_call (c, MG_EV_WRITE, &len);
    }
}

static void
init_closure (struct mg_connection *c)
{
  struct connstate *s = (struct connstate *) (c + 1);
  if (c->is_udp == false && c->is_listening == false
      && c->is_connecting == false)
    { // For TCP conns,
      struct mg_tcpip_if *ifp
	  = (struct mg_tcpip_if *) c->mgr->priv; // send TCP FIN
      uint32_t rem_ip;
      memcpy (&rem_ip, c->rem.ip, sizeof (uint32_t));
      tx_tcp (ifp, s->mac, rem_ip, TH_FIN | TH_ACK, c->loc.port, c->rem.port,
	      mg_htonl (s->seq), mg_htonl (s->ack), NULL, 0);
      settmout (c, MIP_TTYPE_FIN);
    }
}

static void
close_conn (struct mg_connection *c)
{
  struct connstate *s = (struct connstate *) (c + 1);
  mg_iobuf_free (&s->raw); // For TLS connections, release raw data
  mg_close_conn (c);
}

static bool
can_write (struct mg_connection *c)
{
  return c->is_connecting == 0 && c->is_resolving == 0 && c->send.len > 0
	 && c->is_tls_hs == 0 && c->is_arplooking == 0;
}

void
mg_mgr_poll (struct mg_mgr *mgr, int ms)
{
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) mgr->priv;
  struct mg_connection *c, *tmp;
  uint64_t now = mg_millis ();
  mg_timer_poll (&mgr->timers, now);
  if (ifp == NULL || ifp->driver == NULL)
    return;
  mg_tcpip_poll (ifp, now);
  for (c = mgr->conns; c != NULL; c = tmp)
    {
      tmp = c->next;
      struct connstate *s = (struct connstate *) (c + 1);
      mg_call (c, MG_EV_POLL, &now);
      MG_VERBOSE (("%lu .. %c%c%c%c%c", c->id, c->is_tls ? 'T' : 't',
		   c->is_connecting ? 'C' : 'c', c->is_tls_hs ? 'H' : 'h',
		   c->is_resolving ? 'R' : 'r', c->is_closing ? 'C' : 'c'));
      if (c->is_tls && mg_tls_pending (c) > 0)
	handle_tls_recv (c, (struct mg_iobuf *) &c->rtls);
      if (can_write (c))
	write_conn (c);
      if (c->is_draining && c->send.len == 0 && s->ttype != MIP_TTYPE_FIN)
	init_closure (c);
      if (c->is_closing)
	close_conn (c);
    }
  (void) ms;
}

bool
mg_send (struct mg_connection *c, const void *buf, size_t len)
{
  struct mg_tcpip_if *ifp = (struct mg_tcpip_if *) c->mgr->priv;
  bool res = false;
  uint32_t rem_ip;
  memcpy (&rem_ip, c->rem.ip, sizeof (uint32_t));
  if (ifp->ip == 0 || ifp->state != MG_TCPIP_STATE_READY)
    {
      mg_error (c, "net down");
    }
  else if (c->is_udp)
    {
      struct connstate *s = (struct connstate *) (c + 1);
      len = trim_len (c, len); // Trimming length if necessary
      tx_udp (ifp, s->mac, ifp->ip, c->loc.port, rem_ip, c->rem.port, buf,
	      len);
      res = true;
    }
  else
    {
      res = mg_iobuf_add (&c->send, c->send.len, buf, len);
    }
  return res;
}
#endif // MG_ENABLE_TCPIP

#ifdef MG_ENABLE_LINES
#  line 1 "src/ota_dummy.c"
#endif

#if MG_OTA == MG_OTA_NONE
bool
mg_ota_begin (size_t new_firmware_size)
{
  (void) new_firmware_size;
  return true;
}
bool
mg_ota_write (const void *buf, size_t len)
{
  (void) buf, (void) len;
  return true;
}
bool
mg_ota_end (void)
{
  return true;
}
bool
mg_ota_commit (void)
{
  return true;
}
bool
mg_ota_rollback (void)
{
  return true;
}
int
mg_ota_status (int fw)
{
  (void) fw;
  return 0;
}
uint32_t
mg_ota_crc32 (int fw)
{
  (void) fw;
  return 0;
}
uint32_t
mg_ota_timestamp (int fw)
{
  (void) fw;
  return 0;
}
size_t
mg_ota_size (int fw)
{
  (void) fw;
  return 0;
}
MG_IRAM void
mg_ota_boot (void)
{
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/ota_esp32.c"
#endif

#if MG_ARCH == MG_ARCH_ESP32 && MG_OTA == MG_OTA_ESP32

static const esp_partition_t *s_ota_update_partition;
static esp_ota_handle_t s_ota_update_handle;
static bool s_ota_success;

// Those empty macros do nothing, but mark places in the code which could
// potentially trigger a watchdog reboot due to the log flash erase operation
#  define disable_wdt()
#  define enable_wdt()

bool
mg_ota_begin (size_t new_firmware_size)
{
  if (s_ota_update_partition != NULL)
    {
      MG_ERROR (("Update in progress. Call mg_ota_end() ?"));
      return false;
    }
  else
    {
      s_ota_success = false;
      disable_wdt ();
      s_ota_update_partition = esp_ota_get_next_update_partition (NULL);
      esp_err_t err = esp_ota_begin (s_ota_update_partition, new_firmware_size,
				     &s_ota_update_handle);
      enable_wdt ();
      MG_DEBUG (("esp_ota_begin(): %d", err));
      s_ota_success = (err == ESP_OK);
    }
  return s_ota_success;
}

bool
mg_ota_write (const void *buf, size_t len)
{
  disable_wdt ();
  esp_err_t err = esp_ota_write (s_ota_update_handle, buf, len);
  enable_wdt ();
  MG_INFO (("esp_ota_write(): %d", err));
  s_ota_success = err == ESP_OK;
  return s_ota_success;
}

bool
mg_ota_end (void)
{
  esp_err_t err = esp_ota_end (s_ota_update_handle);
  MG_DEBUG (("esp_ota_end(%p): %d", s_ota_update_handle, err));
  if (s_ota_success && err == ESP_OK)
    {
      err = esp_ota_set_boot_partition (s_ota_update_partition);
      s_ota_success = (err == ESP_OK);
    }
  MG_DEBUG (("Finished ESP32 OTA, success: %d", s_ota_success));
  s_ota_update_partition = NULL;
  return s_ota_success;
}

#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/ota_flash.c"
#endif

// This OTA implementation uses the internal flash API outlined in device.h
// It splits flash into 2 equal partitions, and stores OTA status in the
// last sector of the partition.

#if MG_OTA == MG_OTA_FLASH

#  define MG_OTADATA_KEY 0xb07afed0

static char *s_addr;	 // Current address to write to
static size_t s_size;	 // Firmware size to flash. In-progress indicator
static uint32_t s_crc32; // Firmware checksum

struct mg_otadata
{
  uint32_t crc32, size, timestamp, status;
};

bool
mg_ota_begin (size_t new_firmware_size)
{
  bool ok = false;
  if (s_size)
    {
      MG_ERROR (("OTA already in progress. Call mg_ota_end()"));
    }
  else
    {
      size_t half = mg_flash_size () / 2, max = half - mg_flash_sector_size ();
      s_crc32 = 0;
      s_addr = (char *) mg_flash_start () + half;
      MG_DEBUG (("Firmware %lu bytes, max %lu", new_firmware_size, max));
      if (new_firmware_size < max)
	{
	  ok = true;
	  s_size = new_firmware_size;
	  MG_INFO (("Starting OTA, firmware size %lu", s_size));
	}
      else
	{
	  MG_ERROR (
	      ("Firmware %lu is too big to fit %lu", new_firmware_size, max));
	}
    }
  return ok;
}

bool
mg_ota_write (const void *buf, size_t len)
{
  bool ok = false;
  if (s_size == 0)
    {
      MG_ERROR (("OTA is not started, call mg_ota_begin()"));
    }
  else
    {
      size_t align = mg_flash_write_align ();
      size_t len_aligned_down = MG_ROUND_DOWN (len, align);
      if (len_aligned_down)
	ok = mg_flash_write (s_addr, buf, len_aligned_down);
      if (len_aligned_down < len)
	{
	  size_t left = len - len_aligned_down;
	  char tmp[align];
	  memset (tmp, 0xff, sizeof (tmp));
	  memcpy (tmp, (char *) buf + len_aligned_down, left);
	  ok = mg_flash_write (s_addr + len_aligned_down, tmp, sizeof (tmp));
	}
      s_crc32 = mg_crc32 (s_crc32, (char *) buf, len); // Update CRC
      MG_DEBUG (("%#x %p %lu -> %d", s_addr - len, buf, len, ok));
      s_addr += len;
    }
  return ok;
}

MG_IRAM static uint32_t
mg_fwkey (int fw)
{
  uint32_t key = MG_OTADATA_KEY + fw;
  int bank = mg_flash_bank ();
  if (bank == 2 && fw == MG_FIRMWARE_PREVIOUS)
    key--;
  if (bank == 2 && fw == MG_FIRMWARE_CURRENT)
    key++;
  return key;
}

bool
mg_ota_end (void)
{
  char *base = (char *) mg_flash_start () + mg_flash_size () / 2;
  bool ok = false;
  if (s_size)
    {
      size_t size = s_addr - base;
      uint32_t crc32 = mg_crc32 (0, base, s_size);
      if (size == s_size && crc32 == s_crc32)
	{
	  uint32_t now = (uint32_t) (mg_now () / 1000);
	  struct mg_otadata od = { crc32, size, now, MG_OTA_FIRST_BOOT };
	  uint32_t key = mg_fwkey (MG_FIRMWARE_PREVIOUS);
	  ok = mg_flash_save (NULL, key, &od, sizeof (od));
	}
      MG_DEBUG (("CRC: %x/%x, size: %lu/%lu, status: %s", s_crc32, crc32,
		 s_size, size, ok ? "ok" : "fail"));
      s_size = 0;
      if (ok)
	ok = mg_flash_swap_bank ();
    }
  MG_INFO (("Finishing OTA: %s", ok ? "ok" : "fail"));
  return ok;
}

MG_IRAM static struct mg_otadata
mg_otadata (int fw)
{
  uint32_t key = mg_fwkey (fw);
  struct mg_otadata od = {};
  MG_INFO (
      ("Loading %s OTA data", fw == MG_FIRMWARE_CURRENT ? "curr" : "prev"));
  mg_flash_load (NULL, key, &od, sizeof (od));
  // MG_DEBUG(("Loaded OTA data. fw %d, bank %d, key %p", fw, bank, key));
  // mg_hexdump(&od, sizeof(od));
  return od;
}

int
mg_ota_status (int fw)
{
  struct mg_otadata od = mg_otadata (fw);
  return od.status;
}
uint32_t
mg_ota_crc32 (int fw)
{
  struct mg_otadata od = mg_otadata (fw);
  return od.crc32;
}
uint32_t
mg_ota_timestamp (int fw)
{
  struct mg_otadata od = mg_otadata (fw);
  return od.timestamp;
}
size_t
mg_ota_size (int fw)
{
  struct mg_otadata od = mg_otadata (fw);
  return od.size;
}

MG_IRAM bool
mg_ota_commit (void)
{
  bool ok = true;
  struct mg_otadata od = mg_otadata (MG_FIRMWARE_CURRENT);
  if (od.status != MG_OTA_COMMITTED)
    {
      od.status = MG_OTA_COMMITTED;
      MG_INFO (("Committing current firmware, OD size %lu", sizeof (od)));
      ok = mg_flash_save (NULL, mg_fwkey (MG_FIRMWARE_CURRENT), &od,
			  sizeof (od));
    }
  return ok;
}

bool
mg_ota_rollback (void)
{
  MG_DEBUG (("Rolling firmware back"));
  if (mg_flash_bank () == 0)
    {
      // No dual bank support. Mark previous firmware as FIRST_BOOT
      struct mg_otadata prev = mg_otadata (MG_FIRMWARE_PREVIOUS);
      prev.status = MG_OTA_FIRST_BOOT;
      return mg_flash_save (NULL, MG_OTADATA_KEY + MG_FIRMWARE_PREVIOUS, &prev,
			    sizeof (prev));
    }
  else
    {
      return mg_flash_swap_bank ();
    }
}

MG_IRAM void
mg_ota_boot (void)
{
  MG_INFO (("Booting. Flash bank: %d", mg_flash_bank ()));
  struct mg_otadata curr = mg_otadata (MG_FIRMWARE_CURRENT);
  struct mg_otadata prev = mg_otadata (MG_FIRMWARE_PREVIOUS);

  if (curr.status == MG_OTA_FIRST_BOOT)
    {
      if (prev.status == MG_OTA_UNAVAILABLE)
	{
	  MG_INFO (("Setting previous firmware state to committed"));
	  prev.status = MG_OTA_COMMITTED;
	  mg_flash_save (NULL, mg_fwkey (MG_FIRMWARE_PREVIOUS), &prev,
			 sizeof (prev));
	}
      curr.status = MG_OTA_UNCOMMITTED;
      MG_INFO (("First boot, setting status to UNCOMMITTED"));
      mg_flash_save (NULL, mg_fwkey (MG_FIRMWARE_CURRENT), &curr,
		     sizeof (curr));
    }
  else if (prev.status == MG_OTA_FIRST_BOOT && mg_flash_bank () == 0)
    {
      // Swap paritions. Pray power does not disappear
      size_t fs = mg_flash_size (), ss = mg_flash_sector_size ();
      char *partition1 = mg_flash_start ();
      char *partition2 = mg_flash_start () + fs / 2;
      size_t ofs, max = fs / 2 - ss; // Set swap size to the whole partition

      if (curr.status != MG_OTA_UNAVAILABLE
	  && prev.status != MG_OTA_UNAVAILABLE)
	{
	  // We know exact sizes of both firmwares.
	  // Shrink swap size to the MAX(firmware1, firmware2)
	  size_t sz = curr.size > prev.size ? curr.size : prev.size;
	  if (sz > 0 && sz < max)
	    max = sz;
	}

      // MG_OTA_FIRST_BOOT -> MG_OTA_UNCOMMITTED
      prev.status = MG_OTA_UNCOMMITTED;
      mg_flash_save (NULL, MG_OTADATA_KEY + MG_FIRMWARE_CURRENT, &prev,
		     sizeof (prev));
      mg_flash_save (NULL, MG_OTADATA_KEY + MG_FIRMWARE_PREVIOUS, &curr,
		     sizeof (curr));

      MG_INFO (("Swapping partitions, size %u (%u sectors)", max, max / ss));
      MG_INFO (("Do NOT power off..."));
      mg_log_level = MG_LL_NONE;

      // We use the last sector of partition2 for OTA data/config storage
      // Therefore we can use last sector of partition1 for swapping
      char *tmpsector = partition1 + fs / 2 - ss; // Last sector of partition1
      (void) tmpsector;
      for (ofs = 0; ofs < max; ofs += ss)
	{
	  // mg_flash_erase(tmpsector);
	  mg_flash_write (tmpsector, partition1 + ofs, ss);
	  // mg_flash_erase(partition1 + ofs);
	  mg_flash_write (partition1 + ofs, partition2 + ofs, ss);
	  // mg_flash_erase(partition2 + ofs);
	  mg_flash_write (partition2 + ofs, tmpsector, ss);
	}
      mg_device_reset ();
    }
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/printf.c"
#endif

size_t
mg_queue_vprintf (struct mg_queue *q, const char *fmt, va_list *ap)
{
  size_t len = mg_snprintf (NULL, 0, fmt, ap);
  char *buf;
  if (len == 0 || mg_queue_book (q, &buf, len + 1) < len + 1)
    {
      len = 0; // Nah. Not enough space
    }
  else
    {
      len = mg_vsnprintf ((char *) buf, len + 1, fmt, ap);
      mg_queue_add (q, len);
    }
  return len;
}

size_t
mg_queue_printf (struct mg_queue *q, const char *fmt, ...)
{
  va_list ap;
  size_t len;
  va_start (ap, fmt);
  len = mg_queue_vprintf (q, fmt, &ap);
  va_end (ap);
  return len;
}

static void
mg_pfn_iobuf_private (char ch, void *param, bool expand)
{
  struct mg_iobuf *io = (struct mg_iobuf *) param;
  if (expand && io->len + 2 > io->size)
    mg_iobuf_resize (io, io->len + 2);
  if (io->len + 2 <= io->size)
    {
      io->buf[io->len++] = (uint8_t) ch;
      io->buf[io->len] = 0;
    }
  else if (io->len < io->size)
    {
      io->buf[io->len++] = 0; // Guarantee to 0-terminate
    }
}

static void
mg_putchar_iobuf_static (char ch, void *param)
{
  mg_pfn_iobuf_private (ch, param, false);
}

void
mg_pfn_iobuf (char ch, void *param)
{
  mg_pfn_iobuf_private (ch, param, true);
}

size_t
mg_vsnprintf (char *buf, size_t len, const char *fmt, va_list *ap)
{
  struct mg_iobuf io = { (uint8_t *) buf, len, 0, 0 };
  size_t n = mg_vxprintf (mg_putchar_iobuf_static, &io, fmt, ap);
  if (n < len)
    buf[n] = '\0';
  return n;
}

size_t
mg_snprintf (char *buf, size_t len, const char *fmt, ...)
{
  va_list ap;
  size_t n;
  va_start (ap, fmt);
  n = mg_vsnprintf (buf, len, fmt, &ap);
  va_end (ap);
  return n;
}

char *
mg_vmprintf (const char *fmt, va_list *ap)
{
  struct mg_iobuf io = { 0, 0, 0, 256 };
  mg_vxprintf (mg_pfn_iobuf, &io, fmt, ap);
  return (char *) io.buf;
}

char *
mg_mprintf (const char *fmt, ...)
{
  char *s;
  va_list ap;
  va_start (ap, fmt);
  s = mg_vmprintf (fmt, &ap);
  va_end (ap);
  return s;
}

void
mg_pfn_stdout (char c, void *param)
{
  putchar (c);
  (void) param;
}

static size_t
print_ip4 (void (*out) (char, void *), void *arg, uint8_t *p)
{
  return mg_xprintf (out, arg, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
}

static size_t
print_ip6 (void (*out) (char, void *), void *arg, uint16_t *p)
{
  return mg_xprintf (out, arg, "[%x:%x:%x:%x:%x:%x:%x:%x]", mg_ntohs (p[0]),
		     mg_ntohs (p[1]), mg_ntohs (p[2]), mg_ntohs (p[3]),
		     mg_ntohs (p[4]), mg_ntohs (p[5]), mg_ntohs (p[6]),
		     mg_ntohs (p[7]));
}

size_t
mg_print_ip4 (void (*out) (char, void *), void *arg, va_list *ap)
{
  uint8_t *p = va_arg (*ap, uint8_t *);
  return print_ip4 (out, arg, p);
}

size_t
mg_print_ip6 (void (*out) (char, void *), void *arg, va_list *ap)
{
  uint16_t *p = va_arg (*ap, uint16_t *);
  return print_ip6 (out, arg, p);
}

size_t
mg_print_ip (void (*out) (char, void *), void *arg, va_list *ap)
{
  struct mg_addr *addr = va_arg (*ap, struct mg_addr *);
  if (addr->is_ip6)
    return print_ip6 (out, arg, (uint16_t *) addr->ip);
  return print_ip4 (out, arg, (uint8_t *) &addr->ip);
}

size_t
mg_print_ip_port (void (*out) (char, void *), void *arg, va_list *ap)
{
  struct mg_addr *a = va_arg (*ap, struct mg_addr *);
  return mg_xprintf (out, arg, "%M:%hu", mg_print_ip, a, mg_ntohs (a->port));
}

size_t
mg_print_mac (void (*out) (char, void *), void *arg, va_list *ap)
{
  uint8_t *p = va_arg (*ap, uint8_t *);
  return mg_xprintf (out, arg, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1],
		     p[2], p[3], p[4], p[5]);
}

static char
mg_esc (int c, bool esc)
{
  const char *p, *esc1 = "\b\f\n\r\t\\\"", *esc2 = "bfnrt\\\"";
  for (p = esc ? esc1 : esc2; *p != '\0'; p++)
    {
      if (*p == c)
	return esc ? esc2[p - esc1] : esc1[p - esc2];
    }
  return 0;
}

static char
mg_escape (int c)
{
  return mg_esc (c, true);
}

static size_t
qcpy (void (*out) (char, void *), void *ptr, char *buf, size_t len)
{
  size_t i = 0, extra = 0;
  for (i = 0; i < len && buf[i] != '\0'; i++)
    {
      char c = mg_escape (buf[i]);
      if (c)
	{
	  out ('\\', ptr), out (c, ptr), extra++;
	}
      else
	{
	  out (buf[i], ptr);
	}
    }
  return i + extra;
}

static size_t
bcpy (void (*out) (char, void *), void *arg, uint8_t *buf, size_t len)
{
  size_t i, j, n = 0;
  const char *t
      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (i = 0; i < len; i += 3)
    {
      uint8_t c1 = buf[i], c2 = i + 1 < len ? buf[i + 1] : 0,
	      c3 = i + 2 < len ? buf[i + 2] : 0;
      char tmp[4] = { t[c1 >> 2], t[(c1 & 3) << 4 | (c2 >> 4)], '=', '=' };
      if (i + 1 < len)
	tmp[2] = t[(c2 & 15) << 2 | (c3 >> 6)];
      if (i + 2 < len)
	tmp[3] = t[c3 & 63];
      for (j = 0; j < sizeof (tmp) && tmp[j] != '\0'; j++)
	out (tmp[j], arg);
      n += j;
    }
  return n;
}

size_t
mg_print_hex (void (*out) (char, void *), void *arg, va_list *ap)
{
  size_t bl = (size_t) va_arg (*ap, int);
  uint8_t *p = va_arg (*ap, uint8_t *);
  const char *hex = "0123456789abcdef";
  size_t j;
  for (j = 0; j < bl; j++)
    {
      out (hex[(p[j] >> 4) & 0x0F], arg);
      out (hex[p[j] & 0x0F], arg);
    }
  return 2 * bl;
}
size_t
mg_print_base64 (void (*out) (char, void *), void *arg, va_list *ap)
{
  size_t len = (size_t) va_arg (*ap, int);
  uint8_t *buf = va_arg (*ap, uint8_t *);
  return bcpy (out, arg, buf, len);
}

size_t
mg_print_esc (void (*out) (char, void *), void *arg, va_list *ap)
{
  size_t len = (size_t) va_arg (*ap, int);
  char *p = va_arg (*ap, char *);
  if (len == 0)
    len = p == NULL ? 0 : strlen (p);
  return qcpy (out, arg, p, len);
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/queue.c"
#endif

#if (defined(__GNUC__) && (__GNUC__ > 4)                                      \
     || (defined(__GNUC_MINOR__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 1))    \
    || defined(__clang__)
#  define MG_MEMORY_BARRIER() __sync_synchronize ()
#elif defined(_MSC_VER) && _MSC_VER >= 1700
#  define MG_MEMORY_BARRIER() MemoryBarrier ()
#elif !defined(MG_MEMORY_BARRIER)
#  define MG_MEMORY_BARRIER()
#endif

// Every message in a queue is prepended by a 32-bit message length (ML).
// If ML is 0, then it is the end, and reader must wrap to the beginning.
//
//  Queue when q->tail <= q->head:
//  |----- free -----| ML | message1 | ML | message2 |  ----- free ------|
//  ^                ^                               ^                   ^
// buf              tail                            head                len
//
//  Queue when q->tail > q->head:
//  | ML | message2 |----- free ------| ML | message1 | 0 |---- free ----|
//  ^               ^                 ^                                  ^
// buf             head              tail                               len

void
mg_queue_init (struct mg_queue *q, char *buf, size_t size)
{
  q->size = size;
  q->buf = buf;
  q->head = q->tail = 0;
}

static size_t
mg_queue_read_len (struct mg_queue *q)
{
  uint32_t n = 0;
  MG_MEMORY_BARRIER ();
  memcpy (&n, q->buf + q->tail, sizeof (n));
  assert (q->tail + n + sizeof (n) <= q->size);
  return n;
}

static void
mg_queue_write_len (struct mg_queue *q, size_t len)
{
  uint32_t n = (uint32_t) len;
  memcpy (q->buf + q->head, &n, sizeof (n));
  MG_MEMORY_BARRIER ();
}

size_t
mg_queue_book (struct mg_queue *q, char **buf, size_t len)
{
  size_t space = 0, hs = sizeof (uint32_t) * 2; // *2 is for the 0 marker
  if (q->head >= q->tail && q->head + len + hs <= q->size)
    {
      space = q->size - q->head - hs; // There is enough space
    }
  else if (q->head >= q->tail && q->tail > hs)
    {
      mg_queue_write_len (q, 0); // Not enough space ahead
      q->head = 0;		 // Wrap head to the beginning
    }
  if (q->head + hs + len < q->tail)
    space = q->tail - q->head - hs;
  if (buf != NULL)
    *buf = q->buf + q->head + sizeof (uint32_t);
  return space;
}

size_t
mg_queue_next (struct mg_queue *q, char **buf)
{
  size_t len = 0;
  if (q->tail != q->head)
    {
      len = mg_queue_read_len (q);
      if (len == 0)
	{	       // Zero (head wrapped) ?
	  q->tail = 0; // Reset tail to the start
	  if (q->head > q->tail)
	    len = mg_queue_read_len (q); // Read again
	}
    }
  if (buf != NULL)
    *buf = q->buf + q->tail + sizeof (uint32_t);
  assert (q->tail + len <= q->size);
  return len;
}

void
mg_queue_add (struct mg_queue *q, size_t len)
{
  assert (len > 0);
  mg_queue_write_len (q, len);
  assert (q->head + sizeof (uint32_t) * 2 + len <= q->size);
  q->head += len + sizeof (uint32_t);
}

void
mg_queue_del (struct mg_queue *q, size_t len)
{
  q->tail += len + sizeof (uint32_t);
  assert (q->tail + sizeof (uint32_t) <= q->size);
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/rpc.c"
#endif

void
mg_rpc_add (struct mg_rpc **head, struct mg_str method,
	    void (*fn) (struct mg_rpc_req *), void *fn_data)
{
  struct mg_rpc *rpc = (struct mg_rpc *) calloc (1, sizeof (*rpc));
  if (rpc != NULL)
    {
      rpc->method.buf = mg_mprintf ("%.*s", method.len, method.buf);
      rpc->method.len = method.len;
      rpc->fn = fn;
      rpc->fn_data = fn_data;
      rpc->next = *head, *head = rpc;
    }
}

void
mg_rpc_del (struct mg_rpc **head, void (*fn) (struct mg_rpc_req *))
{
  struct mg_rpc *r;
  while ((r = *head) != NULL)
    {
      if (r->fn == fn || fn == NULL)
	{
	  *head = r->next;
	  free ((void *) r->method.buf);
	  free (r);
	}
      else
	{
	  head = &(*head)->next;
	}
    }
}

static void
mg_rpc_call (struct mg_rpc_req *r, struct mg_str method)
{
  struct mg_rpc *h = r->head == NULL ? NULL : *r->head;
  while (h != NULL && !mg_match (method, h->method, NULL))
    h = h->next;
  if (h != NULL)
    {
      r->rpc = h;
      h->fn (r);
    }
  else
    {
      mg_rpc_err (r, -32601, "\"%.*s not found\"", (int) method.len,
		  method.buf);
    }
}

void
mg_rpc_process (struct mg_rpc_req *r)
{
  int len, off = mg_json_get (r->frame, "$.method", &len);
  if (off > 0 && r->frame.buf[off] == '"')
    {
      struct mg_str method
	  = mg_str_n (&r->frame.buf[off + 1], (size_t) len - 2);
      mg_rpc_call (r, method);
    }
  else if ((off = mg_json_get (r->frame, "$.result", &len)) > 0
	   || (off = mg_json_get (r->frame, "$.error", &len)) > 0)
    {
      mg_rpc_call (r, mg_str ("")); // JSON response! call "" method handler
    }
  else
    {
      mg_rpc_err (r, -32700, "%m", mg_print_esc, (int) r->frame.len,
		  r->frame.buf); // Invalid
    }
}

void
mg_rpc_vok (struct mg_rpc_req *r, const char *fmt, va_list *ap)
{
  int len, off = mg_json_get (r->frame, "$.id", &len);
  if (off > 0)
    {
      mg_xprintf (r->pfn, r->pfn_data, "{%m:%.*s,%m:", mg_print_esc, 0, "id",
		  len, &r->frame.buf[off], mg_print_esc, 0, "result");
      mg_vxprintf (r->pfn, r->pfn_data, fmt == NULL ? "null" : fmt, ap);
      mg_xprintf (r->pfn, r->pfn_data, "}");
    }
}

void
mg_rpc_ok (struct mg_rpc_req *r, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  mg_rpc_vok (r, fmt, &ap);
  va_end (ap);
}

void
mg_rpc_verr (struct mg_rpc_req *r, int code, const char *fmt, va_list *ap)
{
  int len, off = mg_json_get (r->frame, "$.id", &len);
  mg_xprintf (r->pfn, r->pfn_data, "{");
  if (off > 0)
    {
      mg_xprintf (r->pfn, r->pfn_data, "%m:%.*s,", mg_print_esc, 0, "id", len,
		  &r->frame.buf[off]);
    }
  mg_xprintf (r->pfn, r->pfn_data, "%m:{%m:%d,%m:", mg_print_esc, 0, "error",
	      mg_print_esc, 0, "code", code, mg_print_esc, 0, "message");
  mg_vxprintf (r->pfn, r->pfn_data, fmt == NULL ? "null" : fmt, ap);
  mg_xprintf (r->pfn, r->pfn_data, "}}");
}

void
mg_rpc_err (struct mg_rpc_req *r, int code, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  mg_rpc_verr (r, code, fmt, &ap);
  va_end (ap);
}

static size_t
print_methods (mg_pfn_t pfn, void *pfn_data, va_list *ap)
{
  struct mg_rpc *h, **head = (struct mg_rpc **) va_arg (*ap, void **);
  size_t len = 0;
  for (h = *head; h != NULL; h = h->next)
    {
      if (h->method.len == 0)
	continue; // Ignore response handler
      len += mg_xprintf (pfn, pfn_data, "%s%m", h == *head ? "" : ",",
			 mg_print_esc, (int) h->method.len, h->method.buf);
    }
  return len;
}

void
mg_rpc_list (struct mg_rpc_req *r)
{
  mg_rpc_ok (r, "[%M]", print_methods, r->head);
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/sha1.c"
#endif
/* Copyright(c) By Steve Reid <steve@edmweb.com> */
/* 100% Public Domain */

union char64long16
{
  unsigned char c[64];
  uint32_t l[16];
};

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static uint32_t
blk0 (union char64long16 *block, int i)
{
  if (MG_BIG_ENDIAN)
    {
    }
  else
    {
      block->l[i] = (rol (block->l[i], 24) & 0xFF00FF00)
		    | (rol (block->l[i], 8) & 0x00FF00FF);
    }
  return block->l[i];
}

/* Avoid redefine warning (ARM /usr/include/sys/ucontext.h define R0~R4) */
#undef blk
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4

#define blk(i)                                                                \
  (block->l[i & 15] = rol (block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15]   \
			       ^ block->l[(i + 2) & 15] ^ block->l[i & 15],   \
			   1))
#define R0(v, w, x, y, z, i)                                                  \
  z += ((w & (x ^ y)) ^ y) + blk0 (block, i) + 0x5A827999 + rol (v, 5);       \
  w = rol (w, 30);
#define R1(v, w, x, y, z, i)                                                  \
  z += ((w & (x ^ y)) ^ y) + blk (i) + 0x5A827999 + rol (v, 5);               \
  w = rol (w, 30);
#define R2(v, w, x, y, z, i)                                                  \
  z += (w ^ x ^ y) + blk (i) + 0x6ED9EBA1 + rol (v, 5);                       \
  w = rol (w, 30);
#define R3(v, w, x, y, z, i)                                                  \
  z += (((w | x) & y) | (w & x)) + blk (i) + 0x8F1BBCDC + rol (v, 5);         \
  w = rol (w, 30);
#define R4(v, w, x, y, z, i)                                                  \
  z += (w ^ x ^ y) + blk (i) + 0xCA62C1D6 + rol (v, 5);                       \
  w = rol (w, 30);

static void
mg_sha1_transform (uint32_t state[5], const unsigned char *buffer)
{
  uint32_t a, b, c, d, e;
  union char64long16 block[1];

  memcpy (block, buffer, 64);
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  R0 (a, b, c, d, e, 0);
  R0 (e, a, b, c, d, 1);
  R0 (d, e, a, b, c, 2);
  R0 (c, d, e, a, b, 3);
  R0 (b, c, d, e, a, 4);
  R0 (a, b, c, d, e, 5);
  R0 (e, a, b, c, d, 6);
  R0 (d, e, a, b, c, 7);
  R0 (c, d, e, a, b, 8);
  R0 (b, c, d, e, a, 9);
  R0 (a, b, c, d, e, 10);
  R0 (e, a, b, c, d, 11);
  R0 (d, e, a, b, c, 12);
  R0 (c, d, e, a, b, 13);
  R0 (b, c, d, e, a, 14);
  R0 (a, b, c, d, e, 15);
  R1 (e, a, b, c, d, 16);
  R1 (d, e, a, b, c, 17);
  R1 (c, d, e, a, b, 18);
  R1 (b, c, d, e, a, 19);
  R2 (a, b, c, d, e, 20);
  R2 (e, a, b, c, d, 21);
  R2 (d, e, a, b, c, 22);
  R2 (c, d, e, a, b, 23);
  R2 (b, c, d, e, a, 24);
  R2 (a, b, c, d, e, 25);
  R2 (e, a, b, c, d, 26);
  R2 (d, e, a, b, c, 27);
  R2 (c, d, e, a, b, 28);
  R2 (b, c, d, e, a, 29);
  R2 (a, b, c, d, e, 30);
  R2 (e, a, b, c, d, 31);
  R2 (d, e, a, b, c, 32);
  R2 (c, d, e, a, b, 33);
  R2 (b, c, d, e, a, 34);
  R2 (a, b, c, d, e, 35);
  R2 (e, a, b, c, d, 36);
  R2 (d, e, a, b, c, 37);
  R2 (c, d, e, a, b, 38);
  R2 (b, c, d, e, a, 39);
  R3 (a, b, c, d, e, 40);
  R3 (e, a, b, c, d, 41);
  R3 (d, e, a, b, c, 42);
  R3 (c, d, e, a, b, 43);
  R3 (b, c, d, e, a, 44);
  R3 (a, b, c, d, e, 45);
  R3 (e, a, b, c, d, 46);
  R3 (d, e, a, b, c, 47);
  R3 (c, d, e, a, b, 48);
  R3 (b, c, d, e, a, 49);
  R3 (a, b, c, d, e, 50);
  R3 (e, a, b, c, d, 51);
  R3 (d, e, a, b, c, 52);
  R3 (c, d, e, a, b, 53);
  R3 (b, c, d, e, a, 54);
  R3 (a, b, c, d, e, 55);
  R3 (e, a, b, c, d, 56);
  R3 (d, e, a, b, c, 57);
  R3 (c, d, e, a, b, 58);
  R3 (b, c, d, e, a, 59);
  R4 (a, b, c, d, e, 60);
  R4 (e, a, b, c, d, 61);
  R4 (d, e, a, b, c, 62);
  R4 (c, d, e, a, b, 63);
  R4 (b, c, d, e, a, 64);
  R4 (a, b, c, d, e, 65);
  R4 (e, a, b, c, d, 66);
  R4 (d, e, a, b, c, 67);
  R4 (c, d, e, a, b, 68);
  R4 (b, c, d, e, a, 69);
  R4 (a, b, c, d, e, 70);
  R4 (e, a, b, c, d, 71);
  R4 (d, e, a, b, c, 72);
  R4 (c, d, e, a, b, 73);
  R4 (b, c, d, e, a, 74);
  R4 (a, b, c, d, e, 75);
  R4 (e, a, b, c, d, 76);
  R4 (d, e, a, b, c, 77);
  R4 (c, d, e, a, b, 78);
  R4 (b, c, d, e, a, 79);
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  /* Erase working structures. The order of operations is important,
   * used to ensure that compiler doesn't optimize those out. */
  memset (block, 0, sizeof (block));
  a = b = c = d = e = 0;
  (void) a;
  (void) b;
  (void) c;
  (void) d;
  (void) e;
}

void
mg_sha1_init (mg_sha1_ctx *context)
{
  context->state[0] = 0x67452301;
  context->state[1] = 0xEFCDAB89;
  context->state[2] = 0x98BADCFE;
  context->state[3] = 0x10325476;
  context->state[4] = 0xC3D2E1F0;
  context->count[0] = context->count[1] = 0;
}

void
mg_sha1_update (mg_sha1_ctx *context, const unsigned char *data, size_t len)
{
  size_t i, j;

  j = context->count[0];
  if ((context->count[0] += (uint32_t) len << 3) < j)
    context->count[1]++;
  context->count[1] += (uint32_t) (len >> 29);
  j = (j >> 3) & 63;
  if ((j + len) > 63)
    {
      memcpy (&context->buffer[j], data, (i = 64 - j));
      mg_sha1_transform (context->state, context->buffer);
      for (; i + 63 < len; i += 64)
	{
	  mg_sha1_transform (context->state, &data[i]);
	}
      j = 0;
    }
  else
    i = 0;
  memcpy (&context->buffer[j], &data[i], len - i);
}

void
mg_sha1_final (unsigned char digest[20], mg_sha1_ctx *context)
{
  unsigned i;
  unsigned char finalcount[8], c;

  for (i = 0; i < 8; i++)
    {
      finalcount[i] = (unsigned char) ((context->count[(i >= 4 ? 0 : 1)]
					>> ((3 - (i & 3)) * 8))
				       & 255);
    }
  c = 0200;
  mg_sha1_update (context, &c, 1);
  while ((context->count[0] & 504) != 448)
    {
      c = 0000;
      mg_sha1_update (context, &c, 1);
    }
  mg_sha1_update (context, finalcount, 8);
  for (i = 0; i < 20; i++)
    {
      digest[i]
	  = (unsigned char) ((context->state[i >> 2] >> ((3 - (i & 3)) * 8))
			     & 255);
    }
  memset (context, '\0', sizeof (*context));
  memset (&finalcount, '\0', sizeof (finalcount));
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/sha256.c"
#endif
// https://github.com/B-Con/crypto-algorithms
// Author:     Brad Conte (brad AT bradconte.com)
// Disclaimer: This code is presented "as is" without any guarantees.
// Details:    Defines the API for the corresponding SHA1 implementation.
// Copyright:  public domain

#define ror(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define ep0(x) (ror (x, 2) ^ ror (x, 13) ^ ror (x, 22))
#define ep1(x) (ror (x, 6) ^ ror (x, 11) ^ ror (x, 25))
#define sig0(x) (ror (x, 7) ^ ror (x, 18) ^ ((x) >> 3))
#define sig1(x) (ror (x, 17) ^ ror (x, 19) ^ ((x) >> 10))

static const uint32_t mg_sha256_k[64]
    = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
	0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
	0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
	0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
	0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
	0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

void
mg_sha256_init (mg_sha256_ctx *ctx)
{
  ctx->len = 0;
  ctx->bits = 0;
  ctx->state[0] = 0x6a09e667;
  ctx->state[1] = 0xbb67ae85;
  ctx->state[2] = 0x3c6ef372;
  ctx->state[3] = 0xa54ff53a;
  ctx->state[4] = 0x510e527f;
  ctx->state[5] = 0x9b05688c;
  ctx->state[6] = 0x1f83d9ab;
  ctx->state[7] = 0x5be0cd19;
}

static void
mg_sha256_chunk (mg_sha256_ctx *ctx)
{
  int i, j;
  uint32_t a, b, c, d, e, f, g, h;
  uint32_t m[64];
  for (i = 0, j = 0; i < 16; ++i, j += 4)
    m[i] = (uint32_t) (((uint32_t) ctx->buffer[j] << 24)
		       | ((uint32_t) ctx->buffer[j + 1] << 16)
		       | ((uint32_t) ctx->buffer[j + 2] << 8)
		       | ((uint32_t) ctx->buffer[j + 3]));
  for (; i < 64; ++i)
    m[i] = sig1 (m[i - 2]) + m[i - 7] + sig0 (m[i - 15]) + m[i - 16];

  a = ctx->state[0];
  b = ctx->state[1];
  c = ctx->state[2];
  d = ctx->state[3];
  e = ctx->state[4];
  f = ctx->state[5];
  g = ctx->state[6];
  h = ctx->state[7];

  for (i = 0; i < 64; ++i)
    {
      uint32_t t1 = h + ep1 (e) + ch (e, f, g) + mg_sha256_k[i] + m[i];
      uint32_t t2 = ep0 (a) + maj (a, b, c);
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;
}

void
mg_sha256_update (mg_sha256_ctx *ctx, const unsigned char *data, size_t len)
{
  size_t i;
  for (i = 0; i < len; i++)
    {
      ctx->buffer[ctx->len] = data[i];
      if ((++ctx->len) == 64)
	{
	  mg_sha256_chunk (ctx);
	  ctx->bits += 512;
	  ctx->len = 0;
	}
    }
}

// TODO: make final reusable (remove side effects)
void
mg_sha256_final (unsigned char digest[32], mg_sha256_ctx *ctx)
{
  uint32_t i = ctx->len;
  if (i < 56)
    {
      ctx->buffer[i++] = 0x80;
      while (i < 56)
	{
	  ctx->buffer[i++] = 0x00;
	}
    }
  else
    {
      ctx->buffer[i++] = 0x80;
      while (i < 64)
	{
	  ctx->buffer[i++] = 0x00;
	}
      mg_sha256_chunk (ctx);
      memset (ctx->buffer, 0, 56);
    }

  ctx->bits += ctx->len * 8;
  ctx->buffer[63] = (uint8_t) ((ctx->bits) & 0xff);
  ctx->buffer[62] = (uint8_t) ((ctx->bits >> 8) & 0xff);
  ctx->buffer[61] = (uint8_t) ((ctx->bits >> 16) & 0xff);
  ctx->buffer[60] = (uint8_t) ((ctx->bits >> 24) & 0xff);
  ctx->buffer[59] = (uint8_t) ((ctx->bits >> 32) & 0xff);
  ctx->buffer[58] = (uint8_t) ((ctx->bits >> 40) & 0xff);
  ctx->buffer[57] = (uint8_t) ((ctx->bits >> 48) & 0xff);
  ctx->buffer[56] = (uint8_t) ((ctx->bits >> 56) & 0xff);
  mg_sha256_chunk (ctx);

  for (i = 0; i < 4; ++i)
    {
      digest[i] = (uint8_t) ((ctx->state[0] >> (24 - i * 8)) & 0xff);
      digest[i + 4] = (uint8_t) ((ctx->state[1] >> (24 - i * 8)) & 0xff);
      digest[i + 8] = (uint8_t) ((ctx->state[2] >> (24 - i * 8)) & 0xff);
      digest[i + 12] = (uint8_t) ((ctx->state[3] >> (24 - i * 8)) & 0xff);
      digest[i + 16] = (uint8_t) ((ctx->state[4] >> (24 - i * 8)) & 0xff);
      digest[i + 20] = (uint8_t) ((ctx->state[5] >> (24 - i * 8)) & 0xff);
      digest[i + 24] = (uint8_t) ((ctx->state[6] >> (24 - i * 8)) & 0xff);
      digest[i + 28] = (uint8_t) ((ctx->state[7] >> (24 - i * 8)) & 0xff);
    }
}

void
mg_hmac_sha256 (uint8_t dst[32], uint8_t *key, size_t keysz, uint8_t *data,
		size_t datasz)
{
  mg_sha256_ctx ctx;
  uint8_t k[64] = { 0 };
  uint8_t o_pad[64], i_pad[64];
  unsigned int i;
  memset (i_pad, 0x36, sizeof (i_pad));
  memset (o_pad, 0x5c, sizeof (o_pad));
  if (keysz < 64)
    {
      if (keysz > 0)
	memmove (k, key, keysz);
    }
  else
    {
      mg_sha256_init (&ctx);
      mg_sha256_update (&ctx, key, keysz);
      mg_sha256_final (k, &ctx);
    }
  for (i = 0; i < sizeof (k); i++)
    {
      i_pad[i] ^= k[i];
      o_pad[i] ^= k[i];
    }
  mg_sha256_init (&ctx);
  mg_sha256_update (&ctx, i_pad, sizeof (i_pad));
  mg_sha256_update (&ctx, data, datasz);
  mg_sha256_final (dst, &ctx);
  mg_sha256_init (&ctx);
  mg_sha256_update (&ctx, o_pad, sizeof (o_pad));
  mg_sha256_update (&ctx, dst, 32);
  mg_sha256_final (dst, &ctx);
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/sntp.c"
#endif

#define SNTP_TIME_OFFSET 2208988800U // (1970 - 1900) in seconds
#define SNTP_MAX_FRAC 4294967295.0   // 2 ** 32 - 1

static int64_t
gettimestamp (const uint32_t *data)
{
  uint32_t sec = mg_ntohl (data[0]), frac = mg_ntohl (data[1]);
  if (sec)
    sec -= SNTP_TIME_OFFSET;
  return ((int64_t) sec) * 1000 + (int64_t) (frac / SNTP_MAX_FRAC * 1000.0);
}

int64_t
mg_sntp_parse (const unsigned char *buf, size_t len)
{
  int64_t res = -1;
  int mode = len > 0 ? buf[0] & 7 : 0;
  int version = len > 0 ? (buf[0] >> 3) & 7 : 0;
  if (len < 48)
    {
      MG_ERROR (("%s", "corrupt packet"));
    }
  else if (mode != 4 && mode != 5)
    {
      MG_ERROR (("%s", "not a server reply"));
    }
  else if (buf[1] == 0)
    {
      MG_ERROR (("%s", "server sent a kiss of death"));
    }
  else if (version == 4 || version == 3)
    {
      // int64_t ref = gettimestamp((uint32_t *) &buf[16]);
      int64_t t0 = gettimestamp ((uint32_t *) &buf[24]);
      int64_t t1 = gettimestamp ((uint32_t *) &buf[32]);
      int64_t t2 = gettimestamp ((uint32_t *) &buf[40]);
      int64_t t3 = (int64_t) mg_millis ();
      int64_t delta = (t3 - t0) - (t2 - t1);
      MG_VERBOSE (("%lld %lld %lld %lld delta:%lld", t0, t1, t2, t3, delta));
      res = t2 + delta / 2;
    }
  else
    {
      MG_ERROR (("unexpected version: %d", version));
    }
  return res;
}

static void
sntp_cb (struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_READ)
    {
      int64_t milliseconds = mg_sntp_parse (c->recv.buf, c->recv.len);
      if (milliseconds > 0)
	{
	  MG_DEBUG (("%lu got time: %lld ms from epoch", c->id, milliseconds));
	  mg_call (c, MG_EV_SNTP_TIME, (uint64_t *) &milliseconds);
	  MG_VERBOSE (("%u.%u", (unsigned) (milliseconds / 1000),
		       (unsigned) (milliseconds % 1000)));
	}
      mg_iobuf_del (&c->recv, 0, c->recv.len); // Free receive buffer
    }
  else if (ev == MG_EV_CONNECT)
    {
      mg_sntp_request (c);
    }
  else if (ev == MG_EV_CLOSE)
    {
    }
  (void) ev_data;
}

void
mg_sntp_request (struct mg_connection *c)
{
  if (c->is_resolving)
    {
      MG_ERROR (("%lu wait until resolved", c->id));
    }
  else
    {
      int64_t now = (int64_t) mg_millis (); // Use int64_t, for vc98
      uint8_t buf[48] = { 0 };
      uint32_t *t = (uint32_t *) &buf[40];
      double frac = ((double) (now % 1000)) / 1000.0 * SNTP_MAX_FRAC;
      buf[0] = (0 << 6) | (4 << 3) | 3;
      t[0] = mg_htonl ((uint32_t) (now / 1000) + SNTP_TIME_OFFSET);
      t[1] = mg_htonl ((uint32_t) frac);
      mg_send (c, buf, sizeof (buf));
    }
}

struct mg_connection *
mg_sntp_connect (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
		 void *fnd)
{
  struct mg_connection *c = NULL;
  if (url == NULL)
    url = "udp://time.google.com:123";
  if ((c = mg_connect (mgr, url, fn, fnd)) != NULL)
    c->pfn = sntp_cb;
  return c;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/sock.c"
#endif

#if MG_ENABLE_SOCKET

#  ifndef closesocket
#    define closesocket(x) close (x)
#  endif

#  define FD(c_) ((MG_SOCKET_TYPE) (size_t) (c_)->fd)
#  define S2PTR(s_) ((void *) (size_t) (s_))

#  ifndef MSG_NONBLOCKING
#    define MSG_NONBLOCKING 0
#  endif

#  ifndef AF_INET6
#    define AF_INET6 10
#  endif

#  ifndef MG_SOCK_ERR
#    define MG_SOCK_ERR(errcode) ((errcode) < 0 ? errno : 0)
#  endif

#  ifndef MG_SOCK_INTR
#    define MG_SOCK_INTR(fd)                                                  \
      (fd == MG_INVALID_SOCKET && MG_SOCK_ERR (-1) == EINTR)
#  endif

#  ifndef MG_SOCK_PENDING
#    define MG_SOCK_PENDING(errcode)                                          \
      (((errcode) < 0) && (errno == EINPROGRESS || errno == EWOULDBLOCK))
#  endif

#  ifndef MG_SOCK_RESET
#    define MG_SOCK_RESET(errcode)                                            \
      (((errcode) < 0) && (errno == EPIPE || errno == ECONNRESET))
#  endif

union usa
{
  struct sockaddr sa;
  struct sockaddr_in sin;
#  if MG_ENABLE_IPV6
  struct sockaddr_in6 sin6;
#  endif
};

static socklen_t
tousa (struct mg_addr *a, union usa *usa)
{
  socklen_t len = sizeof (usa->sin);
  memset (usa, 0, sizeof (*usa));
  usa->sin.sin_family = AF_INET;
  usa->sin.sin_port = a->port;
  memcpy (&usa->sin.sin_addr, a->ip, sizeof (uint32_t));
#  if MG_ENABLE_IPV6
  if (a->is_ip6)
    {
      usa->sin.sin_family = AF_INET6;
      usa->sin6.sin6_port = a->port;
      usa->sin6.sin6_scope_id = a->scope_id;
      memcpy (&usa->sin6.sin6_addr, a->ip, sizeof (a->ip));
      len = sizeof (usa->sin6);
    }
#  endif
  return len;
}

static void
tomgaddr (union usa *usa, struct mg_addr *a, bool is_ip6)
{
  a->is_ip6 = is_ip6;
  a->port = usa->sin.sin_port;
  memcpy (&a->ip, &usa->sin.sin_addr, sizeof (uint32_t));
#  if MG_ENABLE_IPV6
  if (is_ip6)
    {
      memcpy (a->ip, &usa->sin6.sin6_addr, sizeof (a->ip));
      a->port = usa->sin6.sin6_port;
      a->scope_id = (uint8_t) usa->sin6.sin6_scope_id;
    }
#  endif
}

static void
setlocaddr (MG_SOCKET_TYPE fd, struct mg_addr *addr)
{
  union usa usa;
  socklen_t n = sizeof (usa);
  if (getsockname (fd, &usa.sa, &n) == 0)
    {
      tomgaddr (&usa, addr, n != sizeof (usa.sin));
    }
}

static void
iolog (struct mg_connection *c, char *buf, long n, bool r)
{
  if (n == MG_IO_WAIT)
    {
      // Do nothing
    }
  else if (n <= 0)
    {
      c->is_closing = 1; // Termination. Don't call mg_error(): #1529
    }
  else if (n > 0)
    {
      if (c->is_hexdumping)
	{
	  MG_INFO (("\n-- %lu %M %s %M %ld", c->id, mg_print_ip_port, &c->loc,
		    r ? "<-" : "->", mg_print_ip_port, &c->rem, n));
	  mg_hexdump (buf, (size_t) n);
	}
      if (r)
	{
	  c->recv.len += (size_t) n;
	  mg_call (c, MG_EV_READ, &n);
	}
      else
	{
	  mg_iobuf_del (&c->send, 0, (size_t) n);
	  // if (c->send.len == 0) mg_iobuf_resize(&c->send, 0);
	  if (c->send.len == 0)
	    {
	      MG_EPOLL_MOD (c, 0);
	    }
	  mg_call (c, MG_EV_WRITE, &n);
	}
    }
}

long
mg_io_send (struct mg_connection *c, const void *buf, size_t len)
{
  long n;
  if (c->is_udp)
    {
      union usa usa;
      socklen_t slen = tousa (&c->rem, &usa);
      n = sendto (FD (c), (char *) buf, len, 0, &usa.sa, slen);
      if (n > 0)
	setlocaddr (FD (c), &c->loc);
    }
  else
    {
      n = send (FD (c), (char *) buf, len, MSG_NONBLOCKING);
    }
  MG_VERBOSE (("%lu %ld %d", c->id, n, MG_SOCK_ERR (n)));
  if (MG_SOCK_PENDING (n))
    return MG_IO_WAIT;
  if (MG_SOCK_RESET (n))
    return MG_IO_RESET;
  if (n <= 0)
    return MG_IO_ERR;
  return n;
}

bool
mg_send (struct mg_connection *c, const void *buf, size_t len)
{
  if (c->is_udp)
    {
      long n = mg_io_send (c, buf, len);
      MG_DEBUG (("%lu %ld %lu:%lu:%lu %ld err %d", c->id, c->fd, c->send.len,
		 c->recv.len, c->rtls.len, n, MG_SOCK_ERR (n)));
      iolog (c, (char *) buf, n, false);
      return n > 0;
    }
  else
    {
      return mg_iobuf_add (&c->send, c->send.len, buf, len);
    }
}

static void
mg_set_non_blocking_mode (MG_SOCKET_TYPE fd)
{
#  if defined(MG_CUSTOM_NONBLOCK)
  MG_CUSTOM_NONBLOCK (fd);
#  elif MG_ARCH == MG_ARCH_WIN32 && MG_ENABLE_WINSOCK
  unsigned long on = 1;
  ioctlsocket (fd, FIONBIO, &on);
#  elif MG_ENABLE_RL
  unsigned long on = 1;
  ioctlsocket (fd, FIONBIO, &on);
#  elif MG_ENABLE_FREERTOS_TCP
  const BaseType_t off = 0;
  if (setsockopt (fd, 0, FREERTOS_SO_RCVTIMEO, &off, sizeof (off)) != 0)
    (void) 0;
  if (setsockopt (fd, 0, FREERTOS_SO_SNDTIMEO, &off, sizeof (off)) != 0)
    (void) 0;
#  elif MG_ENABLE_LWIP
  lwip_fcntl (fd, F_SETFL, O_NONBLOCK);
#  elif MG_ARCH == MG_ARCH_AZURERTOS
  fcntl (fd, F_SETFL, O_NONBLOCK);
#  elif MG_ARCH == MG_ARCH_TIRTOS
  int val = 0;
  setsockopt (fd, SOL_SOCKET, SO_BLOCKING, &val, sizeof (val));
  // SPRU524J section 3.3.3 page 63, SO_SNDLOWAT
  int sz = sizeof (val);
  getsockopt (fd, SOL_SOCKET, SO_SNDBUF, &val, &sz);
  val /= 2; // set send low-water mark at half send buffer size
  setsockopt (fd, SOL_SOCKET, SO_SNDLOWAT, &val, sizeof (val));
#  else
  fcntl (fd, F_SETFL,
	 fcntl (fd, F_GETFL, 0) | O_NONBLOCK); // Non-blocking mode
  fcntl (fd, F_SETFD, FD_CLOEXEC);	       // Set close-on-exec
#  endif
}

bool
mg_open_listener (struct mg_connection *c, const char *url)
{
  MG_SOCKET_TYPE fd = MG_INVALID_SOCKET;
  bool success = false;
  c->loc.port = mg_htons (mg_url_port (url));
  if (!mg_aton (mg_url_host (url), &c->loc))
    {
      MG_ERROR (("invalid listening URL: %s", url));
    }
  else
    {
      union usa usa;
      socklen_t slen = tousa (&c->loc, &usa);
      int rc, on = 1, af = c->loc.is_ip6 ? AF_INET6 : AF_INET;
      int type = strncmp (url, "udp:", 4) == 0 ? SOCK_DGRAM : SOCK_STREAM;
      int proto = type == SOCK_DGRAM ? IPPROTO_UDP : IPPROTO_TCP;
      (void) on;

      if ((fd = socket (af, type, proto)) == MG_INVALID_SOCKET)
	{
	  MG_ERROR (("socket: %d", MG_SOCK_ERR (-1)));
#  if defined(SO_EXCLUSIVEADDRUSE)
	}
      else if ((rc = setsockopt (fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
				 (char *) &on, sizeof (on)))
	       != 0)
	{
	  // "Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE"
	  MG_ERROR (("setsockopt(SO_EXCLUSIVEADDRUSE): %d %d", on,
		     MG_SOCK_ERR (rc)));
#  elif defined(SO_REUSEADDR) && (!defined(LWIP_SOCKET) || SO_REUSE)
	}
      else if ((rc = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,
				 sizeof (on)))
	       != 0)
	{
	  // 1. SO_REUSEADDR semantics on UNIX and Windows is different.  On
	  // Windows, SO_REUSEADDR allows to bind a socket to a port without
	  // error even if the port is already open by another program. This is
	  // not the behavior SO_REUSEADDR was designed for, and leads to
	  // hard-to-track failure scenarios.
	  //
	  // 2. For LWIP, SO_REUSEADDR should be explicitly enabled by defining
	  // SO_REUSE = 1 in lwipopts.h, otherwise the code below will compile
	  // but won't work! (setsockopt will return EINVAL)
	  MG_ERROR (("setsockopt(SO_REUSEADDR): %d", MG_SOCK_ERR (rc)));
#  endif
#  if MG_IPV6_V6ONLY
	  // Bind only to the V6 address, not V4 address on this port
	}
      else if (c->loc.is_ip6
	       && (rc = setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY,
				    (char *) &on, sizeof (on)))
		      != 0)
	{
	  // See #2089. Allow to bind v4 and v6 sockets on the same port
	  MG_ERROR (("setsockopt(IPV6_V6ONLY): %d", MG_SOCK_ERR (rc)));
#  endif
	}
      else if ((rc = bind (fd, &usa.sa, slen)) != 0)
	{
	  MG_ERROR (("bind: %d", MG_SOCK_ERR (rc)));
	}
      else if ((type == SOCK_STREAM
		&& (rc = listen (fd, MG_SOCK_LISTEN_BACKLOG_SIZE)) != 0))
	{
	  // NOTE(lsm): FreeRTOS uses backlog value as a connection limit
	  // In case port was set to 0, get the real port number
	  MG_ERROR (("listen: %d", MG_SOCK_ERR (rc)));
	}
      else
	{
	  setlocaddr (fd, &c->loc);
	  mg_set_non_blocking_mode (fd);
	  c->fd = S2PTR (fd);
	  MG_EPOLL_ADD (c);
	  success = true;
	}
    }
  if (success == false && fd != MG_INVALID_SOCKET)
    closesocket (fd);
  return success;
}

static long
recv_raw (struct mg_connection *c, void *buf, size_t len)
{
  long n = 0;
  if (c->is_udp)
    {
      union usa usa;
      socklen_t slen = tousa (&c->rem, &usa);
      n = recvfrom (FD (c), (char *) buf, len, 0, &usa.sa, &slen);
      if (n > 0)
	tomgaddr (&usa, &c->rem, slen != sizeof (usa.sin));
    }
  else
    {
      n = recv (FD (c), (char *) buf, len, MSG_NONBLOCKING);
    }
  MG_VERBOSE (("%lu %ld %d", c->id, n, MG_SOCK_ERR (n)));
  if (MG_SOCK_PENDING (n))
    return MG_IO_WAIT;
  if (MG_SOCK_RESET (n))
    return MG_IO_RESET;
  if (n <= 0)
    return MG_IO_ERR;
  return n;
}

static bool
ioalloc (struct mg_connection *c, struct mg_iobuf *io)
{
  bool res = false;
  if (io->len >= MG_MAX_RECV_SIZE)
    {
      mg_error (c, "MG_MAX_RECV_SIZE");
    }
  else if (io->size <= io->len && !mg_iobuf_resize (io, io->size + MG_IO_SIZE))
    {
      mg_error (c, "OOM");
    }
  else
    {
      res = true;
    }
  return res;
}

// NOTE(lsm): do only one iteration of reads, cause some systems
// (e.g. FreeRTOS stack) return 0 instead of -1/EWOULDBLOCK when no data
static void
read_conn (struct mg_connection *c)
{
  if (ioalloc (c, &c->recv))
    {
      char *buf = (char *) &c->recv.buf[c->recv.len];
      size_t len = c->recv.size - c->recv.len;
      long n = -1;
      if (c->is_tls)
	{
	  if (!ioalloc (c, &c->rtls))
	    return;
	  n = recv_raw (c, (char *) &c->rtls.buf[c->rtls.len],
			c->rtls.size - c->rtls.len);
	  if (n == MG_IO_ERR && c->rtls.len == 0)
	    {
	      // Close only if we have fully drained both raw (rtls) and TLS
	      // buffers
	      c->is_closing = 1;
	    }
	  else
	    {
	      if (n > 0)
		c->rtls.len += (size_t) n;
	      if (c->is_tls_hs)
		mg_tls_handshake (c);
	      n = c->is_tls_hs ? (long) MG_IO_WAIT : mg_tls_recv (c, buf, len);
	    }
	}
      else
	{
	  n = recv_raw (c, buf, len);
	}
      MG_DEBUG (("%lu %ld %lu:%lu:%lu %ld err %d", c->id, c->fd, c->send.len,
		 c->recv.len, c->rtls.len, n, MG_SOCK_ERR (n)));
      iolog (c, buf, n, true);
    }
}

static void
write_conn (struct mg_connection *c)
{
  char *buf = (char *) c->send.buf;
  size_t len = c->send.len;
  long n = c->is_tls ? mg_tls_send (c, buf, len) : mg_io_send (c, buf, len);
  MG_DEBUG (("%lu %ld snd %ld/%ld rcv %ld/%ld n=%ld err=%d", c->id, c->fd,
	     (long) c->send.len, (long) c->send.size, (long) c->recv.len,
	     (long) c->recv.size, n, MG_SOCK_ERR (n)));
  iolog (c, buf, n, false);
}

static void
close_conn (struct mg_connection *c)
{
  if (FD (c) != MG_INVALID_SOCKET)
    {
#  if MG_ENABLE_EPOLL
      epoll_ctl (c->mgr->epoll_fd, EPOLL_CTL_DEL, FD (c), NULL);
#  endif
      closesocket (FD (c));
#  if MG_ENABLE_FREERTOS_TCP
      FreeRTOS_FD_CLR (c->fd, c->mgr->ss, eSELECT_ALL);
#  endif
    }
  mg_close_conn (c);
}

static void
connect_conn (struct mg_connection *c)
{
  union usa usa;
  socklen_t n = sizeof (usa);
  // Use getpeername() to test whether we have connected
  if (getpeername (FD (c), &usa.sa, &n) == 0)
    {
      c->is_connecting = 0;
      setlocaddr (FD (c), &c->loc);
      mg_call (c, MG_EV_CONNECT, NULL);
      MG_EPOLL_MOD (c, 0);
      if (c->is_tls_hs)
	mg_tls_handshake (c);
    }
  else
    {
      mg_error (c, "socket error");
    }
}

static void
setsockopts (struct mg_connection *c)
{
#  if MG_ENABLE_FREERTOS_TCP || MG_ARCH == MG_ARCH_AZURERTOS                  \
      || MG_ARCH == MG_ARCH_TIRTOS
  (void) c;
#  else
  int on = 1;
#    if !defined(SOL_TCP)
#      define SOL_TCP IPPROTO_TCP
#    endif
  if (setsockopt (FD (c), SOL_TCP, TCP_NODELAY, (char *) &on, sizeof (on))
      != 0)
    (void) 0;
  if (setsockopt (FD (c), SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof (on))
      != 0)
    (void) 0;
#  endif
}

void
mg_connect_resolved (struct mg_connection *c)
{
  int type = c->is_udp ? SOCK_DGRAM : SOCK_STREAM;
  int rc, af = c->rem.is_ip6 ? AF_INET6 : AF_INET; // c->rem has resolved IP
  c->fd = S2PTR (socket (af, type, 0));		   // Create outbound socket
  c->is_resolving = 0;				   // Clear resolving flag
  if (FD (c) == MG_INVALID_SOCKET)
    {
      mg_error (c, "socket(): %d", MG_SOCK_ERR (-1));
    }
  else if (c->is_udp)
    {
      MG_EPOLL_ADD (c);
#  if MG_ARCH == MG_ARCH_TIRTOS
      union usa usa; // TI-RTOS NDK requires binding to receive on UDP sockets
      socklen_t slen = tousa (&c->loc, &usa);
      if ((rc = bind (c->fd, &usa.sa, slen)) != 0)
	MG_ERROR (("bind: %d", MG_SOCK_ERR (rc)));
#  endif
      setlocaddr (FD (c), &c->loc);
      mg_call (c, MG_EV_RESOLVE, NULL);
      mg_call (c, MG_EV_CONNECT, NULL);
    }
  else
    {
      union usa usa;
      socklen_t slen = tousa (&c->rem, &usa);
      mg_set_non_blocking_mode (FD (c));
      setsockopts (c);
      MG_EPOLL_ADD (c);
      mg_call (c, MG_EV_RESOLVE, NULL);
      rc = connect (FD (c), &usa.sa, slen); // Attempt to connect
      if (rc == 0)
	{ // Success
	  setlocaddr (FD (c), &c->loc);
	  mg_call (c, MG_EV_CONNECT, NULL); // Send MG_EV_CONNECT to the user
	}
      else if (MG_SOCK_PENDING (rc))
	{ // Need to wait for TCP handshake
	  MG_DEBUG (
	      ("%lu %ld -> %M pend", c->id, c->fd, mg_print_ip_port, &c->rem));
	  c->is_connecting = 1;
	}
      else
	{
	  mg_error (c, "connect: %d", MG_SOCK_ERR (rc));
	}
    }
}

static MG_SOCKET_TYPE
raccept (MG_SOCKET_TYPE sock, union usa *usa, socklen_t *len)
{
  MG_SOCKET_TYPE fd = MG_INVALID_SOCKET;
  do
    {
      memset (usa, 0, sizeof (*usa));
      fd = accept (sock, &usa->sa, len);
    }
  while (MG_SOCK_INTR (fd));
  return fd;
}

static void
accept_conn (struct mg_mgr *mgr, struct mg_connection *lsn)
{
  struct mg_connection *c = NULL;
  union usa usa;
  socklen_t sa_len = sizeof (usa);
  MG_SOCKET_TYPE fd = raccept (FD (lsn), &usa, &sa_len);
  if (fd == MG_INVALID_SOCKET)
    {
#  if MG_ARCH == MG_ARCH_AZURERTOS || defined(__ECOS)
      // AzureRTOS, in non-block socket mode can mark listening socket readable
      // even it is not. See comment for 'select' func implementation in
      // nx_bsd.c That's not an error, just should try later
      if (errno != EAGAIN)
#  endif
	MG_ERROR (("%lu accept failed, errno %d", lsn->id, MG_SOCK_ERR (-1)));
#  if (MG_ARCH != MG_ARCH_WIN32) && !MG_ENABLE_FREERTOS_TCP                   \
      && (MG_ARCH != MG_ARCH_TIRTOS) && !MG_ENABLE_POLL && !MG_ENABLE_EPOLL
    }
  else if ((long) fd >= FD_SETSIZE)
    {
      MG_ERROR (("%ld > %ld", (long) fd, (long) FD_SETSIZE));
      closesocket (fd);
#  endif
    }
  else if ((c = mg_alloc_conn (mgr)) == NULL)
    {
      MG_ERROR (("%lu OOM", lsn->id));
      closesocket (fd);
    }
  else
    {
      tomgaddr (&usa, &c->rem, sa_len != sizeof (usa.sin));
      LIST_ADD_HEAD (struct mg_connection, &mgr->conns, c);
      c->fd = S2PTR (fd);
      MG_EPOLL_ADD (c);
      mg_set_non_blocking_mode (FD (c));
      setsockopts (c);
      c->is_accepted = 1;
      c->is_hexdumping = lsn->is_hexdumping;
      c->loc = lsn->loc;
      c->pfn = lsn->pfn;
      c->pfn_data = lsn->pfn_data;
      c->fn = lsn->fn;
      c->fn_data = lsn->fn_data;
      MG_DEBUG (("%lu %ld accepted %M -> %M", c->id, c->fd, mg_print_ip_port,
		 &c->rem, mg_print_ip_port, &c->loc));
      mg_call (c, MG_EV_OPEN, NULL);
      mg_call (c, MG_EV_ACCEPT, NULL);
    }
}

static bool
can_read (const struct mg_connection *c)
{
  return c->is_full == false;
}

static bool
can_write (const struct mg_connection *c)
{
  return c->is_connecting || (c->send.len > 0 && c->is_tls_hs == 0);
}

static bool
skip_iotest (const struct mg_connection *c)
{
  return (c->is_closing || c->is_resolving || FD (c) == MG_INVALID_SOCKET)
	 || (can_read (c) == false && can_write (c) == false);
}

static void
mg_iotest (struct mg_mgr *mgr, int ms)
{
#  if MG_ENABLE_FREERTOS_TCP
  struct mg_connection *c;
  for (c = mgr->conns; c != NULL; c = c->next)
    {
      c->is_readable = c->is_writable = 0;
      if (skip_iotest (c))
	continue;
      if (can_read (c))
	FreeRTOS_FD_SET (c->fd, mgr->ss, eSELECT_READ | eSELECT_EXCEPT);
      if (can_write (c))
	FreeRTOS_FD_SET (c->fd, mgr->ss, eSELECT_WRITE);
      if (c->is_closing)
	ms = 1;
    }
  FreeRTOS_select (mgr->ss, pdMS_TO_TICKS (ms));
  for (c = mgr->conns; c != NULL; c = c->next)
    {
      EventBits_t bits = FreeRTOS_FD_ISSET (c->fd, mgr->ss);
      c->is_readable = bits & (eSELECT_READ | eSELECT_EXCEPT) ? 1U : 0;
      c->is_writable = bits & eSELECT_WRITE ? 1U : 0;
      if (c->fd != MG_INVALID_SOCKET)
	FreeRTOS_FD_CLR (c->fd, mgr->ss,
			 eSELECT_READ | eSELECT_EXCEPT | eSELECT_WRITE);
    }
#  elif MG_ENABLE_EPOLL
  size_t max = 1;
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
      c->is_readable = c->is_writable = 0;
      if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
	ms = 1, c->is_readable = 1;
      if (can_write (c))
	MG_EPOLL_MOD (c, 1);
      if (c->is_closing)
	ms = 1;
      max++;
    }
  struct epoll_event *evs
      = (struct epoll_event *) alloca (max * sizeof (evs[0]));
  int n = epoll_wait (mgr->epoll_fd, evs, (int) max, ms);
  for (int i = 0; i < n; i++)
    {
      struct mg_connection *c = (struct mg_connection *) evs[i].data.ptr;
      if (evs[i].events & EPOLLERR)
	{
	  mg_error (c, "socket error");
	}
      else if (c->is_readable == 0)
	{
	  bool rd = evs[i].events & (EPOLLIN | EPOLLHUP);
	  bool wr = evs[i].events & EPOLLOUT;
	  c->is_readable = can_read (c) && rd ? 1U : 0;
	  c->is_writable = can_write (c) && wr ? 1U : 0;
	  if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
	    c->is_readable = 1;
	}
    }
  (void) skip_iotest;
#  elif MG_ENABLE_POLL
  nfds_t n = 0;
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    n++;
  struct pollfd *fds = (struct pollfd *) alloca (n * sizeof (fds[0]));
  memset (fds, 0, n * sizeof (fds[0]));
  n = 0;
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
      c->is_readable = c->is_writable = 0;
      if (skip_iotest (c))
	{
	  // Socket not valid, ignore
	}
      else if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
	{
	  ms = 1; // Don't wait if TLS is ready
	}
      else
	{
	  fds[n].fd = FD (c);
	  if (can_read (c))
	    fds[n].events |= POLLIN;
	  if (can_write (c))
	    fds[n].events |= POLLOUT;
	  if (c->is_closing)
	    ms = 1;
	  n++;
	}
    }

  // MG_INFO(("poll n=%d ms=%d", (int) n, ms));
  if (poll (fds, n, ms) < 0)
    {
#    if MG_ARCH == MG_ARCH_WIN32
      if (n == 0)
	Sleep (ms); // On Windows, poll fails if no sockets
#    endif
      memset (fds, 0, n * sizeof (fds[0]));
    }
  n = 0;
  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
      if (skip_iotest (c))
	{
	  // Socket not valid, ignore
	}
      else if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
	{
	  c->is_readable = 1;
	}
      else
	{
	  if (fds[n].revents & POLLERR)
	    {
	      mg_error (c, "socket error");
	    }
	  else
	    {
	      c->is_readable
		  = (unsigned) (fds[n].revents & (POLLIN | POLLHUP) ? 1 : 0);
	      c->is_writable = (unsigned) (fds[n].revents & POLLOUT ? 1 : 0);
	      if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
		c->is_readable = 1;
	    }
	  n++;
	}
    }
#  else
  struct timeval tv = { ms / 1000, (ms % 1000) * 1000 }, tv_zero = { 0, 0 },
		 *tvp;
  struct mg_connection *c;
  fd_set rset, wset, eset;
  MG_SOCKET_TYPE maxfd = 0;
  int rc;

  FD_ZERO (&rset);
  FD_ZERO (&wset);
  FD_ZERO (&eset);
  tvp = ms < 0 ? NULL : &tv;
  for (c = mgr->conns; c != NULL; c = c->next)
    {
      c->is_readable = c->is_writable = 0;
      if (skip_iotest (c))
	continue;
      FD_SET (FD (c), &eset);
      if (can_read (c))
	FD_SET (FD (c), &rset);
      if (can_write (c))
	FD_SET (FD (c), &wset);
      if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
	tvp = &tv_zero;
      if (FD (c) > maxfd)
	maxfd = FD (c);
      if (c->is_closing)
	ms = 1;
    }

  if ((rc = select ((int) maxfd + 1, &rset, &wset, &eset, tvp)) < 0)
    {
#    if MG_ARCH == MG_ARCH_WIN32
      if (maxfd == 0)
	Sleep (ms); // On Windows, select fails if no sockets
#    else
      MG_ERROR (("select: %d %d", rc, MG_SOCK_ERR (rc)));
#    endif
      FD_ZERO (&rset);
      FD_ZERO (&wset);
      FD_ZERO (&eset);
    }

  for (c = mgr->conns; c != NULL; c = c->next)
    {
      if (FD (c) != MG_INVALID_SOCKET && FD_ISSET (FD (c), &eset))
	{
	  mg_error (c, "socket error");
	}
      else
	{
	  c->is_readable
	      = FD (c) != MG_INVALID_SOCKET && FD_ISSET (FD (c), &rset);
	  c->is_writable
	      = FD (c) != MG_INVALID_SOCKET && FD_ISSET (FD (c), &wset);
	  if (c->rtls.len > 0 || mg_tls_pending (c) > 0)
	    c->is_readable = 1;
	}
    }
#  endif
}

static bool
mg_socketpair (MG_SOCKET_TYPE sp[2], union usa usa[2])
{
  socklen_t n = sizeof (usa[0].sin);
  bool success = false;

  sp[0] = sp[1] = MG_INVALID_SOCKET;
  (void) memset (&usa[0], 0, sizeof (usa[0]));
  usa[0].sin.sin_family = AF_INET;
  *(uint32_t *) &usa->sin.sin_addr = mg_htonl (0x7f000001U); // 127.0.0.1
  usa[1] = usa[0];

  if ((sp[0] = socket (AF_INET, SOCK_DGRAM, 0)) != MG_INVALID_SOCKET
      && (sp[1] = socket (AF_INET, SOCK_DGRAM, 0)) != MG_INVALID_SOCKET
      && bind (sp[0], &usa[0].sa, n) == 0 &&	  //
      bind (sp[1], &usa[1].sa, n) == 0 &&	  //
      getsockname (sp[0], &usa[0].sa, &n) == 0 && //
      getsockname (sp[1], &usa[1].sa, &n) == 0 && //
      connect (sp[0], &usa[1].sa, n) == 0 &&	  //
      connect (sp[1], &usa[0].sa, n) == 0)
    { //
      success = true;
    }
  if (!success)
    {
      if (sp[0] != MG_INVALID_SOCKET)
	closesocket (sp[0]);
      if (sp[1] != MG_INVALID_SOCKET)
	closesocket (sp[1]);
      sp[0] = sp[1] = MG_INVALID_SOCKET;
    }
  return success;
}

// mg_wakeup() event handler
static void
wufn (struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_READ)
    {
      unsigned long *id = (unsigned long *) c->recv.buf;
      // MG_INFO(("Got data"));
      // mg_hexdump(c->recv.buf, c->recv.len);
      if (c->recv.len >= sizeof (*id))
	{
	  struct mg_connection *t;
	  for (t = c->mgr->conns; t != NULL; t = t->next)
	    {
	      if (t->id == *id)
		{
		  struct mg_str data
		      = mg_str_n ((char *) c->recv.buf + sizeof (*id),
				  c->recv.len - sizeof (*id));
		  mg_call (t, MG_EV_WAKEUP, &data);
		}
	    }
	}
      c->recv.len = 0; // Consume received data
    }
  else if (ev == MG_EV_CLOSE)
    {
      closesocket (c->mgr->pipe);	// When we're closing, close the other
      c->mgr->pipe = MG_INVALID_SOCKET; // side of the socketpair, too
    }
  (void) ev_data;
}

bool
mg_wakeup_init (struct mg_mgr *mgr)
{
  bool ok = false;
  if (mgr->pipe == MG_INVALID_SOCKET)
    {
      union usa usa[2];
      MG_SOCKET_TYPE sp[2] = { MG_INVALID_SOCKET, MG_INVALID_SOCKET };
      struct mg_connection *c = NULL;
      if (!mg_socketpair (sp, usa))
	{
	  MG_ERROR (("Cannot create socket pair"));
	}
      else if ((c = mg_wrapfd (mgr, (int) sp[1], wufn, NULL)) == NULL)
	{
	  closesocket (sp[0]);
	  closesocket (sp[1]);
	  sp[0] = sp[1] = MG_INVALID_SOCKET;
	}
      else
	{
	  tomgaddr (&usa[0], &c->rem, false);
	  MG_DEBUG (("%lu %p pipe %lu", c->id, c->fd, (unsigned long) sp[0]));
	  mgr->pipe = sp[0];
	  ok = true;
	}
    }
  return ok;
}

bool
mg_wakeup (struct mg_mgr *mgr, unsigned long conn_id, const void *buf,
	   size_t len)
{
  if (mgr->pipe != MG_INVALID_SOCKET && conn_id > 0)
    {
      char *extended_buf = (char *) alloca (len + sizeof (conn_id));
      memcpy (extended_buf, &conn_id, sizeof (conn_id));
      memcpy (extended_buf + sizeof (conn_id), buf, len);
      send (mgr->pipe, extended_buf, len + sizeof (conn_id), MSG_NONBLOCKING);
      return true;
    }
  return false;
}

void
mg_mgr_poll (struct mg_mgr *mgr, int ms)
{
  struct mg_connection *c, *tmp;
  uint64_t now;

  mg_iotest (mgr, ms);
  now = mg_millis ();
  mg_timer_poll (&mgr->timers, now);

  for (c = mgr->conns; c != NULL; c = tmp)
    {
      bool is_resp = c->is_resp;
      tmp = c->next;
      mg_call (c, MG_EV_POLL, &now);
      if (is_resp && !c->is_resp)
	{
	  long n = 0;
	  mg_call (c, MG_EV_READ, &n);
	}
      MG_VERBOSE (("%lu %c%c %c%c%c%c%c %lu %lu", c->id,
		   c->is_readable ? 'r' : '-', c->is_writable ? 'w' : '-',
		   c->is_tls ? 'T' : 't', c->is_connecting ? 'C' : 'c',
		   c->is_tls_hs ? 'H' : 'h', c->is_resolving ? 'R' : 'r',
		   c->is_closing ? 'C' : 'c', mg_tls_pending (c),
		   c->rtls.len));
      if (c->is_resolving || c->is_closing)
	{
	  // Do nothing
	}
      else if (c->is_listening && c->is_udp == 0)
	{
	  if (c->is_readable)
	    accept_conn (mgr, c);
	}
      else if (c->is_connecting)
	{
	  if (c->is_readable || c->is_writable)
	    connect_conn (c);
	  //} else if (c->is_tls_hs) {
	  //  if ((c->is_readable || c->is_writable)) mg_tls_handshake(c);
	}
      else
	{
	  if (c->is_readable)
	    read_conn (c);
	  if (c->is_writable)
	    write_conn (c);
	}

      if (c->is_draining && c->send.len == 0)
	c->is_closing = 1;
      if (c->is_closing)
	close_conn (c);
    }
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/ssi.c"
#endif

#ifndef MG_MAX_SSI_DEPTH
#  define MG_MAX_SSI_DEPTH 5
#endif

#ifndef MG_SSI_BUFSIZ
#  define MG_SSI_BUFSIZ 1024
#endif

#if MG_ENABLE_SSI
static char *
mg_ssi (const char *path, const char *root, int depth)
{
  struct mg_iobuf b = { NULL, 0, 0, MG_IO_SIZE };
  FILE *fp = fopen (path, "rb");
  if (fp != NULL)
    {
      char buf[MG_SSI_BUFSIZ], arg[sizeof (buf)];
      int ch, intag = 0;
      size_t len = 0;
      buf[0] = arg[0] = '\0';
      while ((ch = fgetc (fp)) != EOF)
	{
	  if (intag && ch == '>' && buf[len - 1] == '-' && buf[len - 2] == '-')
	    {
	      buf[len++] = (char) (ch & 0xff);
	      buf[len] = '\0';
	      if (sscanf (buf, "<!--#include file=\"%[^\"]", arg))
		{
		  char tmp[MG_PATH_MAX + MG_SSI_BUFSIZ + 10],
		      *p = (char *) path + strlen (path), *data;
		  while (p > path && p[-1] != MG_DIRSEP && p[-1] != '/')
		    p--;
		  mg_snprintf (tmp, sizeof (tmp), "%.*s%s", (int) (p - path),
			       path, arg);
		  if (depth < MG_MAX_SSI_DEPTH
		      && (data = mg_ssi (tmp, root, depth + 1)) != NULL)
		    {
		      mg_iobuf_add (&b, b.len, data, strlen (data));
		      free (data);
		    }
		  else
		    {
		      MG_ERROR (("%s: file=%s error or too deep", path, arg));
		    }
		}
	      else if (sscanf (buf, "<!--#include virtual=\"%[^\"]", arg))
		{
		  char tmp[MG_PATH_MAX + MG_SSI_BUFSIZ + 10], *data;
		  mg_snprintf (tmp, sizeof (tmp), "%s%s", root, arg);
		  if (depth < MG_MAX_SSI_DEPTH
		      && (data = mg_ssi (tmp, root, depth + 1)) != NULL)
		    {
		      mg_iobuf_add (&b, b.len, data, strlen (data));
		      free (data);
		    }
		  else
		    {
		      MG_ERROR (
			  ("%s: virtual=%s error or too deep", path, arg));
		    }
		}
	      else
		{
		  // Unknown SSI tag
		  MG_ERROR (("Unknown SSI tag: %.*s", (int) len, buf));
		  mg_iobuf_add (&b, b.len, buf, len);
		}
	      intag = 0;
	      len = 0;
	    }
	  else if (ch == '<')
	    {
	      intag = 1;
	      if (len > 0)
		mg_iobuf_add (&b, b.len, buf, len);
	      len = 0;
	      buf[len++] = (char) (ch & 0xff);
	    }
	  else if (intag)
	    {
	      if (len == 5 && strncmp (buf, "<!--#", 5) != 0)
		{
		  intag = 0;
		}
	      else if (len >= sizeof (buf) - 2)
		{
		  MG_ERROR (("%s: SSI tag is too large", path));
		  len = 0;
		}
	      buf[len++] = (char) (ch & 0xff);
	    }
	  else
	    {
	      buf[len++] = (char) (ch & 0xff);
	      if (len >= sizeof (buf))
		{
		  mg_iobuf_add (&b, b.len, buf, len);
		  len = 0;
		}
	    }
	}
      if (len > 0)
	mg_iobuf_add (&b, b.len, buf, len);
      if (b.len > 0)
	mg_iobuf_add (&b, b.len, "", 1); // nul-terminate
      fclose (fp);
    }
  (void) depth;
  (void) root;
  return (char *) b.buf;
}

void
mg_http_serve_ssi (struct mg_connection *c, const char *root,
		   const char *fullpath)
{
  const char *headers = "Content-Type: text/html; charset=utf-8\r\n";
  char *data = mg_ssi (fullpath, root, 0);
  mg_http_reply (c, 200, headers, "%s", data == NULL ? "" : data);
  free (data);
}
#else
void
mg_http_serve_ssi (struct mg_connection *c, const char *root,
		   const char *fullpath)
{
  mg_http_reply (c, 501, NULL, "SSI not enabled");
  (void) root, (void) fullpath;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/str.c"
#endif

struct mg_str
mg_str_s (const char *s)
{
  struct mg_str str = { (char *) s, s == NULL ? 0 : strlen (s) };
  return str;
}

struct mg_str
mg_str_n (const char *s, size_t n)
{
  struct mg_str str = { (char *) s, n };
  return str;
}

static int
mg_tolc (char c)
{
  return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A' : c;
}

int
mg_casecmp (const char *s1, const char *s2)
{
  int diff = 0;
  do
    {
      int c = mg_tolc (*s1++), d = mg_tolc (*s2++);
      diff = c - d;
    }
  while (diff == 0 && s1[-1] != '\0');
  return diff;
}

int
mg_strcmp (const struct mg_str str1, const struct mg_str str2)
{
  size_t i = 0;
  while (i < str1.len && i < str2.len)
    {
      int c1 = str1.buf[i];
      int c2 = str2.buf[i];
      if (c1 < c2)
	return -1;
      if (c1 > c2)
	return 1;
      i++;
    }
  if (i < str1.len)
    return 1;
  if (i < str2.len)
    return -1;
  return 0;
}

int
mg_strcasecmp (const struct mg_str str1, const struct mg_str str2)
{
  size_t i = 0;
  while (i < str1.len && i < str2.len)
    {
      int c1 = mg_tolc (str1.buf[i]);
      int c2 = mg_tolc (str2.buf[i]);
      if (c1 < c2)
	return -1;
      if (c1 > c2)
	return 1;
      i++;
    }
  if (i < str1.len)
    return 1;
  if (i < str2.len)
    return -1;
  return 0;
}

bool
mg_match (struct mg_str s, struct mg_str p, struct mg_str *caps)
{
  size_t i = 0, j = 0, ni = 0, nj = 0;
  if (caps)
    caps->buf = NULL, caps->len = 0;
  while (i < p.len || j < s.len)
    {
      if (i < p.len && j < s.len && (p.buf[i] == '?' || s.buf[j] == p.buf[i]))
	{
	  if (caps == NULL)
	    {
	    }
	  else if (p.buf[i] == '?')
	    {
	      caps->buf = &s.buf[j], caps->len = 1;    // Finalize `?` cap
	      caps++, caps->buf = NULL, caps->len = 0; // Init next cap
	    }
	  else if (caps->buf != NULL && caps->len == 0)
	    {
	      caps->len
		  = (size_t) (&s.buf[j] - caps->buf);  // Finalize current cap
	      caps++, caps->len = 0, caps->buf = NULL; // Init next cap
	    }
	  i++, j++;
	}
      else if (i < p.len && (p.buf[i] == '*' || p.buf[i] == '#'))
	{
	  if (caps && !caps->buf)
	    caps->len = 0, caps->buf = &s.buf[j]; // Init cap
	  ni = i++, nj = j + 1;
	}
      else if (nj > 0 && nj <= s.len && (p.buf[ni] == '#' || s.buf[j] != '/'))
	{
	  i = ni, j = nj;
	  if (caps && caps->buf == NULL && caps->len == 0)
	    {
	      caps--, caps->len = 0; // Restart previous cap
	    }
	}
      else
	{
	  return false;
	}
    }
  if (caps && caps->buf && caps->len == 0)
    {
      caps->len = (size_t) (&s.buf[j] - caps->buf);
    }
  return true;
}

bool
mg_span (struct mg_str s, struct mg_str *a, struct mg_str *b, char sep)
{
  if (s.len == 0 || s.buf == NULL)
    {
      return false; // Empty string, nothing to span - fail
    }
  else
    {
      size_t len = 0;
      while (len < s.len && s.buf[len] != sep)
	len++; // Find separator
      if (a)
	*a = mg_str_n (s.buf, len); // Init a
      if (b)
	*b = mg_str_n (s.buf + len, s.len - len); // Init b
      if (b && len < s.len)
	b->buf++, b->len--; // Skip separator
      return true;
    }
}

bool
mg_str_to_num (struct mg_str str, int base, void *val, size_t val_len)
{
  size_t i = 0, ndigits = 0;
  uint64_t max = val_len == sizeof (uint8_t)	? 0xFF
		 : val_len == sizeof (uint16_t) ? 0xFFFF
		 : val_len == sizeof (uint32_t) ? 0xFFFFFFFF
						: (uint64_t) ~0;
  uint64_t result = 0;
  if (max == (uint64_t) ~0 && val_len != sizeof (uint64_t))
    return false;
  if (base == 0 && str.len >= 2)
    {
      if (str.buf[i] == '0')
	{
	  i++;
	  base = str.buf[i] == 'b' ? 2 : str.buf[i] == 'x' ? 16 : 10;
	  if (base != 10)
	    ++i;
	}
      else
	{
	  base = 10;
	}
    }
  switch (base)
    {
    case 2:
      while (i < str.len && (str.buf[i] == '0' || str.buf[i] == '1'))
	{
	  uint64_t digit = (uint64_t) (str.buf[i] - '0');
	  if (result > max / 2)
	    return false; // Overflow
	  result *= 2;
	  if (result > max - digit)
	    return false; // Overflow
	  result += digit;
	  i++, ndigits++;
	}
      break;
    case 10:
      while (i < str.len && str.buf[i] >= '0' && str.buf[i] <= '9')
	{
	  uint64_t digit = (uint64_t) (str.buf[i] - '0');
	  if (result > max / 10)
	    return false; // Overflow
	  result *= 10;
	  if (result > max - digit)
	    return false; // Overflow
	  result += digit;
	  i++, ndigits++;
	}
      break;
    case 16:
      while (i < str.len)
	{
	  char c = str.buf[i];
	  uint64_t digit = (c >= '0' && c <= '9')   ? (uint64_t) (c - '0')
			   : (c >= 'A' && c <= 'F') ? (uint64_t) (c - '7')
			   : (c >= 'a' && c <= 'f') ? (uint64_t) (c - 'W')
						    : (uint64_t) ~0;
	  if (digit == (uint64_t) ~0)
	    break;
	  if (result > max / 16)
	    return false; // Overflow
	  result *= 16;
	  if (result > max - digit)
	    return false; // Overflow
	  result += digit;
	  i++, ndigits++;
	}
      break;
    default:
      return false;
    }
  if (ndigits == 0)
    return false;
  if (i != str.len)
    return false;
  if (val_len == 1)
    {
      *((uint8_t *) val) = (uint8_t) result;
    }
  else if (val_len == 2)
    {
      *((uint16_t *) val) = (uint16_t) result;
    }
  else if (val_len == 4)
    {
      *((uint32_t *) val) = (uint32_t) result;
    }
  else
    {
      *((uint64_t *) val) = (uint64_t) result;
    }
  return true;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/timer.c"
#endif

#define MG_TIMER_CALLED 4

void
mg_timer_init (struct mg_timer **head, struct mg_timer *t, uint64_t ms,
	       unsigned flags, void (*fn) (void *), void *arg)
{
  t->id = 0, t->period_ms = ms, t->expire = 0;
  t->flags = flags, t->fn = fn, t->arg = arg, t->next = *head;
  *head = t;
}

void
mg_timer_free (struct mg_timer **head, struct mg_timer *t)
{
  while (*head && *head != t)
    head = &(*head)->next;
  if (*head)
    *head = t->next;
}

// t: expiration time, prd: period, now: current time. Return true if expired
bool
mg_timer_expired (uint64_t *t, uint64_t prd, uint64_t now)
{
  if (now + prd < *t)
    *t = 0; // Time wrapped? Reset timer
  if (*t == 0)
    *t = now + prd; // Firt poll? Set expiration
  if (*t > now)
    return false;				// Not expired yet, return
  *t = (now - *t) > prd ? now + prd : *t + prd; // Next expiration time
  return true;					// Expired, return true
}

void
mg_timer_poll (struct mg_timer **head, uint64_t now_ms)
{
  struct mg_timer *t, *tmp;
  for (t = *head; t != NULL; t = tmp)
    {
      bool once
	  = t->expire == 0 && (t->flags & MG_TIMER_RUN_NOW)
	    && !(t->flags & MG_TIMER_CALLED); // Handle MG_TIMER_NOW only once
      bool expired = mg_timer_expired (&t->expire, t->period_ms, now_ms);
      tmp = t->next;
      if (!once && !expired)
	continue;
      if ((t->flags & MG_TIMER_REPEAT) || !(t->flags & MG_TIMER_CALLED))
	{
	  t->fn (t->arg);
	}
      t->flags |= MG_TIMER_CALLED;
    }
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_aes128.c"
#endif
/******************************************************************************
 *
 * THIS SOURCE CODE IS HEREBY PLACED INTO THE PUBLIC DOMAIN FOR THE GOOD OF ALL
 *
 * This is a simple and straightforward implementation of the AES Rijndael
 * 128-bit block cipher designed by Vincent Rijmen and Joan Daemen. The focus
 * of this work was correctness & accuracy.  It is written in 'C' without any
 * particular focus upon optimization or speed. It should be endian (memory
 * byte order) neutral since the few places that care are handled explicitly.
 *
 * This implementation of Rijndael was created by Steven M. Gibson of GRC.com.
 *
 * It is intended for general purpose use, but was written in support of GRC's
 * reference implementation of the SQRL (Secure Quick Reliable Login) client.
 *
 * See:    http://csrc.nist.gov/archive/aes/rijndael/wsdindex.html
 *
 * NO COPYRIGHT IS CLAIMED IN THIS WORK, HOWEVER, NEITHER IS ANY WARRANTY MADE
 * REGARDING ITS FITNESS FOR ANY PARTICULAR PURPOSE. USE IT AT YOUR OWN RISK.
 *
 *******************************************************************************/

/******************************************************************************/
#define AES_DECRYPTION 1 // whether AES decryption is supported
/******************************************************************************/

#define MG_ENCRYPT 1 // specify whether we're encrypting
#define MG_DECRYPT 0 // or decrypting

#if MG_TLS == MG_TLS_BUILTIN
/******************************************************************************
 *  AES_INIT_KEYGEN_TABLES : MUST be called once before any AES use
 ******************************************************************************/
static void aes_init_keygen_tables (void);

/******************************************************************************
 *  AES_SETKEY : called to expand the key for encryption or decryption
 ******************************************************************************/
static int aes_setkey (aes_context *ctx, // pointer to context
		       int mode,	 // 1 or 0 for Encrypt/Decrypt
		       const uchar *key, // AES input key
		       uint keysize); // size in bytes (must be 16, 24, 32 for
				      // 128, 192 or 256-bit keys respectively)
				      // returns 0 for success

/******************************************************************************
 *  AES_CIPHER : called to encrypt or decrypt ONE 128-bit block of data
 ******************************************************************************/
static int aes_cipher (aes_context *ctx,      // pointer to context
		       const uchar input[16], // 128-bit block to en/decipher
		       uchar output[16]);     // 128-bit output result block
					      // returns 0 for success

/******************************************************************************
 *  GCM_CONTEXT : GCM context / holds keytables, instance data, and AES ctx
 ******************************************************************************/
typedef struct
{
  int mode;	       // cipher direction: encrypt/decrypt
  uint64_t len;	       // cipher data length processed so far
  uint64_t add_len;    // total add data length
  uint64_t HL[16];     // precalculated lo-half HTable
  uint64_t HH[16];     // precalculated hi-half HTable
  uchar base_ectr[16]; // first counter-mode cipher output for tag
  uchar y[16];	       // the current cipher-input IV|Counter value
  uchar buf[16];       // buf working value
  aes_context aes_ctx; // cipher context used
} gcm_context;

/******************************************************************************
 *  GCM_SETKEY : sets the GCM (and AES) keying material for use
 ******************************************************************************/
static int
gcm_setkey (gcm_context *ctx,  // caller-provided context ptr
	    const uchar *key,  // pointer to cipher key
	    const uint keysize // size in bytes (must be 16, 24, 32 for
			       // 128, 192 or 256-bit keys respectively)
);			       // returns 0 for success

/******************************************************************************
 *
 *  GCM_CRYPT_AND_TAG
 *
 *  This either encrypts or decrypts the user-provided data and, either
 *  way, generates an authentication tag of the requested length. It must be
 *  called with a GCM context whose key has already been set with GCM_SETKEY.
 *
 *  The user would typically call this explicitly to ENCRYPT a buffer of data
 *  and optional associated data, and produce its an authentication tag.
 *
 *  To reverse the process the user would typically call the companion
 *  GCM_AUTH_DECRYPT function to decrypt data and verify a user-provided
 *  authentication tag.  The GCM_AUTH_DECRYPT function calls this function
 *  to perform its decryption and tag generation, which it then compares.
 *
 ******************************************************************************/
static int gcm_crypt_and_tag (
    gcm_context *ctx,	// gcm context with key already setup
    int mode,		// cipher direction: MG_ENCRYPT (1) or MG_DECRYPT (0)
    const uchar *iv,	// pointer to the 12-byte initialization vector
    size_t iv_len,	// byte length if the IV. should always be 12
    const uchar *add,	// pointer to the non-ciphered additional data
    size_t add_len,	// byte length of the additional AEAD data
    const uchar *input, // pointer to the cipher data source
    uchar *output,	// pointer to the cipher data destination
    size_t length,	// byte length of the cipher data
    uchar *tag,		// pointer to the tag to be generated
    size_t tag_len);	// byte length of the tag to be generated

/******************************************************************************
 *
 *  GCM_START
 *
 *  Given a user-provided GCM context, this initializes it, sets the encryption
 *  mode, and preprocesses the initialization vector and additional AEAD data.
 *
 ******************************************************************************/
static int
gcm_start (gcm_context *ctx, // pointer to user-provided GCM context
	   int mode,	     // MG_ENCRYPT (1) or MG_DECRYPT (0)
	   const uchar *iv,  // pointer to initialization vector
	   size_t iv_len,    // IV length in bytes (should == 12)
	   const uchar *add, // pointer to additional AEAD data (NULL if none)
	   size_t add_len);  // length of additional AEAD data (bytes)

/******************************************************************************
 *
 *  GCM_UPDATE
 *
 *  This is called once or more to process bulk plaintext or ciphertext data.
 *  We give this some number of bytes of input and it returns the same number
 *  of output bytes. If called multiple times (which is fine) all but the final
 *  invocation MUST be called with length mod 16 == 0. (Only the final call can
 *  have a partial block length of < 128 bits.)
 *
 ******************************************************************************/
static int
gcm_update (gcm_context *ctx,	// pointer to user-provided GCM context
	    size_t length,	// length, in bytes, of data to process
	    const uchar *input, // pointer to source data
	    uchar *output);	// pointer to destination data

/******************************************************************************
 *
 *  GCM_FINISH
 *
 *  This is called once after all calls to GCM_UPDATE to finalize the GCM.
 *  It performs the final GHASH to produce the resulting authentication TAG.
 *
 ******************************************************************************/
static int
gcm_finish (gcm_context *ctx, // pointer to user-provided GCM context
	    uchar *tag,	      // ptr to tag buffer - NULL if tag_len = 0
	    size_t tag_len);  // length, in bytes, of the tag-receiving buf

/******************************************************************************
 *
 *  GCM_ZERO_CTX
 *
 *  The GCM context contains both the GCM context and the AES context.
 *  This includes keying and key-related material which is security-
 *  sensitive, so it MUST be zeroed after use. This function does that.
 *
 ******************************************************************************/
static void gcm_zero_ctx (gcm_context *ctx);

/******************************************************************************
 *
 * THIS SOURCE CODE IS HEREBY PLACED INTO THE PUBLIC DOMAIN FOR THE GOOD OF ALL
 *
 * This is a simple and straightforward implementation of the AES Rijndael
 * 128-bit block cipher designed by Vincent Rijmen and Joan Daemen. The focus
 * of this work was correctness & accuracy.  It is written in 'C' without any
 * particular focus upon optimization or speed. It should be endian (memory
 * byte order) neutral since the few places that care are handled explicitly.
 *
 * This implementation of Rijndael was created by Steven M. Gibson of GRC.com.
 *
 * It is intended for general purpose use, but was written in support of GRC's
 * reference implementation of the SQRL (Secure Quick Reliable Login) client.
 *
 * See:    http://csrc.nist.gov/archive/aes/rijndael/wsdindex.html
 *
 * NO COPYRIGHT IS CLAIMED IN THIS WORK, HOWEVER, NEITHER IS ANY WARRANTY MADE
 * REGARDING ITS FITNESS FOR ANY PARTICULAR PURPOSE. USE IT AT YOUR OWN RISK.
 *
 *******************************************************************************/

static int aes_tables_inited = 0; // run-once flag for performing key
				  // expasion table generation (see below)
/*
 *  The following static local tables must be filled-in before the first use of
 *  the GCM or AES ciphers. They are used for the AES key expansion/scheduling
 *  and once built are read-only and thread safe. The "gcm_initialize" function
 *  must be called once during system initialization to populate these arrays
 *  for subsequent use by the AES key scheduler. If they have not been built
 *  before attempted use, an error will be returned to the caller.
 *
 *  NOTE: GCM Encryption/Decryption does NOT REQUIRE AES decryption. Since
 *  GCM uses AES in counter-mode, where the AES cipher output is XORed with
 *  the GCM input, we ONLY NEED AES encryption.  Thus, to save space AES
 *  decryption is typically disabled by setting AES_DECRYPTION to 0 in aes.h.
 */
// We always need our forward tables
static uchar FSb[256];	  // Forward substitution box (FSb)
static uint32_t FT0[256]; // Forward key schedule assembly tables
static uint32_t FT1[256];
static uint32_t FT2[256];
static uint32_t FT3[256];

#  if AES_DECRYPTION	  // We ONLY need reverse for decryption
static uchar RSb[256];	  // Reverse substitution box (RSb)
static uint32_t RT0[256]; // Reverse key schedule assembly tables
static uint32_t RT1[256];
static uint32_t RT2[256];
static uint32_t RT3[256];
#  endif /* AES_DECRYPTION */

static uint32_t RCON[10]; // AES round constants

/*
 * Platform Endianness Neutralizing Load and Store Macro definitions
 * AES wants platform-neutral Little Endian (LE) byte ordering
 */
#  define GET_UINT32_LE(n, b, i)                                              \
    {                                                                         \
      (n) = ((uint32_t) (b)[(i)]) | ((uint32_t) (b)[(i) + 1] << 8)            \
	    | ((uint32_t) (b)[(i) + 2] << 16)                                 \
	    | ((uint32_t) (b)[(i) + 3] << 24);                                \
    }

#  define PUT_UINT32_LE(n, b, i)                                              \
    {                                                                         \
      (b)[(i)] = (uchar) ((n));                                               \
      (b)[(i) + 1] = (uchar) ((n) >> 8);                                      \
      (b)[(i) + 2] = (uchar) ((n) >> 16);                                     \
      (b)[(i) + 3] = (uchar) ((n) >> 24);                                     \
    }

/*
 *  AES forward and reverse encryption round processing macros
 */
#  define AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3)                          \
    {                                                                         \
      X0 = *RK++ ^ FT0[(Y0) & 0xFF] ^ FT1[(Y1 >> 8) & 0xFF]                   \
	   ^ FT2[(Y2 >> 16) & 0xFF] ^ FT3[(Y3 >> 24) & 0xFF];                 \
                                                                              \
      X1 = *RK++ ^ FT0[(Y1) & 0xFF] ^ FT1[(Y2 >> 8) & 0xFF]                   \
	   ^ FT2[(Y3 >> 16) & 0xFF] ^ FT3[(Y0 >> 24) & 0xFF];                 \
                                                                              \
      X2 = *RK++ ^ FT0[(Y2) & 0xFF] ^ FT1[(Y3 >> 8) & 0xFF]                   \
	   ^ FT2[(Y0 >> 16) & 0xFF] ^ FT3[(Y1 >> 24) & 0xFF];                 \
                                                                              \
      X3 = *RK++ ^ FT0[(Y3) & 0xFF] ^ FT1[(Y0 >> 8) & 0xFF]                   \
	   ^ FT2[(Y1 >> 16) & 0xFF] ^ FT3[(Y2 >> 24) & 0xFF];                 \
    }

#  define AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3)                          \
    {                                                                         \
      X0 = *RK++ ^ RT0[(Y0) & 0xFF] ^ RT1[(Y3 >> 8) & 0xFF]                   \
	   ^ RT2[(Y2 >> 16) & 0xFF] ^ RT3[(Y1 >> 24) & 0xFF];                 \
                                                                              \
      X1 = *RK++ ^ RT0[(Y1) & 0xFF] ^ RT1[(Y0 >> 8) & 0xFF]                   \
	   ^ RT2[(Y3 >> 16) & 0xFF] ^ RT3[(Y2 >> 24) & 0xFF];                 \
                                                                              \
      X2 = *RK++ ^ RT0[(Y2) & 0xFF] ^ RT1[(Y1 >> 8) & 0xFF]                   \
	   ^ RT2[(Y0 >> 16) & 0xFF] ^ RT3[(Y3 >> 24) & 0xFF];                 \
                                                                              \
      X3 = *RK++ ^ RT0[(Y3) & 0xFF] ^ RT1[(Y2 >> 8) & 0xFF]                   \
	   ^ RT2[(Y1 >> 16) & 0xFF] ^ RT3[(Y0 >> 24) & 0xFF];                 \
    }

/*
 *  These macros improve the readability of the key
 *  generation initialization code by collapsing
 *  repetitive common operations into logical pieces.
 */
#  define ROTL8(x) ((x << 8) & 0xFFFFFFFF) | (x >> 24)
#  define XTIME(x) ((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00))
#  define MUL(x, y) ((x && y) ? pow[(log[x] + log[y]) % 255] : 0)
#  define MIX(x, y)                                                           \
    {                                                                         \
      y = ((y << 1) | (y >> 7)) & 0xFF;                                       \
      x ^= y;                                                                 \
    }
#  define CPY128                                                              \
    {                                                                         \
      *RK++ = *SK++;                                                          \
      *RK++ = *SK++;                                                          \
      *RK++ = *SK++;                                                          \
      *RK++ = *SK++;                                                          \
    }

/******************************************************************************
 *
 *  AES_INIT_KEYGEN_TABLES
 *
 *  Fills the AES key expansion tables allocated above with their static
 *  data. This is not "per key" data, but static system-wide read-only
 *  table data. THIS FUNCTION IS NOT THREAD SAFE. It must be called once
 *  at system initialization to setup the tables for all subsequent use.
 *
 ******************************************************************************/
void
aes_init_keygen_tables (void)
{
  int i, x, y, z; // general purpose iteration and computation locals
  int pow[256];
  int log[256];

  if (aes_tables_inited)
    return;

  // fill the 'pow' and 'log' tables over GF(2^8)
  for (i = 0, x = 1; i < 256; i++)
    {
      pow[i] = x;
      log[x] = i;
      x = (x ^ XTIME (x)) & 0xFF;
    }
  // compute the round constants
  for (i = 0, x = 1; i < 10; i++)
    {
      RCON[i] = (uint32_t) x;
      x = XTIME (x) & 0xFF;
    }
  // fill the forward and reverse substitution boxes
  FSb[0x00] = 0x63;
#  if AES_DECRYPTION // whether AES decryption is supported
  RSb[0x63] = 0x00;
#  endif /* AES_DECRYPTION */

  for (i = 1; i < 256; i++)
    {
      x = y = pow[255 - log[i]];
      MIX (x, y);
      MIX (x, y);
      MIX (x, y);
      MIX (x, y);
      FSb[i] = (uchar) (x ^= 0x63);
#  if AES_DECRYPTION // whether AES decryption is supported
      RSb[x] = (uchar) i;
#  endif /* AES_DECRYPTION */
    }
  // generate the forward and reverse key expansion tables
  for (i = 0; i < 256; i++)
    {
      x = FSb[i];
      y = XTIME (x) & 0xFF;
      z = (y ^ x) & 0xFF;

      FT0[i] = ((uint32_t) y) ^ ((uint32_t) x << 8) ^ ((uint32_t) x << 16)
	       ^ ((uint32_t) z << 24);

      FT1[i] = ROTL8 (FT0[i]);
      FT2[i] = ROTL8 (FT1[i]);
      FT3[i] = ROTL8 (FT2[i]);

#  if AES_DECRYPTION // whether AES decryption is supported
      x = RSb[i];

      RT0[i] = ((uint32_t) MUL (0x0E, x)) ^ ((uint32_t) MUL (0x09, x) << 8)
	       ^ ((uint32_t) MUL (0x0D, x) << 16)
	       ^ ((uint32_t) MUL (0x0B, x) << 24);

      RT1[i] = ROTL8 (RT0[i]);
      RT2[i] = ROTL8 (RT1[i]);
      RT3[i] = ROTL8 (RT2[i]);
#  endif /* AES_DECRYPTION */
    }
  aes_tables_inited = 1; // flag that the tables have been generated
} // to permit subsequent use of the AES cipher

/******************************************************************************
 *
 *  AES_SET_ENCRYPTION_KEY
 *
 *  This is called by 'aes_setkey' when we're establishing a key for
 *  subsequent encryption.  We give it a pointer to the encryption
 *  context, a pointer to the key, and the key's length in bytes.
 *  Valid lengths are: 16, 24 or 32 bytes (128, 192, 256 bits).
 *
 ******************************************************************************/
static int
aes_set_encryption_key (aes_context *ctx, const uchar *key, uint keysize)
{
  uint i;		  // general purpose iteration local
  uint32_t *RK = ctx->rk; // initialize our RoundKey buffer pointer

  for (i = 0; i < (keysize >> 2); i++)
    {
      GET_UINT32_LE (RK[i], key, i << 2);
    }

  switch (ctx->rounds)
    {
    case 10:
      for (i = 0; i < 10; i++, RK += 4)
	{
	  RK[4] = RK[0] ^ RCON[i] ^ ((uint32_t) FSb[(RK[3] >> 8) & 0xFF])
		  ^ ((uint32_t) FSb[(RK[3] >> 16) & 0xFF] << 8)
		  ^ ((uint32_t) FSb[(RK[3] >> 24) & 0xFF] << 16)
		  ^ ((uint32_t) FSb[(RK[3]) & 0xFF] << 24);

	  RK[5] = RK[1] ^ RK[4];
	  RK[6] = RK[2] ^ RK[5];
	  RK[7] = RK[3] ^ RK[6];
	}
      break;

    case 12:
      for (i = 0; i < 8; i++, RK += 6)
	{
	  RK[6] = RK[0] ^ RCON[i] ^ ((uint32_t) FSb[(RK[5] >> 8) & 0xFF])
		  ^ ((uint32_t) FSb[(RK[5] >> 16) & 0xFF] << 8)
		  ^ ((uint32_t) FSb[(RK[5] >> 24) & 0xFF] << 16)
		  ^ ((uint32_t) FSb[(RK[5]) & 0xFF] << 24);

	  RK[7] = RK[1] ^ RK[6];
	  RK[8] = RK[2] ^ RK[7];
	  RK[9] = RK[3] ^ RK[8];
	  RK[10] = RK[4] ^ RK[9];
	  RK[11] = RK[5] ^ RK[10];
	}
      break;

    case 14:
      for (i = 0; i < 7; i++, RK += 8)
	{
	  RK[8] = RK[0] ^ RCON[i] ^ ((uint32_t) FSb[(RK[7] >> 8) & 0xFF])
		  ^ ((uint32_t) FSb[(RK[7] >> 16) & 0xFF] << 8)
		  ^ ((uint32_t) FSb[(RK[7] >> 24) & 0xFF] << 16)
		  ^ ((uint32_t) FSb[(RK[7]) & 0xFF] << 24);

	  RK[9] = RK[1] ^ RK[8];
	  RK[10] = RK[2] ^ RK[9];
	  RK[11] = RK[3] ^ RK[10];

	  RK[12] = RK[4] ^ ((uint32_t) FSb[(RK[11]) & 0xFF])
		   ^ ((uint32_t) FSb[(RK[11] >> 8) & 0xFF] << 8)
		   ^ ((uint32_t) FSb[(RK[11] >> 16) & 0xFF] << 16)
		   ^ ((uint32_t) FSb[(RK[11] >> 24) & 0xFF] << 24);

	  RK[13] = RK[5] ^ RK[12];
	  RK[14] = RK[6] ^ RK[13];
	  RK[15] = RK[7] ^ RK[14];
	}
      break;

    default:
      return -1;
    }
  return (0);
}

#  if AES_DECRYPTION // whether AES decryption is supported

/******************************************************************************
 *
 *  AES_SET_DECRYPTION_KEY
 *
 *  This is called by 'aes_setkey' when we're establishing a
 *  key for subsequent decryption.  We give it a pointer to
 *  the encryption context, a pointer to the key, and the key's
 *  length in bits. Valid lengths are: 128, 192, or 256 bits.
 *
 ******************************************************************************/
static int
aes_set_decryption_key (aes_context *ctx, const uchar *key, uint keysize)
{
  int i, j;
  aes_context cty;	  // a calling aes context for set_encryption_key
  uint32_t *RK = ctx->rk; // initialize our RoundKey buffer pointer
  uint32_t *SK;
  int ret;

  cty.rounds = ctx->rounds; // initialize our local aes context
  cty.rk = cty.buf;	    // round count and key buf pointer

  if ((ret = aes_set_encryption_key (&cty, key, keysize)) != 0)
    return (ret);

  SK = cty.rk + cty.rounds * 4;

  CPY128 // copy a 128-bit block from *SK to *RK

      for (i = ctx->rounds - 1, SK -= 8; i > 0; i--, SK -= 8)
  {
    for (j = 0; j < 4; j++, SK++)
      {
	*RK++ = RT0[FSb[(*SK) & 0xFF]] ^ RT1[FSb[(*SK >> 8) & 0xFF]]
		^ RT2[FSb[(*SK >> 16) & 0xFF]] ^ RT3[FSb[(*SK >> 24) & 0xFF]];
      }
  }
  CPY128 // copy a 128-bit block from *SK to *RK
      memset (&cty, 0, sizeof (aes_context)); // clear local aes context
  return (0);
}

#  endif /* AES_DECRYPTION */

/******************************************************************************
 *
 *  AES_SETKEY
 *
 *  Invoked to establish the key schedule for subsequent encryption/decryption
 *
 ******************************************************************************/
static int
aes_setkey (aes_context *ctx, // AES context provided by our caller
	    int mode,	      // ENCRYPT or DECRYPT flag
	    const uchar *key, // pointer to the key
	    uint keysize)     // key length in bytes
{
  // since table initialization is not thread safe, we could either add
  // system-specific mutexes and init the AES key generation tables on
  // demand, or ask the developer to simply call "gcm_initialize" once during
  // application startup before threading begins. That's what we choose.
  if (!aes_tables_inited)
    return (-1); // fail the call when not inited.

  ctx->mode = mode;   // capture the key type we're creating
  ctx->rk = ctx->buf; // initialize our round key pointer

  switch (keysize) // set the rounds count based upon the keysize
    {
    case 16:
      ctx->rounds = 10;
      break; // 16-byte, 128-bit key
    case 24:
      ctx->rounds = 12;
      break; // 24-byte, 192-bit key
    case 32:
      ctx->rounds = 14;
      break; // 32-byte, 256-bit key
    default:
      return (-1);
    }

#  if AES_DECRYPTION
  if (mode == MG_DECRYPT) // expand our key for encryption or decryption
    return (aes_set_decryption_key (ctx, key, keysize));
  else	 /* MG_ENCRYPT */
#  endif /* AES_DECRYPTION */
    return (aes_set_encryption_key (ctx, key, keysize));
}

/******************************************************************************
 *
 *  AES_CIPHER
 *
 *  Perform AES encryption and decryption.
 *  The AES context will have been setup with the encryption mode
 *  and all keying information appropriate for the task.
 *
 ******************************************************************************/
static int
aes_cipher (aes_context *ctx, const uchar input[16], uchar output[16])
{
  int i;
  uint32_t *RK, X0, X1, X2, X3, Y0, Y1, Y2, Y3; // general purpose locals

  RK = ctx->rk;

  GET_UINT32_LE (X0, input, 0);
  X0 ^= *RK++; // load our 128-bit
  GET_UINT32_LE (X1, input, 4);
  X1 ^= *RK++; // input buffer in a storage
  GET_UINT32_LE (X2, input, 8);
  X2 ^= *RK++; // memory endian-neutral way
  GET_UINT32_LE (X3, input, 12);
  X3 ^= *RK++;

#  if AES_DECRYPTION // whether AES decryption is supported

  if (ctx->mode == MG_DECRYPT)
    {
      for (i = (ctx->rounds >> 1) - 1; i > 0; i--)
	{
	  AES_RROUND (Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	  AES_RROUND (X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	}

      AES_RROUND (Y0, Y1, Y2, Y3, X0, X1, X2, X3);

      X0 = *RK++ ^ ((uint32_t) RSb[(Y0) & 0xFF])
	   ^ ((uint32_t) RSb[(Y3 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) RSb[(Y2 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) RSb[(Y1 >> 24) & 0xFF] << 24);

      X1 = *RK++ ^ ((uint32_t) RSb[(Y1) & 0xFF])
	   ^ ((uint32_t) RSb[(Y0 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) RSb[(Y3 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) RSb[(Y2 >> 24) & 0xFF] << 24);

      X2 = *RK++ ^ ((uint32_t) RSb[(Y2) & 0xFF])
	   ^ ((uint32_t) RSb[(Y1 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) RSb[(Y0 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) RSb[(Y3 >> 24) & 0xFF] << 24);

      X3 = *RK++ ^ ((uint32_t) RSb[(Y3) & 0xFF])
	   ^ ((uint32_t) RSb[(Y2 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) RSb[(Y1 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) RSb[(Y0 >> 24) & 0xFF] << 24);
    }
  else /* MG_ENCRYPT */
    {
#  endif /* AES_DECRYPTION */

      for (i = (ctx->rounds >> 1) - 1; i > 0; i--)
	{
	  AES_FROUND (Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	  AES_FROUND (X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	}

      AES_FROUND (Y0, Y1, Y2, Y3, X0, X1, X2, X3);

      X0 = *RK++ ^ ((uint32_t) FSb[(Y0) & 0xFF])
	   ^ ((uint32_t) FSb[(Y1 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) FSb[(Y2 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) FSb[(Y3 >> 24) & 0xFF] << 24);

      X1 = *RK++ ^ ((uint32_t) FSb[(Y1) & 0xFF])
	   ^ ((uint32_t) FSb[(Y2 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) FSb[(Y3 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) FSb[(Y0 >> 24) & 0xFF] << 24);

      X2 = *RK++ ^ ((uint32_t) FSb[(Y2) & 0xFF])
	   ^ ((uint32_t) FSb[(Y3 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) FSb[(Y0 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) FSb[(Y1 >> 24) & 0xFF] << 24);

      X3 = *RK++ ^ ((uint32_t) FSb[(Y3) & 0xFF])
	   ^ ((uint32_t) FSb[(Y0 >> 8) & 0xFF] << 8)
	   ^ ((uint32_t) FSb[(Y1 >> 16) & 0xFF] << 16)
	   ^ ((uint32_t) FSb[(Y2 >> 24) & 0xFF] << 24);

#  if AES_DECRYPTION // whether AES decryption is supported
    }
#  endif /* AES_DECRYPTION */

  PUT_UINT32_LE (X0, output, 0);
  PUT_UINT32_LE (X1, output, 4);
  PUT_UINT32_LE (X2, output, 8);
  PUT_UINT32_LE (X3, output, 12);

  return (0);
}
/* end of aes.c */
/******************************************************************************
 *
 * THIS SOURCE CODE IS HEREBY PLACED INTO THE PUBLIC DOMAIN FOR THE GOOD OF ALL
 *
 * This is a simple and straightforward implementation of AES-GCM authenticated
 * encryption. The focus of this work was correctness & accuracy. It is written
 * in straight 'C' without any particular focus upon optimization or speed. It
 * should be endian (memory byte order) neutral since the few places that care
 * are handled explicitly.
 *
 * This implementation of AES-GCM was created by Steven M. Gibson of GRC.com.
 *
 * It is intended for general purpose use, but was written in support of GRC's
 * reference implementation of the SQRL (Secure Quick Reliable Login) client.
 *
 * See:    http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
 *         http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/
 *         gcm/gcm-revised-spec.pdf
 *
 * NO COPYRIGHT IS CLAIMED IN THIS WORK, HOWEVER, NEITHER IS ANY WARRANTY MADE
 * REGARDING ITS FITNESS FOR ANY PARTICULAR PURPOSE. USE IT AT YOUR OWN RISK.
 *
 *******************************************************************************/

/******************************************************************************
 *                      ==== IMPLEMENTATION WARNING ====
 *
 *  This code was developed for use within SQRL's fixed environmnent. Thus, it
 *  is somewhat less "general purpose" than it would be if it were designed as
 *  a general purpose AES-GCM library. Specifically, it bothers with almost NO
 *  error checking on parameter limits, buffer bounds, etc. It assumes that it
 *  is being invoked by its author or by someone who understands the values it
 *  expects to receive. Its behavior will be undefined otherwise.
 *
 *  All functions that might fail are defined to return 'ints' to indicate a
 *  problem. Most do not do so now. But this allows for error propagation out
 *  of internal functions if robust error checking should ever be desired.
 *
 ******************************************************************************/

/* Calculating the "GHASH"
 *
 * There are many ways of calculating the so-called GHASH in software, each
 * with a traditional size vs performance tradeoff.  The GHASH (Galois field
 * hash) is an intriguing construction which takes two 128-bit strings (also
 * the cipher's block size and the fundamental operation size for the system)
 * and hashes them into a third 128-bit result.
 *
 * Many implementation solutions have been worked out that use large
 * precomputed table lookups in place of more time consuming bit fiddling, and
 * this approach can be scaled easily upward or downward as needed to change
 * the time/space tradeoff. It's been studied extensively and there's a solid
 * body of theory and practice.  For example, without using any lookup tables
 * an implementation might obtain 119 cycles per byte throughput, whereas using
 * a simple, though large, key-specific 64 kbyte 8-bit lookup table the
 * performance jumps to 13 cycles per byte.
 *
 * And Intel's processors have, since 2010, included an instruction which does
 * the entire 128x128->128 bit job in just several 64x64->128 bit pieces.
 *
 * Since SQRL is interactive, and only processing a few 128-bit blocks, I've
 * settled upon a relatively slower but appealing small-table compromise which
 * folds a bunch of not only time consuming but also bit twiddling into a
 * simple 16-entry table which is attributed to Victor Shoup's 1996 work while
 * at Bellcore: "On Fast and Provably Secure MessageAuthentication Based on
 * Universal Hashing."  See: http://www.shoup.net/papers/macs.pdf
 * See, also section 4.1 of the "gcm-revised-spec" cited above.
 */

/*
 *  This 16-entry table of pre-computed constants is used by the
 *  GHASH multiplier to improve over a strictly table-free but
 *  significantly slower 128x128 bit multiple within GF(2^128).
 */
static const uint64_t last4[16]
    = { 0x0000, 0x1c20, 0x3840, 0x2460, 0x7080, 0x6ca0, 0x48c0, 0x54e0,
	0xe100, 0xfd20, 0xd940, 0xc560, 0x9180, 0x8da0, 0xa9c0, 0xb5e0 };

/*
 * Platform Endianness Neutralizing Load and Store Macro definitions
 * GCM wants platform-neutral Big Endian (BE) byte ordering
 */
#  define GET_UINT32_BE(n, b, i)                                              \
    {                                                                         \
      (n) = ((uint32_t) (b)[(i)] << 24) | ((uint32_t) (b)[(i) + 1] << 16)     \
	    | ((uint32_t) (b)[(i) + 2] << 8) | ((uint32_t) (b)[(i) + 3]);     \
    }

#  define PUT_UINT32_BE(n, b, i)                                              \
    {                                                                         \
      (b)[(i)] = (uchar) ((n) >> 24);                                         \
      (b)[(i) + 1] = (uchar) ((n) >> 16);                                     \
      (b)[(i) + 2] = (uchar) ((n) >> 8);                                      \
      (b)[(i) + 3] = (uchar) ((n));                                           \
    }

/******************************************************************************
 *
 *  GCM_INITIALIZE
 *
 *  Must be called once to initialize the GCM library.
 *
 *  At present, this only calls the AES keygen table generator, which expands
 *  the AES keying tables for use. This is NOT A THREAD-SAFE function, so it
 *  MUST be called during system initialization before a multi-threading
 *  environment is running.
 *
 ******************************************************************************/
int
mg_gcm_initialize (void)
{
  aes_init_keygen_tables ();
  return (0);
}

/******************************************************************************
 *
 *  GCM_MULT
 *
 *  Performs a GHASH operation on the 128-bit input vector 'x', setting
 *  the 128-bit output vector to 'x' times H using our precomputed tables.
 *  'x' and 'output' are seen as elements of GCM's GF(2^128) Galois field.
 *
 ******************************************************************************/
static void
gcm_mult (gcm_context *ctx,  // pointer to established context
	  const uchar x[16], // pointer to 128-bit input vector
	  uchar output[16])  // pointer to 128-bit output vector
{
  int i;
  uchar lo, hi, rem;
  uint64_t zh, zl;

  lo = (uchar) (x[15] & 0x0f);
  hi = (uchar) (x[15] >> 4);
  zh = ctx->HH[lo];
  zl = ctx->HL[lo];

  for (i = 15; i >= 0; i--)
    {
      lo = (uchar) (x[i] & 0x0f);
      hi = (uchar) (x[i] >> 4);

      if (i != 15)
	{
	  rem = (uchar) (zl & 0x0f);
	  zl = (zh << 60) | (zl >> 4);
	  zh = (zh >> 4);
	  zh ^= (uint64_t) last4[rem] << 48;
	  zh ^= ctx->HH[lo];
	  zl ^= ctx->HL[lo];
	}
      rem = (uchar) (zl & 0x0f);
      zl = (zh << 60) | (zl >> 4);
      zh = (zh >> 4);
      zh ^= (uint64_t) last4[rem] << 48;
      zh ^= ctx->HH[hi];
      zl ^= ctx->HL[hi];
    }
  PUT_UINT32_BE (zh >> 32, output, 0);
  PUT_UINT32_BE (zh, output, 4);
  PUT_UINT32_BE (zl >> 32, output, 8);
  PUT_UINT32_BE (zl, output, 12);
}

/******************************************************************************
 *
 *  GCM_SETKEY
 *
 *  This is called to set the AES-GCM key. It initializes the AES key
 *  and populates the gcm context's pre-calculated HTables.
 *
 ******************************************************************************/
static int
gcm_setkey (gcm_context *ctx,	// pointer to caller-provided gcm context
	    const uchar *key,	// pointer to the AES encryption key
	    const uint keysize) // size in bytes (must be 16, 24, 32 for
				// 128, 192 or 256-bit keys respectively)
{
  int ret, i, j;
  uint64_t hi, lo;
  uint64_t vl, vh;
  unsigned char h[16];

  memset (ctx, 0, sizeof (gcm_context)); // zero caller-provided GCM context
  memset (h, 0, 16);			 // initialize the block to encrypt

  // encrypt the null 128-bit block to generate a key-based value
  // which is then used to initialize our GHASH lookup tables
  if ((ret = aes_setkey (&ctx->aes_ctx, MG_ENCRYPT, key, keysize)) != 0)
    return (ret);
  if ((ret = aes_cipher (&ctx->aes_ctx, h, h)) != 0)
    return (ret);

  GET_UINT32_BE (hi, h, 0); // pack h as two 64-bit ints, big-endian
  GET_UINT32_BE (lo, h, 4);
  vh = (uint64_t) hi << 32 | lo;

  GET_UINT32_BE (hi, h, 8);
  GET_UINT32_BE (lo, h, 12);
  vl = (uint64_t) hi << 32 | lo;

  ctx->HL[8] = vl; // 8 = 1000 corresponds to 1 in GF(2^128)
  ctx->HH[8] = vh;
  ctx->HH[0] = 0; // 0 corresponds to 0 in GF(2^128)
  ctx->HL[0] = 0;

  for (i = 4; i > 0; i >>= 1)
    {
      uint32_t T = (uint32_t) (vl & 1) * 0xe1000000U;
      vl = (vh << 63) | (vl >> 1);
      vh = (vh >> 1) ^ ((uint64_t) T << 32);
      ctx->HL[i] = vl;
      ctx->HH[i] = vh;
    }
  for (i = 2; i < 16; i <<= 1)
    {
      uint64_t *HiL = ctx->HL + i, *HiH = ctx->HH + i;
      vh = *HiH;
      vl = *HiL;
      for (j = 1; j < i; j++)
	{
	  HiH[j] = vh ^ ctx->HH[j];
	  HiL[j] = vl ^ ctx->HL[j];
	}
    }
  return (0);
}

/******************************************************************************
 *
 *    GCM processing occurs four phases: SETKEY, START, UPDATE and FINISH.
 *
 *  SETKEY:
 *
 *   START: Sets the Encryption/Decryption mode.
 *          Accepts the initialization vector and additional data.
 *
 *  UPDATE: Encrypts or decrypts the plaintext or ciphertext.
 *
 *  FINISH: Performs a final GHASH to generate the authentication tag.
 *
 ******************************************************************************
 *
 *  GCM_START
 *
 *  Given a user-provided GCM context, this initializes it, sets the encryption
 *  mode, and preprocesses the initialization vector and additional AEAD data.
 *
 ******************************************************************************/
int
gcm_start (gcm_context *ctx, // pointer to user-provided GCM context
	   int mode,	     // GCM_ENCRYPT or GCM_DECRYPT
	   const uchar *iv,  // pointer to initialization vector
	   size_t iv_len,    // IV length in bytes (should == 12)
	   const uchar *add, // ptr to additional AEAD data (NULL if none)
	   size_t add_len)   // length of additional AEAD data (bytes)
{
  int ret;	      // our error return if the AES encrypt fails
  uchar work_buf[16]; // XOR source built from provided IV if len != 16
  const uchar *p;     // general purpose array pointer
  size_t use_len;     // byte count to process, up to 16 bytes
  size_t i;	      // local loop iterator

  // since the context might be reused under the same key
  // we zero the working buffers for this next new process
  memset (ctx->y, 0x00, sizeof (ctx->y));
  memset (ctx->buf, 0x00, sizeof (ctx->buf));
  ctx->len = 0;
  ctx->add_len = 0;

  ctx->mode = mode;		  // set the GCM encryption/decryption mode
  ctx->aes_ctx.mode = MG_ENCRYPT; // GCM *always* runs AES in ENCRYPTION mode

  if (iv_len == 12)
    {				   // GCM natively uses a 12-byte, 96-bit IV
      memcpy (ctx->y, iv, iv_len); // copy the IV to the top of the 'y' buff
      ctx->y[15] = 1;		   // start "counting" from 1 (not 0)
    }
  else // if we don't have a 12-byte IV, we GHASH whatever we've been given
    {
      memset (work_buf, 0x00, 16);		// clear the working buffer
      PUT_UINT32_BE (iv_len * 8, work_buf, 12); // place the IV into buffer

      p = iv;
      while (iv_len > 0)
	{
	  use_len = (iv_len < 16) ? iv_len : 16;
	  for (i = 0; i < use_len; i++)
	    ctx->y[i] ^= p[i];
	  gcm_mult (ctx, ctx->y, ctx->y);
	  iv_len -= use_len;
	  p += use_len;
	}
      for (i = 0; i < 16; i++)
	ctx->y[i] ^= work_buf[i];
      gcm_mult (ctx, ctx->y, ctx->y);
    }
  if ((ret = aes_cipher (&ctx->aes_ctx, ctx->y, ctx->base_ectr)) != 0)
    return (ret);

  ctx->add_len = add_len;
  p = add;
  while (add_len > 0)
    {
      use_len = (add_len < 16) ? add_len : 16;
      for (i = 0; i < use_len; i++)
	ctx->buf[i] ^= p[i];
      gcm_mult (ctx, ctx->buf, ctx->buf);
      add_len -= use_len;
      p += use_len;
    }
  return (0);
}

/******************************************************************************
 *
 *  GCM_UPDATE
 *
 *  This is called once or more to process bulk plaintext or ciphertext data.
 *  We give this some number of bytes of input and it returns the same number
 *  of output bytes. If called multiple times (which is fine) all but the final
 *  invocation MUST be called with length mod 16 == 0. (Only the final call can
 *  have a partial block length of < 128 bits.)
 *
 ******************************************************************************/
int
gcm_update (gcm_context *ctx,	// pointer to user-provided GCM context
	    size_t length,	// length, in bytes, of data to process
	    const uchar *input, // pointer to source data
	    uchar *output)	// pointer to destination data
{
  int ret;	  // our error return if the AES encrypt fails
  uchar ectr[16]; // counter-mode cipher output for XORing
  size_t use_len; // byte count to process, up to 16 bytes
  size_t i;	  // local loop iterator

  ctx->len += length; // bump the GCM context's running length count

  while (length > 0)
    {
      // clamp the length to process at 16 bytes
      use_len = (length < 16) ? length : 16;

      // increment the context's 128-bit IV||Counter 'y' vector
      for (i = 16; i > 12; i--)
	if (++ctx->y[i - 1] != 0)
	  break;

      // encrypt the context's 'y' vector under the established key
      if ((ret = aes_cipher (&ctx->aes_ctx, ctx->y, ectr)) != 0)
	return (ret);

      // encrypt or decrypt the input to the output
      if (ctx->mode == MG_ENCRYPT)
	{
	  for (i = 0; i < use_len; i++)
	    {
	      // XOR the cipher's ouptut vector (ectr) with our input
	      output[i] = (uchar) (ectr[i] ^ input[i]);
	      // now we mix in our data into the authentication hash.
	      // if we're ENcrypting we XOR in the post-XOR (output)
	      // results, but if we're DEcrypting we XOR in the input
	      // data
	      ctx->buf[i] ^= output[i];
	    }
	}
      else
	{
	  for (i = 0; i < use_len; i++)
	    {
	      // but if we're DEcrypting we XOR in the input data first,
	      // i.e. before saving to ouput data, otherwise if the input
	      // and output buffer are the same (inplace decryption) we
	      // would not get the correct auth tag

	      ctx->buf[i] ^= input[i];

	      // XOR the cipher's ouptut vector (ectr) with our input
	      output[i] = (uchar) (ectr[i] ^ input[i]);
	    }
	}
      gcm_mult (ctx, ctx->buf, ctx->buf); // perform a GHASH operation

      length -= use_len; // drop the remaining byte count to process
      input += use_len;	 // bump our input pointer forward
      output += use_len; // bump our output pointer forward
    }
  return (0);
}

/******************************************************************************
 *
 *  GCM_FINISH
 *
 *  This is called once after all calls to GCM_UPDATE to finalize the GCM.
 *  It performs the final GHASH to produce the resulting authentication TAG.
 *
 ******************************************************************************/
int
gcm_finish (gcm_context *ctx, // pointer to user-provided GCM context
	    uchar *tag,	      // pointer to buffer which receives the tag
	    size_t tag_len)   // length, in bytes, of the tag-receiving buf
{
  uchar work_buf[16];
  uint64_t orig_len = ctx->len * 8;
  uint64_t orig_add_len = ctx->add_len * 8;
  size_t i;

  if (tag_len != 0)
    memcpy (tag, ctx->base_ectr, tag_len);

  if (orig_len || orig_add_len)
    {
      memset (work_buf, 0x00, 16);

      PUT_UINT32_BE ((orig_add_len >> 32), work_buf, 0);
      PUT_UINT32_BE ((orig_add_len), work_buf, 4);
      PUT_UINT32_BE ((orig_len >> 32), work_buf, 8);
      PUT_UINT32_BE ((orig_len), work_buf, 12);

      for (i = 0; i < 16; i++)
	ctx->buf[i] ^= work_buf[i];
      gcm_mult (ctx, ctx->buf, ctx->buf);
      for (i = 0; i < tag_len; i++)
	tag[i] ^= ctx->buf[i];
    }
  return (0);
}

/******************************************************************************
 *
 *  GCM_CRYPT_AND_TAG
 *
 *  This either encrypts or decrypts the user-provided data and, either
 *  way, generates an authentication tag of the requested length. It must be
 *  called with a GCM context whose key has already been set with GCM_SETKEY.
 *
 *  The user would typically call this explicitly to ENCRYPT a buffer of data
 *  and optional associated data, and produce its an authentication tag.
 *
 *  To reverse the process the user would typically call the companion
 *  GCM_AUTH_DECRYPT function to decrypt data and verify a user-provided
 *  authentication tag.  The GCM_AUTH_DECRYPT function calls this function
 *  to perform its decryption and tag generation, which it then compares.
 *
 ******************************************************************************/
int
gcm_crypt_and_tag (
    gcm_context *ctx,	// gcm context with key already setup
    int mode,		// cipher direction: GCM_ENCRYPT or GCM_DECRYPT
    const uchar *iv,	// pointer to the 12-byte initialization vector
    size_t iv_len,	// byte length if the IV. should always be 12
    const uchar *add,	// pointer to the non-ciphered additional data
    size_t add_len,	// byte length of the additional AEAD data
    const uchar *input, // pointer to the cipher data source
    uchar *output,	// pointer to the cipher data destination
    size_t length,	// byte length of the cipher data
    uchar *tag,		// pointer to the tag to be generated
    size_t tag_len)	// byte length of the tag to be generated
{			/*
			   assuming that the caller has already invoked gcm_setkey to
			   prepare the gcm context with the keying material, we simply
			   invoke each of the three GCM sub-functions in turn...
			*/
  gcm_start (ctx, mode, iv, iv_len, add, add_len);
  gcm_update (ctx, length, input, output);
  gcm_finish (ctx, tag, tag_len);
  return (0);
}

/******************************************************************************
 *
 *  GCM_ZERO_CTX
 *
 *  The GCM context contains both the GCM context and the AES context.
 *  This includes keying and key-related material which is security-
 *  sensitive, so it MUST be zeroed after use. This function does that.
 *
 ******************************************************************************/
void
gcm_zero_ctx (gcm_context *ctx)
{
  // zero the context originally provided to us
  memset (ctx, 0, sizeof (gcm_context));
}
//
//  aes-gcm.c
//  Pods
//
//  Created by Markus Kosmal on 20/11/14.
//
//

int
mg_aes_gcm_encrypt (unsigned char *output, //
		    const unsigned char *input, size_t input_length,
		    const unsigned char *key, const size_t key_len,
		    const unsigned char *iv, const size_t iv_len,
		    unsigned char *aead, size_t aead_len, unsigned char *tag,
		    const size_t tag_len)
{
  int ret = 0;	   // our return value
  gcm_context ctx; // includes the AES context structure

  gcm_setkey (&ctx, key, (uint) key_len);

  ret = gcm_crypt_and_tag (&ctx, MG_ENCRYPT, iv, iv_len, aead, aead_len, input,
			   output, input_length, tag, tag_len);

  gcm_zero_ctx (&ctx);

  return (ret);
}

int
mg_aes_gcm_decrypt (unsigned char *output, const unsigned char *input,
		    size_t input_length, const unsigned char *key,
		    const size_t key_len, const unsigned char *iv,
		    const size_t iv_len)
{
  int ret = 0;	   // our return value
  gcm_context ctx; // includes the AES context structure

  size_t tag_len = 0;
  unsigned char *tag_buf = NULL;

  gcm_setkey (&ctx, key, (uint) key_len);

  ret = gcm_crypt_and_tag (&ctx, MG_DECRYPT, iv, iv_len, NULL, 0, input,
			   output, input_length, tag_buf, tag_len);

  gcm_zero_ctx (&ctx);

  return (ret);
}
#endif
// End of aes128 PD

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_builtin.c"
#endif

#if MG_TLS == MG_TLS_BUILTIN

/* TLS 1.3 Record Content Type (RFC8446 B.1) */
#  define MG_TLS_CHANGE_CIPHER 20
#  define MG_TLS_ALERT 21
#  define MG_TLS_HANDSHAKE 22
#  define MG_TLS_APP_DATA 23
#  define MG_TLS_HEARTBEAT 24

/* TLS 1.3 Handshake Message Type (RFC8446 B.3) */
#  define MG_TLS_CLIENT_HELLO 1
#  define MG_TLS_SERVER_HELLO 2
#  define MG_TLS_ENCRYPTED_EXTENSIONS 8
#  define MG_TLS_CERTIFICATE 11
#  define MG_TLS_CERTIFICATE_VERIFY 15
#  define MG_TLS_FINISHED 20

// handshake is re-entrant, so we need to keep track of its state state names
// refer to RFC8446#A.1
enum mg_tls_hs_state
{
  // Client state machine:
  MG_TLS_STATE_CLIENT_START,	     // Send ClientHello
  MG_TLS_STATE_CLIENT_WAIT_SH,	     // Wait for ServerHello
  MG_TLS_STATE_CLIENT_WAIT_EE,	     // Wait for EncryptedExtensions
  MG_TLS_STATE_CLIENT_WAIT_CERT,     // Wait for Certificate
  MG_TLS_STATE_CLIENT_WAIT_CV,	     // Wait for CertificateVerify
  MG_TLS_STATE_CLIENT_WAIT_FINISHED, // Wait for Finished
  MG_TLS_STATE_CLIENT_CONNECTED,     // Done

  // Server state machine:
  MG_TLS_STATE_SERVER_START,	  // Wait for ClientHello
  MG_TLS_STATE_SERVER_NEGOTIATED, // Wait for Finished
  MG_TLS_STATE_SERVER_CONNECTED	  // Done
};

// per-connection TLS data
struct tls_data
{
  enum mg_tls_hs_state state; // keep track of connection handshake progress

  struct mg_iobuf send; // For the receive path, we're reusing c->rtls
  struct mg_iobuf recv; // While c->rtls contains full records, recv reuses
			// the same underlying buffer but points at individual
			// decrypted messages
  uint8_t content_type; // Last received record content type

  mg_sha256_ctx sha256; // incremental SHA-256 hash for TLS handshake

  uint32_t sseq; // server sequence number, used in encryption
  uint32_t cseq; // client sequence number, used in decryption

  uint8_t random[32];	  // client random from ClientHello
  uint8_t session_id[32]; // client session ID between the handshake states
  uint8_t x25519_cli[32]; // client X25519 key between the handshake states
  uint8_t x25519_sec[32]; // x25519 secret between the handshake states

  int skip_verification;	 // perform checks on server certificate?
  struct mg_str server_cert_der; // server certificate in DER format
  uint8_t server_key[32];	 // server EC private key
  char hostname[254];		 // server hostname (client extension)

  uint8_t certhash[32]; // certificate message hash
  uint8_t pubkey[64];	// server EC public key to verify cert
  uint8_t sighash[32];	// server EC public key to verify cert

  // keys for AES encryption
  uint8_t handshake_secret[32];
  uint8_t server_write_key[16];
  uint8_t server_write_iv[12];
  uint8_t server_finished_key[32];
  uint8_t client_write_key[16];
  uint8_t client_write_iv[12];
  uint8_t client_finished_key[32];
};

#  define MG_LOAD_BE16(p) ((uint16_t) ((MG_U8P (p)[0] << 8U) | MG_U8P (p)[1]))
#  define MG_LOAD_BE24(p)                                                     \
    ((uint32_t) ((MG_U8P (p)[0] << 16U) | (MG_U8P (p)[1] << 8U)               \
		 | MG_U8P (p)[2]))
#  define MG_STORE_BE16(p, n)                                                 \
    do                                                                        \
      {                                                                       \
	MG_U8P (p)[0] = ((n) >> 8U) & 255;                                    \
	MG_U8P (p)[1] = (n) & 255;                                            \
      }                                                                       \
    while (0)

#  define TLS_RECHDR_SIZE 5 // 1 byte type, 2 bytes version, 2 bytes length
#  define TLS_MSGHDR_SIZE 4 // 1 byte type, 3 bytes length

#  if 1
static void
mg_ssl_key_log (const char *label, uint8_t client_random[32], uint8_t *secret,
		size_t secretsz)
{
  (void) label;
  (void) client_random;
  (void) secret;
  (void) secretsz;
}
#  else
#    include <stdio.h>
static void
mg_ssl_key_log (const char *label, uint8_t client_random[32], uint8_t *secret,
		size_t secretsz)
{
  char *keylogfile = getenv ("SSLKEYLOGFILE");
  if (keylogfile == NULL)
    {
      return;
    }
  FILE *f = fopen (keylogfile, "a");
  fprintf (f, "%s ", label);
  for (int i = 0; i < 32; i++)
    {
      fprintf (f, "%02x", client_random[i]);
    }
  fprintf (f, " ");
  for (unsigned int i = 0; i < secretsz; i++)
    {
      fprintf (f, "%02x", secret[i]);
    }
  fprintf (f, "\n");
  fclose (f);
}
#  endif

// for derived tls keys we need SHA256([0]*32)
static uint8_t zeros[32] = { 0 };
static uint8_t zeros_sha256_digest[32]
    = { 0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
	0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
	0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55 };

// helper to hexdump buffers inline
static void
mg_tls_hexdump (const char *msg, uint8_t *buf, size_t bufsz)
{
  MG_VERBOSE (("%s: %M", msg, mg_print_hex, bufsz, buf));
}

// helper utilities to parse ASN.1 DER
struct mg_der_tlv
{
  uint8_t type;
  uint32_t len;
  uint8_t *value;
};

// parse DER into a TLV record
static int
mg_der_to_tlv (uint8_t *der, size_t dersz, struct mg_der_tlv *tlv)
{
  if (dersz < 2)
    {
      return -1;
    }
  tlv->type = der[0];
  tlv->len = der[1];
  tlv->value = der + 2;
  if (tlv->len > 0x7f)
    {
      uint32_t i, n = tlv->len - 0x80;
      tlv->len = 0;
      for (i = 0; i < n; i++)
	{
	  tlv->len = (tlv->len << 8) | (der[2 + i]);
	}
      tlv->value = der + 2 + n;
    }
  if (der + dersz < tlv->value + tlv->len)
    {
      return -1;
    }
  return 0;
}

static int
mg_der_find (uint8_t *der, size_t dersz, uint8_t *oid, size_t oidsz,
	     struct mg_der_tlv *tlv)
{
  uint8_t *p, *end;
  struct mg_der_tlv child = { 0, 0, NULL };
  if (mg_der_to_tlv (der, dersz, tlv) < 0)
    {
      return -1; // invalid DER
    }
  else if (tlv->type == 6)
    { // found OID, check value
      return (tlv->len == oidsz && memcmp (tlv->value, oid, oidsz) == 0);
    }
  else if ((tlv->type & 0x20) == 0)
    {
      return 0; // Primitive, but not OID: not found
    }
  // Constructed object: scan children
  p = tlv->value;
  end = tlv->value + tlv->len;
  while (end > p)
    {
      int r;
      mg_der_to_tlv (p, (size_t) (end - p), &child);
      r = mg_der_find (p, (size_t) (end - p), oid, oidsz, tlv);
      if (r < 0)
	return -1; // error
      if (r > 0)
	return 1; // found OID!
      p = child.value + child.len;
    }
  return 0; // not found
}

// Did we receive a full TLS record in the c->rtls buffer?
static bool
mg_tls_got_record (struct mg_connection *c)
{
  return c->rtls.len >= (size_t) TLS_RECHDR_SIZE
	 && c->rtls.len
		>= (size_t) (TLS_RECHDR_SIZE + MG_LOAD_BE16 (c->rtls.buf + 3));
}

// Remove a single TLS record from the recv buffer
static void
mg_tls_drop_record (struct mg_connection *c)
{
  struct mg_iobuf *rio = &c->rtls;
  uint16_t n = MG_LOAD_BE16 (rio->buf + 3) + TLS_RECHDR_SIZE;
  mg_iobuf_del (rio, 0, n);
}

// Remove a single TLS message from decrypted buffer, remove the wrapping
// record if it was the last message within a record
static void
mg_tls_drop_message (struct mg_connection *c)
{
  uint32_t len;
  struct tls_data *tls = (struct tls_data *) c->tls;
  if (tls->recv.len == 0)
    {
      return;
    }
  len = MG_LOAD_BE24 (tls->recv.buf + 1);
  mg_sha256_update (&tls->sha256, tls->recv.buf, len + TLS_MSGHDR_SIZE);
  tls->recv.buf += len + TLS_MSGHDR_SIZE;
  tls->recv.len -= len + TLS_MSGHDR_SIZE;
  if (tls->recv.len == 0)
    {
      mg_tls_drop_record (c);
    }
}

// TLS1.3 secret derivation based on the key label
static void
mg_tls_derive_secret (const char *label, uint8_t *key, size_t keysz,
		      uint8_t *data, size_t datasz, uint8_t *hash,
		      size_t hashsz)
{
  size_t labelsz = strlen (label);
  uint8_t secret[32];
  uint8_t packed[256] = { 0, (uint8_t) hashsz, (uint8_t) labelsz };
  // TODO: assert lengths of label, key, data and hash
  if (labelsz > 0)
    memmove (packed + 3, label, labelsz);
  packed[3 + labelsz] = (uint8_t) datasz;
  if (datasz > 0)
    memmove (packed + labelsz + 4, data, datasz);
  packed[4 + labelsz + datasz] = 1;

  mg_hmac_sha256 (secret, key, keysz, packed, 5 + labelsz + datasz);
  memmove (hash, secret, hashsz);
}

// at this point we have x25519 shared secret, we can generate a set of derived
// handshake encryption keys
static void
mg_tls_generate_handshake_keys (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;

  mg_sha256_ctx sha256;
  uint8_t early_secret[32];
  uint8_t pre_extract_secret[32];
  uint8_t hello_hash[32];
  uint8_t server_hs_secret[32];
  uint8_t client_hs_secret[32];

  mg_hmac_sha256 (early_secret, NULL, 0, zeros, sizeof (zeros));
  mg_tls_derive_secret ("tls13 derived", early_secret, 32, zeros_sha256_digest,
			32, pre_extract_secret, 32);
  mg_hmac_sha256 (tls->handshake_secret, pre_extract_secret,
		  sizeof (pre_extract_secret), tls->x25519_sec,
		  sizeof (tls->x25519_sec));
  mg_tls_hexdump ("hs secret", tls->handshake_secret, 32);

  // mg_sha256_final is not idempotent, need to copy sha256 context to
  // calculate the digest
  memmove (&sha256, &tls->sha256, sizeof (mg_sha256_ctx));
  mg_sha256_final (hello_hash, &sha256);

  mg_tls_hexdump ("hello hash", hello_hash, 32);
  // derive keys needed for the rest of the handshake
  mg_tls_derive_secret ("tls13 s hs traffic", tls->handshake_secret, 32,
			hello_hash, 32, server_hs_secret, 32);
  mg_tls_derive_secret ("tls13 key", server_hs_secret, 32, NULL, 0,
			tls->server_write_key, 16);
  mg_tls_derive_secret ("tls13 iv", server_hs_secret, 32, NULL, 0,
			tls->server_write_iv, 12);
  mg_tls_derive_secret ("tls13 finished", server_hs_secret, 32, NULL, 0,
			tls->server_finished_key, 32);

  mg_tls_derive_secret ("tls13 c hs traffic", tls->handshake_secret, 32,
			hello_hash, 32, client_hs_secret, 32);
  mg_tls_derive_secret ("tls13 key", client_hs_secret, 32, NULL, 0,
			tls->client_write_key, 16);
  mg_tls_derive_secret ("tls13 iv", client_hs_secret, 32, NULL, 0,
			tls->client_write_iv, 12);
  mg_tls_derive_secret ("tls13 finished", client_hs_secret, 32, NULL, 0,
			tls->client_finished_key, 32);

  mg_tls_hexdump ("s hs traffic", server_hs_secret, 32);
  mg_tls_hexdump ("s key", tls->server_write_key, 16);
  mg_tls_hexdump ("s iv", tls->server_write_iv, 12);
  mg_tls_hexdump ("s finished", tls->server_finished_key, 32);
  mg_tls_hexdump ("c hs traffic", client_hs_secret, 32);
  mg_tls_hexdump ("c key", tls->client_write_key, 16);
  mg_tls_hexdump ("c iv", tls->client_write_iv, 16);
  mg_tls_hexdump ("c finished", tls->client_finished_key, 32);

  mg_ssl_key_log ("SERVER_HANDSHAKE_TRAFFIC_SECRET", tls->random,
		  server_hs_secret, 32);
  mg_ssl_key_log ("CLIENT_HANDSHAKE_TRAFFIC_SECRET", tls->random,
		  client_hs_secret, 32);
}

static void
mg_tls_generate_application_keys (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  uint8_t hash[32];
  uint8_t premaster_secret[32];
  uint8_t master_secret[32];
  uint8_t server_secret[32];
  uint8_t client_secret[32];

  mg_sha256_ctx sha256;
  memmove (&sha256, &tls->sha256, sizeof (mg_sha256_ctx));
  mg_sha256_final (hash, &sha256);

  mg_tls_derive_secret ("tls13 derived", tls->handshake_secret, 32,
			zeros_sha256_digest, 32, premaster_secret, 32);
  mg_hmac_sha256 (master_secret, premaster_secret, 32, zeros, 32);

  mg_tls_derive_secret ("tls13 s ap traffic", master_secret, 32, hash, 32,
			server_secret, 32);
  mg_tls_derive_secret ("tls13 key", server_secret, 32, NULL, 0,
			tls->server_write_key, 16);
  mg_tls_derive_secret ("tls13 iv", server_secret, 32, NULL, 0,
			tls->server_write_iv, 12);
  mg_tls_derive_secret ("tls13 c ap traffic", master_secret, 32, hash, 32,
			client_secret, 32);
  mg_tls_derive_secret ("tls13 key", client_secret, 32, NULL, 0,
			tls->client_write_key, 16);
  mg_tls_derive_secret ("tls13 iv", client_secret, 32, NULL, 0,
			tls->client_write_iv, 12);

  mg_tls_hexdump ("s ap traffic", server_secret, 32);
  mg_tls_hexdump ("s key", tls->server_write_key, 16);
  mg_tls_hexdump ("s iv", tls->server_write_iv, 12);
  mg_tls_hexdump ("s finished", tls->server_finished_key, 32);
  mg_tls_hexdump ("c ap traffic", client_secret, 32);
  mg_tls_hexdump ("c key", tls->client_write_key, 16);
  mg_tls_hexdump ("c iv", tls->client_write_iv, 16);
  mg_tls_hexdump ("c finished", tls->client_finished_key, 32);
  tls->sseq = tls->cseq = 0;

  mg_ssl_key_log ("SERVER_TRAFFIC_SECRET_0", tls->random, server_secret, 32);
  mg_ssl_key_log ("CLIENT_TRAFFIC_SECRET_0", tls->random, client_secret, 32);
}

// AES GCM encryption of the message + put encoded data into the write buffer
static void
mg_tls_encrypt (struct mg_connection *c, const uint8_t *msg, size_t msgsz,
		uint8_t msgtype)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *wio = &tls->send;
  uint8_t *outmsg;
  uint8_t *tag;
  size_t encsz = msgsz + 16 + 1;
  uint8_t hdr[5]
      = { MG_TLS_APP_DATA, 0x03, 0x03, (uint8_t) ((encsz >> 8) & 0xff),
	  (uint8_t) (encsz & 0xff) };
  uint8_t associated_data[5]
      = { MG_TLS_APP_DATA, 0x03, 0x03, (uint8_t) ((encsz >> 8) & 0xff),
	  (uint8_t) (encsz & 0xff) };
  uint8_t nonce[12];

  mg_gcm_initialize ();

  if (c->is_client)
    {
      memmove (nonce, tls->client_write_iv, sizeof (tls->client_write_iv));
      nonce[8] ^= (uint8_t) ((tls->cseq >> 24) & 255U);
      nonce[9] ^= (uint8_t) ((tls->cseq >> 16) & 255U);
      nonce[10] ^= (uint8_t) ((tls->cseq >> 8) & 255U);
      nonce[11] ^= (uint8_t) ((tls->cseq) & 255U);
    }
  else
    {
      memmove (nonce, tls->server_write_iv, sizeof (tls->server_write_iv));
      nonce[8] ^= (uint8_t) ((tls->sseq >> 24) & 255U);
      nonce[9] ^= (uint8_t) ((tls->sseq >> 16) & 255U);
      nonce[10] ^= (uint8_t) ((tls->sseq >> 8) & 255U);
      nonce[11] ^= (uint8_t) ((tls->sseq) & 255U);
    }

  mg_iobuf_add (wio, wio->len, hdr, sizeof (hdr));
  mg_iobuf_resize (wio, wio->len + encsz);
  outmsg = wio->buf + wio->len;
  tag = wio->buf + wio->len + msgsz + 1;
  memmove (outmsg, msg, msgsz);
  outmsg[msgsz] = msgtype;
  if (c->is_client)
    {
      mg_aes_gcm_encrypt (outmsg, outmsg, msgsz + 1, tls->client_write_key,
			  sizeof (tls->client_write_key), nonce,
			  sizeof (nonce), associated_data,
			  sizeof (associated_data), tag, 16);
      tls->cseq++;
    }
  else
    {
      mg_aes_gcm_encrypt (outmsg, outmsg, msgsz + 1, tls->server_write_key,
			  sizeof (tls->server_write_key), nonce,
			  sizeof (nonce), associated_data,
			  sizeof (associated_data), tag, 16);
      tls->sseq++;
    }
  wio->len += encsz;
}

// read an encrypted record, decrypt it in place
static int
mg_tls_recv_record (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *rio = &c->rtls;
  uint16_t msgsz;
  uint8_t *msg;
  uint8_t nonce[12];
  int r;
  if (tls->recv.len > 0)
    {
      return 0; /* some data from previous record is still present */
    }
  for (;;)
    {
      if (!mg_tls_got_record (c))
	{
	  return MG_IO_WAIT;
	}
      if (rio->buf[0] == MG_TLS_APP_DATA)
	{
	  break;
	}
      else if (rio->buf[0] == MG_TLS_CHANGE_CIPHER)
	{ // Skip ChangeCipher messages
	  mg_tls_drop_record (c);
	}
      else if (rio->buf[0] == MG_TLS_ALERT)
	{ // Skip Alerts
	  MG_INFO (("TLS ALERT packet received"));
	  mg_tls_drop_record (c);
	}
      else
	{
	  mg_error (c, "unexpected packet");
	  return -1;
	}
    }

  mg_gcm_initialize ();
  msgsz = MG_LOAD_BE16 (rio->buf + 3);
  msg = rio->buf + 5;
  if (c->is_client)
    {
      memmove (nonce, tls->server_write_iv, sizeof (tls->server_write_iv));
      nonce[8] ^= (uint8_t) ((tls->sseq >> 24) & 255U);
      nonce[9] ^= (uint8_t) ((tls->sseq >> 16) & 255U);
      nonce[10] ^= (uint8_t) ((tls->sseq >> 8) & 255U);
      nonce[11] ^= (uint8_t) ((tls->sseq) & 255U);
      mg_aes_gcm_decrypt (msg, msg, msgsz - 16, tls->server_write_key,
			  sizeof (tls->server_write_key), nonce,
			  sizeof (nonce));
      tls->sseq++;
    }
  else
    {
      memmove (nonce, tls->client_write_iv, sizeof (tls->client_write_iv));
      nonce[8] ^= (uint8_t) ((tls->cseq >> 24) & 255U);
      nonce[9] ^= (uint8_t) ((tls->cseq >> 16) & 255U);
      nonce[10] ^= (uint8_t) ((tls->cseq >> 8) & 255U);
      nonce[11] ^= (uint8_t) ((tls->cseq) & 255U);
      mg_aes_gcm_decrypt (msg, msg, msgsz - 16, tls->client_write_key,
			  sizeof (tls->client_write_key), nonce,
			  sizeof (nonce));
      tls->cseq++;
    }
  r = msgsz - 16 - 1;
  tls->content_type = msg[msgsz - 16 - 1];
  tls->recv.buf = msg;
  tls->recv.size = tls->recv.len = msgsz - 16 - 1;
  return r;
}

static void
mg_tls_calc_cert_verify_hash (struct mg_connection *c, uint8_t hash[32])
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  uint8_t sig_content[130] = { "                                "
			       "                                "
			       "TLS 1.3, server CertificateVerify\0" };
  mg_sha256_ctx sha256;
  memmove (&sha256, &tls->sha256, sizeof (mg_sha256_ctx));
  mg_sha256_final (sig_content + 98, &sha256);

  mg_sha256_init (&sha256);
  mg_sha256_update (&sha256, sig_content, sizeof (sig_content));
  mg_sha256_final (hash, &sha256);
}

// read and parse ClientHello record
static int
mg_tls_server_recv_hello (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *rio = &c->rtls;
  uint8_t session_id_len;
  uint16_t j;
  uint16_t cipher_suites_len;
  uint16_t ext_len;
  uint8_t *ext;
  uint16_t msgsz;

  if (!mg_tls_got_record (c))
    {
      return MG_IO_WAIT;
    }
  if (rio->buf[0] != MG_TLS_HANDSHAKE || rio->buf[5] != MG_TLS_CLIENT_HELLO)
    {
      mg_error (c, "not a client hello packet");
      return -1;
    }
  msgsz = MG_LOAD_BE16 (rio->buf + 3);
  mg_sha256_update (&tls->sha256, rio->buf + 5, msgsz);
  // store client random
  memmove (tls->random, rio->buf + 11, sizeof (tls->random));
  // store session_id
  session_id_len = rio->buf[43];
  if (session_id_len == sizeof (tls->session_id))
    {
      memmove (tls->session_id, rio->buf + 44, session_id_len);
    }
  else if (session_id_len != 0)
    {
      MG_INFO (("bad session id len"));
    }
  cipher_suites_len = MG_LOAD_BE16 (rio->buf + 44 + session_id_len);
  ext_len = MG_LOAD_BE16 (rio->buf + 48 + session_id_len + cipher_suites_len);
  ext = rio->buf + 50 + session_id_len + cipher_suites_len;
  for (j = 0; j < ext_len;)
    {
      uint16_t k;
      uint16_t key_exchange_len;
      uint8_t *key_exchange;
      uint16_t n = MG_LOAD_BE16 (ext + j + 2);
      if (ext[j] != 0x00 || ext[j + 1] != 0x33)
	{ // not a key share extension, ignore
	  j += (uint16_t) (n + 4);
	  continue;
	}
      key_exchange_len = MG_LOAD_BE16 (ext + j + 5);
      key_exchange = ext + j + 6;
      for (k = 0; k < key_exchange_len;)
	{
	  uint16_t m = MG_LOAD_BE16 (key_exchange + k + 2);
	  if (m == 32 && key_exchange[k] == 0x00
	      && key_exchange[k + 1] == 0x1d)
	    {
	      memmove (tls->x25519_cli, key_exchange + k + 4, m);
	      mg_tls_drop_record (c);
	      return 0;
	    }
	  k += (uint16_t) (m + 4);
	}
      j += (uint16_t) (n + 4);
    }
  mg_error (c, "bad client hello");
  return -1;
}

#  define PLACEHOLDER_8B 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
#  define PLACEHOLDER_16B PLACEHOLDER_8B, PLACEHOLDER_8B
#  define PLACEHOLDER_32B PLACEHOLDER_16B, PLACEHOLDER_16B

// put ServerHello record into wio buffer
static void
mg_tls_server_send_hello (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *wio = &tls->send;

  uint8_t msg_server_hello[122]
      = { // server hello, tls 1.2
	  0x02, 0x00, 0x00, 0x76, 0x03, 0x03,
	  // random (32 bytes)
	  PLACEHOLDER_32B,
	  // session ID length + session ID (32 bytes)
	  0x20, PLACEHOLDER_32B,
#  if defined(CHACHA20) && CHACHA20
	  // TLS_CHACHA20_POLY1305_SHA256 + no compression
	  0x13, 0x03, 0x00,
#  else
	  // TLS_AES_128_GCM_SHA256 + no compression
	  0x13, 0x01, 0x00,
#  endif
	  // extensions + keyshare
	  0x00, 0x2e, 0x00, 0x33, 0x00, 0x24, 0x00, 0x1d, 0x00, 0x20,
	  // x25519 keyshare
	  PLACEHOLDER_32B,
	  // supported versions (tls1.3 == 0x304)
	  0x00, 0x2b, 0x00, 0x02, 0x03, 0x04
	};

  // calculate keyshare
  uint8_t x25519_pub[X25519_BYTES];
  uint8_t x25519_prv[X25519_BYTES];
  mg_random (x25519_prv, sizeof (x25519_prv));
  mg_tls_x25519 (x25519_pub, x25519_prv, X25519_BASE_POINT, 1);
  mg_tls_x25519 (tls->x25519_sec, x25519_prv, tls->x25519_cli, 1);
  mg_tls_hexdump ("s x25519 sec", tls->x25519_sec, sizeof (tls->x25519_sec));

  // fill in the gaps: random + session ID + keyshare
  memmove (msg_server_hello + 6, tls->random, sizeof (tls->random));
  memmove (msg_server_hello + 39, tls->session_id, sizeof (tls->session_id));
  memmove (msg_server_hello + 84, x25519_pub, sizeof (x25519_pub));

  // server hello message
  mg_iobuf_add (wio, wio->len, "\x16\x03\x03\x00\x7a", 5);
  mg_iobuf_add (wio, wio->len, msg_server_hello, sizeof (msg_server_hello));
  mg_sha256_update (&tls->sha256, msg_server_hello, sizeof (msg_server_hello));

  // change cipher message
  mg_iobuf_add (wio, wio->len, "\x14\x03\x03\x00\x01\x01", 6);
}

static void
mg_tls_server_send_ext (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  // server extensions
  uint8_t ext[6] = { 0x08, 0, 0, 2, 0, 0 };
  mg_sha256_update (&tls->sha256, ext, sizeof (ext));
  mg_tls_encrypt (c, ext, sizeof (ext), MG_TLS_HANDSHAKE);
}

static void
mg_tls_server_send_cert (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  // server DER certificate (empty)
  size_t n = tls->server_cert_der.len;
  uint8_t *cert = (uint8_t *) calloc (1, 13 + n);
  if (cert == NULL)
    {
      mg_error (c, "tls cert oom");
      return;
    }
  cert[0] = 0x0b;				// handshake header
  cert[1] = (uint8_t) (((n + 9) >> 16) & 255U); // 3 bytes: payload length
  cert[2] = (uint8_t) (((n + 9) >> 8) & 255U);
  cert[3] = (uint8_t) ((n + 9) & 255U);
  cert[4] = 0;					// request context
  cert[5] = (uint8_t) (((n + 5) >> 16) & 255U); // 3 bytes: cert (s) length
  cert[6] = (uint8_t) (((n + 5) >> 8) & 255U);
  cert[7] = (uint8_t) ((n + 5) & 255U);
  cert[8]
      = (uint8_t) (((n) >> 16) & 255U); // 3 bytes: first (and only) cert len
  cert[9] = (uint8_t) (((n) >> 8) & 255U);
  cert[10] = (uint8_t) (n & 255U);
  // bytes 11+ are certificate in DER format
  memmove (cert + 11, tls->server_cert_der.buf, n);
  cert[11 + n] = cert[12 + n] = 0; // certificate extensions (none)
  mg_sha256_update (&tls->sha256, cert, 13 + n);
  mg_tls_encrypt (c, cert, 13 + n, MG_TLS_HANDSHAKE);
  free (cert);
}

// type adapter between uECC hash context and our sha256 implementation
typedef struct SHA256_HashContext
{
  MG_UECC_HashContext uECC;
  mg_sha256_ctx ctx;
} SHA256_HashContext;

static void
init_SHA256 (const MG_UECC_HashContext *base)
{
  SHA256_HashContext *c = (SHA256_HashContext *) base;
  mg_sha256_init (&c->ctx);
}

static void
update_SHA256 (const MG_UECC_HashContext *base, const uint8_t *message,
	       unsigned message_size)
{
  SHA256_HashContext *c = (SHA256_HashContext *) base;
  mg_sha256_update (&c->ctx, message, message_size);
}
static void
finish_SHA256 (const MG_UECC_HashContext *base, uint8_t *hash_result)
{
  SHA256_HashContext *c = (SHA256_HashContext *) base;
  mg_sha256_final (hash_result, &c->ctx);
}

static void
mg_tls_server_send_cert_verify (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  // server certificate verify packet
  uint8_t verify[82] = { 0x0f, 0x00, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00 };
  size_t sigsz, verifysz = 0;
  uint8_t hash[32] = { 0 }, tmp[2 * 32 + 64] = { 0 };
  struct SHA256_HashContext ctx
      = { { &init_SHA256, &update_SHA256, &finish_SHA256, 64, 32, tmp },
	  { { 0 }, 0, 0, { 0 } } };
  int neg1, neg2;
  uint8_t sig[64] = { 0 };

  mg_tls_calc_cert_verify_hash (c, (uint8_t *) hash);

  mg_uecc_sign_deterministic (tls->server_key, hash, sizeof (hash), &ctx.uECC,
			      sig, mg_uecc_secp256r1 ());

  neg1 = !!(sig[0] & 0x80);
  neg2 = !!(sig[32] & 0x80);
  verify[8] = 0x30; // ASN.1 SEQUENCE
  verify[9] = (uint8_t) (68 + neg1 + neg2);
  verify[10] = 0x02; // ASN.1 INTEGER
  verify[11] = (uint8_t) (32 + neg1);
  memmove (verify + 12 + neg1, sig, 32);
  verify[12 + 32 + neg1] = 0x02; // ASN.1 INTEGER
  verify[13 + 32 + neg1] = (uint8_t) (32 + neg2);
  memmove (verify + 14 + 32 + neg1 + neg2, sig + 32, 32);

  sigsz = (size_t) (70 + neg1 + neg2);
  verifysz = 8U + sigsz;
  verify[3] = (uint8_t) (sigsz + 4);
  verify[7] = (uint8_t) sigsz;

  mg_sha256_update (&tls->sha256, verify, verifysz);
  mg_tls_encrypt (c, verify, verifysz, MG_TLS_HANDSHAKE);
}

static void
mg_tls_server_send_finish (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *wio = &tls->send;
  mg_sha256_ctx sha256;
  uint8_t hash[32];
  uint8_t finish[36] = { 0x14, 0, 0, 32 };
  memmove (&sha256, &tls->sha256, sizeof (mg_sha256_ctx));
  mg_sha256_final (hash, &sha256);
  mg_hmac_sha256 (finish + 4, tls->server_finished_key, 32, hash, 32);
  mg_tls_encrypt (c, finish, sizeof (finish), MG_TLS_HANDSHAKE);
  mg_io_send (c, wio->buf, wio->len);
  wio->len = 0;

  mg_sha256_update (&tls->sha256, finish, sizeof (finish));
}

static int
mg_tls_server_recv_finish (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  // we have to backup sha256 value to restore it later, since Finished record
  // is exceptional and is not supposed to be added to the rolling hash
  // calculation.
  mg_sha256_ctx sha256 = tls->sha256;
  if (mg_tls_recv_record (c) < 0)
    {
      return -1;
    }
  if (tls->recv.buf[0] != MG_TLS_FINISHED)
    {
      mg_error (c, "expected Finish but got msg 0x%02x", tls->recv.buf[0]);
      return -1;
    }
  mg_tls_drop_message (c);

  // restore hash
  tls->sha256 = sha256;
  return 0;
}

static void
mg_tls_client_send_hello (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *wio = &tls->send;

  const char *hostname = tls->hostname;
  size_t hostnamesz = strlen (tls->hostname);
  uint8_t x25519_pub[X25519_BYTES];

  uint8_t msg_client_hello[162 + 32]
      = { // TLS Client Hello header reported as TLS1.2 (5)
	  0x16, 0x03, 0x01, 0x00, 0xfe,
	  // server hello, tls 1.2 (6)
	  0x01, 0x00, 0x00, 0x8c, 0x03, 0x03,
	  // random (32 bytes)
	  PLACEHOLDER_32B,
	  // session ID length + session ID (32 bytes)
	  0x20, PLACEHOLDER_32B,
#  if defined(CHACHA20) && CHACHA20
	  // TLS_CHACHA20_POLY1305_SHA256 + no compression
	  0x13, 0x03, 0x00,
#  else
	  0x00,
	  0x02, // size = 2 bytes
	  0x13,
	  0x01, // TLS_AES_128_GCM_SHA256
	  0x01,
	  0x00, // no compression
#  endif

	  // extensions + keyshare
	  0x00, 0xfe,
	  // x25519 keyshare
	  0x00, 0x33, 0x00, 0x26, 0x00, 0x24, 0x00, 0x1d, 0x00, 0x20,
	  PLACEHOLDER_32B,
	  // supported groups (x25519)
	  0x00, 0x0a, 0x00, 0x04, 0x00, 0x02, 0x00, 0x1d,
	  // supported versions (tls1.3 == 0x304)
	  0x00, 0x2b, 0x00, 0x03, 0x02, 0x03, 0x04,
	  // session ticket (none)
	  0x00, 0x23, 0x00, 0x00,
	  // signature algorithms (we don't care, so list all the common ones)
	  0x00, 0x0d, 0x00, 0x24, 0x00, 0x22, 0x04, 0x03, 0x05, 0x03, 0x06,
	  0x03, 0x08, 0x07, 0x08, 0x08, 0x08, 0x1a, 0x08, 0x1b, 0x08, 0x1c,
	  0x08, 0x09, 0x08, 0x0a, 0x08, 0x0b, 0x08, 0x04, 0x08, 0x05, 0x08,
	  0x06, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01,
	  // server name
	  0x00, 0x00, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0x00, 0xfe
	};

  // patch ClientHello with correct hostname length + offset:
  MG_STORE_BE16 (msg_client_hello + 3, hostnamesz + 189);
  MG_STORE_BE16 (msg_client_hello + 7, hostnamesz + 185);
  MG_STORE_BE16 (msg_client_hello + 82, hostnamesz + 110);
  MG_STORE_BE16 (msg_client_hello + 187, hostnamesz + 5);
  MG_STORE_BE16 (msg_client_hello + 189, hostnamesz + 3);
  MG_STORE_BE16 (msg_client_hello + 192, hostnamesz);

  // calculate keyshare
  mg_random (tls->x25519_cli, sizeof (tls->x25519_cli));
  mg_tls_x25519 (x25519_pub, tls->x25519_cli, X25519_BASE_POINT, 1);

  // fill in the gaps: random + session ID + keyshare
  mg_random (tls->session_id, sizeof (tls->session_id));
  mg_random (tls->random, sizeof (tls->random));
  memmove (msg_client_hello + 11, tls->random, sizeof (tls->random));
  memmove (msg_client_hello + 44, tls->session_id, sizeof (tls->session_id));
  memmove (msg_client_hello + 94, x25519_pub, sizeof (x25519_pub));

  // server hello message
  mg_iobuf_add (wio, wio->len, msg_client_hello, sizeof (msg_client_hello));
  mg_iobuf_add (wio, wio->len, hostname, strlen (hostname));
  mg_sha256_update (&tls->sha256, msg_client_hello + 5,
		    sizeof (msg_client_hello) - 5);
  mg_sha256_update (&tls->sha256, (uint8_t *) hostname, strlen (hostname));

  // change cipher message
  mg_iobuf_add (wio, wio->len, (const char *) "\x14\x03\x03\x00\x01\x01", 6);
  mg_io_send (c, wio->buf, wio->len);
  wio->len = 0;
}

static int
mg_tls_client_recv_hello (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *rio = &c->rtls;
  uint16_t msgsz;
  uint8_t *ext;
  uint16_t ext_len;
  int j;

  if (!mg_tls_got_record (c))
    {
      return MG_IO_WAIT;
    }
  if (rio->buf[0] != MG_TLS_HANDSHAKE || rio->buf[5] != MG_TLS_SERVER_HELLO)
    {
      if (rio->buf[0] == MG_TLS_ALERT && rio->len >= 7)
	{
	  mg_error (c, "tls alert %d", rio->buf[6]);
	  return -1;
	}
      MG_INFO (("got packet type 0x%02x/0x%02x", rio->buf[0], rio->buf[5]));
      mg_error (c, "not a server hello packet");
      return -1;
    }

  msgsz = MG_LOAD_BE16 (rio->buf + 3);
  mg_sha256_update (&tls->sha256, rio->buf + 5, msgsz);

  ext_len = MG_LOAD_BE16 (rio->buf + 5 + 39 + 32 + 3);
  ext = rio->buf + 5 + 39 + 32 + 3 + 2;

  for (j = 0; j < ext_len;)
    {
      uint16_t ext_type = MG_LOAD_BE16 (ext + j);
      uint16_t ext_len2 = MG_LOAD_BE16 (ext + j + 2);
      uint16_t group;
      uint8_t *key_exchange;
      uint16_t key_exchange_len;
      if (ext_type != 0x0033)
	{ // not a key share extension, ignore
	  j += (uint16_t) (ext_len2 + 4);
	  continue;
	}
      group = MG_LOAD_BE16 (ext + j + 4);
      if (group != 0x001d)
	{
	  mg_error (c, "bad key exchange group");
	  return -1;
	}
      key_exchange_len = MG_LOAD_BE16 (ext + j + 6);
      key_exchange = ext + j + 8;
      if (key_exchange_len != 32)
	{
	  mg_error (c, "bad key exchange length");
	  return -1;
	}
      mg_tls_x25519 (tls->x25519_sec, tls->x25519_cli, key_exchange, 1);
      mg_tls_hexdump ("c x25519 sec", tls->x25519_sec, 32);
      mg_tls_drop_record (c);
      /* generate handshake keys */
      mg_tls_generate_handshake_keys (c);
      return 0;
    }
  mg_error (c, "bad client hello");
  return -1;
}

static int
mg_tls_client_recv_ext (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  if (mg_tls_recv_record (c) < 0)
    {
      return -1;
    }
  if (tls->recv.buf[0] != MG_TLS_ENCRYPTED_EXTENSIONS)
    {
      mg_error (c, "expected server extensions but got msg 0x%02x",
		tls->recv.buf[0]);
      return -1;
    }
  mg_tls_drop_message (c);
  return 0;
}

static int
mg_tls_client_recv_cert (struct mg_connection *c)
{
  uint8_t *cert;
  uint32_t certsz;
  struct mg_der_tlv oid, pubkey, seq, subj;
  int subj_match = 0;
  struct tls_data *tls = (struct tls_data *) c->tls;
  if (mg_tls_recv_record (c) < 0)
    {
      return -1;
    }
  if (tls->recv.buf[0] != MG_TLS_CERTIFICATE)
    {
      mg_error (c, "expected server certificate but got msg 0x%02x",
		tls->recv.buf[0]);
      return -1;
    }
  if (tls->skip_verification)
    {
      mg_tls_drop_message (c);
      return 0;
    }

  if (tls->recv.len < 11)
    {
      mg_error (c, "certificate list too short");
      return -1;
    }

  cert = tls->recv.buf + 11;
  certsz = MG_LOAD_BE24 (tls->recv.buf + 8);
  if (certsz > tls->recv.len - 11)
    {
      mg_error (c, "certificate too long: %d vs %d", certsz,
		tls->recv.len - 11);
      return -1;
    }

  do
    {
      // secp256r1 public key
      if (mg_der_find (cert, certsz,
		       (uint8_t *) "\x2A\x86\x48\xCE\x3D\x03\x01\x07", 8, &oid)
	  < 0)
	{
	  mg_error (c, "certificate secp256r1 public key OID not found");
	  return -1;
	}
      if (mg_der_to_tlv (oid.value + oid.len,
			 (size_t) (cert + certsz - (oid.value + oid.len)),
			 &pubkey)
	  < 0)
	{
	  mg_error (c, "certificate secp256r1 public key not found");
	  return -1;
	}

      // expect BIT STRING, unpadded, uncompressed: [0]+[4]+32+32 content bytes
      if (pubkey.type != 3 || pubkey.len != 66 || pubkey.value[0] != 0
	  || pubkey.value[1] != 4)
	{
	  mg_error (c, "unsupported public key bitstring encoding");
	  return -1;
	}
      memmove (tls->pubkey, pubkey.value + 2, pubkey.len - 2);
    }
  while (0);

  // Subject Alternative Names
  do
    {
      if (mg_der_find (cert, certsz, (uint8_t *) "\x55\x1d\x11", 3, &oid) < 0)
	{
	  mg_error (c,
		    "certificate does not contain subject alternative names");
	  return -1;
	}
      if (mg_der_to_tlv (oid.value + oid.len,
			 (size_t) (cert + certsz - (oid.value + oid.len)),
			 &seq)
	  < 0)
	{
	  mg_error (c, "certificate subject alternative names not found");
	  return -1;
	}
      if (mg_der_to_tlv (seq.value, seq.len, &seq) < 0)
	{
	  mg_error (c, "certificate subject alternative names is not a "
		       "constructed object");
	  return -1;
	}
      MG_VERBOSE (("verify hostname %s", tls->hostname));
      while (seq.len > 0)
	{
	  if (mg_der_to_tlv (seq.value, seq.len, &subj) < 0)
	    {
	      mg_error (c, "bad subject alternative name");
	      return -1;
	    }
	  MG_VERBOSE (("subj=%.*s", subj.len, subj.value));
	  if (mg_match (mg_str ((const char *) tls->hostname),
			mg_str_n ((const char *) subj.value, subj.len), NULL))
	    {
	      subj_match = 1;
	      break;
	    }
	  seq.len = (uint32_t) (seq.value + seq.len - (subj.value + subj.len));
	  seq.value = subj.value + subj.len;
	}
      if (!subj_match)
	{
	  mg_error (c, "certificate did not match the hostname");
	  return -1;
	}
    }
  while (0);

  mg_tls_drop_message (c);
  mg_tls_calc_cert_verify_hash (c, tls->sighash);
  return 0;
}

static int
mg_tls_client_recv_cert_verify (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  if (mg_tls_recv_record (c) < 0)
    {
      return -1;
    }
  if (tls->recv.buf[0] != MG_TLS_CERTIFICATE_VERIFY)
    {
      mg_error (c, "expected server certificate verify but got msg 0x%02x",
		tls->recv.buf[0]);
      return -1;
    }
  // Ignore CertificateVerify is strict checks are not required
  if (tls->skip_verification)
    {
      mg_tls_drop_message (c);
      return 0;
    }

  // Extract certificate signature and verify it using pubkey and sighash
  do
    {
      uint8_t sig[64];
      struct mg_der_tlv seq, a, b;
      if (mg_der_to_tlv (tls->recv.buf + 8, tls->recv.len - 8, &seq) < 0)
	{
	  mg_error (c, "verification message is not an ASN.1 DER sequence");
	  return -1;
	}
      if (mg_der_to_tlv (seq.value, seq.len, &a) < 0)
	{
	  mg_error (c, "missing first part of the signature");
	  return -1;
	}
      if (mg_der_to_tlv (a.value + a.len, seq.len - a.len, &b) < 0)
	{
	  mg_error (c, "missing second part of the signature");
	  return -1;
	}
      // Integers may be padded with zeroes
      if (a.len > 32)
	{
	  a.value = a.value + (a.len - 32);
	  a.len = 32;
	}
      if (b.len > 32)
	{
	  b.value = b.value + (b.len - 32);
	  b.len = 32;
	}

      memmove (sig, a.value, a.len);
      memmove (sig + 32, b.value, b.len);

      if (mg_uecc_verify (tls->pubkey, tls->sighash, sizeof (tls->sighash),
			  sig, mg_uecc_secp256r1 ())
	  != 1)
	{
	  mg_error (c, "failed to verify certificate");
	  return -1;
	}
    }
  while (0);

  mg_tls_drop_message (c);
  return 0;
}

static int
mg_tls_client_recv_finish (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  if (mg_tls_recv_record (c) < 0)
    {
      return -1;
    }
  if (tls->recv.buf[0] != MG_TLS_FINISHED)
    {
      mg_error (c, "expected server finished but got msg 0x%02x",
		tls->recv.buf[0]);
      return -1;
    }
  mg_tls_drop_message (c);
  return 0;
}

static void
mg_tls_client_send_finish (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  struct mg_iobuf *wio = &tls->send;
  mg_sha256_ctx sha256;
  uint8_t hash[32];
  uint8_t finish[36] = { 0x14, 0, 0, 32 };
  memmove (&sha256, &tls->sha256, sizeof (mg_sha256_ctx));
  mg_sha256_final (hash, &sha256);
  mg_hmac_sha256 (finish + 4, tls->client_finished_key, 32, hash, 32);
  mg_tls_encrypt (c, finish, sizeof (finish), MG_TLS_HANDSHAKE);
  mg_io_send (c, wio->buf, wio->len);
  wio->len = 0;
}

static void
mg_tls_client_handshake (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  switch (tls->state)
    {
    case MG_TLS_STATE_CLIENT_START:
      mg_tls_client_send_hello (c);
      tls->state = MG_TLS_STATE_CLIENT_WAIT_SH;
      // Fallthrough
    case MG_TLS_STATE_CLIENT_WAIT_SH:
      if (mg_tls_client_recv_hello (c) < 0)
	{
	  break;
	}
      tls->state = MG_TLS_STATE_CLIENT_WAIT_EE;
      // Fallthrough
    case MG_TLS_STATE_CLIENT_WAIT_EE:
      if (mg_tls_client_recv_ext (c) < 0)
	{
	  break;
	}
      tls->state = MG_TLS_STATE_CLIENT_WAIT_CERT;
      // Fallthrough
    case MG_TLS_STATE_CLIENT_WAIT_CERT:
      if (mg_tls_client_recv_cert (c) < 0)
	{
	  break;
	}
      tls->state = MG_TLS_STATE_CLIENT_WAIT_CV;
      // Fallthrough
    case MG_TLS_STATE_CLIENT_WAIT_CV:
      if (mg_tls_client_recv_cert_verify (c) < 0)
	{
	  break;
	}
      tls->state = MG_TLS_STATE_CLIENT_WAIT_FINISHED;
      // Fallthrough
    case MG_TLS_STATE_CLIENT_WAIT_FINISHED:
      if (mg_tls_client_recv_finish (c) < 0)
	{
	  break;
	}
      mg_tls_client_send_finish (c);
      mg_tls_generate_application_keys (c);
      tls->state = MG_TLS_STATE_CLIENT_CONNECTED;
      c->is_tls_hs = 0;
      break;
    default:
      mg_error (c, "unexpected client state: %d", tls->state);
      break;
    }
}

static void
mg_tls_server_handshake (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  switch (tls->state)
    {
    case MG_TLS_STATE_SERVER_START:
      if (mg_tls_server_recv_hello (c) < 0)
	{
	  return;
	}
      mg_tls_server_send_hello (c);
      mg_tls_generate_handshake_keys (c);
      mg_tls_server_send_ext (c);
      mg_tls_server_send_cert (c);
      mg_tls_server_send_cert_verify (c);
      mg_tls_server_send_finish (c);
      tls->state = MG_TLS_STATE_SERVER_NEGOTIATED;
      // fallthrough
    case MG_TLS_STATE_SERVER_NEGOTIATED:
      if (mg_tls_server_recv_finish (c) < 0)
	{
	  return;
	}
      mg_tls_generate_application_keys (c);
      tls->state = MG_TLS_STATE_SERVER_CONNECTED;
      c->is_tls_hs = 0;
      return;
    default:
      mg_error (c, "unexpected server state: %d", tls->state);
      break;
    }
}

void
mg_tls_handshake (struct mg_connection *c)
{
  if (c->is_client)
    {
      mg_tls_client_handshake (c);
    }
  else
    {
      mg_tls_server_handshake (c);
    }
}

static int
mg_parse_pem (const struct mg_str pem, const struct mg_str label,
	      struct mg_str *der)
{
  size_t n = 0, m = 0;
  char *s;
  const char *c;
  struct mg_str caps[5];
  if (!mg_match (pem, mg_str ("#-----BEGIN #-----#-----END #-----#"), caps))
    {
      der->buf = mg_mprintf ("%.*s", pem.len, pem.buf);
      der->len = pem.len;
      return 0;
    }
  if (mg_strcmp (caps[1], label) != 0 || mg_strcmp (caps[3], label) != 0)
    {
      return -1; // bad label
    }
  if ((s = (char *) calloc (1, caps[2].len)) == NULL)
    {
      return -1;
    }

  for (c = caps[2].buf; c < caps[2].buf + caps[2].len; c++)
    {
      if (*c == ' ' || *c == '\n' || *c == '\r' || *c == '\t')
	{
	  continue;
	}
      s[n++] = *c;
    }
  m = mg_base64_decode (s, n, s, n);
  if (m == 0)
    {
      free (s);
      return -1;
    }
  der->buf = s;
  der->len = m;
  return 0;
}

void
mg_tls_init (struct mg_connection *c, const struct mg_tls_opts *opts)
{
  struct mg_str key;
  struct tls_data *tls
      = (struct tls_data *) calloc (1, sizeof (struct tls_data));
  if (tls == NULL)
    {
      mg_error (c, "tls oom");
      return;
    }

  tls->state
      = c->is_client ? MG_TLS_STATE_CLIENT_START : MG_TLS_STATE_SERVER_START;

  tls->skip_verification = opts->skip_verification;
  tls->send.align = MG_IO_SIZE;

  c->tls = tls;
  c->is_tls = c->is_tls_hs = 1;
  mg_sha256_init (&tls->sha256);

  // save hostname (client extension)
  if (opts->name.len > 0)
    {
      if (opts->name.len >= sizeof (tls->hostname) - 1)
	{
	  mg_error (c, "hostname too long");
	}
      strncpy ((char *) tls->hostname, opts->name.buf,
	       sizeof (tls->hostname) - 1);
      tls->hostname[opts->name.len] = 0;
    }

  if (c->is_client)
    {
      tls->server_cert_der.buf = NULL;
      return;
    }

  // parse PEM or DER certificate
  if (mg_parse_pem (opts->cert, mg_str_s ("CERTIFICATE"),
		    &tls->server_cert_der)
      < 0)
    {
      MG_ERROR (("Failed to load certificate"));
      return;
    }

  // parse PEM or DER EC key
  if (opts->key.buf == NULL)
    {
      mg_error (c, "certificate provided without a private key");
      return;
    }

  if (mg_parse_pem (opts->key, mg_str_s ("EC PRIVATE KEY"), &key) == 0)
    {
      if (key.len < 39)
	{
	  MG_ERROR (("EC private key too short"));
	  return;
	}
      // expect ASN.1 SEQUENCE=[INTEGER=1, BITSTRING of 32 bytes, ...]
      // 30 nn 02 01 01 04 20 [key] ...
      if (key.buf[0] != 0x30 || (key.buf[1] & 0x80) != 0)
	{
	  MG_ERROR (("EC private key: ASN.1 bad sequence"));
	  return;
	}
      if (memcmp (key.buf + 2, "\x02\x01\x01\x04\x20", 5) != 0)
	{
	  MG_ERROR (("EC private key: ASN.1 bad data"));
	}
      memmove (tls->server_key, key.buf + 7, 32);
      free ((void *) key.buf);
    }
  else if (mg_parse_pem (opts->key, mg_str_s ("PRIVATE KEY"), &key) == 0)
    {
      mg_error (c, "PKCS8 private key format is not supported");
    }
  else
    {
      mg_error (c, "expected EC PRIVATE KEY or PRIVATE KEY");
    }
}

void
mg_tls_free (struct mg_connection *c)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  if (tls != NULL)
    {
      mg_iobuf_free (&tls->send);
      free ((void *) tls->server_cert_der.buf);
    }
  free (c->tls);
  c->tls = NULL;
}

long
mg_tls_send (struct mg_connection *c, const void *buf, size_t len)
{
  struct tls_data *tls = (struct tls_data *) c->tls;
  long n = MG_IO_WAIT;
  if (len > MG_IO_SIZE)
    len = MG_IO_SIZE;
  mg_tls_encrypt (c, (const uint8_t *) buf, len, MG_TLS_APP_DATA);
  while (tls->send.len > 0
	 && (n = mg_io_send (c, tls->send.buf, tls->send.len)) > 0)
    {
      mg_iobuf_del (&tls->send, 0, (size_t) n);
    }
  if (n == MG_IO_ERR || n == MG_IO_WAIT)
    return n;
  return (long) len;
}

long
mg_tls_recv (struct mg_connection *c, void *buf, size_t len)
{
  int r = 0;
  struct tls_data *tls = (struct tls_data *) c->tls;
  size_t minlen;

  r = mg_tls_recv_record (c);
  if (r < 0)
    {
      return r;
    }
  if (tls->content_type != MG_TLS_APP_DATA)
    {
      tls->recv.len = 0;
      mg_tls_drop_record (c);
      return MG_IO_WAIT;
    }
  minlen = len < tls->recv.len ? len : tls->recv.len;
  memmove (buf, tls->recv.buf, minlen);
  tls->recv.buf += minlen;
  tls->recv.len -= minlen;
  if (tls->recv.len == 0)
    {
      mg_tls_drop_record (c);
    }
  return (long) minlen;
}

size_t
mg_tls_pending (struct mg_connection *c)
{
  return mg_tls_got_record (c) ? 1 : 0;
}

void
mg_tls_ctx_init (struct mg_mgr *mgr)
{
  (void) mgr;
}

void
mg_tls_ctx_free (struct mg_mgr *mgr)
{
  (void) mgr;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_dummy.c"
#endif

#if MG_TLS == MG_TLS_NONE
void
mg_tls_init (struct mg_connection *c, const struct mg_tls_opts *opts)
{
  (void) opts;
  mg_error (c, "TLS is not enabled");
}
void
mg_tls_handshake (struct mg_connection *c)
{
  (void) c;
}
void
mg_tls_free (struct mg_connection *c)
{
  (void) c;
}
long
mg_tls_recv (struct mg_connection *c, void *buf, size_t len)
{
  return c == NULL || buf == NULL || len == 0 ? 0 : -1;
}
long
mg_tls_send (struct mg_connection *c, const void *buf, size_t len)
{
  return c == NULL || buf == NULL || len == 0 ? 0 : -1;
}
size_t
mg_tls_pending (struct mg_connection *c)
{
  (void) c;
  return 0;
}
void
mg_tls_ctx_init (struct mg_mgr *mgr)
{
  (void) mgr;
}
void
mg_tls_ctx_free (struct mg_mgr *mgr)
{
  (void) mgr;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_mbed.c"
#endif

#if MG_TLS == MG_TLS_MBED

#  if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_NUMBER >= 0x03000000
#    define MG_MBEDTLS_RNG_GET , mg_mbed_rng, NULL
#  else
#    define MG_MBEDTLS_RNG_GET
#  endif

static int
mg_mbed_rng (void *ctx, unsigned char *buf, size_t len)
{
  mg_random (buf, len);
  (void) ctx;
  return 0;
}

static bool
mg_load_cert (struct mg_str str, mbedtls_x509_crt *p)
{
  int rc;
  if (str.buf == NULL || str.buf[0] == '\0' || str.buf[0] == '*')
    return true;
  if (str.buf[0] == '-')
    str.len++; // PEM, include trailing NUL
  if ((rc = mbedtls_x509_crt_parse (p, (uint8_t *) str.buf, str.len)) != 0)
    {
      MG_ERROR (("cert err %#x", -rc));
      return false;
    }
  return true;
}

static bool
mg_load_key (struct mg_str str, mbedtls_pk_context *p)
{
  int rc;
  if (str.buf == NULL || str.buf[0] == '\0' || str.buf[0] == '*')
    return true;
  if (str.buf[0] == '-')
    str.len++; // PEM, include trailing NUL
  if ((rc = mbedtls_pk_parse_key (p, (uint8_t *) str.buf, str.len, NULL,
				  0 MG_MBEDTLS_RNG_GET))
      != 0)
    {
      MG_ERROR (("key err %#x", -rc));
      return false;
    }
  return true;
}

void
mg_tls_free (struct mg_connection *c)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  if (tls != NULL)
    {
      mbedtls_ssl_free (&tls->ssl);
      mbedtls_pk_free (&tls->pk);
      mbedtls_x509_crt_free (&tls->ca);
      mbedtls_x509_crt_free (&tls->cert);
      mbedtls_ssl_config_free (&tls->conf);
#  ifdef MBEDTLS_SSL_SESSION_TICKETS
      mbedtls_ssl_ticket_free (&tls->ticket);
#  endif
      free (tls);
      c->tls = NULL;
    }
}

static int
mg_net_send (void *ctx, const unsigned char *buf, size_t len)
{
  long n = mg_io_send ((struct mg_connection *) ctx, buf, len);
  MG_VERBOSE (
      ("%lu n=%ld e=%d", ((struct mg_connection *) ctx)->id, n, errno));
  if (n == MG_IO_WAIT)
    return MBEDTLS_ERR_SSL_WANT_WRITE;
  if (n == MG_IO_RESET)
    return MBEDTLS_ERR_NET_CONN_RESET;
  if (n == MG_IO_ERR)
    return MBEDTLS_ERR_NET_SEND_FAILED;
  return (int) n;
}

static int
mg_net_recv (void *ctx, unsigned char *buf, size_t len)
{
  long n = mg_io_recv ((struct mg_connection *) ctx, buf, len);
  MG_VERBOSE (("%lu n=%ld", ((struct mg_connection *) ctx)->id, n));
  if (n == MG_IO_WAIT)
    return MBEDTLS_ERR_SSL_WANT_WRITE;
  if (n == MG_IO_RESET)
    return MBEDTLS_ERR_NET_CONN_RESET;
  if (n == MG_IO_ERR)
    return MBEDTLS_ERR_NET_RECV_FAILED;
  return (int) n;
}

void
mg_tls_handshake (struct mg_connection *c)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int rc = mbedtls_ssl_handshake (&tls->ssl);
  if (rc == 0)
    { // Success
      MG_DEBUG (("%lu success", c->id));
      c->is_tls_hs = 0;
      mg_call (c, MG_EV_TLS_HS, NULL);
    }
  else if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE)
    { // Still pending
      MG_VERBOSE (("%lu pending, %d%d %d (-%#x)", c->id, c->is_connecting,
		   c->is_tls_hs, rc, -rc));
    }
  else
    {
      mg_error (c, "TLS handshake: -%#x", -rc); // Error
    }
}

static void
debug_cb (void *c, int lev, const char *s, int n, const char *s2)
{
  n = (int) strlen (s2) - 1;
  MG_INFO (("%lu %d %.*s", ((struct mg_connection *) c)->id, lev, n, s2));
  (void) s;
}

void
mg_tls_init (struct mg_connection *c, const struct mg_tls_opts *opts)
{
  struct mg_tls *tls = (struct mg_tls *) calloc (1, sizeof (*tls));
  int rc = 0;
  c->tls = tls;
  if (c->tls == NULL)
    {
      mg_error (c, "TLS OOM");
      goto fail;
    }
  if (c->is_listening)
    goto fail;
  MG_DEBUG (("%lu Setting TLS", c->id));
  MG_PROF_ADD (c, "mbedtls_init_start");
#  if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_NUMBER >= 0x03000000 \
      && defined(MBEDTLS_PSA_CRYPTO_C)
  psa_crypto_init (); // https://github.com/Mbed-TLS/mbedtls/issues/9072#issuecomment-2084845711
#  endif
  mbedtls_ssl_init (&tls->ssl);
  mbedtls_ssl_config_init (&tls->conf);
  mbedtls_x509_crt_init (&tls->ca);
  mbedtls_x509_crt_init (&tls->cert);
  mbedtls_pk_init (&tls->pk);
  mbedtls_ssl_conf_dbg (&tls->conf, debug_cb, c);
#  if defined(MG_MBEDTLS_DEBUG_LEVEL)
  mbedtls_debug_set_threshold (MG_MBEDTLS_DEBUG_LEVEL);
#  endif
  if ((rc = mbedtls_ssl_config_defaults (
	   &tls->conf,
	   c->is_client ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
	   MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT))
      != 0)
    {
      mg_error (c, "tls defaults %#x", -rc);
      goto fail;
    }
  mbedtls_ssl_conf_rng (&tls->conf, mg_mbed_rng, c);

  if (opts->ca.len == 0 || mg_strcmp (opts->ca, mg_str ("*")) == 0)
    {
      // NOTE: MBEDTLS_SSL_VERIFY_NONE is not supported for TLS1.3 on client
      // side See https://github.com/Mbed-TLS/mbedtls/issues/7075
      mbedtls_ssl_conf_authmode (&tls->conf, MBEDTLS_SSL_VERIFY_NONE);
    }
  else
    {
      if (mg_load_cert (opts->ca, &tls->ca) == false)
	goto fail;
      mbedtls_ssl_conf_ca_chain (&tls->conf, &tls->ca, NULL);
      if (c->is_client && opts->name.buf != NULL && opts->name.buf[0] != '\0')
	{
	  char *host = mg_mprintf ("%.*s", opts->name.len, opts->name.buf);
	  mbedtls_ssl_set_hostname (&tls->ssl, host);
	  MG_DEBUG (("%lu hostname verification: %s", c->id, host));
	  free (host);
	}
      mbedtls_ssl_conf_authmode (&tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }
  if (!mg_load_cert (opts->cert, &tls->cert))
    goto fail;
  if (!mg_load_key (opts->key, &tls->pk))
    goto fail;
  if (tls->cert.version
      && (rc = mbedtls_ssl_conf_own_cert (&tls->conf, &tls->cert, &tls->pk))
	     != 0)
    {
      mg_error (c, "own cert %#x", -rc);
      goto fail;
    }

#  ifdef MBEDTLS_SSL_SESSION_TICKETS
  mbedtls_ssl_conf_session_tickets_cb (
      &tls->conf, mbedtls_ssl_ticket_write, mbedtls_ssl_ticket_parse,
      &((struct mg_tls_ctx *) c->mgr->tls_ctx)->tickets);
#  endif

  if ((rc = mbedtls_ssl_setup (&tls->ssl, &tls->conf)) != 0)
    {
      mg_error (c, "setup err %#x", -rc);
      goto fail;
    }
  c->is_tls = 1;
  c->is_tls_hs = 1;
  mbedtls_ssl_set_bio (&tls->ssl, c, mg_net_send, mg_net_recv, 0);
  MG_PROF_ADD (c, "mbedtls_init_end");
  if (c->is_client && c->is_resolving == 0 && c->is_connecting == 0)
    {
      mg_tls_handshake (c);
    }
  return;
fail:
  mg_tls_free (c);
}

size_t
mg_tls_pending (struct mg_connection *c)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  return tls == NULL ? 0 : mbedtls_ssl_get_bytes_avail (&tls->ssl);
}

long
mg_tls_recv (struct mg_connection *c, void *buf, size_t len)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  long n = mbedtls_ssl_read (&tls->ssl, (unsigned char *) buf, len);
  if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE)
    return MG_IO_WAIT;
#  if defined(MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET)
  if (n == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET)
    {
      return MG_IO_WAIT;
    }
#  endif
  if (n <= 0)
    return MG_IO_ERR;
  return n;
}

long
mg_tls_send (struct mg_connection *c, const void *buf, size_t len)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  long n = mbedtls_ssl_write (&tls->ssl, (unsigned char *) buf, len);
  if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE)
    return MG_IO_WAIT;
  if (n <= 0)
    return MG_IO_ERR;
  return n;
}

void
mg_tls_ctx_init (struct mg_mgr *mgr)
{
  struct mg_tls_ctx *ctx = (struct mg_tls_ctx *) calloc (1, sizeof (*ctx));
  if (ctx == NULL)
    {
      MG_ERROR (("TLS context init OOM"));
    }
  else
    {
#  ifdef MBEDTLS_SSL_SESSION_TICKETS
      int rc;
      mbedtls_ssl_ticket_init (&ctx->tickets);
      if ((rc = mbedtls_ssl_ticket_setup (&ctx->tickets, mg_mbed_rng, NULL,
					  MBEDTLS_CIPHER_AES_128_GCM, 86400))
	  != 0)
	{
	  MG_ERROR ((" mbedtls_ssl_ticket_setup %#x", -rc));
	}
#  endif
      mgr->tls_ctx = ctx;
    }
}

void
mg_tls_ctx_free (struct mg_mgr *mgr)
{
  struct mg_tls_ctx *ctx = (struct mg_tls_ctx *) mgr->tls_ctx;
  if (ctx != NULL)
    {
#  ifdef MBEDTLS_SSL_SESSION_TICKETS
      mbedtls_ssl_ticket_free (&ctx->tickets);
#  endif
      free (ctx);
      mgr->tls_ctx = NULL;
    }
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_openssl.c"
#endif

#if MG_TLS == MG_TLS_OPENSSL || MG_TLS == MG_TLS_WOLFSSL

static int
tls_err_cb (const char *s, size_t len, void *c)
{
  int n = (int) len - 1;
  MG_ERROR (("%lu %.*s", ((struct mg_connection *) c)->id, n, s));
  return 0; // undocumented
}

static int
mg_tls_err (struct mg_connection *c, struct mg_tls *tls, int res)
{
  int err = SSL_get_error (tls->ssl, res);
  // We've just fetched the last error from the queue.
  // Now we need to clear the error queue. If we do not, then the following
  // can happen (actually reported):
  //  - A new connection is accept()-ed with cert error (e.g. self-signed cert)
  //  - Since all accept()-ed connections share listener's context,
  //  - *ALL* SSL accepted connection report read error on the next poll cycle.
  //    Thus a single errored connection can close all the rest, unrelated
  //    ones.
  // Clearing the error keeps the shared SSL_CTX in an OK state.

  if (err != 0)
    ERR_print_errors_cb (tls_err_cb, c);
  ERR_clear_error ();
  if (err == SSL_ERROR_WANT_READ)
    return 0;
  if (err == SSL_ERROR_WANT_WRITE)
    return 0;
  return err;
}

static STACK_OF (X509_INFO) * load_ca_certs (struct mg_str ca)
{
  BIO *bio = BIO_new_mem_buf (ca.buf, (int) ca.len);
  STACK_OF (X509_INFO) *certs
      = bio ? PEM_X509_INFO_read_bio (bio, NULL, NULL, NULL) : NULL;
  if (bio)
    BIO_free (bio);
  return certs;
}

static bool
add_ca_certs (SSL_CTX *ctx, STACK_OF (X509_INFO) * certs)
{
  X509_STORE *cert_store = SSL_CTX_get_cert_store (ctx);
  for (int i = 0; i < sk_X509_INFO_num (certs); i++)
    {
      X509_INFO *cert_info = sk_X509_INFO_value (certs, i);
      if (cert_info->x509
	  && !X509_STORE_add_cert (cert_store, cert_info->x509))
	return false;
    }
  return true;
}

static EVP_PKEY *
load_key (struct mg_str s)
{
  BIO *bio = BIO_new_mem_buf (s.buf, (int) (long) s.len);
  EVP_PKEY *key = bio ? PEM_read_bio_PrivateKey (bio, NULL, 0, NULL) : NULL;
  if (bio)
    BIO_free (bio);
  return key;
}

static X509 *
load_cert (struct mg_str s)
{
  BIO *bio = BIO_new_mem_buf (s.buf, (int) (long) s.len);
  X509 *cert = bio == NULL ? NULL
	       : s.buf[0] == '-'
		   ? PEM_read_bio_X509 (bio, NULL, NULL, NULL) // PEM
		   : d2i_X509_bio (bio, NULL);		       // DER
  if (bio)
    BIO_free (bio);
  return cert;
}

static long
mg_bio_ctrl (BIO *b, int cmd, long larg, void *pargs)
{
  long ret = 0;
  if (cmd == BIO_CTRL_PUSH)
    ret = 1;
  if (cmd == BIO_CTRL_POP)
    ret = 1;
  if (cmd == BIO_CTRL_FLUSH)
    ret = 1;
#  if MG_TLS == MG_TLS_OPENSSL
  if (cmd == BIO_C_SET_NBIO)
    ret = 1;
#  endif
  // MG_DEBUG(("%d -> %ld", cmd, ret));
  (void) b, (void) cmd, (void) larg, (void) pargs;
  return ret;
}

static int
mg_bio_read (BIO *bio, char *buf, int len)
{
  struct mg_connection *c = (struct mg_connection *) BIO_get_data (bio);
  long res = mg_io_recv (c, buf, (size_t) len);
  // MG_DEBUG(("%p %d %ld", buf, len, res));
  len = res > 0 ? (int) res : -1;
  if (res == MG_IO_WAIT)
    BIO_set_retry_read (bio);
  return len;
}

static int
mg_bio_write (BIO *bio, const char *buf, int len)
{
  struct mg_connection *c = (struct mg_connection *) BIO_get_data (bio);
  long res = mg_io_send (c, buf, (size_t) len);
  // MG_DEBUG(("%p %d %ld", buf, len, res));
  len = res > 0 ? (int) res : -1;
  if (res == MG_IO_WAIT)
    BIO_set_retry_write (bio);
  return len;
}

void
mg_tls_init (struct mg_connection *c, const struct mg_tls_opts *opts)
{
  struct mg_tls *tls = (struct mg_tls *) calloc (1, sizeof (*tls));
  const char *id = "mongoose";
  static unsigned char s_initialised = 0;
  BIO *bio = NULL;
  int rc;

  if (tls == NULL)
    {
      mg_error (c, "TLS OOM");
      goto fail;
    }

  if (!s_initialised)
    {
      SSL_library_init ();
      s_initialised++;
    }
  MG_DEBUG (("%lu Setting TLS", c->id));
  tls->ctx = c->is_client ? SSL_CTX_new (SSLv23_client_method ())
			  : SSL_CTX_new (SSLv23_server_method ());
  if ((tls->ssl = SSL_new (tls->ctx)) == NULL)
    {
      mg_error (c, "SSL_new");
      goto fail;
    }
  SSL_set_session_id_context (tls->ssl, (const uint8_t *) id,
			      (unsigned) strlen (id));
  // Disable deprecated protocols
  SSL_set_options (tls->ssl, SSL_OP_NO_SSLv2);
  SSL_set_options (tls->ssl, SSL_OP_NO_SSLv3);
  SSL_set_options (tls->ssl, SSL_OP_NO_TLSv1);
  SSL_set_options (tls->ssl, SSL_OP_NO_TLSv1_1);
#  ifdef MG_ENABLE_OPENSSL_NO_COMPRESSION
  SSL_set_options (tls->ssl, SSL_OP_NO_COMPRESSION);
#  endif
#  ifdef MG_ENABLE_OPENSSL_CIPHER_SERVER_PREFERENCE
  SSL_set_options (tls->ssl, SSL_OP_CIPHER_SERVER_PREFERENCE);
#  endif

#  if MG_TLS == MG_TLS_WOLFSSL && !defined(OPENSSL_COMPATIBLE_DEFAULTS)
  if (opts->ca.len == 0 || mg_strcmp (opts->ca, mg_str ("*")) == 0)
    {
      // Older versions require that either the CA is loaded or SSL_VERIFY_NONE
      // explicitly set
      SSL_set_verify (tls->ssl, SSL_VERIFY_NONE, NULL);
    }
#  endif
  if (opts->ca.buf != NULL && opts->ca.buf[0] != '\0')
    {
      SSL_set_verify (tls->ssl,
		      SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
      STACK_OF (X509_INFO) *certs = load_ca_certs (opts->ca);
      rc = add_ca_certs (tls->ctx, certs);
      sk_X509_INFO_pop_free (certs, X509_INFO_free);
      if (!rc)
	{
	  mg_error (c, "CA err");
	  goto fail;
	}
    }
  if (opts->cert.buf != NULL && opts->cert.buf[0] != '\0')
    {
      X509 *cert = load_cert (opts->cert);
      rc = cert == NULL ? 0 : SSL_use_certificate (tls->ssl, cert);
      X509_free (cert);
      if (cert == NULL || rc != 1)
	{
	  mg_error (c, "CERT err %d", mg_tls_err (c, tls, rc));
	  goto fail;
	}
    }
  if (opts->key.buf != NULL && opts->key.buf[0] != '\0')
    {
      EVP_PKEY *key = load_key (opts->key);
      rc = key == NULL ? 0 : SSL_use_PrivateKey (tls->ssl, key);
      EVP_PKEY_free (key);
      if (key == NULL || rc != 1)
	{
	  mg_error (c, "KEY err %d", mg_tls_err (c, tls, rc));
	  goto fail;
	}
    }

  SSL_set_mode (tls->ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#  if MG_TLS == MG_TLS_OPENSSL && OPENSSL_VERSION_NUMBER > 0x10002000L
  (void) SSL_set_ecdh_auto (tls->ssl, 1);
#  endif
#  if OPENSSL_VERSION_NUMBER >= 0x10100000L
  if (opts->name.len > 0)
    {
      char *s = mg_mprintf ("%.*s", (int) opts->name.len, opts->name.buf);
#    if MG_TLS != MG_TLS_WOLFSSL || LIBWOLFSSL_VERSION_HEX >= 0x05005002
      SSL_set1_host (tls->ssl, s);
#    else
      X509_VERIFY_PARAM_set1_host (SSL_get0_param (tls->ssl), s, 0);
#    endif
      SSL_set_tlsext_host_name (tls->ssl, s);
      free (s);
    }
#  endif
#  if MG_TLS == MG_TLS_WOLFSSL
  tls->bm = BIO_meth_new (0, "bio_mg");
#  else
  tls->bm
      = BIO_meth_new (BIO_get_new_index () | BIO_TYPE_SOURCE_SINK, "bio_mg");
#  endif
  BIO_meth_set_write (tls->bm, mg_bio_write);
  BIO_meth_set_read (tls->bm, mg_bio_read);
  BIO_meth_set_ctrl (tls->bm, mg_bio_ctrl);

  bio = BIO_new (tls->bm);
  BIO_set_data (bio, c);
  SSL_set_bio (tls->ssl, bio, bio);

  c->tls = tls;
  c->is_tls = 1;
  c->is_tls_hs = 1;
  if (c->is_client && c->is_resolving == 0 && c->is_connecting == 0)
    {
      mg_tls_handshake (c);
    }
  MG_DEBUG (("%lu SSL %s OK", c->id, c->is_accepted ? "accept" : "client"));
  return;
fail:
  free (tls);
}

void
mg_tls_handshake (struct mg_connection *c)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int rc = c->is_client ? SSL_connect (tls->ssl) : SSL_accept (tls->ssl);
  if (rc == 1)
    {
      MG_DEBUG (("%lu success", c->id));
      c->is_tls_hs = 0;
      mg_call (c, MG_EV_TLS_HS, NULL);
    }
  else
    {
      int code = mg_tls_err (c, tls, rc);
      if (code != 0)
	mg_error (c, "tls hs: rc %d, err %d", rc, code);
    }
}

void
mg_tls_free (struct mg_connection *c)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  if (tls == NULL)
    return;
  SSL_free (tls->ssl);
  SSL_CTX_free (tls->ctx);
  BIO_meth_free (tls->bm);
  free (tls);
  c->tls = NULL;
}

size_t
mg_tls_pending (struct mg_connection *c)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  return tls == NULL ? 0 : (size_t) SSL_pending (tls->ssl);
}

long
mg_tls_recv (struct mg_connection *c, void *buf, size_t len)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int n = SSL_read (tls->ssl, buf, (int) len);
  if (n < 0 && mg_tls_err (c, tls, n) == 0)
    return MG_IO_WAIT;
  if (n <= 0)
    return MG_IO_ERR;
  return n;
}

long
mg_tls_send (struct mg_connection *c, const void *buf, size_t len)
{
  struct mg_tls *tls = (struct mg_tls *) c->tls;
  int n = SSL_write (tls->ssl, buf, (int) len);
  if (n < 0 && mg_tls_err (c, tls, n) == 0)
    return MG_IO_WAIT;
  if (n <= 0)
    return MG_IO_ERR;
  return n;
}

void
mg_tls_ctx_init (struct mg_mgr *mgr)
{
  (void) mgr;
}

void
mg_tls_ctx_free (struct mg_mgr *mgr)
{
  (void) mgr;
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_uecc.c"
#endif
/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#if MG_TLS == MG_TLS_BUILTIN

#  ifndef MG_UECC_RNG_MAX_TRIES
#    define MG_UECC_RNG_MAX_TRIES 64
#  endif

#  if MG_UECC_ENABLE_VLI_API
#    define MG_UECC_VLI_API
#  else
#    define MG_UECC_VLI_API static
#  endif

#  if (MG_UECC_PLATFORM == mg_uecc_avr) || (MG_UECC_PLATFORM == mg_uecc_arm)  \
      || (MG_UECC_PLATFORM == mg_uecc_arm_thumb)                              \
      || (MG_UECC_PLATFORM == mg_uecc_arm_thumb2)
#    define CONCATX(a, ...) a##__VA_ARGS__
#    define CONCAT(a, ...) CONCATX (a, __VA_ARGS__)

#    define STRX(a) #a
#    define STR(a) STRX (a)

#    define EVAL(...) EVAL1 (EVAL1 (EVAL1 (EVAL1 (__VA_ARGS__))))
#    define EVAL1(...) EVAL2 (EVAL2 (EVAL2 (EVAL2 (__VA_ARGS__))))
#    define EVAL2(...) EVAL3 (EVAL3 (EVAL3 (EVAL3 (__VA_ARGS__))))
#    define EVAL3(...) EVAL4 (EVAL4 (EVAL4 (EVAL4 (__VA_ARGS__))))
#    define EVAL4(...) __VA_ARGS__

#    define DEC_1 0
#    define DEC_2 1
#    define DEC_3 2
#    define DEC_4 3
#    define DEC_5 4
#    define DEC_6 5
#    define DEC_7 6
#    define DEC_8 7
#    define DEC_9 8
#    define DEC_10 9
#    define DEC_11 10
#    define DEC_12 11
#    define DEC_13 12
#    define DEC_14 13
#    define DEC_15 14
#    define DEC_16 15
#    define DEC_17 16
#    define DEC_18 17
#    define DEC_19 18
#    define DEC_20 19
#    define DEC_21 20
#    define DEC_22 21
#    define DEC_23 22
#    define DEC_24 23
#    define DEC_25 24
#    define DEC_26 25
#    define DEC_27 26
#    define DEC_28 27
#    define DEC_29 28
#    define DEC_30 29
#    define DEC_31 30
#    define DEC_32 31

#    define DEC(N) CONCAT (DEC_, N)

#    define SECOND_ARG(_, val, ...) val
#    define SOME_CHECK_0 ~, 0
#    define GET_SECOND_ARG(...) SECOND_ARG (__VA_ARGS__, SOME, )
#    define SOME_OR_0(N) GET_SECOND_ARG (CONCAT (SOME_CHECK_, N))

#    define EMPTY(...)
#    define DEFER(...) __VA_ARGS__ EMPTY ()

#    define REPEAT_NAME_0() REPEAT_0
#    define REPEAT_NAME_SOME() REPEAT_SOME
#    define REPEAT_0(...)
#    define REPEAT_SOME(N, stuff)                                             \
      DEFER (CONCAT (REPEAT_NAME_, SOME_OR_0 (DEC (N)))) () (DEC (N),         \
							     stuff) stuff
#    define REPEAT(N, stuff) EVAL (REPEAT_SOME (N, stuff))

#    define REPEATM_NAME_0() REPEATM_0
#    define REPEATM_NAME_SOME() REPEATM_SOME
#    define REPEATM_0(...)
#    define REPEATM_SOME(N, macro)                                            \
      macro (N) DEFER (CONCAT (REPEATM_NAME_, SOME_OR_0 (DEC (N)))) () (      \
	  DEC (N), macro)
#    define REPEATM(N, macro) EVAL (REPEATM_SOME (N, macro))
#  endif

//

#  if (MG_UECC_WORD_SIZE == 1)
#    if MG_UECC_SUPPORTS_secp160r1
#      define MG_UECC_MAX_WORDS 21 /* Due to the size of curve_n. */
#    endif
#    if MG_UECC_SUPPORTS_secp192r1
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 24
#    endif
#    if MG_UECC_SUPPORTS_secp224r1
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 28
#    endif
#    if (MG_UECC_SUPPORTS_secp256r1 || MG_UECC_SUPPORTS_secp256k1)
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 32
#    endif
#  elif (MG_UECC_WORD_SIZE == 4)
#    if MG_UECC_SUPPORTS_secp160r1
#      define MG_UECC_MAX_WORDS 6 /* Due to the size of curve_n. */
#    endif
#    if MG_UECC_SUPPORTS_secp192r1
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 6
#    endif
#    if MG_UECC_SUPPORTS_secp224r1
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 7
#    endif
#    if (MG_UECC_SUPPORTS_secp256r1 || MG_UECC_SUPPORTS_secp256k1)
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 8
#    endif
#  elif (MG_UECC_WORD_SIZE == 8)
#    if MG_UECC_SUPPORTS_secp160r1
#      define MG_UECC_MAX_WORDS 3
#    endif
#    if MG_UECC_SUPPORTS_secp192r1
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 3
#    endif
#    if MG_UECC_SUPPORTS_secp224r1
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 4
#    endif
#    if (MG_UECC_SUPPORTS_secp256r1 || MG_UECC_SUPPORTS_secp256k1)
#      undef MG_UECC_MAX_WORDS
#      define MG_UECC_MAX_WORDS 4
#    endif
#  endif /* MG_UECC_WORD_SIZE */

#  define BITS_TO_WORDS(num_bits)                                             \
    ((wordcount_t) ((num_bits + ((MG_UECC_WORD_SIZE * 8) - 1))                \
		    / (MG_UECC_WORD_SIZE * 8)))
#  define BITS_TO_BYTES(num_bits) ((num_bits + 7) / 8)

struct MG_UECC_Curve_t
{
  wordcount_t num_words;
  wordcount_t num_bytes;
  bitcount_t num_n_bits;
  mg_uecc_word_t p[MG_UECC_MAX_WORDS];
  mg_uecc_word_t n[MG_UECC_MAX_WORDS];
  mg_uecc_word_t G[MG_UECC_MAX_WORDS * 2];
  mg_uecc_word_t b[MG_UECC_MAX_WORDS];
  void (*double_jacobian) (mg_uecc_word_t *X1, mg_uecc_word_t *Y1,
			   mg_uecc_word_t *Z1, MG_UECC_Curve curve);
#  if MG_UECC_SUPPORT_COMPRESSED_POINT
  void (*mod_sqrt) (mg_uecc_word_t *a, MG_UECC_Curve curve);
#  endif
  void (*x_side) (mg_uecc_word_t *result, const mg_uecc_word_t *x,
		  MG_UECC_Curve curve);
#  if (MG_UECC_OPTIMIZATION_LEVEL > 0)
  void (*mmod_fast) (mg_uecc_word_t *result, mg_uecc_word_t *product);
#  endif
};

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
static void
bcopy (uint8_t *dst, const uint8_t *src, unsigned num_bytes)
{
  while (0 != num_bytes)
    {
      num_bytes--;
      dst[num_bytes] = src[num_bytes];
    }
}
#  endif

static cmpresult_t mg_uecc_vli_cmp_unsafe (const mg_uecc_word_t *left,
					   const mg_uecc_word_t *right,
					   wordcount_t num_words);

#  if (MG_UECC_PLATFORM == mg_uecc_arm                                        \
       || MG_UECC_PLATFORM == mg_uecc_arm_thumb                               \
       || MG_UECC_PLATFORM == mg_uecc_arm_thumb2)

#  endif

#  if (MG_UECC_PLATFORM == mg_uecc_avr)

#  endif

#  ifndef asm_clear
#    define asm_clear 0
#  endif
#  ifndef asm_set
#    define asm_set 0
#  endif
#  ifndef asm_add
#    define asm_add 0
#  endif
#  ifndef asm_sub
#    define asm_sub 0
#  endif
#  ifndef asm_mult
#    define asm_mult 0
#  endif
#  ifndef asm_rshift1
#    define asm_rshift1 0
#  endif
#  ifndef asm_mmod_fast_secp256r1
#    define asm_mmod_fast_secp256r1 0
#  endif

#  if defined(default_RNG_defined) && default_RNG_defined
static MG_UECC_RNG_Function g_rng_function = &default_RNG;
#  else
static MG_UECC_RNG_Function g_rng_function = 0;
#  endif

void
mg_uecc_set_rng (MG_UECC_RNG_Function rng_function)
{
  g_rng_function = rng_function;
}

MG_UECC_RNG_Function
mg_uecc_get_rng (void)
{
  return g_rng_function;
}

int
mg_uecc_curve_private_key_size (MG_UECC_Curve curve)
{
  return BITS_TO_BYTES (curve->num_n_bits);
}

int
mg_uecc_curve_public_key_size (MG_UECC_Curve curve)
{
  return 2 * curve->num_bytes;
}

#  if !asm_clear
MG_UECC_VLI_API void
mg_uecc_vli_clear (mg_uecc_word_t *vli, wordcount_t num_words)
{
  wordcount_t i;
  for (i = 0; i < num_words; ++i)
    {
      vli[i] = 0;
    }
}
#  endif /* !asm_clear */

/* Constant-time comparison to zero - secure way to compare long integers */
/* Returns 1 if vli == 0, 0 otherwise. */
MG_UECC_VLI_API mg_uecc_word_t
mg_uecc_vli_isZero (const mg_uecc_word_t *vli, wordcount_t num_words)
{
  mg_uecc_word_t bits = 0;
  wordcount_t i;
  for (i = 0; i < num_words; ++i)
    {
      bits |= vli[i];
    }
  return (bits == 0);
}

/* Returns nonzero if bit 'bit' of vli is set. */
MG_UECC_VLI_API mg_uecc_word_t
mg_uecc_vli_testBit (const mg_uecc_word_t *vli, bitcount_t bit)
{
  return (vli[bit >> MG_UECC_WORD_BITS_SHIFT]
	  & ((mg_uecc_word_t) 1 << (bit & MG_UECC_WORD_BITS_MASK)));
}

/* Counts the number of words in vli. */
static wordcount_t
vli_numDigits (const mg_uecc_word_t *vli, const wordcount_t max_words)
{
  wordcount_t i;
  /* Search from the end until we find a non-zero digit.
     We do it in reverse because we expect that most digits will be nonzero. */
  for (i = max_words - 1; i >= 0 && vli[i] == 0; --i)
    {
    }

  return (i + 1);
}

/* Counts the number of bits required to represent vli. */
MG_UECC_VLI_API bitcount_t
mg_uecc_vli_numBits (const mg_uecc_word_t *vli, const wordcount_t max_words)
{
  mg_uecc_word_t i;
  mg_uecc_word_t digit;

  wordcount_t num_digits = vli_numDigits (vli, max_words);
  if (num_digits == 0)
    {
      return 0;
    }

  digit = vli[num_digits - 1];
  for (i = 0; digit; ++i)
    {
      digit >>= 1;
    }

  return (((bitcount_t) ((num_digits - 1) << MG_UECC_WORD_BITS_SHIFT))
	  + (bitcount_t) i);
}

/* Sets dest = src. */
#  if !asm_set
MG_UECC_VLI_API void
mg_uecc_vli_set (mg_uecc_word_t *dest, const mg_uecc_word_t *src,
		 wordcount_t num_words)
{
  wordcount_t i;
  for (i = 0; i < num_words; ++i)
    {
      dest[i] = src[i];
    }
}
#  endif /* !asm_set */

/* Returns sign of left - right. */
static cmpresult_t
mg_uecc_vli_cmp_unsafe (const mg_uecc_word_t *left,
			const mg_uecc_word_t *right, wordcount_t num_words)
{
  wordcount_t i;
  for (i = num_words - 1; i >= 0; --i)
    {
      if (left[i] > right[i])
	{
	  return 1;
	}
      else if (left[i] < right[i])
	{
	  return -1;
	}
    }
  return 0;
}

/* Constant-time comparison function - secure way to compare long integers */
/* Returns one if left == right, zero otherwise. */
MG_UECC_VLI_API mg_uecc_word_t
mg_uecc_vli_equal (const mg_uecc_word_t *left, const mg_uecc_word_t *right,
		   wordcount_t num_words)
{
  mg_uecc_word_t diff = 0;
  wordcount_t i;
  for (i = num_words - 1; i >= 0; --i)
    {
      diff |= (left[i] ^ right[i]);
    }
  return (diff == 0);
}

MG_UECC_VLI_API mg_uecc_word_t mg_uecc_vli_sub (mg_uecc_word_t *result,
						const mg_uecc_word_t *left,
						const mg_uecc_word_t *right,
						wordcount_t num_words);

/* Returns sign of left - right, in constant time. */
MG_UECC_VLI_API cmpresult_t
mg_uecc_vli_cmp (const mg_uecc_word_t *left, const mg_uecc_word_t *right,
		 wordcount_t num_words)
{
  mg_uecc_word_t tmp[MG_UECC_MAX_WORDS];
  mg_uecc_word_t neg = !!mg_uecc_vli_sub (tmp, left, right, num_words);
  mg_uecc_word_t equal = mg_uecc_vli_isZero (tmp, num_words);
  return (cmpresult_t) (!equal - 2 * neg);
}

/* Computes vli = vli >> 1. */
#  if !asm_rshift1
MG_UECC_VLI_API void
mg_uecc_vli_rshift1 (mg_uecc_word_t *vli, wordcount_t num_words)
{
  mg_uecc_word_t *end = vli;
  mg_uecc_word_t carry = 0;

  vli += num_words;
  while (vli-- > end)
    {
      mg_uecc_word_t temp = *vli;
      *vli = (temp >> 1) | carry;
      carry = temp << (MG_UECC_WORD_BITS - 1);
    }
}
#  endif /* !asm_rshift1 */

/* Computes result = left + right, returning carry. Can modify in place. */
#  if !asm_add
MG_UECC_VLI_API mg_uecc_word_t
mg_uecc_vli_add (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		 const mg_uecc_word_t *right, wordcount_t num_words)
{
  mg_uecc_word_t carry = 0;
  wordcount_t i;
  for (i = 0; i < num_words; ++i)
    {
      mg_uecc_word_t sum = left[i] + right[i] + carry;
      if (sum != left[i])
	{
	  carry = (sum < left[i]);
	}
      result[i] = sum;
    }
  return carry;
}
#  endif /* !asm_add */

/* Computes result = left - right, returning borrow. Can modify in place. */
#  if !asm_sub
MG_UECC_VLI_API mg_uecc_word_t
mg_uecc_vli_sub (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		 const mg_uecc_word_t *right, wordcount_t num_words)
{
  mg_uecc_word_t borrow = 0;
  wordcount_t i;
  for (i = 0; i < num_words; ++i)
    {
      mg_uecc_word_t diff = left[i] - right[i] - borrow;
      if (diff != left[i])
	{
	  borrow = (diff > left[i]);
	}
      result[i] = diff;
    }
  return borrow;
}
#  endif /* !asm_sub */

#  if !asm_mult || (MG_UECC_SQUARE_FUNC && !asm_square)                       \
      || (MG_UECC_SUPPORTS_secp256k1 && (MG_UECC_OPTIMIZATION_LEVEL > 0)      \
	  && ((MG_UECC_WORD_SIZE == 1) || (MG_UECC_WORD_SIZE == 8)))
static void
muladd (mg_uecc_word_t a, mg_uecc_word_t b, mg_uecc_word_t *r0,
	mg_uecc_word_t *r1, mg_uecc_word_t *r2)
{
#    if MG_UECC_WORD_SIZE == 8
  uint64_t a0 = a & 0xffffffff;
  uint64_t a1 = a >> 32;
  uint64_t b0 = b & 0xffffffff;
  uint64_t b1 = b >> 32;

  uint64_t i0 = a0 * b0;
  uint64_t i1 = a0 * b1;
  uint64_t i2 = a1 * b0;
  uint64_t i3 = a1 * b1;

  uint64_t p0, p1;

  i2 += (i0 >> 32);
  i2 += i1;
  if (i2 < i1)
    { /* overflow */
      i3 += 0x100000000;
    }

  p0 = (i0 & 0xffffffff) | (i2 << 32);
  p1 = i3 + (i2 >> 32);

  *r0 += p0;
  *r1 += (p1 + (*r0 < p0));
  *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#    else
  mg_uecc_dword_t p = (mg_uecc_dword_t) a * b;
  mg_uecc_dword_t r01 = ((mg_uecc_dword_t) (*r1) << MG_UECC_WORD_BITS) | *r0;
  r01 += p;
  *r2 += (r01 < p);
  *r1 = (mg_uecc_word_t) (r01 >> MG_UECC_WORD_BITS);
  *r0 = (mg_uecc_word_t) r01;
#    endif
}
#  endif /* muladd needed */

#  if !asm_mult
MG_UECC_VLI_API void
mg_uecc_vli_mult (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		  const mg_uecc_word_t *right, wordcount_t num_words)
{
  mg_uecc_word_t r0 = 0;
  mg_uecc_word_t r1 = 0;
  mg_uecc_word_t r2 = 0;
  wordcount_t i, k;

  /* Compute each digit of result in sequence, maintaining the carries. */
  for (k = 0; k < num_words; ++k)
    {
      for (i = 0; i <= k; ++i)
	{
	  muladd (left[i], right[k - i], &r0, &r1, &r2);
	}
      result[k] = r0;
      r0 = r1;
      r1 = r2;
      r2 = 0;
    }
  for (k = num_words; k < num_words * 2 - 1; ++k)
    {
      for (i = (wordcount_t) ((k + 1) - num_words); i < num_words; ++i)
	{
	  muladd (left[i], right[k - i], &r0, &r1, &r2);
	}
      result[k] = r0;
      r0 = r1;
      r1 = r2;
      r2 = 0;
    }
  result[num_words * 2 - 1] = r0;
}
#  endif /* !asm_mult */

#  if MG_UECC_SQUARE_FUNC

#    if !asm_square
static void
mul2add (mg_uecc_word_t a, mg_uecc_word_t b, mg_uecc_word_t *r0,
	 mg_uecc_word_t *r1, mg_uecc_word_t *r2)
{
#      if MG_UECC_WORD_SIZE == 8
  uint64_t a0 = a & 0xffffffffull;
  uint64_t a1 = a >> 32;
  uint64_t b0 = b & 0xffffffffull;
  uint64_t b1 = b >> 32;

  uint64_t i0 = a0 * b0;
  uint64_t i1 = a0 * b1;
  uint64_t i2 = a1 * b0;
  uint64_t i3 = a1 * b1;

  uint64_t p0, p1;

  i2 += (i0 >> 32);
  i2 += i1;
  if (i2 < i1)
    { /* overflow */
      i3 += 0x100000000ull;
    }

  p0 = (i0 & 0xffffffffull) | (i2 << 32);
  p1 = i3 + (i2 >> 32);

  *r2 += (p1 >> 63);
  p1 = (p1 << 1) | (p0 >> 63);
  p0 <<= 1;

  *r0 += p0;
  *r1 += (p1 + (*r0 < p0));
  *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#      else
  mg_uecc_dword_t p = (mg_uecc_dword_t) a * b;
  mg_uecc_dword_t r01 = ((mg_uecc_dword_t) (*r1) << MG_UECC_WORD_BITS) | *r0;
  *r2 += (p >> (MG_UECC_WORD_BITS * 2 - 1));
  p *= 2;
  r01 += p;
  *r2 += (r01 < p);
  *r1 = r01 >> MG_UECC_WORD_BITS;
  *r0 = (mg_uecc_word_t) r01;
#      endif
}

MG_UECC_VLI_API void
mg_uecc_vli_square (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		    wordcount_t num_words)
{
  mg_uecc_word_t r0 = 0;
  mg_uecc_word_t r1 = 0;
  mg_uecc_word_t r2 = 0;

  wordcount_t i, k;

  for (k = 0; k < num_words * 2 - 1; ++k)
    {
      mg_uecc_word_t min = (k < num_words ? 0 : (k + 1) - num_words);
      for (i = min; i <= k && i <= k - i; ++i)
	{
	  if (i < k - i)
	    {
	      mul2add (left[i], left[k - i], &r0, &r1, &r2);
	    }
	  else
	    {
	      muladd (left[i], left[k - i], &r0, &r1, &r2);
	    }
	}
      result[k] = r0;
      r0 = r1;
      r1 = r2;
      r2 = 0;
    }

  result[num_words * 2 - 1] = r0;
}
#    endif /* !asm_square */

#  else /* MG_UECC_SQUARE_FUNC */

#    if MG_UECC_ENABLE_VLI_API
MG_UECC_VLI_API void
mg_uecc_vli_square (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		    wordcount_t num_words)
{
  mg_uecc_vli_mult (result, left, left, num_words);
}
#    endif /* MG_UECC_ENABLE_VLI_API */

#  endif /* MG_UECC_SQUARE_FUNC */

/* Computes result = (left + right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap
   mod. */
MG_UECC_VLI_API void
mg_uecc_vli_modAdd (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		    const mg_uecc_word_t *right, const mg_uecc_word_t *mod,
		    wordcount_t num_words)
{
  mg_uecc_word_t carry = mg_uecc_vli_add (result, left, right, num_words);
  if (carry || mg_uecc_vli_cmp_unsafe (mod, result, num_words) != 1)
    {
      /* result > mod (result = mod + remainder), so subtract mod to get
       * remainder. */
      mg_uecc_vli_sub (result, result, mod, num_words);
    }
}

/* Computes result = (left - right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap
   mod. */
MG_UECC_VLI_API void
mg_uecc_vli_modSub (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		    const mg_uecc_word_t *right, const mg_uecc_word_t *mod,
		    wordcount_t num_words)
{
  mg_uecc_word_t l_borrow = mg_uecc_vli_sub (result, left, right, num_words);
  if (l_borrow)
    {
      /* In this case, result == -diff == (max int) - diff. Since -x % d == d -
	 x, we can get the correct result from result + mod (with overflow). */
      mg_uecc_vli_add (result, result, mod, num_words);
    }
}

/* Computes result = product % mod, where product is 2N words long. */
/* Currently only designed to work for curve_p or curve_n. */
MG_UECC_VLI_API void
mg_uecc_vli_mmod (mg_uecc_word_t *result, mg_uecc_word_t *product,
		  const mg_uecc_word_t *mod, wordcount_t num_words)
{
  mg_uecc_word_t mod_multiple[2 * MG_UECC_MAX_WORDS];
  mg_uecc_word_t tmp[2 * MG_UECC_MAX_WORDS];
  mg_uecc_word_t *v[2] = { tmp, product };
  mg_uecc_word_t index;

  /* Shift mod so its highest set bit is at the maximum position. */
  bitcount_t shift = (bitcount_t) ((num_words * 2 * MG_UECC_WORD_BITS)
				   - mg_uecc_vli_numBits (mod, num_words));
  wordcount_t word_shift = (wordcount_t) (shift / MG_UECC_WORD_BITS);
  wordcount_t bit_shift = (wordcount_t) (shift % MG_UECC_WORD_BITS);
  mg_uecc_word_t carry = 0;
  mg_uecc_vli_clear (mod_multiple, word_shift);
  if (bit_shift > 0)
    {
      for (index = 0; index < (mg_uecc_word_t) num_words; ++index)
	{
	  mod_multiple[(mg_uecc_word_t) word_shift + index]
	      = (mg_uecc_word_t) (mod[index] << bit_shift) | carry;
	  carry = mod[index] >> (MG_UECC_WORD_BITS - bit_shift);
	}
    }
  else
    {
      mg_uecc_vli_set (mod_multiple + word_shift, mod, num_words);
    }

  for (index = 1; shift >= 0; --shift)
    {
      mg_uecc_word_t borrow = 0;
      wordcount_t i;
      for (i = 0; i < num_words * 2; ++i)
	{
	  mg_uecc_word_t diff = v[index][i] - mod_multiple[i] - borrow;
	  if (diff != v[index][i])
	    {
	      borrow = (diff > v[index][i]);
	    }
	  v[1 - index][i] = diff;
	}
      index = !(index ^ borrow); /* Swap the index if there was no borrow */
      mg_uecc_vli_rshift1 (mod_multiple, num_words);
      mod_multiple[num_words - 1] |= mod_multiple[num_words]
				     << (MG_UECC_WORD_BITS - 1);
      mg_uecc_vli_rshift1 (mod_multiple + num_words, num_words);
    }
  mg_uecc_vli_set (result, v[index], num_words);
}

/* Computes result = (left * right) % mod. */
MG_UECC_VLI_API void
mg_uecc_vli_modMult (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		     const mg_uecc_word_t *right, const mg_uecc_word_t *mod,
		     wordcount_t num_words)
{
  mg_uecc_word_t product[2 * MG_UECC_MAX_WORDS];
  mg_uecc_vli_mult (product, left, right, num_words);
  mg_uecc_vli_mmod (result, product, mod, num_words);
}

MG_UECC_VLI_API void
mg_uecc_vli_modMult_fast (mg_uecc_word_t *result, const mg_uecc_word_t *left,
			  const mg_uecc_word_t *right, MG_UECC_Curve curve)
{
  mg_uecc_word_t product[2 * MG_UECC_MAX_WORDS];
  mg_uecc_vli_mult (product, left, right, curve->num_words);
#  if (MG_UECC_OPTIMIZATION_LEVEL > 0)
  curve->mmod_fast (result, product);
#  else
  mg_uecc_vli_mmod (result, product, curve->p, curve->num_words);
#  endif
}

#  if MG_UECC_SQUARE_FUNC

#    if MG_UECC_ENABLE_VLI_API
/* Computes result = left^2 % mod. */
MG_UECC_VLI_API void
mg_uecc_vli_modSquare (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		       const mg_uecc_word_t *mod, wordcount_t num_words)
{
  mg_uecc_word_t product[2 * MG_UECC_MAX_WORDS];
  mg_uecc_vli_square (product, left, num_words);
  mg_uecc_vli_mmod (result, product, mod, num_words);
}
#    endif /* MG_UECC_ENABLE_VLI_API */

MG_UECC_VLI_API void
mg_uecc_vli_modSquare_fast (mg_uecc_word_t *result, const mg_uecc_word_t *left,
			    MG_UECC_Curve curve)
{
  mg_uecc_word_t product[2 * MG_UECC_MAX_WORDS];
  mg_uecc_vli_square (product, left, curve->num_words);
#    if (MG_UECC_OPTIMIZATION_LEVEL > 0)
  curve->mmod_fast (result, product);
#    else
  mg_uecc_vli_mmod (result, product, curve->p, curve->num_words);
#    endif
}

#  else /* MG_UECC_SQUARE_FUNC */

#    if MG_UECC_ENABLE_VLI_API
MG_UECC_VLI_API void
mg_uecc_vli_modSquare (mg_uecc_word_t *result, const mg_uecc_word_t *left,
		       const mg_uecc_word_t *mod, wordcount_t num_words)
{
  mg_uecc_vli_modMult (result, left, left, mod, num_words);
}
#    endif /* MG_UECC_ENABLE_VLI_API */

MG_UECC_VLI_API void
mg_uecc_vli_modSquare_fast (mg_uecc_word_t *result, const mg_uecc_word_t *left,
			    MG_UECC_Curve curve)
{
  mg_uecc_vli_modMult_fast (result, left, left, curve);
}

#  endif /* MG_UECC_SQUARE_FUNC */

#  define EVEN(vli) (!(vli[0] & 1))
static void
vli_modInv_update (mg_uecc_word_t *uv, const mg_uecc_word_t *mod,
		   wordcount_t num_words)
{
  mg_uecc_word_t carry = 0;
  if (!EVEN (uv))
    {
      carry = mg_uecc_vli_add (uv, uv, mod, num_words);
    }
  mg_uecc_vli_rshift1 (uv, num_words);
  if (carry)
    {
      uv[num_words - 1] |= HIGH_BIT_SET;
    }
}

/* Computes result = (1 / input) % mod. All VLIs are the same size.
   See "From Euclid's GCD to Montgomery Multiplication to the Great Divide" */
MG_UECC_VLI_API void
mg_uecc_vli_modInv (mg_uecc_word_t *result, const mg_uecc_word_t *input,
		    const mg_uecc_word_t *mod, wordcount_t num_words)
{
  mg_uecc_word_t a[MG_UECC_MAX_WORDS], b[MG_UECC_MAX_WORDS],
      u[MG_UECC_MAX_WORDS], v[MG_UECC_MAX_WORDS];
  cmpresult_t cmpResult;

  if (mg_uecc_vli_isZero (input, num_words))
    {
      mg_uecc_vli_clear (result, num_words);
      return;
    }

  mg_uecc_vli_set (a, input, num_words);
  mg_uecc_vli_set (b, mod, num_words);
  mg_uecc_vli_clear (u, num_words);
  u[0] = 1;
  mg_uecc_vli_clear (v, num_words);
  while ((cmpResult = mg_uecc_vli_cmp_unsafe (a, b, num_words)) != 0)
    {
      if (EVEN (a))
	{
	  mg_uecc_vli_rshift1 (a, num_words);
	  vli_modInv_update (u, mod, num_words);
	}
      else if (EVEN (b))
	{
	  mg_uecc_vli_rshift1 (b, num_words);
	  vli_modInv_update (v, mod, num_words);
	}
      else if (cmpResult > 0)
	{
	  mg_uecc_vli_sub (a, a, b, num_words);
	  mg_uecc_vli_rshift1 (a, num_words);
	  if (mg_uecc_vli_cmp_unsafe (u, v, num_words) < 0)
	    {
	      mg_uecc_vli_add (u, u, mod, num_words);
	    }
	  mg_uecc_vli_sub (u, u, v, num_words);
	  vli_modInv_update (u, mod, num_words);
	}
      else
	{
	  mg_uecc_vli_sub (b, b, a, num_words);
	  mg_uecc_vli_rshift1 (b, num_words);
	  if (mg_uecc_vli_cmp_unsafe (v, u, num_words) < 0)
	    {
	      mg_uecc_vli_add (v, v, mod, num_words);
	    }
	  mg_uecc_vli_sub (v, v, u, num_words);
	  vli_modInv_update (v, mod, num_words);
	}
    }
  mg_uecc_vli_set (result, u, num_words);
}

/* ------ Point operations ------ */

/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#  ifndef _UECC_CURVE_SPECIFIC_H_
#    define _UECC_CURVE_SPECIFIC_H_

#    define num_bytes_secp160r1 20
#    define num_bytes_secp192r1 24
#    define num_bytes_secp224r1 28
#    define num_bytes_secp256r1 32
#    define num_bytes_secp256k1 32

#    if (MG_UECC_WORD_SIZE == 1)

#      define num_words_secp160r1 20
#      define num_words_secp192r1 24
#      define num_words_secp224r1 28
#      define num_words_secp256r1 32
#      define num_words_secp256k1 32

#      define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h)                        \
	0x##a, 0x##b, 0x##c, 0x##d, 0x##e, 0x##f, 0x##g, 0x##h
#      define BYTES_TO_WORDS_4(a, b, c, d) 0x##a, 0x##b, 0x##c, 0x##d

#    elif (MG_UECC_WORD_SIZE == 4)

#      define num_words_secp160r1 5
#      define num_words_secp192r1 6
#      define num_words_secp224r1 7
#      define num_words_secp256r1 8
#      define num_words_secp256k1 8

#      define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h)                        \
	0x##d##c##b##a, 0x##h##g##f##e
#      define BYTES_TO_WORDS_4(a, b, c, d) 0x##d##c##b##a

#    elif (MG_UECC_WORD_SIZE == 8)

#      define num_words_secp160r1 3
#      define num_words_secp192r1 3
#      define num_words_secp224r1 4
#      define num_words_secp256r1 4
#      define num_words_secp256k1 4

#      define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h)                        \
	0x##h##g##f##e##d##c##b##a##U
#      define BYTES_TO_WORDS_4(a, b, c, d) 0x##d##c##b##a##U

#    endif /* MG_UECC_WORD_SIZE */

#    if MG_UECC_SUPPORTS_secp160r1 || MG_UECC_SUPPORTS_secp192r1              \
	|| MG_UECC_SUPPORTS_secp224r1 || MG_UECC_SUPPORTS_secp256r1
static void
double_jacobian_default (mg_uecc_word_t *X1, mg_uecc_word_t *Y1,
			 mg_uecc_word_t *Z1, MG_UECC_Curve curve)
{
  /* t1 = X, t2 = Y, t3 = Z */
  mg_uecc_word_t t4[MG_UECC_MAX_WORDS];
  mg_uecc_word_t t5[MG_UECC_MAX_WORDS];
  wordcount_t num_words = curve->num_words;

  if (mg_uecc_vli_isZero (Z1, num_words))
    {
      return;
    }

  mg_uecc_vli_modSquare_fast (t4, Y1, curve);	/* t4 = y1^2 */
  mg_uecc_vli_modMult_fast (t5, X1, t4, curve); /* t5 = x1*y1^2 = A */
  mg_uecc_vli_modSquare_fast (t4, t4, curve);	/* t4 = y1^4 */
  mg_uecc_vli_modMult_fast (Y1, Y1, Z1, curve); /* t2 = y1*z1 = z3 */
  mg_uecc_vli_modSquare_fast (Z1, Z1, curve);	/* t3 = z1^2 */

  mg_uecc_vli_modAdd (X1, X1, Z1, curve->p, num_words); /* t1 = x1 + z1^2 */
  mg_uecc_vli_modAdd (Z1, Z1, Z1, curve->p, num_words); /* t3 = 2*z1^2 */
  mg_uecc_vli_modSub (Z1, X1, Z1, curve->p, num_words); /* t3 = x1 - z1^2 */
  mg_uecc_vli_modMult_fast (X1, X1, Z1, curve);		/* t1 = x1^2 - z1^4 */

  mg_uecc_vli_modAdd (Z1, X1, X1, curve->p,
		      num_words); /* t3 = 2*(x1^2 - z1^4) */
  mg_uecc_vli_modAdd (X1, X1, Z1, curve->p,
		      num_words); /* t1 = 3*(x1^2 - z1^4) */
  if (mg_uecc_vli_testBit (X1, 0))
    {
      mg_uecc_word_t l_carry = mg_uecc_vli_add (X1, X1, curve->p, num_words);
      mg_uecc_vli_rshift1 (X1, num_words);
      X1[num_words - 1] |= l_carry << (MG_UECC_WORD_BITS - 1);
    }
  else
    {
      mg_uecc_vli_rshift1 (X1, num_words);
    }
  /* t1 = 3/2*(x1^2 - z1^4) = B */

  mg_uecc_vli_modSquare_fast (Z1, X1, curve);		/* t3 = B^2 */
  mg_uecc_vli_modSub (Z1, Z1, t5, curve->p, num_words); /* t3 = B^2 - A */
  mg_uecc_vli_modSub (Z1, Z1, t5, curve->p,
		      num_words); /* t3 = B^2 - 2A = x3 */
  mg_uecc_vli_modSub (t5, t5, Z1, curve->p, num_words); /* t5 = A - x3 */
  mg_uecc_vli_modMult_fast (X1, X1, t5, curve);		/* t1 = B * (A - x3) */
  mg_uecc_vli_modSub (t4, X1, t4, curve->p,
		      num_words); /* t4 = B * (A - x3) - y1^4 = y3 */

  mg_uecc_vli_set (X1, Z1, num_words);
  mg_uecc_vli_set (Z1, Y1, num_words);
  mg_uecc_vli_set (Y1, t4, num_words);
}

/* Computes result = x^3 + ax + b. result must not overlap x. */
static void
x_side_default (mg_uecc_word_t *result, const mg_uecc_word_t *x,
		MG_UECC_Curve curve)
{
  mg_uecc_word_t _3[MG_UECC_MAX_WORDS] = { 3 }; /* -a = 3 */
  wordcount_t num_words = curve->num_words;

  mg_uecc_vli_modSquare_fast (result, x, curve); /* r = x^2 */
  mg_uecc_vli_modSub (result, result, _3, curve->p,
		      num_words);		       /* r = x^2 - 3 */
  mg_uecc_vli_modMult_fast (result, result, x, curve); /* r = x^3 - 3x */
  mg_uecc_vli_modAdd (result, result, curve->b, curve->p,
		      num_words); /* r = x^3 - 3x + b */
}
#    endif /* MG_UECC_SUPPORTS_secp... */

#    if MG_UECC_SUPPORT_COMPRESSED_POINT
#      if MG_UECC_SUPPORTS_secp160r1 || MG_UECC_SUPPORTS_secp192r1            \
	  || MG_UECC_SUPPORTS_secp256r1 || MG_UECC_SUPPORTS_secp256k1
/* Compute a = sqrt(a) (mod curve_p). */
static void
mod_sqrt_default (mg_uecc_word_t *a, MG_UECC_Curve curve)
{
  bitcount_t i;
  mg_uecc_word_t p1[MG_UECC_MAX_WORDS] = { 1 };
  mg_uecc_word_t l_result[MG_UECC_MAX_WORDS] = { 1 };
  wordcount_t num_words = curve->num_words;

  /* When curve->p == 3 (mod 4), we can compute
     sqrt(a) = a^((curve->p + 1) / 4) (mod curve->p). */
  mg_uecc_vli_add (p1, curve->p, p1, num_words); /* p1 = curve_p + 1 */
  for (i = mg_uecc_vli_numBits (p1, num_words) - 1; i > 1; --i)
    {
      mg_uecc_vli_modSquare_fast (l_result, l_result, curve);
      if (mg_uecc_vli_testBit (p1, i))
	{
	  mg_uecc_vli_modMult_fast (l_result, l_result, a, curve);
	}
    }
  mg_uecc_vli_set (a, l_result, num_words);
}
#      endif /* MG_UECC_SUPPORTS_secp... */
#    endif   /* MG_UECC_SUPPORT_COMPRESSED_POINT */

#    if MG_UECC_SUPPORTS_secp160r1

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp160r1 (mg_uecc_word_t *result,
				     mg_uecc_word_t *product);
#      endif

static const struct MG_UECC_Curve_t curve_secp160r1
    = { num_words_secp160r1,
	num_bytes_secp160r1,
	161, /* num_n_bits */
	{ BYTES_TO_WORDS_8 (FF, FF, FF, 7F, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_4 (FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (57, 22, 75, CA, D3, AE, 27, F9),
	  BYTES_TO_WORDS_8 (C8, F4, 01, 00, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, 01, 00, 00, 00) },
	{ BYTES_TO_WORDS_8 (82, FC, CB, 13, B9, 8B, C3, 68),
	  BYTES_TO_WORDS_8 (89, 69, 64, 46, 28, 73, F5, 8E),
	  BYTES_TO_WORDS_4 (68, B5, 96, 4A),

	  BYTES_TO_WORDS_8 (32, FB, C5, 7A, 37, 51, 23, 04),
	  BYTES_TO_WORDS_8 (12, C9, DC, 59, 7D, 94, 68, 31),
	  BYTES_TO_WORDS_4 (55, 28, A6, 23) },
	{ BYTES_TO_WORDS_8 (45, FA, 65, C5, AD, D4, D4, 81),
	  BYTES_TO_WORDS_8 (9F, F8, AC, 65, 8B, 7A, BD, 54),
	  BYTES_TO_WORDS_4 (FC, BE, 97, 1C) },
	&double_jacobian_default,
#      if MG_UECC_SUPPORT_COMPRESSED_POINT
	&mod_sqrt_default,
#      endif
	&x_side_default,
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
	&vli_mmod_fast_secp160r1
#      endif
      };

MG_UECC_Curve
mg_uecc_secp160r1 (void)
{
  return &curve_secp160r1;
}

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp160r1)
/* Computes result = product % curve_p
    see http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf page 354

    Note that this only works if log2(omega) < log2(p) / 2 */
static void omega_mult_secp160r1 (mg_uecc_word_t *result,
				  const mg_uecc_word_t *right);
#	if MG_UECC_WORD_SIZE == 8
static void
vli_mmod_fast_secp160r1 (mg_uecc_word_t *result, mg_uecc_word_t *product)
{
  mg_uecc_word_t tmp[2 * num_words_secp160r1];
  mg_uecc_word_t copy;

  mg_uecc_vli_clear (tmp, num_words_secp160r1);
  mg_uecc_vli_clear (tmp + num_words_secp160r1, num_words_secp160r1);

  omega_mult_secp160r1 (tmp, product + num_words_secp160r1
				 - 1); /* (Rq, q) = q * c */

  product[num_words_secp160r1 - 1] &= 0xffffffff;
  copy = tmp[num_words_secp160r1 - 1];
  tmp[num_words_secp160r1 - 1] &= 0xffffffff;
  mg_uecc_vli_add (result, product, tmp,
		   num_words_secp160r1); /* (C, r) = r + q */
  mg_uecc_vli_clear (product, num_words_secp160r1);
  tmp[num_words_secp160r1 - 1] = copy;
  omega_mult_secp160r1 (product, tmp + num_words_secp160r1 - 1); /* Rq*c */
  mg_uecc_vli_add (result, result, product,
		   num_words_secp160r1); /* (C1, r) = r + Rq*c */

  while (
      mg_uecc_vli_cmp_unsafe (result, curve_secp160r1.p, num_words_secp160r1)
      > 0)
    {
      mg_uecc_vli_sub (result, result, curve_secp160r1.p, num_words_secp160r1);
    }
}

static void
omega_mult_secp160r1 (uint64_t *result, const uint64_t *right)
{
  uint32_t carry;
  unsigned i;

  /* Multiply by (2^31 + 1). */
  carry = 0;
  for (i = 0; i < num_words_secp160r1; ++i)
    {
      uint64_t tmp = (right[i] >> 32) | (right[i + 1] << 32);
      result[i] = (tmp << 31) + tmp + carry;
      carry = (tmp >> 33) + (result[i] < tmp || (carry && result[i] == tmp));
    }
  result[i] = carry;
}
#	else
static void
vli_mmod_fast_secp160r1 (mg_uecc_word_t *result, mg_uecc_word_t *product)
{
  mg_uecc_word_t tmp[2 * num_words_secp160r1];
  mg_uecc_word_t carry;

  mg_uecc_vli_clear (tmp, num_words_secp160r1);
  mg_uecc_vli_clear (tmp + num_words_secp160r1, num_words_secp160r1);

  omega_mult_secp160r1 (tmp,
			product + num_words_secp160r1); /* (Rq, q) = q * c */

  carry = mg_uecc_vli_add (result, product, tmp,
			   num_words_secp160r1); /* (C, r) = r + q */
  mg_uecc_vli_clear (product, num_words_secp160r1);
  omega_mult_secp160r1 (product, tmp + num_words_secp160r1); /* Rq*c */
  carry += mg_uecc_vli_add (result, result, product,
			    num_words_secp160r1); /* (C1, r) = r + Rq*c */

  while (carry > 0)
    {
      --carry;
      mg_uecc_vli_sub (result, result, curve_secp160r1.p, num_words_secp160r1);
    }
  if (mg_uecc_vli_cmp_unsafe (result, curve_secp160r1.p, num_words_secp160r1)
      > 0)
    {
      mg_uecc_vli_sub (result, result, curve_secp160r1.p, num_words_secp160r1);
    }
}
#	endif

#	if MG_UECC_WORD_SIZE == 1
static void
omega_mult_secp160r1 (uint8_t *result, const uint8_t *right)
{
  uint8_t carry;
  uint8_t i;

  /* Multiply by (2^31 + 1). */
  mg_uecc_vli_set (result + 4, right, num_words_secp160r1); /* 2^32 */
  mg_uecc_vli_rshift1 (result + 4, num_words_secp160r1);    /* 2^31 */
  result[3] = right[0] << 7; /* get last bit from shift */

  carry = mg_uecc_vli_add (result, result, right,
			   num_words_secp160r1); /* 2^31 + 1 */
  for (i = num_words_secp160r1; carry; ++i)
    {
      uint16_t sum = (uint16_t) result[i] + carry;
      result[i] = (uint8_t) sum;
      carry = sum >> 8;
    }
}
#	elif MG_UECC_WORD_SIZE == 4
static void
omega_mult_secp160r1 (uint32_t *result, const uint32_t *right)
{
  uint32_t carry;
  unsigned i;

  /* Multiply by (2^31 + 1). */
  mg_uecc_vli_set (result + 1, right, num_words_secp160r1); /* 2^32 */
  mg_uecc_vli_rshift1 (result + 1, num_words_secp160r1);    /* 2^31 */
  result[0] = right[0] << 31; /* get last bit from shift */

  carry = mg_uecc_vli_add (result, result, right,
			   num_words_secp160r1); /* 2^31 + 1 */
  for (i = num_words_secp160r1; carry; ++i)
    {
      uint64_t sum = (uint64_t) result[i] + carry;
      result[i] = (uint32_t) sum;
      carry = sum >> 32;
    }
}
#	endif /* MG_UECC_WORD_SIZE */
#      endif /* (MG_UECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp160r1)  \
	      */

#    endif /* MG_UECC_SUPPORTS_secp160r1 */

#    if MG_UECC_SUPPORTS_secp192r1

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp192r1 (mg_uecc_word_t *result,
				     mg_uecc_word_t *product);
#      endif

static const struct MG_UECC_Curve_t curve_secp192r1
    = { num_words_secp192r1,
	num_bytes_secp192r1,
	192, /* num_n_bits */
	{ BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FE, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (31, 28, D2, B4, B1, C9, 6B, 14),
	  BYTES_TO_WORDS_8 (36, F8, DE, 99, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (12, 10, FF, 82, FD, 0A, FF, F4),
	  BYTES_TO_WORDS_8 (00, 88, A1, 43, EB, 20, BF, 7C),
	  BYTES_TO_WORDS_8 (F6, 90, 30, B0, 0E, A8, 8D, 18),

	  BYTES_TO_WORDS_8 (11, 48, 79, 1E, A1, 77, F9, 73),
	  BYTES_TO_WORDS_8 (D5, CD, 24, 6B, ED, 11, 10, 63),
	  BYTES_TO_WORDS_8 (78, DA, C8, FF, 95, 2B, 19, 07) },
	{ BYTES_TO_WORDS_8 (B1, B9, 46, C1, EC, DE, B8, FE),
	  BYTES_TO_WORDS_8 (49, 30, 24, 72, AB, E9, A7, 0F),
	  BYTES_TO_WORDS_8 (E7, 80, 9C, E5, 19, 05, 21, 64) },
	&double_jacobian_default,
#      if MG_UECC_SUPPORT_COMPRESSED_POINT
	&mod_sqrt_default,
#      endif
	&x_side_default,
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
	&vli_mmod_fast_secp192r1
#      endif
      };

MG_UECC_Curve
mg_uecc_secp192r1 (void)
{
  return &curve_secp192r1;
}

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
/* Computes result = product % curve_p.
   See algorithm 5 and 6 from
   http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf
 */
#	if MG_UECC_WORD_SIZE == 1
static void
vli_mmod_fast_secp192r1 (uint8_t *result, uint8_t *product)
{
  uint8_t tmp[num_words_secp192r1];
  uint8_t carry;

  mg_uecc_vli_set (result, product, num_words_secp192r1);

  mg_uecc_vli_set (tmp, &product[24], num_words_secp192r1);
  carry = mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  tmp[0] = tmp[1] = tmp[2] = tmp[3] = tmp[4] = tmp[5] = tmp[6] = tmp[7] = 0;
  tmp[8] = product[24];
  tmp[9] = product[25];
  tmp[10] = product[26];
  tmp[11] = product[27];
  tmp[12] = product[28];
  tmp[13] = product[29];
  tmp[14] = product[30];
  tmp[15] = product[31];
  tmp[16] = product[32];
  tmp[17] = product[33];
  tmp[18] = product[34];
  tmp[19] = product[35];
  tmp[20] = product[36];
  tmp[21] = product[37];
  tmp[22] = product[38];
  tmp[23] = product[39];
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  tmp[0] = tmp[8] = product[40];
  tmp[1] = tmp[9] = product[41];
  tmp[2] = tmp[10] = product[42];
  tmp[3] = tmp[11] = product[43];
  tmp[4] = tmp[12] = product[44];
  tmp[5] = tmp[13] = product[45];
  tmp[6] = tmp[14] = product[46];
  tmp[7] = tmp[15] = product[47];
  tmp[16] = tmp[17] = tmp[18] = tmp[19] = tmp[20] = tmp[21] = tmp[22] = tmp[23]
      = 0;
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  while (carry
	 || mg_uecc_vli_cmp_unsafe (curve_secp192r1.p, result,
				    num_words_secp192r1)
		!= 1)
    {
      carry -= mg_uecc_vli_sub (result, result, curve_secp192r1.p,
				num_words_secp192r1);
    }
}
#	elif MG_UECC_WORD_SIZE == 4
static void
vli_mmod_fast_secp192r1 (uint32_t *result, uint32_t *product)
{
  uint32_t tmp[num_words_secp192r1];
  int carry;

  mg_uecc_vli_set (result, product, num_words_secp192r1);

  mg_uecc_vli_set (tmp, &product[6], num_words_secp192r1);
  carry = mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  tmp[0] = tmp[1] = 0;
  tmp[2] = product[6];
  tmp[3] = product[7];
  tmp[4] = product[8];
  tmp[5] = product[9];
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  tmp[0] = tmp[2] = product[10];
  tmp[1] = tmp[3] = product[11];
  tmp[4] = tmp[5] = 0;
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  while (carry
	 || mg_uecc_vli_cmp_unsafe (curve_secp192r1.p, result,
				    num_words_secp192r1)
		!= 1)
    {
      carry -= mg_uecc_vli_sub (result, result, curve_secp192r1.p,
				num_words_secp192r1);
    }
}
#	else
static void
vli_mmod_fast_secp192r1 (uint64_t *result, uint64_t *product)
{
  uint64_t tmp[num_words_secp192r1];
  int carry;

  mg_uecc_vli_set (result, product, num_words_secp192r1);

  mg_uecc_vli_set (tmp, &product[3], num_words_secp192r1);
  carry = (int) mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  tmp[0] = 0;
  tmp[1] = product[3];
  tmp[2] = product[4];
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  tmp[0] = tmp[1] = product[5];
  tmp[2] = 0;
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp192r1);

  while (carry
	 || mg_uecc_vli_cmp_unsafe (curve_secp192r1.p, result,
				    num_words_secp192r1)
		!= 1)
    {
      carry -= mg_uecc_vli_sub (result, result, curve_secp192r1.p,
				num_words_secp192r1);
    }
}
#	endif /* MG_UECC_WORD_SIZE */
#      endif   /* (MG_UECC_OPTIMIZATION_LEVEL > 0) */

#    endif /* MG_UECC_SUPPORTS_secp192r1 */

#    if MG_UECC_SUPPORTS_secp224r1

#      if MG_UECC_SUPPORT_COMPRESSED_POINT
static void mod_sqrt_secp224r1 (mg_uecc_word_t *a, MG_UECC_Curve curve);
#      endif
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp224r1 (mg_uecc_word_t *result,
				     mg_uecc_word_t *product);
#      endif

static const struct MG_UECC_Curve_t curve_secp224r1
    = { num_words_secp224r1,
	num_bytes_secp224r1,
	224, /* num_n_bits */
	{ BYTES_TO_WORDS_8 (01, 00, 00, 00, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_4 (FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (3D, 2A, 5C, 5C, 45, 29, DD, 13),
	  BYTES_TO_WORDS_8 (3E, F0, B8, E0, A2, 16, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_4 (FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (21, 1D, 5C, 11, D6, 80, 32, 34),
	  BYTES_TO_WORDS_8 (22, 11, C2, 56, D3, C1, 03, 4A),
	  BYTES_TO_WORDS_8 (B9, 90, 13, 32, 7F, BF, B4, 6B),
	  BYTES_TO_WORDS_4 (BD, 0C, 0E, B7),

	  BYTES_TO_WORDS_8 (34, 7E, 00, 85, 99, 81, D5, 44),
	  BYTES_TO_WORDS_8 (64, 47, 07, 5A, A0, 75, 43, CD),
	  BYTES_TO_WORDS_8 (E6, DF, 22, 4C, FB, 23, F7, B5),
	  BYTES_TO_WORDS_4 (88, 63, 37, BD) },
	{ BYTES_TO_WORDS_8 (B4, FF, 55, 23, 43, 39, 0B, 27),
	  BYTES_TO_WORDS_8 (BA, D8, BF, D7, B7, B0, 44, 50),
	  BYTES_TO_WORDS_8 (56, 32, 41, F5, AB, B3, 04, 0C),
	  BYTES_TO_WORDS_4 (85, 0A, 05, B4) },
	&double_jacobian_default,
#      if MG_UECC_SUPPORT_COMPRESSED_POINT
	&mod_sqrt_secp224r1,
#      endif
	&x_side_default,
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
	&vli_mmod_fast_secp224r1
#      endif
      };

MG_UECC_Curve
mg_uecc_secp224r1 (void)
{
  return &curve_secp224r1;
}

#      if MG_UECC_SUPPORT_COMPRESSED_POINT
/* Routine 3.2.4 RS;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void
mod_sqrt_secp224r1_rs (mg_uecc_word_t *d1, mg_uecc_word_t *e1,
		       mg_uecc_word_t *f1, const mg_uecc_word_t *d0,
		       const mg_uecc_word_t *e0, const mg_uecc_word_t *f0)
{
  mg_uecc_word_t t[num_words_secp224r1];

  mg_uecc_vli_modSquare_fast (t, d0, &curve_secp224r1);	   /* t <-- d0 ^ 2 */
  mg_uecc_vli_modMult_fast (e1, d0, e0, &curve_secp224r1); /* e1 <-- d0 * e0 */
  mg_uecc_vli_modAdd (d1, t, f0, curve_secp224r1.p,
		      num_words_secp224r1); /* d1 <-- t  + f0 */
  mg_uecc_vli_modAdd (e1, e1, e1, curve_secp224r1.p,
		      num_words_secp224r1);		  /* e1 <-- e1 + e1 */
  mg_uecc_vli_modMult_fast (f1, t, f0, &curve_secp224r1); /* f1 <-- t  * f0 */
  mg_uecc_vli_modAdd (f1, f1, f1, curve_secp224r1.p,
		      num_words_secp224r1); /* f1 <-- f1 + f1 */
  mg_uecc_vli_modAdd (f1, f1, f1, curve_secp224r1.p,
		      num_words_secp224r1); /* f1 <-- f1 + f1 */
}

/* Routine 3.2.5 RSS;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void
mod_sqrt_secp224r1_rss (mg_uecc_word_t *d1, mg_uecc_word_t *e1,
			mg_uecc_word_t *f1, const mg_uecc_word_t *d0,
			const mg_uecc_word_t *e0, const mg_uecc_word_t *f0,
			const bitcount_t j)
{
  bitcount_t i;

  mg_uecc_vli_set (d1, d0, num_words_secp224r1); /* d1 <-- d0 */
  mg_uecc_vli_set (e1, e0, num_words_secp224r1); /* e1 <-- e0 */
  mg_uecc_vli_set (f1, f0, num_words_secp224r1); /* f1 <-- f0 */
  for (i = 1; i <= j; i++)
    {
      mod_sqrt_secp224r1_rs (d1, e1, f1, d1, e1,
			     f1); /* RS (d1,e1,f1,d1,e1,f1) */
    }
}

/* Routine 3.2.6 RM;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void
mod_sqrt_secp224r1_rm (mg_uecc_word_t *d2, mg_uecc_word_t *e2,
		       mg_uecc_word_t *f2, const mg_uecc_word_t *c,
		       const mg_uecc_word_t *d0, const mg_uecc_word_t *e0,
		       const mg_uecc_word_t *d1, const mg_uecc_word_t *e1)
{
  mg_uecc_word_t t1[num_words_secp224r1];
  mg_uecc_word_t t2[num_words_secp224r1];

  mg_uecc_vli_modMult_fast (t1, e0, e1, &curve_secp224r1); /* t1 <-- e0 * e1 */
  mg_uecc_vli_modMult_fast (t1, t1, c, &curve_secp224r1);  /* t1 <-- t1 * c */
  /* t1 <-- p  - t1 */
  mg_uecc_vli_modSub (t1, curve_secp224r1.p, t1, curve_secp224r1.p,
		      num_words_secp224r1);
  mg_uecc_vli_modMult_fast (t2, d0, d1, &curve_secp224r1); /* t2 <-- d0 * d1 */
  mg_uecc_vli_modAdd (t2, t2, t1, curve_secp224r1.p,
		      num_words_secp224r1);		   /* t2 <-- t2 + t1 */
  mg_uecc_vli_modMult_fast (t1, d0, e1, &curve_secp224r1); /* t1 <-- d0 * e1 */
  mg_uecc_vli_modMult_fast (e2, d1, e0, &curve_secp224r1); /* e2 <-- d1 * e0 */
  mg_uecc_vli_modAdd (e2, e2, t1, curve_secp224r1.p,
		      num_words_secp224r1);		  /* e2 <-- e2 + t1 */
  mg_uecc_vli_modSquare_fast (f2, e2, &curve_secp224r1);  /* f2 <-- e2^2 */
  mg_uecc_vli_modMult_fast (f2, f2, c, &curve_secp224r1); /* f2 <-- f2 * c */
  /* f2 <-- p  - f2 */
  mg_uecc_vli_modSub (f2, curve_secp224r1.p, f2, curve_secp224r1.p,
		      num_words_secp224r1);
  mg_uecc_vli_set (d2, t2, num_words_secp224r1); /* d2 <-- t2 */
}

/* Routine 3.2.7 RP;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void
mod_sqrt_secp224r1_rp (mg_uecc_word_t *d1, mg_uecc_word_t *e1,
		       mg_uecc_word_t *f1, const mg_uecc_word_t *c,
		       const mg_uecc_word_t *r)
{
  wordcount_t i;
  wordcount_t pow2i = 1;
  mg_uecc_word_t d0[num_words_secp224r1];
  mg_uecc_word_t e0[num_words_secp224r1] = { 1 }; /* e0 <-- 1 */
  mg_uecc_word_t f0[num_words_secp224r1];

  mg_uecc_vli_set (d0, r, num_words_secp224r1); /* d0 <-- r */
  /* f0 <-- p  - c */
  mg_uecc_vli_modSub (f0, curve_secp224r1.p, c, curve_secp224r1.p,
		      num_words_secp224r1);
  for (i = 0; i <= 6; i++)
    {
      mod_sqrt_secp224r1_rss (d1, e1, f1, d0, e0, f0,
			      pow2i); /* RSS (d1,e1,f1,d0,e0,f0,2^i) */
      mod_sqrt_secp224r1_rm (d1, e1, f1, c, d1, e1, d0,
			     e0); /* RM (d1,e1,f1,c,d1,e1,d0,e0) */
      mg_uecc_vli_set (d0, d1, num_words_secp224r1); /* d0 <-- d1 */
      mg_uecc_vli_set (e0, e1, num_words_secp224r1); /* e0 <-- e1 */
      mg_uecc_vli_set (f0, f1, num_words_secp224r1); /* f0 <-- f1 */
      pow2i *= 2;
    }
}

/* Compute a = sqrt(a) (mod curve_p). */
/* Routine 3.2.8 mp_mod_sqrt_224; from
 * http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void
mod_sqrt_secp224r1 (mg_uecc_word_t *a, MG_UECC_Curve curve)
{
  (void) curve;
  bitcount_t i;
  mg_uecc_word_t e1[num_words_secp224r1];
  mg_uecc_word_t f1[num_words_secp224r1];
  mg_uecc_word_t d0[num_words_secp224r1];
  mg_uecc_word_t e0[num_words_secp224r1];
  mg_uecc_word_t f0[num_words_secp224r1];
  mg_uecc_word_t d1[num_words_secp224r1];

  /* s = a; using constant instead of random value */
  mod_sqrt_secp224r1_rp (d0, e0, f0, a, a); /* RP (d0, e0, f0, c, s) */
  mod_sqrt_secp224r1_rs (d1, e1, f1, d0, e0,
			 f0); /* RS (d1, e1, f1, d0, e0, f0) */
  for (i = 1; i <= 95; i++)
    {
      mg_uecc_vli_set (d0, d1, num_words_secp224r1); /* d0 <-- d1 */
      mg_uecc_vli_set (e0, e1, num_words_secp224r1); /* e0 <-- e1 */
      mg_uecc_vli_set (f0, f1, num_words_secp224r1); /* f0 <-- f1 */
      mod_sqrt_secp224r1_rs (d1, e1, f1, d0, e0,
			     f0); /* RS (d1, e1, f1, d0, e0, f0) */
      if (mg_uecc_vli_isZero (d1, num_words_secp224r1))
	{ /* if d1 == 0 */
	  break;
	}
    }
  mg_uecc_vli_modInv (f1, e0, curve_secp224r1.p,
		      num_words_secp224r1);		  /* f1 <-- 1 / e0 */
  mg_uecc_vli_modMult_fast (a, d0, f1, &curve_secp224r1); /* a  <-- d0 / e0 */
}
#      endif /* MG_UECC_SUPPORT_COMPRESSED_POINT */

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
/* Computes result = product % curve_p
   from http://www.nsa.gov/ia/_files/nist-routines.pdf */
#	if MG_UECC_WORD_SIZE == 1
static void
vli_mmod_fast_secp224r1 (uint8_t *result, uint8_t *product)
{
  uint8_t tmp[num_words_secp224r1];
  int8_t carry;

  /* t */
  mg_uecc_vli_set (result, product, num_words_secp224r1);

  /* s1 */
  tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
  tmp[4] = tmp[5] = tmp[6] = tmp[7] = 0;
  tmp[8] = tmp[9] = tmp[10] = tmp[11] = 0;
  tmp[12] = product[28];
  tmp[13] = product[29];
  tmp[14] = product[30];
  tmp[15] = product[31];
  tmp[16] = product[32];
  tmp[17] = product[33];
  tmp[18] = product[34];
  tmp[19] = product[35];
  tmp[20] = product[36];
  tmp[21] = product[37];
  tmp[22] = product[38];
  tmp[23] = product[39];
  tmp[24] = product[40];
  tmp[25] = product[41];
  tmp[26] = product[42];
  tmp[27] = product[43];
  carry = mg_uecc_vli_add (result, result, tmp, num_words_secp224r1);

  /* s2 */
  tmp[12] = product[44];
  tmp[13] = product[45];
  tmp[14] = product[46];
  tmp[15] = product[47];
  tmp[16] = product[48];
  tmp[17] = product[49];
  tmp[18] = product[50];
  tmp[19] = product[51];
  tmp[20] = product[52];
  tmp[21] = product[53];
  tmp[22] = product[54];
  tmp[23] = product[55];
  tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp224r1);

  /* d1 */
  tmp[0] = product[28];
  tmp[1] = product[29];
  tmp[2] = product[30];
  tmp[3] = product[31];
  tmp[4] = product[32];
  tmp[5] = product[33];
  tmp[6] = product[34];
  tmp[7] = product[35];
  tmp[8] = product[36];
  tmp[9] = product[37];
  tmp[10] = product[38];
  tmp[11] = product[39];
  tmp[12] = product[40];
  tmp[13] = product[41];
  tmp[14] = product[42];
  tmp[15] = product[43];
  tmp[16] = product[44];
  tmp[17] = product[45];
  tmp[18] = product[46];
  tmp[19] = product[47];
  tmp[20] = product[48];
  tmp[21] = product[49];
  tmp[22] = product[50];
  tmp[23] = product[51];
  tmp[24] = product[52];
  tmp[25] = product[53];
  tmp[26] = product[54];
  tmp[27] = product[55];
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp224r1);

  /* d2 */
  tmp[0] = product[44];
  tmp[1] = product[45];
  tmp[2] = product[46];
  tmp[3] = product[47];
  tmp[4] = product[48];
  tmp[5] = product[49];
  tmp[6] = product[50];
  tmp[7] = product[51];
  tmp[8] = product[52];
  tmp[9] = product[53];
  tmp[10] = product[54];
  tmp[11] = product[55];
  tmp[12] = tmp[13] = tmp[14] = tmp[15] = 0;
  tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
  tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
  tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp224r1);

  if (carry < 0)
    {
      do
	{
	  carry += mg_uecc_vli_add (result, result, curve_secp224r1.p,
				    num_words_secp224r1);
	}
      while (carry < 0);
    }
  else
    {
      while (carry
	     || mg_uecc_vli_cmp_unsafe (curve_secp224r1.p, result,
					num_words_secp224r1)
		    != 1)
	{
	  carry -= mg_uecc_vli_sub (result, result, curve_secp224r1.p,
				    num_words_secp224r1);
	}
    }
}
#	elif MG_UECC_WORD_SIZE == 4
static void
vli_mmod_fast_secp224r1 (uint32_t *result, uint32_t *product)
{
  uint32_t tmp[num_words_secp224r1];
  int carry;

  /* t */
  mg_uecc_vli_set (result, product, num_words_secp224r1);

  /* s1 */
  tmp[0] = tmp[1] = tmp[2] = 0;
  tmp[3] = product[7];
  tmp[4] = product[8];
  tmp[5] = product[9];
  tmp[6] = product[10];
  carry = mg_uecc_vli_add (result, result, tmp, num_words_secp224r1);

  /* s2 */
  tmp[3] = product[11];
  tmp[4] = product[12];
  tmp[5] = product[13];
  tmp[6] = 0;
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp224r1);

  /* d1 */
  tmp[0] = product[7];
  tmp[1] = product[8];
  tmp[2] = product[9];
  tmp[3] = product[10];
  tmp[4] = product[11];
  tmp[5] = product[12];
  tmp[6] = product[13];
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp224r1);

  /* d2 */
  tmp[0] = product[11];
  tmp[1] = product[12];
  tmp[2] = product[13];
  tmp[3] = tmp[4] = tmp[5] = tmp[6] = 0;
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp224r1);

  if (carry < 0)
    {
      do
	{
	  carry += mg_uecc_vli_add (result, result, curve_secp224r1.p,
				    num_words_secp224r1);
	}
      while (carry < 0);
    }
  else
    {
      while (carry
	     || mg_uecc_vli_cmp_unsafe (curve_secp224r1.p, result,
					num_words_secp224r1)
		    != 1)
	{
	  carry -= mg_uecc_vli_sub (result, result, curve_secp224r1.p,
				    num_words_secp224r1);
	}
    }
}
#	else
static void
vli_mmod_fast_secp224r1 (uint64_t *result, uint64_t *product)
{
  uint64_t tmp[num_words_secp224r1];
  int carry = 0;

  /* t */
  mg_uecc_vli_set (result, product, num_words_secp224r1);
  result[num_words_secp224r1 - 1] &= 0xffffffff;

  /* s1 */
  tmp[0] = 0;
  tmp[1] = product[3] & 0xffffffff00000000ull;
  tmp[2] = product[4];
  tmp[3] = product[5] & 0xffffffff;
  mg_uecc_vli_add (result, result, tmp, num_words_secp224r1);

  /* s2 */
  tmp[1] = product[5] & 0xffffffff00000000ull;
  tmp[2] = product[6];
  tmp[3] = 0;
  mg_uecc_vli_add (result, result, tmp, num_words_secp224r1);

  /* d1 */
  tmp[0] = (product[3] >> 32) | (product[4] << 32);
  tmp[1] = (product[4] >> 32) | (product[5] << 32);
  tmp[2] = (product[5] >> 32) | (product[6] << 32);
  tmp[3] = product[6] >> 32;
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp224r1);

  /* d2 */
  tmp[0] = (product[5] >> 32) | (product[6] << 32);
  tmp[1] = product[6] >> 32;
  tmp[2] = tmp[3] = 0;
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp224r1);

  if (carry < 0)
    {
      do
	{
	  carry += mg_uecc_vli_add (result, result, curve_secp224r1.p,
				    num_words_secp224r1);
	}
      while (carry < 0);
    }
  else
    {
      while (mg_uecc_vli_cmp_unsafe (curve_secp224r1.p, result,
				     num_words_secp224r1)
	     != 1)
	{
	  mg_uecc_vli_sub (result, result, curve_secp224r1.p,
			   num_words_secp224r1);
	}
    }
}
#	endif /* MG_UECC_WORD_SIZE */
#      endif   /* (MG_UECC_OPTIMIZATION_LEVEL > 0) */

#    endif /* MG_UECC_SUPPORTS_secp224r1 */

#    if MG_UECC_SUPPORTS_secp256r1

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp256r1 (mg_uecc_word_t *result,
				     mg_uecc_word_t *product);
#      endif

static const struct MG_UECC_Curve_t curve_secp256r1
    = { num_words_secp256r1,
	num_bytes_secp256r1,
	256, /* num_n_bits */
	{ BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (01, 00, 00, 00, FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (51, 25, 63, FC, C2, CA, B9, F3),
	  BYTES_TO_WORDS_8 (84, 9E, 17, A7, AD, FA, E6, BC),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (96, C2, 98, D8, 45, 39, A1, F4),
	  BYTES_TO_WORDS_8 (A0, 33, EB, 2D, 81, 7D, 03, 77),
	  BYTES_TO_WORDS_8 (F2, 40, A4, 63, E5, E6, BC, F8),
	  BYTES_TO_WORDS_8 (47, 42, 2C, E1, F2, D1, 17, 6B),

	  BYTES_TO_WORDS_8 (F5, 51, BF, 37, 68, 40, B6, CB),
	  BYTES_TO_WORDS_8 (CE, 5E, 31, 6B, 57, 33, CE, 2B),
	  BYTES_TO_WORDS_8 (16, 9E, 0F, 7C, 4A, EB, E7, 8E),
	  BYTES_TO_WORDS_8 (9B, 7F, 1A, FE, E2, 42, E3, 4F) },
	{ BYTES_TO_WORDS_8 (4B, 60, D2, 27, 3E, 3C, CE, 3B),
	  BYTES_TO_WORDS_8 (F6, B0, 53, CC, B0, 06, 1D, 65),
	  BYTES_TO_WORDS_8 (BC, 86, 98, 76, 55, BD, EB, B3),
	  BYTES_TO_WORDS_8 (E7, 93, 3A, AA, D8, 35, C6, 5A) },
	&double_jacobian_default,
#      if MG_UECC_SUPPORT_COMPRESSED_POINT
	&mod_sqrt_default,
#      endif
	&x_side_default,
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
	&vli_mmod_fast_secp256r1
#      endif
      };

MG_UECC_Curve
mg_uecc_secp256r1 (void)
{
  return &curve_secp256r1;
}

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256r1)
/* Computes result = product % curve_p
   from http://www.nsa.gov/ia/_files/nist-routines.pdf */
#	if MG_UECC_WORD_SIZE == 1
static void
vli_mmod_fast_secp256r1 (uint8_t *result, uint8_t *product)
{
  uint8_t tmp[num_words_secp256r1];
  int8_t carry;

  /* t */
  mg_uecc_vli_set (result, product, num_words_secp256r1);

  /* s1 */
  tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
  tmp[4] = tmp[5] = tmp[6] = tmp[7] = 0;
  tmp[8] = tmp[9] = tmp[10] = tmp[11] = 0;
  tmp[12] = product[44];
  tmp[13] = product[45];
  tmp[14] = product[46];
  tmp[15] = product[47];
  tmp[16] = product[48];
  tmp[17] = product[49];
  tmp[18] = product[50];
  tmp[19] = product[51];
  tmp[20] = product[52];
  tmp[21] = product[53];
  tmp[22] = product[54];
  tmp[23] = product[55];
  tmp[24] = product[56];
  tmp[25] = product[57];
  tmp[26] = product[58];
  tmp[27] = product[59];
  tmp[28] = product[60];
  tmp[29] = product[61];
  tmp[30] = product[62];
  tmp[31] = product[63];
  carry = mg_uecc_vli_add (tmp, tmp, tmp, num_words_secp256r1);
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s2 */
  tmp[12] = product[48];
  tmp[13] = product[49];
  tmp[14] = product[50];
  tmp[15] = product[51];
  tmp[16] = product[52];
  tmp[17] = product[53];
  tmp[18] = product[54];
  tmp[19] = product[55];
  tmp[20] = product[56];
  tmp[21] = product[57];
  tmp[22] = product[58];
  tmp[23] = product[59];
  tmp[24] = product[60];
  tmp[25] = product[61];
  tmp[26] = product[62];
  tmp[27] = product[63];
  tmp[28] = tmp[29] = tmp[30] = tmp[31] = 0;
  carry += mg_uecc_vli_add (tmp, tmp, tmp, num_words_secp256r1);
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s3 */
  tmp[0] = product[32];
  tmp[1] = product[33];
  tmp[2] = product[34];
  tmp[3] = product[35];
  tmp[4] = product[36];
  tmp[5] = product[37];
  tmp[6] = product[38];
  tmp[7] = product[39];
  tmp[8] = product[40];
  tmp[9] = product[41];
  tmp[10] = product[42];
  tmp[11] = product[43];
  tmp[12] = tmp[13] = tmp[14] = tmp[15] = 0;
  tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
  tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
  tmp[24] = product[56];
  tmp[25] = product[57];
  tmp[26] = product[58];
  tmp[27] = product[59];
  tmp[28] = product[60];
  tmp[29] = product[61];
  tmp[30] = product[62];
  tmp[31] = product[63];
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s4 */
  tmp[0] = product[36];
  tmp[1] = product[37];
  tmp[2] = product[38];
  tmp[3] = product[39];
  tmp[4] = product[40];
  tmp[5] = product[41];
  tmp[6] = product[42];
  tmp[7] = product[43];
  tmp[8] = product[44];
  tmp[9] = product[45];
  tmp[10] = product[46];
  tmp[11] = product[47];
  tmp[12] = product[52];
  tmp[13] = product[53];
  tmp[14] = product[54];
  tmp[15] = product[55];
  tmp[16] = product[56];
  tmp[17] = product[57];
  tmp[18] = product[58];
  tmp[19] = product[59];
  tmp[20] = product[60];
  tmp[21] = product[61];
  tmp[22] = product[62];
  tmp[23] = product[63];
  tmp[24] = product[52];
  tmp[25] = product[53];
  tmp[26] = product[54];
  tmp[27] = product[55];
  tmp[28] = product[32];
  tmp[29] = product[33];
  tmp[30] = product[34];
  tmp[31] = product[35];
  carry += mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* d1 */
  tmp[0] = product[44];
  tmp[1] = product[45];
  tmp[2] = product[46];
  tmp[3] = product[47];
  tmp[4] = product[48];
  tmp[5] = product[49];
  tmp[6] = product[50];
  tmp[7] = product[51];
  tmp[8] = product[52];
  tmp[9] = product[53];
  tmp[10] = product[54];
  tmp[11] = product[55];
  tmp[12] = tmp[13] = tmp[14] = tmp[15] = 0;
  tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
  tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
  tmp[24] = product[32];
  tmp[25] = product[33];
  tmp[26] = product[34];
  tmp[27] = product[35];
  tmp[28] = product[40];
  tmp[29] = product[41];
  tmp[30] = product[42];
  tmp[31] = product[43];
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d2 */
  tmp[0] = product[48];
  tmp[1] = product[49];
  tmp[2] = product[50];
  tmp[3] = product[51];
  tmp[4] = product[52];
  tmp[5] = product[53];
  tmp[6] = product[54];
  tmp[7] = product[55];
  tmp[8] = product[56];
  tmp[9] = product[57];
  tmp[10] = product[58];
  tmp[11] = product[59];
  tmp[12] = product[60];
  tmp[13] = product[61];
  tmp[14] = product[62];
  tmp[15] = product[63];
  tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
  tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
  tmp[24] = product[36];
  tmp[25] = product[37];
  tmp[26] = product[38];
  tmp[27] = product[39];
  tmp[28] = product[44];
  tmp[29] = product[45];
  tmp[30] = product[46];
  tmp[31] = product[47];
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d3 */
  tmp[0] = product[52];
  tmp[1] = product[53];
  tmp[2] = product[54];
  tmp[3] = product[55];
  tmp[4] = product[56];
  tmp[5] = product[57];
  tmp[6] = product[58];
  tmp[7] = product[59];
  tmp[8] = product[60];
  tmp[9] = product[61];
  tmp[10] = product[62];
  tmp[11] = product[63];
  tmp[12] = product[32];
  tmp[13] = product[33];
  tmp[14] = product[34];
  tmp[15] = product[35];
  tmp[16] = product[36];
  tmp[17] = product[37];
  tmp[18] = product[38];
  tmp[19] = product[39];
  tmp[20] = product[40];
  tmp[21] = product[41];
  tmp[22] = product[42];
  tmp[23] = product[43];
  tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
  tmp[28] = product[48];
  tmp[29] = product[49];
  tmp[30] = product[50];
  tmp[31] = product[51];
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d4 */
  tmp[0] = product[56];
  tmp[1] = product[57];
  tmp[2] = product[58];
  tmp[3] = product[59];
  tmp[4] = product[60];
  tmp[5] = product[61];
  tmp[6] = product[62];
  tmp[7] = product[63];
  tmp[8] = tmp[9] = tmp[10] = tmp[11] = 0;
  tmp[12] = product[36];
  tmp[13] = product[37];
  tmp[14] = product[38];
  tmp[15] = product[39];
  tmp[16] = product[40];
  tmp[17] = product[41];
  tmp[18] = product[42];
  tmp[19] = product[43];
  tmp[20] = product[44];
  tmp[21] = product[45];
  tmp[22] = product[46];
  tmp[23] = product[47];
  tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
  tmp[28] = product[52];
  tmp[29] = product[53];
  tmp[30] = product[54];
  tmp[31] = product[55];
  carry -= mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  if (carry < 0)
    {
      do
	{
	  carry += mg_uecc_vli_add (result, result, curve_secp256r1.p,
				    num_words_secp256r1);
	}
      while (carry < 0);
    }
  else
    {
      while (carry
	     || mg_uecc_vli_cmp_unsafe (curve_secp256r1.p, result,
					num_words_secp256r1)
		    != 1)
	{
	  carry -= mg_uecc_vli_sub (result, result, curve_secp256r1.p,
				    num_words_secp256r1);
	}
    }
}
#	elif MG_UECC_WORD_SIZE == 4
static void
vli_mmod_fast_secp256r1 (uint32_t *result, uint32_t *product)
{
  uint32_t tmp[num_words_secp256r1];
  int carry;

  /* t */
  mg_uecc_vli_set (result, product, num_words_secp256r1);

  /* s1 */
  tmp[0] = tmp[1] = tmp[2] = 0;
  tmp[3] = product[11];
  tmp[4] = product[12];
  tmp[5] = product[13];
  tmp[6] = product[14];
  tmp[7] = product[15];
  carry = (int) mg_uecc_vli_add (tmp, tmp, tmp, num_words_secp256r1);
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s2 */
  tmp[3] = product[12];
  tmp[4] = product[13];
  tmp[5] = product[14];
  tmp[6] = product[15];
  tmp[7] = 0;
  carry += (int) mg_uecc_vli_add (tmp, tmp, tmp, num_words_secp256r1);
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s3 */
  tmp[0] = product[8];
  tmp[1] = product[9];
  tmp[2] = product[10];
  tmp[3] = tmp[4] = tmp[5] = 0;
  tmp[6] = product[14];
  tmp[7] = product[15];
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s4 */
  tmp[0] = product[9];
  tmp[1] = product[10];
  tmp[2] = product[11];
  tmp[3] = product[13];
  tmp[4] = product[14];
  tmp[5] = product[15];
  tmp[6] = product[13];
  tmp[7] = product[8];
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* d1 */
  tmp[0] = product[11];
  tmp[1] = product[12];
  tmp[2] = product[13];
  tmp[3] = tmp[4] = tmp[5] = 0;
  tmp[6] = product[8];
  tmp[7] = product[10];
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d2 */
  tmp[0] = product[12];
  tmp[1] = product[13];
  tmp[2] = product[14];
  tmp[3] = product[15];
  tmp[4] = tmp[5] = 0;
  tmp[6] = product[9];
  tmp[7] = product[11];
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d3 */
  tmp[0] = product[13];
  tmp[1] = product[14];
  tmp[2] = product[15];
  tmp[3] = product[8];
  tmp[4] = product[9];
  tmp[5] = product[10];
  tmp[6] = 0;
  tmp[7] = product[12];
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d4 */
  tmp[0] = product[14];
  tmp[1] = product[15];
  tmp[2] = 0;
  tmp[3] = product[9];
  tmp[4] = product[10];
  tmp[5] = product[11];
  tmp[6] = 0;
  tmp[7] = product[13];
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  if (carry < 0)
    {
      do
	{
	  carry += (int) mg_uecc_vli_add (result, result, curve_secp256r1.p,
					  num_words_secp256r1);
	}
      while (carry < 0);
    }
  else
    {
      while (carry
	     || mg_uecc_vli_cmp_unsafe (curve_secp256r1.p, result,
					num_words_secp256r1)
		    != 1)
	{
	  carry -= (int) mg_uecc_vli_sub (result, result, curve_secp256r1.p,
					  num_words_secp256r1);
	}
    }
}
#	else
static void
vli_mmod_fast_secp256r1 (uint64_t *result, uint64_t *product)
{
  uint64_t tmp[num_words_secp256r1];
  int carry;

  /* t */
  mg_uecc_vli_set (result, product, num_words_secp256r1);

  /* s1 */
  tmp[0] = 0;
  tmp[1] = product[5] & 0xffffffff00000000U;
  tmp[2] = product[6];
  tmp[3] = product[7];
  carry = (int) mg_uecc_vli_add (tmp, tmp, tmp, num_words_secp256r1);
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s2 */
  tmp[1] = product[6] << 32;
  tmp[2] = (product[6] >> 32) | (product[7] << 32);
  tmp[3] = product[7] >> 32;
  carry += (int) mg_uecc_vli_add (tmp, tmp, tmp, num_words_secp256r1);
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s3 */
  tmp[0] = product[4];
  tmp[1] = product[5] & 0xffffffff;
  tmp[2] = 0;
  tmp[3] = product[7];
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* s4 */
  tmp[0] = (product[4] >> 32) | (product[5] << 32);
  tmp[1] = (product[5] >> 32) | (product[6] & 0xffffffff00000000U);
  tmp[2] = product[7];
  tmp[3] = (product[6] >> 32) | (product[4] << 32);
  carry += (int) mg_uecc_vli_add (result, result, tmp, num_words_secp256r1);

  /* d1 */
  tmp[0] = (product[5] >> 32) | (product[6] << 32);
  tmp[1] = (product[6] >> 32);
  tmp[2] = 0;
  tmp[3] = (product[4] & 0xffffffff) | (product[5] << 32);
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d2 */
  tmp[0] = product[6];
  tmp[1] = product[7];
  tmp[2] = 0;
  tmp[3] = (product[4] >> 32) | (product[5] & 0xffffffff00000000);
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d3 */
  tmp[0] = (product[6] >> 32) | (product[7] << 32);
  tmp[1] = (product[7] >> 32) | (product[4] << 32);
  tmp[2] = (product[4] >> 32) | (product[5] << 32);
  tmp[3] = (product[6] << 32);
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  /* d4 */
  tmp[0] = product[7];
  tmp[1] = product[4] & 0xffffffff00000000U;
  tmp[2] = product[5];
  tmp[3] = product[6] & 0xffffffff00000000U;
  carry -= (int) mg_uecc_vli_sub (result, result, tmp, num_words_secp256r1);

  if (carry < 0)
    {
      do
	{
	  carry += (int) mg_uecc_vli_add (result, result, curve_secp256r1.p,
					  num_words_secp256r1);
	}
      while (carry < 0);
    }
  else
    {
      while (carry
	     || mg_uecc_vli_cmp_unsafe (curve_secp256r1.p, result,
					num_words_secp256r1)
		    != 1)
	{
	  carry -= (int) mg_uecc_vli_sub (result, result, curve_secp256r1.p,
					  num_words_secp256r1);
	}
    }
}
#	endif /* MG_UECC_WORD_SIZE */
#      endif /* (MG_UECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256r1)  \
	      */

#    endif /* MG_UECC_SUPPORTS_secp256r1 */

#    if MG_UECC_SUPPORTS_secp256k1

static void double_jacobian_secp256k1 (mg_uecc_word_t *X1, mg_uecc_word_t *Y1,
				       mg_uecc_word_t *Z1,
				       MG_UECC_Curve curve);
static void x_side_secp256k1 (mg_uecc_word_t *result, const mg_uecc_word_t *x,
			      MG_UECC_Curve curve);
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp256k1 (mg_uecc_word_t *result,
				     mg_uecc_word_t *product);
#      endif

static const struct MG_UECC_Curve_t curve_secp256k1
    = { num_words_secp256k1,
	num_bytes_secp256k1,
	256, /* num_n_bits */
	{ BYTES_TO_WORDS_8 (2F, FC, FF, FF, FE, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (41, 41, 36, D0, 8C, 5E, D2, BF),
	  BYTES_TO_WORDS_8 (3B, A0, 48, AF, E6, DC, AE, BA),
	  BYTES_TO_WORDS_8 (FE, FF, FF, FF, FF, FF, FF, FF),
	  BYTES_TO_WORDS_8 (FF, FF, FF, FF, FF, FF, FF, FF) },
	{ BYTES_TO_WORDS_8 (98, 17, F8, 16, 5B, 81, F2, 59),
	  BYTES_TO_WORDS_8 (D9, 28, CE, 2D, DB, FC, 9B, 02),
	  BYTES_TO_WORDS_8 (07, 0B, 87, CE, 95, 62, A0, 55),
	  BYTES_TO_WORDS_8 (AC, BB, DC, F9, 7E, 66, BE, 79),

	  BYTES_TO_WORDS_8 (B8, D4, 10, FB, 8F, D0, 47, 9C),
	  BYTES_TO_WORDS_8 (19, 54, 85, A6, 48, B4, 17, FD),
	  BYTES_TO_WORDS_8 (A8, 08, 11, 0E, FC, FB, A4, 5D),
	  BYTES_TO_WORDS_8 (65, C4, A3, 26, 77, DA, 3A, 48) },
	{ BYTES_TO_WORDS_8 (07, 00, 00, 00, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, 00, 00, 00, 00),
	  BYTES_TO_WORDS_8 (00, 00, 00, 00, 00, 00, 00, 00) },
	&double_jacobian_secp256k1,
#      if MG_UECC_SUPPORT_COMPRESSED_POINT
	&mod_sqrt_default,
#      endif
	&x_side_secp256k1,
#      if (MG_UECC_OPTIMIZATION_LEVEL > 0)
	&vli_mmod_fast_secp256k1
#      endif
      };

MG_UECC_Curve
mg_uecc_secp256k1 (void)
{
  return &curve_secp256k1;
}

/* Double in place */
static void
double_jacobian_secp256k1 (mg_uecc_word_t *X1, mg_uecc_word_t *Y1,
			   mg_uecc_word_t *Z1, MG_UECC_Curve curve)
{
  /* t1 = X, t2 = Y, t3 = Z */
  mg_uecc_word_t t4[num_words_secp256k1];
  mg_uecc_word_t t5[num_words_secp256k1];

  if (mg_uecc_vli_isZero (Z1, num_words_secp256k1))
    {
      return;
    }

  mg_uecc_vli_modSquare_fast (t5, Y1, curve);	/* t5 = y1^2 */
  mg_uecc_vli_modMult_fast (t4, X1, t5, curve); /* t4 = x1*y1^2 = A */
  mg_uecc_vli_modSquare_fast (X1, X1, curve);	/* t1 = x1^2 */
  mg_uecc_vli_modSquare_fast (t5, t5, curve);	/* t5 = y1^4 */
  mg_uecc_vli_modMult_fast (Z1, Y1, Z1, curve); /* t3 = y1*z1 = z3 */

  mg_uecc_vli_modAdd (Y1, X1, X1, curve->p,
		      num_words_secp256k1); /* t2 = 2*x1^2 */
  mg_uecc_vli_modAdd (Y1, Y1, X1, curve->p,
		      num_words_secp256k1); /* t2 = 3*x1^2 */
  if (mg_uecc_vli_testBit (Y1, 0))
    {
      mg_uecc_word_t carry
	  = mg_uecc_vli_add (Y1, Y1, curve->p, num_words_secp256k1);
      mg_uecc_vli_rshift1 (Y1, num_words_secp256k1);
      Y1[num_words_secp256k1 - 1] |= carry << (MG_UECC_WORD_BITS - 1);
    }
  else
    {
      mg_uecc_vli_rshift1 (Y1, num_words_secp256k1);
    }
  /* t2 = 3/2*(x1^2) = B */

  mg_uecc_vli_modSquare_fast (X1, Y1, curve); /* t1 = B^2 */
  mg_uecc_vli_modSub (X1, X1, t4, curve->p,
		      num_words_secp256k1); /* t1 = B^2 - A */
  mg_uecc_vli_modSub (X1, X1, t4, curve->p,
		      num_words_secp256k1); /* t1 = B^2 - 2A = x3 */

  mg_uecc_vli_modSub (t4, t4, X1, curve->p,
		      num_words_secp256k1);	/* t4 = A - x3 */
  mg_uecc_vli_modMult_fast (Y1, Y1, t4, curve); /* t2 = B * (A - x3) */
  mg_uecc_vli_modSub (Y1, Y1, t5, curve->p,
		      num_words_secp256k1); /* t2 = B * (A - x3) - y1^4 = y3 */
}

/* Computes result = x^3 + b. result must not overlap x. */
static void
x_side_secp256k1 (mg_uecc_word_t *result, const mg_uecc_word_t *x,
		  MG_UECC_Curve curve)
{
  mg_uecc_vli_modSquare_fast (result, x, curve);       /* r = x^2 */
  mg_uecc_vli_modMult_fast (result, result, x, curve); /* r = x^3 */
  mg_uecc_vli_modAdd (result, result, curve->b, curve->p,
		      num_words_secp256k1); /* r = x^3 + b */
}

#      if (MG_UECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256k1)
static void omega_mult_secp256k1 (mg_uecc_word_t *result,
				  const mg_uecc_word_t *right);
static void
vli_mmod_fast_secp256k1 (mg_uecc_word_t *result, mg_uecc_word_t *product)
{
  mg_uecc_word_t tmp[2 * num_words_secp256k1];
  mg_uecc_word_t carry;

  mg_uecc_vli_clear (tmp, num_words_secp256k1);
  mg_uecc_vli_clear (tmp + num_words_secp256k1, num_words_secp256k1);

  omega_mult_secp256k1 (tmp,
			product + num_words_secp256k1); /* (Rq, q) = q * c */

  carry = mg_uecc_vli_add (result, product, tmp,
			   num_words_secp256k1); /* (C, r) = r + q       */
  mg_uecc_vli_clear (product, num_words_secp256k1);
  omega_mult_secp256k1 (product, tmp + num_words_secp256k1); /* Rq*c */
  carry += mg_uecc_vli_add (result, result, product,
			    num_words_secp256k1); /* (C1, r) = r + Rq*c */

  while (carry > 0)
    {
      --carry;
      mg_uecc_vli_sub (result, result, curve_secp256k1.p, num_words_secp256k1);
    }
  if (mg_uecc_vli_cmp_unsafe (result, curve_secp256k1.p, num_words_secp256k1)
      > 0)
    {
      mg_uecc_vli_sub (result, result, curve_secp256k1.p, num_words_secp256k1);
    }
}

#	if MG_UECC_WORD_SIZE == 1
static void
omega_mult_secp256k1 (uint8_t *result, const uint8_t *right)
{
  /* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
  mg_uecc_word_t r0 = 0;
  mg_uecc_word_t r1 = 0;
  mg_uecc_word_t r2 = 0;
  wordcount_t k;

  /* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
  muladd (0xD1, right[0], &r0, &r1, &r2);
  result[0] = r0;
  r0 = r1;
  r1 = r2;
  /* r2 is still 0 */

  for (k = 1; k < num_words_secp256k1; ++k)
    {
      muladd (0x03, right[k - 1], &r0, &r1, &r2);
      muladd (0xD1, right[k], &r0, &r1, &r2);
      result[k] = r0;
      r0 = r1;
      r1 = r2;
      r2 = 0;
    }
  muladd (0x03, right[num_words_secp256k1 - 1], &r0, &r1, &r2);
  result[num_words_secp256k1] = r0;
  result[num_words_secp256k1 + 1] = r1;
  /* add the 2^32 multiple */
  result[4 + num_words_secp256k1]
      = mg_uecc_vli_add (result + 4, result + 4, right, num_words_secp256k1);
}
#	elif MG_UECC_WORD_SIZE == 4
static void
omega_mult_secp256k1 (uint32_t *result, const uint32_t *right)
{
  /* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
  uint32_t carry = 0;
  wordcount_t k;

  for (k = 0; k < num_words_secp256k1; ++k)
    {
      uint64_t p = (uint64_t) 0x3D1 * right[k] + carry;
      result[k] = (uint32_t) p;
      carry = p >> 32;
    }
  result[num_words_secp256k1] = carry;
  /* add the 2^32 multiple */
  result[1 + num_words_secp256k1]
      = mg_uecc_vli_add (result + 1, result + 1, right, num_words_secp256k1);
}
#	else
static void
omega_mult_secp256k1 (uint64_t *result, const uint64_t *right)
{
  mg_uecc_word_t r0 = 0;
  mg_uecc_word_t r1 = 0;
  mg_uecc_word_t r2 = 0;
  wordcount_t k;

  /* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
  for (k = 0; k < num_words_secp256k1; ++k)
    {
      muladd (0x1000003D1ull, right[k], &r0, &r1, &r2);
      result[k] = r0;
      r0 = r1;
      r1 = r2;
      r2 = 0;
    }
  result[num_words_secp256k1] = r0;
}
#	endif /* MG_UECC_WORD_SIZE */
#      endif   /* (MG_UECC_OPTIMIZATION_LEVEL > 0 &&  &&                      \
		  !asm_mmod_fast_secp256k1) */

#    endif /* MG_UECC_SUPPORTS_secp256k1 */

#  endif /* _UECC_CURVE_SPECIFIC_H_ */

/* Returns 1 if 'point' is the point at infinity, 0 otherwise. */
#  define EccPoint_isZero(point, curve)                                       \
    mg_uecc_vli_isZero ((point), (wordcount_t) ((curve)->num_words * 2))

/* Point multiplication algorithm using Montgomery's ladder with co-Z
coordinates. From http://eprint.iacr.org/2011/338.pdf
*/

/* Modify (x1, y1) => (x1 * z^2, y1 * z^3) */
static void
apply_z (mg_uecc_word_t *X1, mg_uecc_word_t *Y1, const mg_uecc_word_t *const Z,
	 MG_UECC_Curve curve)
{
  mg_uecc_word_t t1[MG_UECC_MAX_WORDS];

  mg_uecc_vli_modSquare_fast (t1, Z, curve);	/* z^2 */
  mg_uecc_vli_modMult_fast (X1, X1, t1, curve); /* x1 * z^2 */
  mg_uecc_vli_modMult_fast (t1, t1, Z, curve);	/* z^3 */
  mg_uecc_vli_modMult_fast (Y1, Y1, t1, curve); /* y1 * z^3 */
}

/* P = (x1, y1) => 2P, (x2, y2) => P' */
static void
XYcZ_initial_double (mg_uecc_word_t *X1, mg_uecc_word_t *Y1,
		     mg_uecc_word_t *X2, mg_uecc_word_t *Y2,
		     const mg_uecc_word_t *const initial_Z,
		     MG_UECC_Curve curve)
{
  mg_uecc_word_t z[MG_UECC_MAX_WORDS];
  wordcount_t num_words = curve->num_words;
  if (initial_Z)
    {
      mg_uecc_vli_set (z, initial_Z, num_words);
    }
  else
    {
      mg_uecc_vli_clear (z, num_words);
      z[0] = 1;
    }

  mg_uecc_vli_set (X2, X1, num_words);
  mg_uecc_vli_set (Y2, Y1, num_words);

  apply_z (X1, Y1, z, curve);
  curve->double_jacobian (X1, Y1, z, curve);
  apply_z (X2, Y2, z, curve);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P' = (x1', y1', Z3), P + Q = (x3, y3, Z3)
   or P => P', Q => P + Q
*/
static void
XYcZ_add (mg_uecc_word_t *X1, mg_uecc_word_t *Y1, mg_uecc_word_t *X2,
	  mg_uecc_word_t *Y2, MG_UECC_Curve curve)
{
  /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
  mg_uecc_word_t t5[MG_UECC_MAX_WORDS] = { 0 };
  wordcount_t num_words = curve->num_words;

  mg_uecc_vli_modSub (t5, X2, X1, curve->p, num_words); /* t5 = x2 - x1 */
  mg_uecc_vli_modSquare_fast (t5, t5, curve);	/* t5 = (x2 - x1)^2 = A */
  mg_uecc_vli_modMult_fast (X1, X1, t5, curve); /* t1 = x1*A = B */
  mg_uecc_vli_modMult_fast (X2, X2, t5, curve); /* t3 = x2*A = C */
  mg_uecc_vli_modSub (Y2, Y2, Y1, curve->p, num_words); /* t4 = y2 - y1 */
  mg_uecc_vli_modSquare_fast (t5, Y2, curve); /* t5 = (y2 - y1)^2 = D */

  mg_uecc_vli_modSub (t5, t5, X1, curve->p, num_words); /* t5 = D - B */
  mg_uecc_vli_modSub (t5, t5, X2, curve->p,
		      num_words); /* t5 = D - B - C = x3 */
  mg_uecc_vli_modSub (X2, X2, X1, curve->p, num_words); /* t3 = C - B */
  mg_uecc_vli_modMult_fast (Y1, Y1, X2, curve);		/* t2 = y1*(C - B) */
  mg_uecc_vli_modSub (X2, X1, t5, curve->p, num_words); /* t3 = B - x3 */
  mg_uecc_vli_modMult_fast (Y2, Y2, X2, curve); /* t4 = (y2 - y1)*(B - x3) */
  mg_uecc_vli_modSub (Y2, Y2, Y1, curve->p, num_words); /* t4 = y3 */

  mg_uecc_vli_set (X2, t5, num_words);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P + Q = (x3, y3, Z3), P - Q = (x3', y3', Z3)
   or P => P - Q, Q => P + Q
*/
static void
XYcZ_addC (mg_uecc_word_t *X1, mg_uecc_word_t *Y1, mg_uecc_word_t *X2,
	   mg_uecc_word_t *Y2, MG_UECC_Curve curve)
{
  /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
  mg_uecc_word_t t5[MG_UECC_MAX_WORDS] = { 0 };
  mg_uecc_word_t t6[MG_UECC_MAX_WORDS];
  mg_uecc_word_t t7[MG_UECC_MAX_WORDS];
  wordcount_t num_words = curve->num_words;

  mg_uecc_vli_modSub (t5, X2, X1, curve->p, num_words); /* t5 = x2 - x1 */
  mg_uecc_vli_modSquare_fast (t5, t5, curve);	/* t5 = (x2 - x1)^2 = A */
  mg_uecc_vli_modMult_fast (X1, X1, t5, curve); /* t1 = x1*A = B */
  mg_uecc_vli_modMult_fast (X2, X2, t5, curve); /* t3 = x2*A = C */
  mg_uecc_vli_modAdd (t5, Y2, Y1, curve->p, num_words); /* t5 = y2 + y1 */
  mg_uecc_vli_modSub (Y2, Y2, Y1, curve->p, num_words); /* t4 = y2 - y1 */

  mg_uecc_vli_modSub (t6, X2, X1, curve->p, num_words); /* t6 = C - B */
  mg_uecc_vli_modMult_fast (Y1, Y1, t6, curve); /* t2 = y1 * (C - B) = E */
  mg_uecc_vli_modAdd (t6, X1, X2, curve->p, num_words); /* t6 = B + C */
  mg_uecc_vli_modSquare_fast (X2, Y2, curve); /* t3 = (y2 - y1)^2 = D */
  mg_uecc_vli_modSub (X2, X2, t6, curve->p,
		      num_words); /* t3 = D - (B + C) = x3 */

  mg_uecc_vli_modSub (t7, X1, X2, curve->p, num_words); /* t7 = B - x3 */
  mg_uecc_vli_modMult_fast (Y2, Y2, t7, curve); /* t4 = (y2 - y1)*(B - x3) */
  mg_uecc_vli_modSub (Y2, Y2, Y1, curve->p,
		      num_words); /* t4 = (y2 - y1)*(B - x3) - E = y3 */

  mg_uecc_vli_modSquare_fast (t7, t5, curve); /* t7 = (y2 + y1)^2 = F */
  mg_uecc_vli_modSub (t7, t7, t6, curve->p,
		      num_words); /* t7 = F - (B + C) = x3' */
  mg_uecc_vli_modSub (t6, t7, X1, curve->p, num_words); /* t6 = x3' - B */
  mg_uecc_vli_modMult_fast (t6, t6, t5, curve); /* t6 = (y2+y1)*(x3' - B) */
  mg_uecc_vli_modSub (Y1, t6, Y1, curve->p,
		      num_words); /* t2 = (y2+y1)*(x3' - B) - E = y3' */

  mg_uecc_vli_set (X1, t7, num_words);
}

/* result may overlap point. */
static void
EccPoint_mult (mg_uecc_word_t *result, const mg_uecc_word_t *point,
	       const mg_uecc_word_t *scalar, const mg_uecc_word_t *initial_Z,
	       bitcount_t num_bits, MG_UECC_Curve curve)
{
  /* R0 and R1 */
  mg_uecc_word_t Rx[2][MG_UECC_MAX_WORDS];
  mg_uecc_word_t Ry[2][MG_UECC_MAX_WORDS];
  mg_uecc_word_t z[MG_UECC_MAX_WORDS];
  bitcount_t i;
  mg_uecc_word_t nb;
  wordcount_t num_words = curve->num_words;

  mg_uecc_vli_set (Rx[1], point, num_words);
  mg_uecc_vli_set (Ry[1], point + num_words, num_words);

  XYcZ_initial_double (Rx[1], Ry[1], Rx[0], Ry[0], initial_Z, curve);

  for (i = num_bits - 2; i > 0; --i)
    {
      nb = !mg_uecc_vli_testBit (scalar, i);
      XYcZ_addC (Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb], curve);
      XYcZ_add (Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb], curve);
    }

  nb = !mg_uecc_vli_testBit (scalar, 0);
  XYcZ_addC (Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb], curve);

  /* Find final 1/Z value. */
  mg_uecc_vli_modSub (z, Rx[1], Rx[0], curve->p, num_words); /* X1 - X0 */
  mg_uecc_vli_modMult_fast (z, z, Ry[1 - nb], curve); /* Yb * (X1 - X0) */
  mg_uecc_vli_modMult_fast (z, z, point, curve);      /* xP * Yb * (X1 - X0) */
  mg_uecc_vli_modInv (z, z, curve->p,
		      num_words); /* 1 / (xP * Yb * (X1 - X0)) */
  /* yP / (xP * Yb * (X1 - X0)) */
  mg_uecc_vli_modMult_fast (z, z, point + num_words, curve);
  mg_uecc_vli_modMult_fast (z, z, Rx[1 - nb],
			    curve); /* Xb * yP / (xP * Yb * (X1 - X0)) */
  /* End 1/Z calculation */

  XYcZ_add (Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb], curve);
  apply_z (Rx[0], Ry[0], z, curve);

  mg_uecc_vli_set (result, Rx[0], num_words);
  mg_uecc_vli_set (result + num_words, Ry[0], num_words);
}

static mg_uecc_word_t
regularize_k (const mg_uecc_word_t *const k, mg_uecc_word_t *k0,
	      mg_uecc_word_t *k1, MG_UECC_Curve curve)
{
  wordcount_t num_n_words = BITS_TO_WORDS (curve->num_n_bits);
  bitcount_t num_n_bits = curve->num_n_bits;
  mg_uecc_word_t carry
      = mg_uecc_vli_add (k0, k, curve->n, num_n_words)
	|| (num_n_bits < ((bitcount_t) num_n_words * MG_UECC_WORD_SIZE * 8)
	    && mg_uecc_vli_testBit (k0, num_n_bits));
  mg_uecc_vli_add (k1, k0, curve->n, num_n_words);
  return carry;
}

/* Generates a random integer in the range 0 < random < top.
   Both random and top have num_words words. */
MG_UECC_VLI_API int
mg_uecc_generate_random_int (mg_uecc_word_t *random, const mg_uecc_word_t *top,
			     wordcount_t num_words)
{
  mg_uecc_word_t mask = (mg_uecc_word_t) -1;
  mg_uecc_word_t tries;
  bitcount_t num_bits = mg_uecc_vli_numBits (top, num_words);

  if (!g_rng_function)
    {
      return 0;
    }

  for (tries = 0; tries < MG_UECC_RNG_MAX_TRIES; ++tries)
    {
      if (!g_rng_function ((uint8_t *) random,
			   (unsigned int) (num_words * MG_UECC_WORD_SIZE)))
	{
	  return 0;
	}
      random[num_words - 1]
	  &= mask
	     >> ((bitcount_t) (num_words * MG_UECC_WORD_SIZE * 8 - num_bits));
      if (!mg_uecc_vli_isZero (random, num_words)
	  && mg_uecc_vli_cmp (top, random, num_words) == 1)
	{
	  return 1;
	}
    }
  return 0;
}

static mg_uecc_word_t
EccPoint_compute_public_key (mg_uecc_word_t *result,
			     mg_uecc_word_t *private_key, MG_UECC_Curve curve)
{
  mg_uecc_word_t tmp1[MG_UECC_MAX_WORDS];
  mg_uecc_word_t tmp2[MG_UECC_MAX_WORDS];
  mg_uecc_word_t *p2[2] = { tmp1, tmp2 };
  mg_uecc_word_t *initial_Z = 0;
  mg_uecc_word_t carry;

  /* Regularize the bitcount for the private key so that attackers cannot use a
     side channel attack to learn the number of leading zeros. */
  carry = regularize_k (private_key, tmp1, tmp2, curve);

  /* If an RNG function was specified, try to get a random initial Z value to
     improve protection against side-channel attacks. */
  if (g_rng_function)
    {
      if (!mg_uecc_generate_random_int (p2[carry], curve->p, curve->num_words))
	{
	  return 0;
	}
      initial_Z = p2[carry];
    }
  EccPoint_mult (result, curve->G, p2[!carry], initial_Z,
		 (bitcount_t) (curve->num_n_bits + 1), curve);

  if (EccPoint_isZero (result, curve))
    {
      return 0;
    }
  return 1;
}

#  if MG_UECC_WORD_SIZE == 1

MG_UECC_VLI_API void
mg_uecc_vli_nativeToBytes (uint8_t *bytes, int num_bytes,
			   const uint8_t *native)
{
  wordcount_t i;
  for (i = 0; i < num_bytes; ++i)
    {
      bytes[i] = native[(num_bytes - 1) - i];
    }
}

MG_UECC_VLI_API void
mg_uecc_vli_bytesToNative (uint8_t *native, const uint8_t *bytes,
			   int num_bytes)
{
  mg_uecc_vli_nativeToBytes (native, num_bytes, bytes);
}

#  else

MG_UECC_VLI_API void
mg_uecc_vli_nativeToBytes (uint8_t *bytes, int num_bytes,
			   const mg_uecc_word_t *native)
{
  int i;
  for (i = 0; i < num_bytes; ++i)
    {
      unsigned b = (unsigned) (num_bytes - 1 - i);
      bytes[i] = (uint8_t) (native[b / MG_UECC_WORD_SIZE]
			    >> (8 * (b % MG_UECC_WORD_SIZE)));
    }
}

MG_UECC_VLI_API void
mg_uecc_vli_bytesToNative (mg_uecc_word_t *native, const uint8_t *bytes,
			   int num_bytes)
{
  int i;
  mg_uecc_vli_clear (native,
		     (wordcount_t) ((num_bytes + (MG_UECC_WORD_SIZE - 1))
				    / MG_UECC_WORD_SIZE));
  for (i = 0; i < num_bytes; ++i)
    {
      unsigned b = (unsigned) (num_bytes - 1 - i);
      native[b / MG_UECC_WORD_SIZE] |= (mg_uecc_word_t) bytes[i]
				       << (8 * (b % MG_UECC_WORD_SIZE));
    }
}

#  endif /* MG_UECC_WORD_SIZE */

int
mg_uecc_make_key (uint8_t *public_key, uint8_t *private_key,
		  MG_UECC_Curve curve)
{
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  mg_uecc_word_t *_private = (mg_uecc_word_t *) private_key;
  mg_uecc_word_t *_public = (mg_uecc_word_t *) public_key;
#  else
  mg_uecc_word_t _private[MG_UECC_MAX_WORDS];
  mg_uecc_word_t _public[MG_UECC_MAX_WORDS * 2];
#  endif
  mg_uecc_word_t tries;

  for (tries = 0; tries < MG_UECC_RNG_MAX_TRIES; ++tries)
    {
      if (!mg_uecc_generate_random_int (_private, curve->n,
					BITS_TO_WORDS (curve->num_n_bits)))
	{
	  return 0;
	}

      if (EccPoint_compute_public_key (_public, _private, curve))
	{
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
	  mg_uecc_vli_nativeToBytes (
	      private_key, BITS_TO_BYTES (curve->num_n_bits), _private);
	  mg_uecc_vli_nativeToBytes (public_key, curve->num_bytes, _public);
	  mg_uecc_vli_nativeToBytes (public_key + curve->num_bytes,
				     curve->num_bytes,
				     _public + curve->num_words);
#  endif
	  return 1;
	}
    }
  return 0;
}

int
mg_uecc_shared_secret (const uint8_t *public_key, const uint8_t *private_key,
		       uint8_t *secret, MG_UECC_Curve curve)
{
  mg_uecc_word_t _public[MG_UECC_MAX_WORDS * 2];
  mg_uecc_word_t _private[MG_UECC_MAX_WORDS];

  mg_uecc_word_t tmp[MG_UECC_MAX_WORDS];
  mg_uecc_word_t *p2[2] = { _private, tmp };
  mg_uecc_word_t *initial_Z = 0;
  mg_uecc_word_t carry;
  wordcount_t num_words = curve->num_words;
  wordcount_t num_bytes = curve->num_bytes;

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy ((uint8_t *) _private, private_key, num_bytes);
  bcopy ((uint8_t *) _public, public_key, num_bytes * 2);
#  else
  mg_uecc_vli_bytesToNative (_private, private_key,
			     BITS_TO_BYTES (curve->num_n_bits));
  mg_uecc_vli_bytesToNative (_public, public_key, num_bytes);
  mg_uecc_vli_bytesToNative (_public + num_words, public_key + num_bytes,
			     num_bytes);
#  endif

  /* Regularize the bitcount for the private key so that attackers cannot use a
     side channel attack to learn the number of leading zeros. */
  carry = regularize_k (_private, _private, tmp, curve);

  /* If an RNG function was specified, try to get a random initial Z value to
     improve protection against side-channel attacks. */
  if (g_rng_function)
    {
      if (!mg_uecc_generate_random_int (p2[carry], curve->p, num_words))
	{
	  return 0;
	}
      initial_Z = p2[carry];
    }

  EccPoint_mult (_public, _public, p2[!carry], initial_Z,
		 (bitcount_t) (curve->num_n_bits + 1), curve);
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy ((uint8_t *) secret, (uint8_t *) _public, num_bytes);
#  else
  mg_uecc_vli_nativeToBytes (secret, num_bytes, _public);
#  endif
  return !EccPoint_isZero (_public, curve);
}

#  if MG_UECC_SUPPORT_COMPRESSED_POINT
void
mg_uecc_compress (const uint8_t *public_key, uint8_t *compressed,
		  MG_UECC_Curve curve)
{
  wordcount_t i;
  for (i = 0; i < curve->num_bytes; ++i)
    {
      compressed[i + 1] = public_key[i];
    }
#    if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  compressed[0] = 2 + (public_key[curve->num_bytes] & 0x01);
#    else
  compressed[0] = 2 + (public_key[curve->num_bytes * 2 - 1] & 0x01);
#    endif
}

void
mg_uecc_decompress (const uint8_t *compressed, uint8_t *public_key,
		    MG_UECC_Curve curve)
{
#    if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  mg_uecc_word_t *point = (mg_uecc_word_t *) public_key;
#    else
  mg_uecc_word_t point[MG_UECC_MAX_WORDS * 2];
#    endif
  mg_uecc_word_t *y = point + curve->num_words;
#    if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy (public_key, compressed + 1, curve->num_bytes);
#    else
  mg_uecc_vli_bytesToNative (point, compressed + 1, curve->num_bytes);
#    endif
  curve->x_side (y, point, curve);
  curve->mod_sqrt (y, curve);

  if ((uint8_t) (y[0] & 0x01) != (compressed[0] & 0x01))
    {
      mg_uecc_vli_sub (y, curve->p, y, curve->num_words);
    }

#    if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
  mg_uecc_vli_nativeToBytes (public_key, curve->num_bytes, point);
  mg_uecc_vli_nativeToBytes (public_key + curve->num_bytes, curve->num_bytes,
			     y);
#    endif
}
#  endif /* MG_UECC_SUPPORT_COMPRESSED_POINT */

MG_UECC_VLI_API int
mg_uecc_valid_point (const mg_uecc_word_t *point, MG_UECC_Curve curve)
{
  mg_uecc_word_t tmp1[MG_UECC_MAX_WORDS];
  mg_uecc_word_t tmp2[MG_UECC_MAX_WORDS];
  wordcount_t num_words = curve->num_words;

  /* The point at infinity is invalid. */
  if (EccPoint_isZero (point, curve))
    {
      return 0;
    }

  /* x and y must be smaller than p. */
  if (mg_uecc_vli_cmp_unsafe (curve->p, point, num_words) != 1
      || mg_uecc_vli_cmp_unsafe (curve->p, point + num_words, num_words) != 1)
    {
      return 0;
    }

  mg_uecc_vli_modSquare_fast (tmp1, point + num_words, curve);
  curve->x_side (tmp2, point, curve); /* tmp2 = x^3 + ax + b */

  /* Make sure that y^2 == x^3 + ax + b */
  return (int) (mg_uecc_vli_equal (tmp1, tmp2, num_words));
}

int
mg_uecc_valid_public_key (const uint8_t *public_key, MG_UECC_Curve curve)
{
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  mg_uecc_word_t *_public = (mg_uecc_word_t *) public_key;
#  else
  mg_uecc_word_t _public[MG_UECC_MAX_WORDS * 2];
#  endif

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
  mg_uecc_vli_bytesToNative (_public, public_key, curve->num_bytes);
  mg_uecc_vli_bytesToNative (_public + curve->num_words,
			     public_key + curve->num_bytes, curve->num_bytes);
#  endif
  return mg_uecc_valid_point (_public, curve);
}

int
mg_uecc_compute_public_key (const uint8_t *private_key, uint8_t *public_key,
			    MG_UECC_Curve curve)
{
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  mg_uecc_word_t *_private = (mg_uecc_word_t *) private_key;
  mg_uecc_word_t *_public = (mg_uecc_word_t *) public_key;
#  else
  mg_uecc_word_t _private[MG_UECC_MAX_WORDS];
  mg_uecc_word_t _public[MG_UECC_MAX_WORDS * 2];
#  endif

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
  mg_uecc_vli_bytesToNative (_private, private_key,
			     BITS_TO_BYTES (curve->num_n_bits));
#  endif

  /* Make sure the private key is in the range [1, n-1]. */
  if (mg_uecc_vli_isZero (_private, BITS_TO_WORDS (curve->num_n_bits)))
    {
      return 0;
    }

  if (mg_uecc_vli_cmp (curve->n, _private, BITS_TO_WORDS (curve->num_n_bits))
      != 1)
    {
      return 0;
    }

  /* Compute public key. */
  if (!EccPoint_compute_public_key (_public, _private, curve))
    {
      return 0;
    }

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
  mg_uecc_vli_nativeToBytes (public_key, curve->num_bytes, _public);
  mg_uecc_vli_nativeToBytes (public_key + curve->num_bytes, curve->num_bytes,
			     _public + curve->num_words);
#  endif
  return 1;
}

/* -------- ECDSA code -------- */

static void
bits2int (mg_uecc_word_t *native, const uint8_t *bits, unsigned bits_size,
	  MG_UECC_Curve curve)
{
  unsigned num_n_bytes = (unsigned) BITS_TO_BYTES (curve->num_n_bits);
  unsigned num_n_words = (unsigned) BITS_TO_WORDS (curve->num_n_bits);
  int shift;
  mg_uecc_word_t carry;
  mg_uecc_word_t *ptr;

  if (bits_size > num_n_bytes)
    {
      bits_size = num_n_bytes;
    }

  mg_uecc_vli_clear (native, (wordcount_t) num_n_words);
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy ((uint8_t *) native, bits, bits_size);
#  else
  mg_uecc_vli_bytesToNative (native, bits, (int) bits_size);
#  endif
  if (bits_size * 8 <= (unsigned) curve->num_n_bits)
    {
      return;
    }
  shift = (int) bits_size * 8 - curve->num_n_bits;
  carry = 0;
  ptr = native + num_n_words;
  while (ptr-- > native)
    {
      mg_uecc_word_t temp = *ptr;
      *ptr = (temp >> shift) | carry;
      carry = temp << (MG_UECC_WORD_BITS - shift);
    }

  /* Reduce mod curve_n */
  if (mg_uecc_vli_cmp_unsafe (curve->n, native, (wordcount_t) num_n_words)
      != 1)
    {
      mg_uecc_vli_sub (native, native, curve->n, (wordcount_t) num_n_words);
    }
}

static int
mg_uecc_sign_with_k_internal (const uint8_t *private_key,
			      const uint8_t *message_hash, unsigned hash_size,
			      mg_uecc_word_t *k, uint8_t *signature,
			      MG_UECC_Curve curve)
{
  mg_uecc_word_t tmp[MG_UECC_MAX_WORDS];
  mg_uecc_word_t s[MG_UECC_MAX_WORDS];
  mg_uecc_word_t *k2[2] = { tmp, s };
  mg_uecc_word_t *initial_Z = 0;
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  mg_uecc_word_t *p = (mg_uecc_word_t *) signature;
#  else
  mg_uecc_word_t p[MG_UECC_MAX_WORDS * 2];
#  endif
  mg_uecc_word_t carry;
  wordcount_t num_words = curve->num_words;
  wordcount_t num_n_words = BITS_TO_WORDS (curve->num_n_bits);
  bitcount_t num_n_bits = curve->num_n_bits;

  /* Make sure 0 < k < curve_n */
  if (mg_uecc_vli_isZero (k, num_words)
      || mg_uecc_vli_cmp (curve->n, k, num_n_words) != 1)
    {
      return 0;
    }

  carry = regularize_k (k, tmp, s, curve);
  /* If an RNG function was specified, try to get a random initial Z value to
     improve protection against side-channel attacks. */
  if (g_rng_function)
    {
      if (!mg_uecc_generate_random_int (k2[carry], curve->p, num_words))
	{
	  return 0;
	}
      initial_Z = k2[carry];
    }
  EccPoint_mult (p, curve->G, k2[!carry], initial_Z,
		 (bitcount_t) (num_n_bits + 1), curve);
  if (mg_uecc_vli_isZero (p, num_words))
    {
      return 0;
    }

  /* If an RNG function was specified, get a random number
     to prevent side channel analysis of k. */
  if (!g_rng_function)
    {
      mg_uecc_vli_clear (tmp, num_n_words);
      tmp[0] = 1;
    }
  else if (!mg_uecc_generate_random_int (tmp, curve->n, num_n_words))
    {
      return 0;
    }

  /* Prevent side channel analysis of mg_uecc_vli_modInv() to determine
     bits of k / the private key by premultiplying by a random number */
  mg_uecc_vli_modMult (k, k, tmp, curve->n, num_n_words); /* k' = rand * k */
  mg_uecc_vli_modInv (k, k, curve->n, num_n_words);	  /* k = 1 / k' */
  mg_uecc_vli_modMult (k, k, tmp, curve->n, num_n_words); /* k = 1 / k */

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
  mg_uecc_vli_nativeToBytes (signature, curve->num_bytes, p); /* store r */
#  endif

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy ((uint8_t *) tmp, private_key, BITS_TO_BYTES (curve->num_n_bits));
#  else
  mg_uecc_vli_bytesToNative (tmp, private_key,
			     BITS_TO_BYTES (curve->num_n_bits)); /* tmp = d */
#  endif

  s[num_n_words - 1] = 0;
  mg_uecc_vli_set (s, p, num_words);
  mg_uecc_vli_modMult (s, tmp, s, curve->n, num_n_words); /* s = r*d */

  bits2int (tmp, message_hash, hash_size, curve);
  mg_uecc_vli_modAdd (s, tmp, s, curve->n, num_n_words); /* s = e + r*d */
  mg_uecc_vli_modMult (s, s, k, curve->n, num_n_words); /* s = (e + r*d) / k */
  if (mg_uecc_vli_numBits (s, num_n_words) > (bitcount_t) curve->num_bytes * 8)
    {
      return 0;
    }
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy ((uint8_t *) signature + curve->num_bytes, (uint8_t *) s,
	 curve->num_bytes);
#  else
  mg_uecc_vli_nativeToBytes (signature + curve->num_bytes, curve->num_bytes,
			     s);
#  endif
  return 1;
}

#  if 0
/* For testing - sign with an explicitly specified k value */
int mg_uecc_sign_with_k(const uint8_t *private_key, const uint8_t *message_hash,
                     unsigned hash_size, const uint8_t *k, uint8_t *signature,
                     MG_UECC_Curve curve) {
  mg_uecc_word_t k2[MG_UECC_MAX_WORDS];
  bits2int(k2, k, (unsigned) BITS_TO_BYTES(curve->num_n_bits), curve);
  return mg_uecc_sign_with_k_internal(private_key, message_hash, hash_size, k2,
                                   signature, curve);
}
#  endif

int
mg_uecc_sign (const uint8_t *private_key, const uint8_t *message_hash,
	      unsigned hash_size, uint8_t *signature, MG_UECC_Curve curve)
{
  mg_uecc_word_t k[MG_UECC_MAX_WORDS];
  mg_uecc_word_t tries;

  for (tries = 0; tries < MG_UECC_RNG_MAX_TRIES; ++tries)
    {
      if (!mg_uecc_generate_random_int (k, curve->n,
					BITS_TO_WORDS (curve->num_n_bits)))
	{
	  return 0;
	}

      if (mg_uecc_sign_with_k_internal (private_key, message_hash, hash_size,
					k, signature, curve))
	{
	  return 1;
	}
    }
  return 0;
}

/* Compute an HMAC using K as a key (as in RFC 6979). Note that K is always
   the same size as the hash result size. */
static void
HMAC_init (const MG_UECC_HashContext *hash_context, const uint8_t *K)
{
  uint8_t *pad = hash_context->tmp + 2 * hash_context->result_size;
  unsigned i;
  for (i = 0; i < hash_context->result_size; ++i)
    pad[i] = K[i] ^ 0x36;
  for (; i < hash_context->block_size; ++i)
    pad[i] = 0x36;

  hash_context->init_hash (hash_context);
  hash_context->update_hash (hash_context, pad, hash_context->block_size);
}

static void
HMAC_update (const MG_UECC_HashContext *hash_context, const uint8_t *message,
	     unsigned message_size)
{
  hash_context->update_hash (hash_context, message, message_size);
}

static void
HMAC_finish (const MG_UECC_HashContext *hash_context, const uint8_t *K,
	     uint8_t *result)
{
  uint8_t *pad = hash_context->tmp + 2 * hash_context->result_size;
  unsigned i;
  for (i = 0; i < hash_context->result_size; ++i)
    pad[i] = K[i] ^ 0x5c;
  for (; i < hash_context->block_size; ++i)
    pad[i] = 0x5c;

  hash_context->finish_hash (hash_context, result);

  hash_context->init_hash (hash_context);
  hash_context->update_hash (hash_context, pad, hash_context->block_size);
  hash_context->update_hash (hash_context, result, hash_context->result_size);
  hash_context->finish_hash (hash_context, result);
}

/* V = HMAC_K(V) */
static void
update_V (const MG_UECC_HashContext *hash_context, uint8_t *K, uint8_t *V)
{
  HMAC_init (hash_context, K);
  HMAC_update (hash_context, V, hash_context->result_size);
  HMAC_finish (hash_context, K, V);
}

/* Deterministic signing, similar to RFC 6979. Differences are:
    * We just use H(m) directly rather than bits2octets(H(m))
      (it is not reduced modulo curve_n).
    * We generate a value for k (aka T) directly rather than converting
   endianness.

   Layout of hash_context->tmp: <K> | <V> | (1 byte overlapped 0x00 or 0x01) /
   <HMAC pad> */
int
mg_uecc_sign_deterministic (const uint8_t *private_key,
			    const uint8_t *message_hash, unsigned hash_size,
			    const MG_UECC_HashContext *hash_context,
			    uint8_t *signature, MG_UECC_Curve curve)
{
  uint8_t *K = hash_context->tmp;
  uint8_t *V = K + hash_context->result_size;
  wordcount_t num_bytes = curve->num_bytes;
  wordcount_t num_n_words = BITS_TO_WORDS (curve->num_n_bits);
  bitcount_t num_n_bits = curve->num_n_bits;
  mg_uecc_word_t tries;
  unsigned i;
  for (i = 0; i < hash_context->result_size; ++i)
    {
      V[i] = 0x01;
      K[i] = 0;
    }

  /* K = HMAC_K(V || 0x00 || int2octets(x) || h(m)) */
  HMAC_init (hash_context, K);
  V[hash_context->result_size] = 0x00;
  HMAC_update (hash_context, V, hash_context->result_size + 1);
  HMAC_update (hash_context, private_key, (unsigned int) num_bytes);
  HMAC_update (hash_context, message_hash, hash_size);
  HMAC_finish (hash_context, K, K);

  update_V (hash_context, K, V);

  /* K = HMAC_K(V || 0x01 || int2octets(x) || h(m)) */
  HMAC_init (hash_context, K);
  V[hash_context->result_size] = 0x01;
  HMAC_update (hash_context, V, hash_context->result_size + 1);
  HMAC_update (hash_context, private_key, (unsigned int) num_bytes);
  HMAC_update (hash_context, message_hash, hash_size);
  HMAC_finish (hash_context, K, K);

  update_V (hash_context, K, V);

  for (tries = 0; tries < MG_UECC_RNG_MAX_TRIES; ++tries)
    {
      mg_uecc_word_t T[MG_UECC_MAX_WORDS];
      uint8_t *T_ptr = (uint8_t *) T;
      wordcount_t T_bytes = 0;
      for (;;)
	{
	  update_V (hash_context, K, V);
	  for (i = 0; i < hash_context->result_size; ++i)
	    {
	      T_ptr[T_bytes++] = V[i];
	      if (T_bytes >= num_n_words * MG_UECC_WORD_SIZE)
		{
		  goto filled;
		}
	    }
	}
    filled:
      if ((bitcount_t) num_n_words * MG_UECC_WORD_SIZE * 8 > num_n_bits)
	{
	  mg_uecc_word_t mask = (mg_uecc_word_t) -1;
	  T[num_n_words - 1]
	      &= mask >> ((bitcount_t) (num_n_words * MG_UECC_WORD_SIZE * 8
					- num_n_bits));
	}

      if (mg_uecc_sign_with_k_internal (private_key, message_hash, hash_size,
					T, signature, curve))
	{
	  return 1;
	}

      /* K = HMAC_K(V || 0x00) */
      HMAC_init (hash_context, K);
      V[hash_context->result_size] = 0x00;
      HMAC_update (hash_context, V, hash_context->result_size + 1);
      HMAC_finish (hash_context, K, K);

      update_V (hash_context, K, V);
    }
  return 0;
}

static bitcount_t
smax (bitcount_t a, bitcount_t b)
{
  return (a > b ? a : b);
}

int
mg_uecc_verify (const uint8_t *public_key, const uint8_t *message_hash,
		unsigned hash_size, const uint8_t *signature,
		MG_UECC_Curve curve)
{
  mg_uecc_word_t u1[MG_UECC_MAX_WORDS], u2[MG_UECC_MAX_WORDS];
  mg_uecc_word_t z[MG_UECC_MAX_WORDS];
  mg_uecc_word_t sum[MG_UECC_MAX_WORDS * 2];
  mg_uecc_word_t rx[MG_UECC_MAX_WORDS];
  mg_uecc_word_t ry[MG_UECC_MAX_WORDS];
  mg_uecc_word_t tx[MG_UECC_MAX_WORDS];
  mg_uecc_word_t ty[MG_UECC_MAX_WORDS];
  mg_uecc_word_t tz[MG_UECC_MAX_WORDS];
  const mg_uecc_word_t *points[4];
  const mg_uecc_word_t *point;
  bitcount_t num_bits;
  bitcount_t i;
#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  mg_uecc_word_t *_public = (mg_uecc_word_t *) public_key;
#  else
  mg_uecc_word_t _public[MG_UECC_MAX_WORDS * 2];
#  endif
  mg_uecc_word_t r[MG_UECC_MAX_WORDS], s[MG_UECC_MAX_WORDS];
  wordcount_t num_words = curve->num_words;
  wordcount_t num_n_words = BITS_TO_WORDS (curve->num_n_bits);

  rx[num_n_words - 1] = 0;
  r[num_n_words - 1] = 0;
  s[num_n_words - 1] = 0;

#  if MG_UECC_VLI_NATIVE_LITTLE_ENDIAN
  bcopy ((uint8_t *) r, signature, curve->num_bytes);
  bcopy ((uint8_t *) s, signature + curve->num_bytes, curve->num_bytes);
#  else
  mg_uecc_vli_bytesToNative (_public, public_key, curve->num_bytes);
  mg_uecc_vli_bytesToNative (_public + num_words,
			     public_key + curve->num_bytes, curve->num_bytes);
  mg_uecc_vli_bytesToNative (r, signature, curve->num_bytes);
  mg_uecc_vli_bytesToNative (s, signature + curve->num_bytes,
			     curve->num_bytes);
#  endif

  /* r, s must not be 0. */
  if (mg_uecc_vli_isZero (r, num_words) || mg_uecc_vli_isZero (s, num_words))
    {
      return 0;
    }

  /* r, s must be < n. */
  if (mg_uecc_vli_cmp_unsafe (curve->n, r, num_n_words) != 1
      || mg_uecc_vli_cmp_unsafe (curve->n, s, num_n_words) != 1)
    {
      return 0;
    }

  /* Calculate u1 and u2. */
  mg_uecc_vli_modInv (z, s, curve->n, num_n_words); /* z = 1/s */
  u1[num_n_words - 1] = 0;
  bits2int (u1, message_hash, hash_size, curve);
  mg_uecc_vli_modMult (u1, u1, z, curve->n, num_n_words); /* u1 = e/s */
  mg_uecc_vli_modMult (u2, r, z, curve->n, num_n_words);  /* u2 = r/s */

  /* Calculate sum = G + Q. */
  mg_uecc_vli_set (sum, _public, num_words);
  mg_uecc_vli_set (sum + num_words, _public + num_words, num_words);
  mg_uecc_vli_set (tx, curve->G, num_words);
  mg_uecc_vli_set (ty, curve->G + num_words, num_words);
  mg_uecc_vli_modSub (z, sum, tx, curve->p, num_words); /* z = x2 - x1 */
  XYcZ_add (tx, ty, sum, sum + num_words, curve);
  mg_uecc_vli_modInv (z, z, curve->p, num_words); /* z = 1/z */
  apply_z (sum, sum + num_words, z, curve);

  /* Use Shamir's trick to calculate u1*G + u2*Q */
  points[0] = 0;
  points[1] = curve->G;
  points[2] = _public;
  points[3] = sum;
  num_bits = smax (mg_uecc_vli_numBits (u1, num_n_words),
		   mg_uecc_vli_numBits (u2, num_n_words));
  point = points[(!!mg_uecc_vli_testBit (u1, (bitcount_t) (num_bits - 1)))
		 | ((!!mg_uecc_vli_testBit (u2, (bitcount_t) (num_bits - 1)))
		    << 1)];
  mg_uecc_vli_set (rx, point, num_words);
  mg_uecc_vli_set (ry, point + num_words, num_words);
  mg_uecc_vli_clear (z, num_words);
  z[0] = 1;

  for (i = num_bits - 2; i >= 0; --i)
    {
      mg_uecc_word_t index;
      curve->double_jacobian (rx, ry, z, curve);

      index = (!!mg_uecc_vli_testBit (u1, i))
	      | (mg_uecc_word_t) ((!!mg_uecc_vli_testBit (u2, i)) << 1);
      point = points[index];
      if (point)
	{
	  mg_uecc_vli_set (tx, point, num_words);
	  mg_uecc_vli_set (ty, point + num_words, num_words);
	  apply_z (tx, ty, z, curve);
	  mg_uecc_vli_modSub (tz, rx, tx, curve->p,
			      num_words); /* Z = x2 - x1 */
	  XYcZ_add (tx, ty, rx, ry, curve);
	  mg_uecc_vli_modMult_fast (z, z, tz, curve);
	}
    }

  mg_uecc_vli_modInv (z, z, curve->p, num_words); /* Z = 1/Z */
  apply_z (rx, ry, z, curve);

  /* v = x1 (mod n) */
  if (mg_uecc_vli_cmp_unsafe (curve->n, rx, num_n_words) != 1)
    {
      mg_uecc_vli_sub (rx, rx, curve->n, num_n_words);
    }

  /* Accept only if v == r. */
  return (int) (mg_uecc_vli_equal (rx, r, num_words));
}

#  if MG_UECC_ENABLE_VLI_API

unsigned
mg_uecc_curve_num_words (MG_UECC_Curve curve)
{
  return curve->num_words;
}

unsigned
mg_uecc_curve_num_bytes (MG_UECC_Curve curve)
{
  return curve->num_bytes;
}

unsigned
mg_uecc_curve_num_bits (MG_UECC_Curve curve)
{
  return curve->num_bytes * 8;
}

unsigned
mg_uecc_curve_num_n_words (MG_UECC_Curve curve)
{
  return BITS_TO_WORDS (curve->num_n_bits);
}

unsigned
mg_uecc_curve_num_n_bytes (MG_UECC_Curve curve)
{
  return BITS_TO_BYTES (curve->num_n_bits);
}

unsigned
mg_uecc_curve_num_n_bits (MG_UECC_Curve curve)
{
  return curve->num_n_bits;
}

const mg_uecc_word_t *
mg_uecc_curve_p (MG_UECC_Curve curve)
{
  return curve->p;
}

const mg_uecc_word_t *
mg_uecc_curve_n (MG_UECC_Curve curve)
{
  return curve->n;
}

const mg_uecc_word_t *
mg_uecc_curve_G (MG_UECC_Curve curve)
{
  return curve->G;
}

const mg_uecc_word_t *
mg_uecc_curve_b (MG_UECC_Curve curve)
{
  return curve->b;
}

#    if MG_UECC_SUPPORT_COMPRESSED_POINT
void
mg_uecc_vli_mod_sqrt (mg_uecc_word_t *a, MG_UECC_Curve curve)
{
  curve->mod_sqrt (a, curve);
}
#    endif

void
mg_uecc_vli_mmod_fast (mg_uecc_word_t *result, mg_uecc_word_t *product,
		       MG_UECC_Curve curve)
{
#    if (MG_UECC_OPTIMIZATION_LEVEL > 0)
  curve->mmod_fast (result, product);
#    else
  mg_uecc_vli_mmod (result, product, curve->p, curve->num_words);
#    endif
}

void
mg_uecc_point_mult (mg_uecc_word_t *result, const mg_uecc_word_t *point,
		    const mg_uecc_word_t *scalar, MG_UECC_Curve curve)
{
  mg_uecc_word_t tmp1[MG_UECC_MAX_WORDS];
  mg_uecc_word_t tmp2[MG_UECC_MAX_WORDS];
  mg_uecc_word_t *p2[2] = { tmp1, tmp2 };
  mg_uecc_word_t carry = regularize_k (scalar, tmp1, tmp2, curve);

  EccPoint_mult (result, point, p2[!carry], 0, curve->num_n_bits + 1, curve);
}

#  endif /* MG_UECC_ENABLE_VLI_API */
#endif	 // MG_TLS_BUILTIN
// End of uecc BSD-2

#ifdef MG_ENABLE_LINES
#  line 1 "src/tls_x25519.c"
#endif
/**
 * Adapted from STROBE: https://strobe.sourceforge.io/
 * Copyright (c) 2015-2016 Cryptography Research, Inc.
 * Author: Mike Hamburg
 * License: MIT License
 */

const uint8_t X25519_BASE_POINT[X25519_BYTES] = { 9 };

#define X25519_WBITS 32

typedef uint32_t limb_t;
typedef uint64_t dlimb_t;
typedef int64_t sdlimb_t;

#define NLIMBS (256 / X25519_WBITS)
typedef limb_t mg_fe[NLIMBS];

static limb_t
umaal (limb_t *carry, limb_t acc, limb_t mand, limb_t mier)
{
  dlimb_t tmp = (dlimb_t) mand * mier + acc + *carry;
  *carry = (limb_t) (tmp >> X25519_WBITS);
  return (limb_t) tmp;
}

// These functions are implemented in terms of umaal on ARM
static limb_t
adc (limb_t *carry, limb_t acc, limb_t mand)
{
  dlimb_t total = (dlimb_t) *carry + acc + mand;
  *carry = (limb_t) (total >> X25519_WBITS);
  return (limb_t) total;
}

static limb_t
adc0 (limb_t *carry, limb_t acc)
{
  dlimb_t total = (dlimb_t) *carry + acc;
  *carry = (limb_t) (total >> X25519_WBITS);
  return (limb_t) total;
}

// - Precondition: carry is small.
// - Invariant: result of propagate is < 2^255 + 1 word
// - In particular, always less than 2p.
// - Also, output x >= min(x,19)
static void
propagate (mg_fe x, limb_t over)
{
  unsigned i;
  limb_t carry;
  over = x[NLIMBS - 1] >> (X25519_WBITS - 1) | over << 1;
  x[NLIMBS - 1] &= ~((limb_t) 1 << (X25519_WBITS - 1));

  carry = over * 19;
  for (i = 0; i < NLIMBS; i++)
    {
      x[i] = adc0 (&carry, x[i]);
    }
}

static void
add (mg_fe out, const mg_fe a, const mg_fe b)
{
  unsigned i;
  limb_t carry = 0;
  for (i = 0; i < NLIMBS; i++)
    {
      out[i] = adc (&carry, a[i], b[i]);
    }
  propagate (out, carry);
}

static void
sub (mg_fe out, const mg_fe a, const mg_fe b)
{
  unsigned i;
  sdlimb_t carry = -38;
  for (i = 0; i < NLIMBS; i++)
    {
      carry = carry + a[i] - b[i];
      out[i] = (limb_t) carry;
      carry >>= X25519_WBITS;
    }
  propagate (out, (limb_t) (1 + carry));
}

// `b` can contain less than 8 limbs, thus we use `limb_t *` instead of `mg_fe`
// to avoid build warnings
static void
mul (mg_fe out, const mg_fe a, const limb_t *b, unsigned nb)
{
  limb_t accum[2 * NLIMBS] = { 0 };
  unsigned i, j;

  limb_t carry2;
  for (i = 0; i < nb; i++)
    {
      limb_t mand = b[i];
      carry2 = 0;
      for (j = 0; j < NLIMBS; j++)
	{
	  limb_t tmp;			      // "a" may be misaligned
	  memcpy (&tmp, &a[j], sizeof (tmp)); // So make an aligned copy
	  accum[i + j] = umaal (&carry2, accum[i + j], mand, tmp);
	}
      accum[i + j] = carry2;
    }

  carry2 = 0;
  for (j = 0; j < NLIMBS; j++)
    {
      out[j] = umaal (&carry2, accum[j], 38, accum[j + NLIMBS]);
    }
  propagate (out, carry2);
}

static void
sqr (mg_fe out, const mg_fe a)
{
  mul (out, a, a, NLIMBS);
}
static void
mul1 (mg_fe out, const mg_fe a)
{
  mul (out, a, out, NLIMBS);
}
static void
sqr1 (mg_fe a)
{
  mul1 (a, a);
}

static void
condswap (limb_t a[2 * NLIMBS], limb_t b[2 * NLIMBS], limb_t doswap)
{
  unsigned i;
  for (i = 0; i < 2 * NLIMBS; i++)
    {
      limb_t xor_ab = (a[i] ^ b[i]) & doswap;
      a[i] ^= xor_ab;
      b[i] ^= xor_ab;
    }
}

// Canonicalize a field element x, reducing it to the least residue which is
// congruent to it mod 2^255-19
// - Precondition: x < 2^255 + 1 word
static limb_t
canon (mg_fe x)
{
  // First, add 19.
  unsigned i;
  limb_t carry0 = 19;
  limb_t res;
  sdlimb_t carry;
  for (i = 0; i < NLIMBS; i++)
    {
      x[i] = adc0 (&carry0, x[i]);
    }
  propagate (x, carry0);

  // Here, 19 <= x2 < 2^255
  // - This is because we added 19, so before propagate it can't be less
  // than 19. After propagate, it still can't be less than 19, because if
  // propagate does anything it adds 19.
  // - We know that the high bit must be clear, because either the input was ~
  // 2^255 + one word + 19 (in which case it propagates to at most 2 words) or
  // it was < 2^255. So now, if we subtract 19, we will get back to something
  // in [0,2^255-19).
  carry = -19;
  res = 0;
  for (i = 0; i < NLIMBS; i++)
    {
      carry += x[i];
      res |= x[i] = (limb_t) carry;
      carry >>= X25519_WBITS;
    }
  return (limb_t) (((dlimb_t) res - 1) >> X25519_WBITS);
}

static const limb_t a24[1] = { 121665 };

static void
ladder_part1 (mg_fe xs[5])
{
  limb_t *x2 = xs[0], *z2 = xs[1], *x3 = xs[2], *z3 = xs[3], *t1 = xs[4];
  add (t1, x2, z2);				     // t1 = A
  sub (z2, x2, z2);				     // z2 = B
  add (x2, x3, z3);				     // x2 = C
  sub (z3, x3, z3);				     // z3 = D
  mul1 (z3, t1);				     // z3 = DA
  mul1 (x2, z2);				     // x3 = BC
  add (x3, z3, x2);				     // x3 = DA+CB
  sub (z3, z3, x2);				     // z3 = DA-CB
  sqr1 (t1);					     // t1 = AA
  sqr1 (z2);					     // z2 = BB
  sub (x2, t1, z2);				     // x2 = E = AA-BB
  mul (z2, x2, a24, sizeof (a24) / sizeof (a24[0])); // z2 = E*a24
  add (z2, z2, t1);				     // z2 = E*a24 + AA
}

static void
ladder_part2 (mg_fe xs[5], const mg_fe x1)
{
  limb_t *x2 = xs[0], *z2 = xs[1], *x3 = xs[2], *z3 = xs[3], *t1 = xs[4];
  sqr1 (z3);	    // z3 = (DA-CB)^2
  mul1 (z3, x1);    // z3 = x1 * (DA-CB)^2
  sqr1 (x3);	    // x3 = (DA+CB)^2
  mul1 (z2, x2);    // z2 = AA*(E*a24+AA)
  sub (x2, t1, x2); // x2 = BB again
  mul1 (x2, t1);    // x2 = AA*BB
}

static void
x25519_core (mg_fe xs[5], const uint8_t scalar[X25519_BYTES],
	     const uint8_t *x1, int clamp)
{
  int i;
  mg_fe x1_limbs;
  limb_t swap = 0;
  limb_t *x2 = xs[0], *x3 = xs[2], *z3 = xs[3];
  memset (xs, 0, 4 * sizeof (mg_fe));
  x2[0] = z3[0] = 1;
  for (i = 0; i < NLIMBS; i++)
    {
      x3[i] = x1_limbs[i]
	  = MG_U32 (x1[i * 4 + 3], x1[i * 4 + 2], x1[i * 4 + 1], x1[i * 4]);
    }

  for (i = 255; i >= 0; i--)
    {
      uint8_t bytei = scalar[i / 8];
      limb_t doswap;
      if (clamp)
	{
	  if (i / 8 == 0)
	    {
	      bytei &= (uint8_t) ~7U;
	    }
	  else if (i / 8 == X25519_BYTES - 1)
	    {
	      bytei &= 0x7F;
	      bytei |= 0x40;
	    }
	}
      doswap = 0 - (limb_t) ((bytei >> (i % 8)) & 1);
      condswap (x2, x3, swap ^ doswap);
      swap = doswap;

      ladder_part1 (xs);
      ladder_part2 (xs, (const limb_t *) x1_limbs);
    }
  condswap (x2, x3, swap);
}

int
mg_tls_x25519 (uint8_t out[X25519_BYTES], const uint8_t scalar[X25519_BYTES],
	       const uint8_t x1[X25519_BYTES], int clamp)
{
  int i, ret;
  mg_fe xs[5], out_limbs;
  limb_t *x2, *z2, *z3, *prev;
  static const struct
  {
    uint8_t a, c, n;
  } steps[13] = { { 2, 1, 1 },	{ 2, 1, 1 },   { 4, 2, 3 },  { 2, 4, 6 },
		  { 3, 1, 1 },	{ 3, 2, 12 },  { 4, 3, 25 }, { 2, 3, 25 },
		  { 2, 4, 50 }, { 3, 2, 125 }, { 3, 1, 2 },  { 3, 1, 2 },
		  { 3, 1, 1 } };
  x25519_core (xs, scalar, x1, clamp);

  // Precomputed inversion chain
  x2 = xs[0];
  z2 = xs[1];
  z3 = xs[3];

  prev = z2;
  for (i = 0; i < 13; i++)
    {
      int j;
      limb_t *a = xs[steps[i].a];
      for (j = steps[i].n; j > 0; j--)
	{
	  sqr (a, prev);
	  prev = a;
	}
      mul1 (a, xs[steps[i].c]);
    }

  // Here prev = z3
  // x2 /= z2
  mul (out_limbs, x2, z3, NLIMBS);
  ret = (int) canon (out_limbs);
  if (!clamp)
    ret = 0;
  for (i = 0; i < NLIMBS; i++)
    {
      uint32_t n = out_limbs[i];
      out[i * 4] = (uint8_t) (n & 0xff);
      out[i * 4 + 1] = (uint8_t) ((n >> 8) & 0xff);
      out[i * 4 + 2] = (uint8_t) ((n >> 16) & 0xff);
      out[i * 4 + 3] = (uint8_t) ((n >> 24) & 0xff);
    }
  return ret;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/url.c"
#endif

struct url
{
  size_t key, user, pass, host, port, uri, end;
};

int
mg_url_is_ssl (const char *url)
{
  return strncmp (url, "wss:", 4) == 0 || strncmp (url, "https:", 6) == 0
	 || strncmp (url, "mqtts:", 6) == 0 || strncmp (url, "ssl:", 4) == 0
	 || strncmp (url, "tls:", 4) == 0 || strncmp (url, "tcps:", 5) == 0;
}

static struct url
urlparse (const char *url)
{
  size_t i;
  struct url u;
  memset (&u, 0, sizeof (u));
  for (i = 0; url[i] != '\0'; i++)
    {
      if (url[i] == '/' && i > 0 && u.host == 0 && url[i - 1] == '/')
	{
	  u.host = i + 1;
	  u.port = 0;
	}
      else if (url[i] == ']')
	{
	  u.port = 0; // IPv6 URLs, like http://[::1]/bar
	}
      else if (url[i] == ':' && u.port == 0 && u.uri == 0)
	{
	  u.port = i + 1;
	}
      else if (url[i] == '@' && u.user == 0 && u.pass == 0 && u.uri == 0)
	{
	  u.user = u.host;
	  u.pass = u.port;
	  u.host = i + 1;
	  u.port = 0;
	}
      else if (url[i] == '/' && u.host && u.uri == 0)
	{
	  u.uri = i;
	}
    }
  u.end = i;
#if 0
  printf("[%s] %d %d %d %d %d\n", url, u.user, u.pass, u.host, u.port, u.uri);
#endif
  return u;
}

struct mg_str
mg_url_host (const char *url)
{
  struct url u = urlparse (url);
  size_t n = u.port  ? u.port - u.host - 1
	     : u.uri ? u.uri - u.host
		     : u.end - u.host;
  struct mg_str s = mg_str_n (url + u.host, n);
  return s;
}

const char *
mg_url_uri (const char *url)
{
  struct url u = urlparse (url);
  return u.uri ? url + u.uri : "/";
}

unsigned short
mg_url_port (const char *url)
{
  struct url u = urlparse (url);
  unsigned short port = 0;
  if (strncmp (url, "http:", 5) == 0 || strncmp (url, "ws:", 3) == 0)
    port = 80;
  if (strncmp (url, "wss:", 4) == 0 || strncmp (url, "https:", 6) == 0)
    port = 443;
  if (strncmp (url, "mqtt:", 5) == 0)
    port = 1883;
  if (strncmp (url, "mqtts:", 6) == 0)
    port = 8883;
  if (u.port)
    port = (unsigned short) atoi (url + u.port);
  return port;
}

struct mg_str
mg_url_user (const char *url)
{
  struct url u = urlparse (url);
  struct mg_str s = mg_str ("");
  if (u.user && (u.pass || u.host))
    {
      size_t n = u.pass ? u.pass - u.user - 1 : u.host - u.user - 1;
      s = mg_str_n (url + u.user, n);
    }
  return s;
}

struct mg_str
mg_url_pass (const char *url)
{
  struct url u = urlparse (url);
  struct mg_str s = mg_str_n ("", 0UL);
  if (u.pass && u.host)
    {
      size_t n = u.host - u.pass - 1;
      s = mg_str_n (url + u.pass, n);
    }
  return s;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/util.c"
#endif

// Not using memset for zeroing memory, cause it can be dropped by compiler
// See https://github.com/cesanta/mongoose/pull/1265
void
mg_bzero (volatile unsigned char *buf, size_t len)
{
  if (buf != NULL)
    {
      while (len--)
	*buf++ = 0;
    }
}

#if MG_ENABLE_CUSTOM_RANDOM
#else
void
mg_random (void *buf, size_t len)
{
  bool done = false;
  unsigned char *p = (unsigned char *) buf;
#  if MG_ARCH == MG_ARCH_ESP32
  while (len--)
    *p++ = (unsigned char) (esp_random () & 255);
  done = true;
#  elif MG_ARCH == MG_ARCH_WIN32
#  elif MG_ARCH == MG_ARCH_UNIX
  FILE *fp = fopen ("/dev/urandom", "rb");
  if (fp != NULL)
    {
      if (fread (buf, 1, len, fp) == len)
	done = true;
      fclose (fp);
    }
#  endif
  // If everything above did not work, fallback to a pseudo random generator
  while (!done && len--)
    *p++ = (unsigned char) (rand () & 255);
}
#endif

char *
mg_random_str (char *buf, size_t len)
{
  size_t i;
  mg_random (buf, len);
  for (i = 0; i < len; i++)
    {
      uint8_t c = ((uint8_t *) buf)[i] % 62U;
      buf[i] = i == len - 1 ? (char) '\0'	    // 0-terminate last byte
	       : c < 26	    ? (char) ('a' + c)	    // lowercase
	       : c < 52	    ? (char) ('A' + c - 26) // uppercase
			    : (char) ('0' + c - 52);    // numeric
    }
  return buf;
}

uint32_t
mg_ntohl (uint32_t net)
{
  uint8_t data[4] = { 0, 0, 0, 0 };
  memcpy (&data, &net, sizeof (data));
  return (((uint32_t) data[3]) << 0) | (((uint32_t) data[2]) << 8)
	 | (((uint32_t) data[1]) << 16) | (((uint32_t) data[0]) << 24);
}

uint16_t
mg_ntohs (uint16_t net)
{
  uint8_t data[2] = { 0, 0 };
  memcpy (&data, &net, sizeof (data));
  return (uint16_t) ((uint16_t) data[1] | (((uint16_t) data[0]) << 8));
}

uint32_t
mg_crc32 (uint32_t crc, const char *buf, size_t len)
{
  static const uint32_t crclut[16] = {
    // table for polynomial 0xEDB88320 (reflected)
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4,
    0x4DB26158, 0x5005713C, 0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
  };
  crc = ~crc;
  while (len--)
    {
      uint8_t b = *(uint8_t *) buf++;
      crc = crclut[(crc ^ b) & 0x0F] ^ (crc >> 4);
      crc = crclut[(crc ^ (b >> 4)) & 0x0F] ^ (crc >> 4);
    }
  return ~crc;
}

static int
isbyte (int n)
{
  return n >= 0 && n <= 255;
}

static int
parse_net (const char *spec, uint32_t *net, uint32_t *mask)
{
  int n, a, b, c, d, slash = 32, len = 0;
  if ((sscanf (spec, "%d.%d.%d.%d/%d%n", &a, &b, &c, &d, &slash, &n) == 5
       || sscanf (spec, "%d.%d.%d.%d%n", &a, &b, &c, &d, &n) == 4)
      && isbyte (a) && isbyte (b) && isbyte (c) && isbyte (d) && slash >= 0
      && slash < 33)
    {
      len = n;
      *net = ((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8)
	     | (uint32_t) d;
      *mask = slash ? (uint32_t) (0xffffffffU << (32 - slash)) : (uint32_t) 0;
    }
  return len;
}

int
mg_check_ip_acl (struct mg_str acl, struct mg_addr *remote_ip)
{
  struct mg_str entry;
  int allowed = acl.len == 0 ? '+' : '-'; // If any ACL is set, deny by default
  uint32_t remote_ip4;
  if (remote_ip->is_ip6)
    {
      return -1; // TODO(): handle IPv6 ACL and addresses
    }
  else
    { // IPv4
      memcpy ((void *) &remote_ip4, remote_ip->ip, sizeof (remote_ip4));
      while (mg_span (acl, &entry, &acl, ','))
	{
	  uint32_t net, mask;
	  if (entry.buf[0] != '+' && entry.buf[0] != '-')
	    return -1;
	  if (parse_net (&entry.buf[1], &net, &mask) == 0)
	    return -2;
	  if ((mg_ntohl (remote_ip4) & mask) == net)
	    allowed = entry.buf[0];
	}
    }
  return allowed == '+';
}

bool
mg_path_is_sane (const struct mg_str path)
{
  const char *s = path.buf;
  size_t n = path.len;
  if (path.buf[0] == '.' && path.buf[1] == '.')
    return false; // Starts with ..
  for (; s[0] != '\0' && n > 0; s++, n--)
    {
      if ((s[0] == '/' || s[0] == '\\') && n >= 2)
	{ // Subdir?
	  if (s[1] == '.' && s[2] == '.')
	    return false; // Starts with ..
	}
    }
  return true;
}

#if MG_ENABLE_CUSTOM_MILLIS
#else
uint64_t
mg_millis (void)
{
#  if MG_ARCH == MG_ARCH_WIN32
  return GetTickCount ();
#  elif MG_ARCH == MG_ARCH_RP2040
  return time_us_64 () / 1000;
#  elif MG_ARCH == MG_ARCH_ESP8266 || MG_ARCH == MG_ARCH_ESP32                \
      || MG_ARCH == MG_ARCH_FREERTOS
  return xTaskGetTickCount () * portTICK_PERIOD_MS;
#  elif MG_ARCH == MG_ARCH_AZURERTOS
  return tx_time_get () * (1000 /* MS per SEC */ / TX_TIMER_TICKS_PER_SECOND);
#  elif MG_ARCH == MG_ARCH_TIRTOS
  return (uint64_t) Clock_getTicks ();
#  elif MG_ARCH == MG_ARCH_ZEPHYR
  return (uint64_t) k_uptime_get ();
#  elif MG_ARCH == MG_ARCH_CMSIS_RTOS1
  return (uint64_t) rt_time_get ();
#  elif MG_ARCH == MG_ARCH_CMSIS_RTOS2
  return (uint64_t) ((osKernelGetTickCount () * 1000)
		     / osKernelGetTickFreq ());
#  elif MG_ARCH == MG_ARCH_RTTHREAD
  return (uint64_t) ((rt_tick_get () * 1000) / RT_TICK_PER_SECOND);
#  elif MG_ARCH == MG_ARCH_UNIX && defined(__APPLE__)
  // Apple CLOCK_MONOTONIC_RAW is equivalent to CLOCK_BOOTTIME on linux
  // Apple CLOCK_UPTIME_RAW is equivalent to CLOCK_MONOTONIC_RAW on linux
  return clock_gettime_nsec_np (CLOCK_UPTIME_RAW) / 1000000;
#  elif MG_ARCH == MG_ARCH_UNIX
  struct timespec ts = { 0, 0 };
  // See #1615 - prefer monotonic clock
#    if defined(CLOCK_MONOTONIC_RAW)
  // Raw hardware-based time that is not subject to NTP adjustment
  clock_gettime (CLOCK_MONOTONIC_RAW, &ts);
#    elif defined(CLOCK_MONOTONIC)
  // Affected by the incremental adjustments performed by adjtime and NTP
  clock_gettime (CLOCK_MONOTONIC, &ts);
#    else
  // Affected by discontinuous jumps in the system time and by the incremental
  // adjustments performed by adjtime and NTP
  clock_gettime (CLOCK_REALTIME, &ts);
#    endif
  return ((uint64_t) ts.tv_sec * 1000 + (uint64_t) ts.tv_nsec / 1000000);
#  elif defined(ARDUINO)
  return (uint64_t) millis ();
#  else
  return (uint64_t) (time (NULL) * 1000);
#  endif
}
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/ws.c"
#endif

struct ws_msg
{
  uint8_t flags;
  size_t header_len;
  size_t data_len;
};

size_t
mg_ws_vprintf (struct mg_connection *c, int op, const char *fmt, va_list *ap)
{
  size_t len = c->send.len;
  size_t n = mg_vxprintf (mg_pfn_iobuf, &c->send, fmt, ap);
  mg_ws_wrap (c, c->send.len - len, op);
  return n;
}

size_t
mg_ws_printf (struct mg_connection *c, int op, const char *fmt, ...)
{
  size_t len = 0;
  va_list ap;
  va_start (ap, fmt);
  len = mg_ws_vprintf (c, op, fmt, &ap);
  va_end (ap);
  return len;
}

static void
ws_handshake (struct mg_connection *c, const struct mg_str *wskey,
	      const struct mg_str *wsproto, const char *fmt, va_list *ap)
{
  const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  unsigned char sha[20], b64_sha[30];

  mg_sha1_ctx sha_ctx;
  mg_sha1_init (&sha_ctx);
  mg_sha1_update (&sha_ctx, (unsigned char *) wskey->buf, wskey->len);
  mg_sha1_update (&sha_ctx, (unsigned char *) magic, 36);
  mg_sha1_final (sha, &sha_ctx);
  mg_base64_encode (sha, sizeof (sha), (char *) b64_sha, sizeof (b64_sha));
  mg_xprintf (mg_pfn_iobuf, &c->send,
	      "HTTP/1.1 101 Switching Protocols\r\n"
	      "Upgrade: websocket\r\n"
	      "Connection: Upgrade\r\n"
	      "Sec-WebSocket-Accept: %s\r\n",
	      b64_sha);
  if (fmt != NULL)
    mg_vxprintf (mg_pfn_iobuf, &c->send, fmt, ap);
  if (wsproto != NULL)
    {
      mg_printf (c, "Sec-WebSocket-Protocol: %.*s\r\n", (int) wsproto->len,
		 wsproto->buf);
    }
  mg_send (c, "\r\n", 2);
}

static uint32_t
be32 (const uint8_t *p)
{
  return (((uint32_t) p[3]) << 0) | (((uint32_t) p[2]) << 8)
	 | (((uint32_t) p[1]) << 16) | (((uint32_t) p[0]) << 24);
}

static size_t
ws_process (uint8_t *buf, size_t len, struct ws_msg *msg)
{
  size_t i, n = 0, mask_len = 0;
  memset (msg, 0, sizeof (*msg));
  if (len >= 2)
    {
      n = buf[1] & 0x7f;	       // Frame length
      mask_len = buf[1] & 128 ? 4 : 0; // last bit is a mask bit
      msg->flags = buf[0];
      if (n < 126 && len >= mask_len)
	{
	  msg->data_len = n;
	  msg->header_len = 2 + mask_len;
	}
      else if (n == 126 && len >= 4 + mask_len)
	{
	  msg->header_len = 4 + mask_len;
	  msg->data_len = (((size_t) buf[2]) << 8) | buf[3];
	}
      else if (len >= 10 + mask_len)
	{
	  msg->header_len = 10 + mask_len;
	  msg->data_len
	      = (size_t) (((uint64_t) be32 (buf + 2) << 32) + be32 (buf + 6));
	}
    }
  // Sanity check, and integer overflow protection for the boundary check below
  // data_len should not be larger than 1 Gb
  if (msg->data_len > 1024 * 1024 * 1024)
    return 0;
  if (msg->header_len + msg->data_len > len)
    return 0;
  if (mask_len > 0)
    {
      uint8_t *p = buf + msg->header_len, *m = p - mask_len;
      for (i = 0; i < msg->data_len; i++)
	p[i] ^= m[i & 3];
    }
  return msg->header_len + msg->data_len;
}

static size_t
mkhdr (size_t len, int op, bool is_client, uint8_t *buf)
{
  size_t n = 0;
  buf[0] = (uint8_t) (op | 128);
  if (len < 126)
    {
      buf[1] = (unsigned char) len;
      n = 2;
    }
  else if (len < 65536)
    {
      uint16_t tmp = mg_htons ((uint16_t) len);
      buf[1] = 126;
      memcpy (&buf[2], &tmp, sizeof (tmp));
      n = 4;
    }
  else
    {
      uint32_t tmp;
      buf[1] = 127;
      tmp = mg_htonl ((uint32_t) (((uint64_t) len) >> 32));
      memcpy (&buf[2], &tmp, sizeof (tmp));
      tmp = mg_htonl ((uint32_t) (len & 0xffffffffU));
      memcpy (&buf[6], &tmp, sizeof (tmp));
      n = 10;
    }
  if (is_client)
    {
      buf[1] |= 1 << 7; // Set masking flag
      mg_random (&buf[n], 4);
      n += 4;
    }
  return n;
}

static void
mg_ws_mask (struct mg_connection *c, size_t len)
{
  if (c->is_client && c->send.buf != NULL)
    {
      size_t i;
      uint8_t *p = c->send.buf + c->send.len - len, *mask = p - 4;
      for (i = 0; i < len; i++)
	p[i] ^= mask[i & 3];
    }
}

size_t
mg_ws_send (struct mg_connection *c, const void *buf, size_t len, int op)
{
  uint8_t header[14];
  size_t header_len = mkhdr (len, op, c->is_client, header);
  mg_send (c, header, header_len);
  MG_VERBOSE (("WS out: %d [%.*s]", (int) len, (int) len, buf));
  mg_send (c, buf, len);
  mg_ws_mask (c, len);
  return header_len + len;
}

static bool
mg_ws_client_handshake (struct mg_connection *c)
{
  int n = mg_http_get_request_len (c->recv.buf, c->recv.len);
  if (n < 0)
    {
      mg_error (c, "not http"); // Some just, not an HTTP request
    }
  else if (n > 0)
    {
      if (n < 15 || memcmp (c->recv.buf + 9, "101", 3) != 0)
	{
	  mg_error (c, "ws handshake error");
	}
      else
	{
	  struct mg_http_message hm;
	  if (mg_http_parse ((char *) c->recv.buf, c->recv.len, &hm))
	    {
	      c->is_websocket = 1;
	      mg_call (c, MG_EV_WS_OPEN, &hm);
	    }
	  else
	    {
	      mg_error (c, "ws handshake error");
	    }
	}
      mg_iobuf_del (&c->recv, 0, (size_t) n);
    }
  else
    {
      return true; // Request is not yet received, quit event handler
    }
  return false; // Continue event handler
}

static void
mg_ws_cb (struct mg_connection *c, int ev, void *ev_data)
{
  struct ws_msg msg;
  size_t ofs = (size_t) c->pfn_data;

  // assert(ofs < c->recv.len);
  if (ev == MG_EV_READ)
    {
      if (c->is_client && !c->is_websocket && mg_ws_client_handshake (c))
	return;

      while (ws_process (c->recv.buf + ofs, c->recv.len - ofs, &msg) > 0)
	{
	  char *s = (char *) c->recv.buf + ofs + msg.header_len;
	  struct mg_ws_message m = { { s, msg.data_len }, msg.flags };
	  size_t len = msg.header_len + msg.data_len;
	  uint8_t final = msg.flags & 128, op = msg.flags & 15;
	  // MG_VERBOSE ("fin %d op %d len %d [%.*s]", final, op,
	  //                       (int) m.data.len, (int) m.data.len,
	  //                       m.data.buf));
	  switch (op)
	    {
	    case WEBSOCKET_OP_CONTINUE:
	      mg_call (c, MG_EV_WS_CTL, &m);
	      break;
	    case WEBSOCKET_OP_PING:
	      MG_DEBUG (("%s", "WS PONG"));
	      mg_ws_send (c, s, msg.data_len, WEBSOCKET_OP_PONG);
	      mg_call (c, MG_EV_WS_CTL, &m);
	      break;
	    case WEBSOCKET_OP_PONG:
	      mg_call (c, MG_EV_WS_CTL, &m);
	      break;
	    case WEBSOCKET_OP_TEXT:
	    case WEBSOCKET_OP_BINARY:
	      if (final)
		mg_call (c, MG_EV_WS_MSG, &m);
	      break;
	    case WEBSOCKET_OP_CLOSE:
	      MG_DEBUG (("%lu WS CLOSE", c->id));
	      mg_call (c, MG_EV_WS_CTL, &m);
	      // Echo the payload of the received CLOSE message back to the
	      // sender
	      mg_ws_send (c, m.data.buf, m.data.len, WEBSOCKET_OP_CLOSE);
	      c->is_draining = 1;
	      break;
	    default:
	      // Per RFC6455, close conn when an unknown op is recvd
	      mg_error (c, "unknown WS op %d", op);
	      break;
	    }

	  // Handle fragmented frames: strip header, keep in c->recv
	  if (final == 0 || op == 0)
	    {
	      if (op)
		ofs++, len--, msg.header_len--;		    // First frame
	      mg_iobuf_del (&c->recv, ofs, msg.header_len); // Strip header
	      len -= msg.header_len;
	      ofs += len;
	      c->pfn_data = (void *) ofs;
	      // MG_INFO(("FRAG %d [%.*s]", (int) ofs, (int) ofs,
	      // c->recv.buf));
	    }
	  // Remove non-fragmented frame
	  if (final && op)
	    mg_iobuf_del (&c->recv, ofs, len);
	  // Last chunk of the fragmented frame
	  if (final && !op)
	    {
	      m.flags = c->recv.buf[0];
	      m.data = mg_str_n ((char *) &c->recv.buf[1], (size_t) (ofs - 1));
	      mg_call (c, MG_EV_WS_MSG, &m);
	      mg_iobuf_del (&c->recv, 0, ofs);
	      ofs = 0;
	      c->pfn_data = NULL;
	    }
	}
    }
  (void) ev_data;
}

struct mg_connection *
mg_ws_connect (struct mg_mgr *mgr, const char *url, mg_event_handler_t fn,
	       void *fn_data, const char *fmt, ...)
{
  struct mg_connection *c = mg_connect (mgr, url, fn, fn_data);
  if (c != NULL)
    {
      char nonce[16], key[30];
      struct mg_str host = mg_url_host (url);
      mg_random (nonce, sizeof (nonce));
      mg_base64_encode ((unsigned char *) nonce, sizeof (nonce), key,
			sizeof (key));
      mg_xprintf (mg_pfn_iobuf, &c->send,
		  "GET %s HTTP/1.1\r\n"
		  "Upgrade: websocket\r\n"
		  "Host: %.*s\r\n"
		  "Connection: Upgrade\r\n"
		  "Sec-WebSocket-Version: 13\r\n"
		  "Sec-WebSocket-Key: %s\r\n",
		  mg_url_uri (url), (int) host.len, host.buf, key);
      if (fmt != NULL)
	{
	  va_list ap;
	  va_start (ap, fmt);
	  mg_vxprintf (mg_pfn_iobuf, &c->send, fmt, &ap);
	  va_end (ap);
	}
      mg_xprintf (mg_pfn_iobuf, &c->send, "\r\n");
      c->pfn = mg_ws_cb;
      c->pfn_data = NULL;
    }
  return c;
}

void
mg_ws_upgrade (struct mg_connection *c, struct mg_http_message *hm,
	       const char *fmt, ...)
{
  struct mg_str *wskey = mg_http_get_header (hm, "Sec-WebSocket-Key");
  c->pfn = mg_ws_cb;
  c->pfn_data = NULL;
  if (wskey == NULL)
    {
      mg_http_reply (c, 426, "", "WS upgrade expected\n");
      c->is_draining = 1;
    }
  else
    {
      struct mg_str *wsproto
	  = mg_http_get_header (hm, "Sec-WebSocket-Protocol");
      va_list ap;
      va_start (ap, fmt);
      ws_handshake (c, wskey, wsproto, fmt, &ap);
      va_end (ap);
      c->is_websocket = 1;
      c->is_resp = 0;
      mg_call (c, MG_EV_WS_OPEN, hm);
    }
}

size_t
mg_ws_wrap (struct mg_connection *c, size_t len, int op)
{
  uint8_t header[14], *p;
  size_t header_len = mkhdr (len, op, c->is_client, header);

  // NOTE: order of operations is important!
  mg_iobuf_add (&c->send, c->send.len, NULL, header_len);
  p = &c->send.buf[c->send.len - len];	       // p points to data
  memmove (p, p - header_len, len);	       // Shift data
  memcpy (p - header_len, header, header_len); // Prepend header
  mg_ws_mask (c, len);			       // Mask data

  return c->send.len;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/cmsis.c"
#endif
// https://arm-software.github.io/CMSIS_5/Driver/html/index.html

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_CMSIS)                        \
    && MG_ENABLE_DRIVER_CMSIS

extern ARM_DRIVER_ETH_MAC Driver_ETH_MAC0;
extern ARM_DRIVER_ETH_PHY Driver_ETH_PHY0;

static struct mg_tcpip_if *s_ifp;

static void mac_cb (uint32_t);
static bool cmsis_init (struct mg_tcpip_if *);
static bool cmsis_up (struct mg_tcpip_if *);
static size_t cmsis_tx (const void *, size_t, struct mg_tcpip_if *);
static size_t cmsis_rx (void *, size_t, struct mg_tcpip_if *);

struct mg_tcpip_driver mg_tcpip_driver_cmsis
    = { cmsis_init, cmsis_tx, NULL, cmsis_up };

static bool
cmsis_init (struct mg_tcpip_if *ifp)
{
  ARM_ETH_MAC_ADDR addr;
  s_ifp = ifp;

  ARM_DRIVER_ETH_MAC *mac = &Driver_ETH_MAC0;
  ARM_DRIVER_ETH_PHY *phy = &Driver_ETH_PHY0;
  ARM_ETH_MAC_CAPABILITIES cap = mac->GetCapabilities ();
  if (mac->Initialize (mac_cb) != ARM_DRIVER_OK)
    return false;
  if (phy->Initialize (mac->PHY_Read, mac->PHY_Write) != ARM_DRIVER_OK)
    return false;
  if (cap.event_rx_frame == 0) // polled mode driver
    mg_tcpip_driver_cmsis.rx = cmsis_rx;
  mac->PowerControl (ARM_POWER_FULL);
  if (cap.mac_address)
    { // driver provides MAC address
      mac->GetMacAddress (&addr);
      memcpy (ifp->mac, &addr, sizeof (ifp->mac));
    }
  else
    { // we provide MAC address
      memcpy (&addr, ifp->mac, sizeof (addr));
      mac->SetMacAddress (&addr);
    }
  phy->PowerControl (ARM_POWER_FULL);
  phy->SetInterface (cap.media_interface);
  phy->SetMode (ARM_ETH_PHY_AUTO_NEGOTIATE);
  return true;
}

static size_t
cmsis_tx (const void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  ARM_DRIVER_ETH_MAC *mac = &Driver_ETH_MAC0;
  if (mac->SendFrame (buf, (uint32_t) len, 0) != ARM_DRIVER_OK)
    {
      ifp->nerr++;
      return 0;
    }
  ifp->nsent++;
  return len;
}

static bool
cmsis_up (struct mg_tcpip_if *ifp)
{
  ARM_DRIVER_ETH_PHY *phy = &Driver_ETH_PHY0;
  ARM_DRIVER_ETH_MAC *mac = &Driver_ETH_MAC0;
  bool up = (phy->GetLinkState () == ARM_ETH_LINK_UP) ? 1 : 0; // link state
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // just went up
      ARM_ETH_LINK_INFO st = phy->GetLinkInfo ();
      mac->Control (ARM_ETH_MAC_CONFIGURE,
		    (st.speed << ARM_ETH_MAC_SPEED_Pos)
			| (st.duplex << ARM_ETH_MAC_DUPLEX_Pos)
			| ARM_ETH_MAC_ADDRESS_BROADCAST);
      MG_DEBUG (("Link is %uM %s-duplex",
		 (st.speed == 2) ? 1000
		 : st.speed	 ? 100
				 : 10,
		 st.duplex ? "full" : "half"));
      mac->Control (ARM_ETH_MAC_CONTROL_TX, 1);
      mac->Control (ARM_ETH_MAC_CONTROL_RX, 1);
    }
  else if ((ifp->state != MG_TCPIP_STATE_DOWN) && !up)
    { // just went down
      mac->Control (ARM_ETH_MAC_FLUSH,
		    ARM_ETH_MAC_FLUSH_TX | ARM_ETH_MAC_FLUSH_RX);
      mac->Control (ARM_ETH_MAC_CONTROL_TX, 0);
      mac->Control (ARM_ETH_MAC_CONTROL_RX, 0);
    }
  return up;
}

static void
mac_cb (uint32_t ev)
{
  if ((ev & ARM_ETH_MAC_EVENT_RX_FRAME) == 0)
    return;
  ARM_DRIVER_ETH_MAC *mac = &Driver_ETH_MAC0;
  uint32_t len = mac->GetRxFrameSize (); // CRC already stripped
  if (len >= 60 && len <= 1518)
    { // proper frame
      char *p;
      if (mg_queue_book (&s_ifp->recv_queue, &p, len) >= len)
	{ // have room
	  if ((len = mac->ReadFrame ((uint8_t *) p, len)) > 0)
	    { // copy succeeds
	      mg_queue_add (&s_ifp->recv_queue, len);
	      s_ifp->nrecv++;
	    }
	  return;
	}
      s_ifp->ndrop++;
    }
  mac->ReadFrame (NULL, 0); // otherwise, discard
}

static size_t
cmsis_rx (void *buf, size_t buflen, struct mg_tcpip_if *ifp)
{
  ARM_DRIVER_ETH_MAC *mac = &Driver_ETH_MAC0;
  uint32_t len = mac->GetRxFrameSize (); // CRC already stripped
  if (len >= 60 && len <= 1518
      && ((len = mac->ReadFrame (buf, (uint32_t) buflen)) > 0))
    return len;
  if (len > 0)
    mac->ReadFrame (NULL, 0); // discard bad frames
  (void) ifp;
  return 0;
}

#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/imxrt.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_IMXRT)                        \
    && MG_ENABLE_DRIVER_IMXRT
struct imxrt_enet
{
  volatile uint32_t RESERVED0, EIR, EIMR, RESERVED1, RDAR, TDAR, RESERVED2[3],
      ECR, RESERVED3[6], MMFR, MSCR, RESERVED4[7], MIBC, RESERVED5[7], RCR,
      RESERVED6[15], TCR, RESERVED7[7], PALR, PAUR, OPD, TXIC0, TXIC1, TXIC2,
      RESERVED8, RXIC0, RXIC1, RXIC2, RESERVED9[3], IAUR, IALR, GAUR, GALR,
      RESERVED10[7], TFWR, RESERVED11[14], RDSR, TDSR, MRBR[2], RSFL, RSEM,
      RAEM, RAFL, TSEM, TAEM, TAFL, TIPG, FTRL, RESERVED12[3], TACC, RACC,
      RESERVED13[15], RMON_T_PACKETS, RMON_T_BC_PKT, RMON_T_MC_PKT,
      RMON_T_CRC_ALIGN, RMON_T_UNDERSIZE, RMON_T_OVERSIZE, RMON_T_FRAG,
      RMON_T_JAB, RMON_T_COL, RMON_T_P64, RMON_T_P65TO127, RMON_T_P128TO255,
      RMON_T_P256TO511, RMON_T_P512TO1023, RMON_T_P1024TO2048, RMON_T_GTE2048,
      RMON_T_OCTETS, IEEE_T_DROP, IEEE_T_FRAME_OK, IEEE_T_1COL, IEEE_T_MCOL,
      IEEE_T_DEF, IEEE_T_LCOL, IEEE_T_EXCOL, IEEE_T_MACERR, IEEE_T_CSERR,
      IEEE_T_SQE, IEEE_T_FDXFC, IEEE_T_OCTETS_OK, RESERVED14[3],
      RMON_R_PACKETS, RMON_R_BC_PKT, RMON_R_MC_PKT, RMON_R_CRC_ALIGN,
      RMON_R_UNDERSIZE, RMON_R_OVERSIZE, RMON_R_FRAG, RMON_R_JAB, RESERVED15,
      RMON_R_P64, RMON_R_P65TO127, RMON_R_P128TO255, RMON_R_P256TO511,
      RMON_R_P512TO1023, RMON_R_P1024TO2047, RMON_R_GTE2048, RMON_R_OCTETS,
      IEEE_R_DROP, IEEE_R_FRAME_OK, IEEE_R_CRC, IEEE_R_ALIGN, IEEE_R_MACERR,
      IEEE_R_FDXFC, IEEE_R_OCTETS_OK, RESERVED16[71], ATCR, ATVR, ATOFF, ATPER,
      ATCOR, ATINC, ATSTMP, RESERVED17[122], TGSR, TCSR0, TCCR0, TCSR1, TCCR1,
      TCSR2, TCCR2, TCSR3;
};

#  undef ENET
#  if defined(MG_DRIVER_IMXRT_RT11) && MG_DRIVER_IMXRT_RT11
#    define ENET ((struct imxrt_enet *) (uintptr_t) 0x40424000U)
#    define ETH_DESC_CNT 5 // Descriptors count
#  else
#    define ENET ((struct imxrt_enet *) (uintptr_t) 0x402D8000U)
#    define ETH_DESC_CNT 4 // Descriptors count
#  endif

#  define ETH_PKT_SIZE 1536 // Max frame size, 64-bit aligned

struct enet_desc
{
  uint16_t length;  // Data length
  uint16_t control; // Control and status
  uint32_t *buffer; // Data ptr
};

// TODO(): handle these in a portable compiler-independent CMSIS-friendly way
#  define MG_64BYTE_ALIGNED __attribute__ ((aligned ((64U))))

// Descriptors: in non-cached area (TODO(scaprile)), (37.5.1.22.2 37.5.1.23.2)
// Buffers: 64-byte aligned (37.3.14)
static volatile struct enet_desc s_rxdesc[ETH_DESC_CNT] MG_64BYTE_ALIGNED;
static volatile struct enet_desc s_txdesc[ETH_DESC_CNT] MG_64BYTE_ALIGNED;
static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE] MG_64BYTE_ALIGNED;
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE] MG_64BYTE_ALIGNED;
static struct mg_tcpip_if *s_ifp; // MIP interface

static uint16_t
enet_read_phy (uint8_t addr, uint8_t reg)
{
  ENET->EIR |= MG_BIT (23); // MII interrupt clear
  ENET->MMFR = (1 << 30) | (2 << 28) | (addr << 23) | (reg << 18) | (2 << 16);
  while ((ENET->EIR & MG_BIT (23)) == 0)
    (void) 0;
  return ENET->MMFR & 0xffff;
}

static void
enet_write_phy (uint8_t addr, uint8_t reg, uint16_t val)
{
  ENET->EIR |= MG_BIT (23); // MII interrupt clear
  ENET->MMFR
      = (1 << 30) | (1 << 28) | (addr << 23) | (reg << 18) | (2 << 16) | val;
  while ((ENET->EIR & MG_BIT (23)) == 0)
    (void) 0;
}

//  MDC clock is generated from IPS Bus clock (ipg_clk); as per 802.3,
//  it must not exceed 2.5MHz
// The PHY receives the PLL6-generated 50MHz clock
static bool
mg_tcpip_driver_imxrt_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_imxrt_data *d
      = (struct mg_tcpip_driver_imxrt_data *) ifp->driver_data;
  s_ifp = ifp;

  // Init RX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i].control = MG_BIT (15);	    // Own (E)
      s_rxdesc[i].buffer = (uint32_t *) s_rxbuf[i]; // Point to data buffer
    }
  s_rxdesc[ETH_DESC_CNT - 1].control |= MG_BIT (13); // Wrap last descriptor

  // Init TX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      // s_txdesc[i].control = MG_BIT(10);  // Own (TC)
      s_txdesc[i].buffer = (uint32_t *) s_txbuf[i];
    }
  s_txdesc[ETH_DESC_CNT - 1].control |= MG_BIT (13); // Wrap last descriptor

  ENET->ECR = MG_BIT (0); // Software reset, disable
  while ((ENET->ECR & MG_BIT (0)))
    (void) 0; // Wait until done

  // Set MDC clock divider. If user told us the value, use it.
  // TODO(): Otherwise, guess (currently assuming max freq)
  int cr = (d == NULL || d->mdc_cr < 0) ? 24 : d->mdc_cr;
  ENET->MSCR = (1 << 8) | ((cr & 0x3f) << 1); // HOLDTIME 2 clks
  struct mg_phy phy = { enet_read_phy, enet_write_phy };
  mg_phy_init (&phy, d->phy_addr, MG_PHY_LEDS_ACTIVE_HIGH); // MAC clocks PHY
  // Select RMII mode, 100M, keep CRC, set max rx length, disable loop
  ENET->RCR = (1518 << 16) | MG_BIT (8) | MG_BIT (2);
  // ENET->RCR |= MG_BIT(3);     // Receive all
  ENET->TCR = MG_BIT (2); // Full-duplex
  ENET->RDSR = (uint32_t) (uintptr_t) s_rxdesc;
  ENET->TDSR = (uint32_t) (uintptr_t) s_txdesc;
  ENET->MRBR[0] = ETH_PKT_SIZE; // Same size for RX/TX buffers
  // MAC address filtering (bytes in reversed order)
  ENET->PAUR = ((uint32_t) ifp->mac[4] << 24U) | (uint32_t) ifp->mac[5] << 16U;
  ENET->PALR = (uint32_t) (ifp->mac[0] << 24U)
	       | ((uint32_t) ifp->mac[1] << 16U)
	       | ((uint32_t) ifp->mac[2] << 8U) | ifp->mac[3];
  ENET->ECR = MG_BIT (8) | MG_BIT (1); // Little-endian CPU, Enable
  ENET->EIMR = MG_BIT (25);	       // Set interrupt mask
  ENET->RDAR = MG_BIT (24);	       // Receive Descriptors have changed
  ENET->TDAR = MG_BIT (24);	       // Transmit Descriptors have changed
  // ENET->OPD = 0x10014;
  return true;
}

// Transmit frame
static size_t
mg_tcpip_driver_imxrt_tx (const void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  static int s_txno; // Current descriptor index
  if (len > sizeof (s_txbuf[ETH_DESC_CNT]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = (size_t) -1; // fail
    }
  else if ((s_txdesc[s_txno].control & MG_BIT (15)))
    {
      ifp->nerr++;
      MG_ERROR (("No descriptors available"));
      len = 0; // retry later
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);	// Copy data
      s_txdesc[s_txno].length = (uint16_t) len; // Set data len
      // Table 37-34, R, L, TC (Ready, last, transmit CRC after frame
      s_txdesc[s_txno].control
	  |= (uint16_t) (MG_BIT (15) | MG_BIT (11) | MG_BIT (10));
      ENET->TDAR = MG_BIT (24); // Descriptor ring updated
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }
  (void) ifp;
  return len;
}

static bool
mg_tcpip_driver_imxrt_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_imxrt_data *d
      = (struct mg_tcpip_driver_imxrt_data *) ifp->driver_data;
  uint8_t speed = MG_PHY_SPEED_10M;
  bool up = false, full_duplex = false;
  struct mg_phy phy = { enet_read_phy, enet_write_phy };
  up = mg_phy_up (&phy, d->phy_addr, &full_duplex, &speed);
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      // tmp = reg with flags set to the most likely situation: 100M
      // full-duplex if(link is slow or half) set flags otherwise reg = tmp
      uint32_t tcr = ENET->TCR | MG_BIT (2);  // Full-duplex
      uint32_t rcr = ENET->RCR & ~MG_BIT (9); // 100M
      if (speed == MG_PHY_SPEED_10M)
	rcr |= MG_BIT (9); // 10M
      if (full_duplex == false)
	tcr &= ~MG_BIT (2); // Half-duplex
      ENET->TCR = tcr;	    // IRQ handler does not fiddle with these registers
      ENET->RCR = rcr;
      MG_DEBUG (("Link is %uM %s-duplex", rcr & MG_BIT (9) ? 10 : 100,
		 tcr & MG_BIT (2) ? "full" : "half"));
    }
  return up;
}

void ENET_IRQHandler (void);
static uint32_t s_rxno;
void
ENET_IRQHandler (void)
{
  ENET->EIR = MG_BIT (25); // Ack IRQ
  // Frame received, loop
  for (uint32_t i = 0; i < 10; i++)
    { // read as they arrive but not forever
      uint32_t r = s_rxdesc[s_rxno].control;
      if (r & MG_BIT (15))
	break; // exit when done
      // skip partial/errored frames (Table 37-32)
      if ((r & MG_BIT (11))
	  && !(r
	       & (MG_BIT (5) | MG_BIT (4) | MG_BIT (2) | MG_BIT (1)
		  | MG_BIT (0))))
	{
	  size_t len = s_rxdesc[s_rxno].length;
	  mg_tcpip_qwrite (s_rxbuf[s_rxno], len > 4 ? len - 4 : len, s_ifp);
	}
      s_rxdesc[s_rxno].control |= MG_BIT (15);
      if (++s_rxno >= ETH_DESC_CNT)
	s_rxno = 0;
    }
  ENET->RDAR = MG_BIT (24); // Receive Descriptors have changed
  // If b24 == 0, descriptors were exhausted and probably frames were dropped
}

struct mg_tcpip_driver mg_tcpip_driver_imxrt
    = { mg_tcpip_driver_imxrt_init, mg_tcpip_driver_imxrt_tx, NULL,
	mg_tcpip_driver_imxrt_up };

#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/phy.c"
#endif

enum
{			   // ID1  ID2
  MG_PHY_KSZ8x = 0x22,	   // 0022 1561 - KSZ8081RNB
  MG_PHY_DP83x = 0x2000,   // 2000 a140 - TI DP83825I
  MG_PHY_DP83867 = 0xa231, // 2000 a231 - TI DP83867I
  MG_PHY_LAN87x = 0x7,	   // 0007 c0fx - LAN8720
  MG_PHY_RTL8201 = 0x1C	   // 001c c816 - RTL8201
};

enum
{
  MG_PHY_REG_BCR = 0,
  MG_PHY_REG_BSR = 1,
  MG_PHY_REG_ID1 = 2,
  MG_PHY_REG_ID2 = 3,
  MG_PHY_DP83x_REG_PHYSTS = 16,
  MG_PHY_DP83867_REG_PHYSTS = 17,
  MG_PHY_DP83x_REG_RCSR = 23,
  MG_PHY_DP83x_REG_LEDCR = 24,
  MG_PHY_KSZ8x_REG_PC1R = 30,
  MG_PHY_KSZ8x_REG_PC2R = 31,
  MG_PHY_LAN87x_REG_SCSR = 31,
  MG_PHY_RTL8201_REG_RMSR = 16, // in page 7
  MG_PHY_RTL8201_REG_PAGESEL = 31
};

static const char *
mg_phy_id_to_str (uint16_t id1, uint16_t id2)
{
  switch (id1)
    {
    case MG_PHY_DP83x:
      switch (id2)
	{
	case MG_PHY_DP83867:
	  return "DP83867";
	default:
	  return "DP83x";
	}
    case MG_PHY_KSZ8x:
      return "KSZ8x";
    case MG_PHY_LAN87x:
      return "LAN87x";
    case MG_PHY_RTL8201:
      return "RTL8201";
    default:
      return "unknown";
    }
  (void) id2;
}

void
mg_phy_init (struct mg_phy *phy, uint8_t phy_addr, uint8_t config)
{
  uint16_t id1, id2;
  phy->write_reg (phy_addr, MG_PHY_REG_BCR, MG_BIT (15)); // Reset PHY
  while (phy->read_reg (phy_addr, MG_PHY_REG_BCR) & MG_BIT (15))
    (void) 0;
  // MG_PHY_REG_BCR[12]: Autonegotiation is default unless hw says otherwise

  id1 = phy->read_reg (phy_addr, MG_PHY_REG_ID1);
  id2 = phy->read_reg (phy_addr, MG_PHY_REG_ID2);
  MG_INFO (
      ("PHY ID: %#04x %#04x (%s)", id1, id2, mg_phy_id_to_str (id1, id2)));

  if (id1 == MG_PHY_DP83x && id2 == MG_PHY_DP83867)
    {
      phy->write_reg (phy_addr, 0x0d,
		      0x1f); // write 0x10d to IO_MUX_CFG (0x0170)
      phy->write_reg (phy_addr, 0x0e, 0x170);
      phy->write_reg (phy_addr, 0x0d, 0x401f);
      phy->write_reg (phy_addr, 0x0e, 0x10d);
    }

  if (config & MG_PHY_CLOCKS_MAC)
    {
      // Use PHY crystal oscillator (preserve defaults)
      // nothing to do
    }
  else
    { // MAC clocks PHY, PHY has no xtal
      // Enable 50 MHz external ref clock at XI (preserve defaults)
      if (id1 == MG_PHY_DP83x && id2 != MG_PHY_DP83867)
	{
	  phy->write_reg (phy_addr, MG_PHY_DP83x_REG_RCSR,
			  MG_BIT (7) | MG_BIT (0));
	}
      else if (id1 == MG_PHY_KSZ8x)
	{
	  phy->write_reg (phy_addr, MG_PHY_KSZ8x_REG_PC2R,
			  MG_BIT (15) | MG_BIT (8) | MG_BIT (7));
	}
      else if (id1 == MG_PHY_LAN87x)
	{
	  // nothing to do
	}
      else if (id1 == MG_PHY_RTL8201)
	{
	  // assume PHY has been hardware strapped properly
#if 0
      phy->write_reg(phy_addr, MG_PHY_RTL8201_REG_PAGESEL, 7);  // Select page 7
      phy->write_reg(phy_addr, MG_PHY_RTL8201_REG_RMSR, 0x1ffa);
      phy->write_reg(phy_addr, MG_PHY_RTL8201_REG_PAGESEL, 0);  // Select page 0
#endif
	}
    }

  if (config & MG_PHY_LEDS_ACTIVE_HIGH && id1 == MG_PHY_DP83x)
    {
      phy->write_reg (phy_addr, MG_PHY_DP83x_REG_LEDCR,
		      MG_BIT (9) | MG_BIT (7)); // LED status, active high
    } // Other PHYs do not support this feature
}

bool
mg_phy_up (struct mg_phy *phy, uint8_t phy_addr, bool *full_duplex,
	   uint8_t *speed)
{
  bool up = false;
  uint16_t bsr = phy->read_reg (phy_addr, MG_PHY_REG_BSR);
  if ((bsr & MG_BIT (5)) && !(bsr & MG_BIT (2))) // some PHYs latch down events
    bsr = phy->read_reg (phy_addr, MG_PHY_REG_BSR); // read again
  up = bsr & MG_BIT (2);
  if (up && full_duplex != NULL && speed != NULL)
    {
      uint16_t id1 = phy->read_reg (phy_addr, MG_PHY_REG_ID1);
      if (id1 == MG_PHY_DP83x)
	{
	  uint16_t id2 = phy->read_reg (phy_addr, MG_PHY_REG_ID2);
	  if (id2 == MG_PHY_DP83867)
	    {
	      uint16_t physts
		  = phy->read_reg (phy_addr, MG_PHY_DP83867_REG_PHYSTS);
	      *full_duplex = physts & MG_BIT (13);
	      *speed = (physts & MG_BIT (15))	? MG_PHY_SPEED_1000M
		       : (physts & MG_BIT (14)) ? MG_PHY_SPEED_100M
						: MG_PHY_SPEED_10M;
	    }
	  else
	    {
	      uint16_t physts
		  = phy->read_reg (phy_addr, MG_PHY_DP83x_REG_PHYSTS);
	      *full_duplex = physts & MG_BIT (2);
	      *speed = (physts & MG_BIT (1)) ? MG_PHY_SPEED_10M
					     : MG_PHY_SPEED_100M;
	    }
	}
      else if (id1 == MG_PHY_KSZ8x)
	{
	  uint16_t pc1r = phy->read_reg (phy_addr, MG_PHY_KSZ8x_REG_PC1R);
	  *full_duplex = pc1r & MG_BIT (2);
	  *speed = (pc1r & 3) == 1 ? MG_PHY_SPEED_10M : MG_PHY_SPEED_100M;
	}
      else if (id1 == MG_PHY_LAN87x)
	{
	  uint16_t scsr = phy->read_reg (phy_addr, MG_PHY_LAN87x_REG_SCSR);
	  *full_duplex = scsr & MG_BIT (4);
	  *speed = (scsr & MG_BIT (3)) ? MG_PHY_SPEED_100M : MG_PHY_SPEED_10M;
	}
      else if (id1 == MG_PHY_RTL8201)
	{
	  uint16_t bcr = phy->read_reg (phy_addr, MG_PHY_REG_BCR);
	  *full_duplex = bcr & MG_BIT (8);
	  *speed = (bcr & MG_BIT (13)) ? MG_PHY_SPEED_100M : MG_PHY_SPEED_10M;
	}
    }
  return up;
}

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/ra.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_RA) && MG_ENABLE_DRIVER_RA
struct ra_etherc
{
  volatile uint32_t ECMR, RESERVED, RFLR, RESERVED1, ECSR, RESERVED2, ECSIPR,
      RESERVED3, PIR, RESERVED4, PSR, RESERVED5[5], RDMLR, RESERVED6[3], IPGR,
      APR, MPR, RESERVED7, RFCF, TPAUSER, TPAUSECR, BCFRR, RESERVED8[20], MAHR,
      RESERVED9, MALR, RESERVED10, TROCR, CDCR, LCCR, CNDCR, RESERVED11, CEFCR,
      FRECR, TSFRCR, TLFRCR, RFCR, MAFCR;
};

struct ra_edmac
{
  volatile uint32_t EDMR, RESERVED, EDTRR, RESERVED1, EDRRR, RESERVED2, TDLAR,
      RESERVED3, RDLAR, RESERVED4, EESR, RESERVED5, EESIPR, RESERVED6, TRSCER,
      RESERVED7, RMFCR, RESERVED8, TFTR, RESERVED9, FDR, RESERVED10, RMCR,
      RESERVED11[2], TFUCR, RFOCR, IOSR, FCFTR, RESERVED12, RPADIR, TRIMD,
      RESERVED13[18], RBWAR, RDFAR, RESERVED14, TBRAR, TDFAR;
};

#  undef ETHERC
#  define ETHERC ((struct ra_etherc *) (uintptr_t) 0x40114100U)
#  undef EDMAC
#  define EDMAC ((struct ra_edmac *) (uintptr_t) 0x40114000U)
#  undef RASYSC
#  define RASYSC ((uint32_t *) (uintptr_t) 0x4001E000U)
#  undef ICU_IELSR
#  define ICU_IELSR ((uint32_t *) (uintptr_t) 0x40006300U)

#  define ETH_PKT_SIZE 1536 // Max frame size, multiple of 32
#  define ETH_DESC_CNT 4    // Descriptors count

// TODO(): handle these in a portable compiler-independent CMSIS-friendly way
#  define MG_16BYTE_ALIGNED __attribute__ ((aligned ((16U))))
#  define MG_32BYTE_ALIGNED __attribute__ ((aligned ((32U))))

// Descriptors: 16-byte aligned
// Buffers: 32-byte aligned (27.3.1)
static volatile uint32_t s_rxdesc[ETH_DESC_CNT][4] MG_16BYTE_ALIGNED;
static volatile uint32_t s_txdesc[ETH_DESC_CNT][4] MG_16BYTE_ALIGNED;
static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE] MG_32BYTE_ALIGNED;
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE] MG_32BYTE_ALIGNED;
static struct mg_tcpip_if *s_ifp; // MIP interface

// fastest is 3 cycles (SUB + BNE) on a 3-stage pipeline or equivalent
static inline void
raspin (volatile uint32_t count)
{
  while (count--)
    (void) 0;
}
// count to get the 200ns SMC semi-cycle period (2.5MHz) calling raspin():
// SYS_FREQUENCY * 200ns / 3 = SYS_FREQUENCY / 15000000
static uint32_t s_smispin;

// Bit-banged SMI
static void
smi_preamble (void)
{
  unsigned int i = 32;
  uint32_t pir = MG_BIT (1) | MG_BIT (2); // write, mdio = 1, mdc = 0
  ETHERC->PIR = pir;
  while (i--)
    {
      pir &= ~MG_BIT (0); // mdc = 0
      ETHERC->PIR = pir;
      raspin (s_smispin);
      pir |= MG_BIT (0); // mdc = 1
      ETHERC->PIR = pir;
      raspin (s_smispin);
    }
}
static void
smi_wr (uint16_t header, uint16_t data)
{
  uint32_t word = (header << 16) | data;
  smi_preamble ();
  unsigned int i = 32;
  while (i--)
    {
      uint32_t pir
	  = MG_BIT (1)
	    | (word & 0x80000000 ? MG_BIT (2) : 0); // write, mdc = 0, data
      ETHERC->PIR = pir;
      raspin (s_smispin);
      pir |= MG_BIT (0); // mdc = 1
      ETHERC->PIR = pir;
      raspin (s_smispin);
      word <<= 1;
    }
}
static uint16_t
smi_rd (uint16_t header)
{
  smi_preamble ();
  unsigned int i = 16; // 2 LSb as turnaround
  uint32_t pir;
  while (i--)
    {
      pir = (i > 1 ? MG_BIT (1) : 0)
	    | (header & 0x8000
		   ? MG_BIT (2)
		   : 0); // mdc = 0, header, set read direction at turnaround
      ETHERC->PIR = pir;
      raspin (s_smispin);
      pir |= MG_BIT (0); // mdc = 1
      ETHERC->PIR = pir;
      raspin (s_smispin);
      header <<= 1;
    }
  i = 16;
  uint16_t data = 0;
  while (i--)
    {
      data <<= 1;
      pir = 0; // read, mdc = 0
      ETHERC->PIR = pir;
      raspin (s_smispin / 2); // 1/4 clock period, 300ns max access time
      data |= (uint16_t) (ETHERC->PIR & MG_BIT (3) ? 1 : 0); // read mdio
      raspin (s_smispin / 2); // 1/4 clock period
      pir |= MG_BIT (0);      // mdc = 1
      ETHERC->PIR = pir;
      raspin (s_smispin);
    }
  return data;
}

static uint16_t
raeth_read_phy (uint8_t addr, uint8_t reg)
{
  return smi_rd ((uint16_t) ((1 << 14) | (2 << 12) | (addr << 7) | (reg << 2)
			     | (2 << 0)));
}

static void
raeth_write_phy (uint8_t addr, uint8_t reg, uint16_t val)
{
  smi_wr (
      (uint16_t) ((1 << 14) | (1 << 12) | (addr << 7) | (reg << 2) | (2 << 0)),
      val);
}

// MDC clock is generated manually; as per 802.3, it must not exceed 2.5MHz
static bool
mg_tcpip_driver_ra_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_ra_data *d
      = (struct mg_tcpip_driver_ra_data *) ifp->driver_data;
  s_ifp = ifp;

  // Init SMI clock timing. If user told us the clock value, use it.
  // TODO(): Otherwise, guess
  s_smispin = d->clock / 15000000;

  // Init RX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i][0] = MG_BIT (31);	      // RACT
      s_rxdesc[i][1] = ETH_PKT_SIZE << 16;    // RBL
      s_rxdesc[i][2] = (uint32_t) s_rxbuf[i]; // Point to data buffer
    }
  s_rxdesc[ETH_DESC_CNT - 1][0] |= MG_BIT (30); // Wrap last descriptor

  // Init TX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      // TACT = 0
      s_txdesc[i][2] = (uint32_t) s_txbuf[i];
    }
  s_txdesc[ETH_DESC_CNT - 1][0] |= MG_BIT (30); // Wrap last descriptor

  EDMAC->EDMR = MG_BIT (0); // Software reset, wait 64 PCLKA clocks (27.2.1)
  uint32_t sckdivcr = RASYSC[8]; // get divisors from SCKDIVCR (8.2.2)
  uint32_t ick = 1 << ((sckdivcr >> 24) & 7);  // sys_clock div
  uint32_t pcka = 1 << ((sckdivcr >> 12) & 7); // pclka div
  raspin ((64U * pcka) / (3U * ick));
  EDMAC->EDMR = MG_BIT (6); // Initialize, little-endian (27.2.1)

  MG_DEBUG (("PHY addr: %d, smispin: %d", d->phy_addr, s_smispin));
  struct mg_phy phy = { raeth_read_phy, raeth_write_phy };
  mg_phy_init (&phy, d->phy_addr, 0); // MAC clocks PHY

  // Select RMII mode,
  ETHERC->ECMR = MG_BIT (2) | MG_BIT (1); // 100M, Full-duplex, CRC
  // ETHERC->ECMR |= MG_BIT(0);             // Receive all
  ETHERC->RFLR = 1518; // Set max rx length

  EDMAC->RDLAR = (uint32_t) (uintptr_t) s_rxdesc;
  EDMAC->TDLAR = (uint32_t) (uintptr_t) s_txdesc;
  // MAC address filtering (bytes in reversed order)
  ETHERC->MAHR = (uint32_t) (ifp->mac[0] << 24U)
		 | ((uint32_t) ifp->mac[1] << 16U)
		 | ((uint32_t) ifp->mac[2] << 8U) | ifp->mac[3];
  ETHERC->MALR = ((uint32_t) ifp->mac[4] << 8U) | ifp->mac[5];

  EDMAC->TFTR = 0;			   // Store and forward (27.2.10)
  EDMAC->FDR = 0x070f;			   // (27.2.11)
  EDMAC->RMCR = MG_BIT (0);		   // (27.2.12)
  ETHERC->ECMR |= MG_BIT (6) | MG_BIT (5); // TE RE
  EDMAC->EESIPR = MG_BIT (18);		   // Enable Rx IRQ
  EDMAC->EDRRR = MG_BIT (0);		   // Receive Descriptors have changed
  EDMAC->EDTRR = MG_BIT (0);		   // Transmit Descriptors have changed
  return true;
}

// Transmit frame
static size_t
mg_tcpip_driver_ra_tx (const void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  static int s_txno; // Current descriptor index
  if (len > sizeof (s_txbuf[ETH_DESC_CNT]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = (size_t) -1; // fail
    }
  else if ((s_txdesc[s_txno][0] & MG_BIT (31)))
    {
      ifp->nerr++;
      MG_ERROR (("No descriptors available"));
      len = 0; // retry later
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);	    // Copy data
      s_txdesc[s_txno][1] = len << 16;		    // Set data len
      s_txdesc[s_txno][0] |= MG_BIT (31) | 3 << 28; // (27.3.1.1) mark valid
      EDMAC->EDTRR = MG_BIT (0);		    // Transmit request
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }
  return len;
}

static bool
mg_tcpip_driver_ra_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_ra_data *d
      = (struct mg_tcpip_driver_ra_data *) ifp->driver_data;
  uint8_t speed = MG_PHY_SPEED_10M;
  bool up = false, full_duplex = false;
  struct mg_phy phy = { raeth_read_phy, raeth_write_phy };
  up = mg_phy_up (&phy, d->phy_addr, &full_duplex, &speed);
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      // tmp = reg with flags set to the most likely situation: 100M
      // full-duplex if(link is slow or half) set flags otherwise reg = tmp
      uint32_t ecmr
	  = ETHERC->ECMR | MG_BIT (2) | MG_BIT (1); // 100M Full-duplex
      if (speed == MG_PHY_SPEED_10M)
	ecmr &= ~MG_BIT (2); // 10M
      if (full_duplex == false)
	ecmr &= ~MG_BIT (1); // Half-duplex
      ETHERC->ECMR = ecmr; // IRQ handler does not fiddle with these registers
      MG_DEBUG (("Link is %uM %s-duplex", ecmr & MG_BIT (2) ? 100 : 10,
		 ecmr & MG_BIT (1) ? "full" : "half"));
    }
  return up;
}

void EDMAC_IRQHandler (void);
static uint32_t s_rxno;
void
EDMAC_IRQHandler (void)
{
  struct mg_tcpip_driver_ra_data *d
      = (struct mg_tcpip_driver_ra_data *) s_ifp->driver_data;
  EDMAC->EESR = MG_BIT (18);	       // Ack IRQ in EDMAC 1st
  ICU_IELSR[d->irqno] &= ~MG_BIT (16); // Ack IRQ in ICU last
  // Frame received, loop
  for (uint32_t i = 0; i < 10; i++)
    { // read as they arrive but not forever
      uint32_t r = s_rxdesc[s_rxno][0];
      if (r & MG_BIT (31))
	break; // exit when done
      // skip partial/errored frames (27.3.1.2)
      if ((r & (MG_BIT (29) | MG_BIT (28)) && !(r & MG_BIT (27))))
	{
	  size_t len = s_rxdesc[s_rxno][1] & 0xffff;
	  mg_tcpip_qwrite (s_rxbuf[s_rxno], len,
			   s_ifp); // CRC already stripped
	}
      s_rxdesc[s_rxno][0] |= MG_BIT (31);
      if (++s_rxno >= ETH_DESC_CNT)
	s_rxno = 0;
    }
  EDMAC->EDRRR = MG_BIT (0); // Receive Descriptors have changed
  // If b0 == 0, descriptors were exhausted and probably frames were dropped,
  // (27.2.9 RMFCR counts them)
}

struct mg_tcpip_driver mg_tcpip_driver_ra
    = { mg_tcpip_driver_ra_init, mg_tcpip_driver_ra_tx, NULL,
	mg_tcpip_driver_ra_up };

#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/same54.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_SAME54)                       \
    && MG_ENABLE_DRIVER_SAME54

#  include <sam.h>

#  define ETH_PKT_SIZE 1536 // Max frame size
#  define ETH_DESC_CNT 4    // Descriptors count
#  define ETH_DS 2	    // Descriptor size (words)

static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE];
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE];
static uint32_t s_rxdesc[ETH_DESC_CNT][ETH_DS]; // RX descriptors
static uint32_t s_txdesc[ETH_DESC_CNT][ETH_DS]; // TX descriptors
static uint8_t s_txno;				// Current TX descriptor
static uint8_t s_rxno;				// Current RX descriptor

static struct mg_tcpip_if *s_ifp; // MIP interface
enum
{
  MG_PHY_ADDR = 0,
  MG_PHYREG_BCR = 0,
  MG_PHYREG_BSR = 1
};

#  define MG_PHYREGBIT_BCR_DUPLEX_MODE MG_BIT (8)
#  define MG_PHYREGBIT_BCR_SPEED MG_BIT (13)
#  define MG_PHYREGBIT_BSR_LINK_STATUS MG_BIT (2)

static uint16_t
eth_read_phy (uint8_t addr, uint8_t reg)
{
  GMAC_REGS->GMAC_MAN
      = GMAC_MAN_CLTTO_Msk | GMAC_MAN_OP (2) |	  // Setting the read operation
	GMAC_MAN_WTN (2) | GMAC_MAN_PHYA (addr) | // PHY address
	GMAC_MAN_REGA (reg);			  // Setting the register
  while (!(GMAC_REGS->GMAC_NSR & GMAC_NSR_IDLE_Msk))
    (void) 0;
  return GMAC_REGS->GMAC_MAN & GMAC_MAN_DATA_Msk; // Getting the read value
}

#  if 0
static void eth_write_phy(uint8_t addr, uint8_t reg, uint16_t val) {
  GMAC_REGS->GMAC_MAN = GMAC_MAN_CLTTO_Msk | GMAC_MAN_OP(1) |   // Setting the write operation
                        GMAC_MAN_WTN(2) | GMAC_MAN_PHYA(addr) | // PHY address
                        GMAC_MAN_REGA(reg) | GMAC_MAN_DATA(val);  // Setting the register
  while (!(GMAC_REGS->GMAC_NSR & GMAC_NSR_IDLE_Msk)); // Waiting until the write op is complete
}
#  endif

int
get_clock_rate (struct mg_tcpip_driver_same54_data *d)
{
  if (d && d->mdc_cr >= 0 && d->mdc_cr <= 5)
    {
      return d->mdc_cr;
    }
  else
    {
      // get MCLK from GCLK_GENERATOR 0
      uint32_t div = 512;
      uint32_t mclk;
      if (!(GCLK_REGS->GCLK_GENCTRL[0] & GCLK_GENCTRL_DIVSEL_Msk))
	{
	  div = ((GCLK_REGS->GCLK_GENCTRL[0] & 0x00FF0000) >> 16);
	  if (div == 0)
	    div = 1;
	}
      switch (GCLK_REGS->GCLK_GENCTRL[0] & GCLK_GENCTRL_SRC_Msk)
	{
	case GCLK_GENCTRL_SRC_XOSC0_Val:
	  mclk = 32000000UL; /* 32MHz */
	  break;
	case GCLK_GENCTRL_SRC_XOSC1_Val:
	  mclk = 32000000UL; /* 32MHz */
	  break;
	case GCLK_GENCTRL_SRC_OSCULP32K_Val:
	  mclk = 32000UL;
	  break;
	case GCLK_GENCTRL_SRC_XOSC32K_Val:
	  mclk = 32000UL;
	  break;
	case GCLK_GENCTRL_SRC_DFLL_Val:
	  mclk = 48000000UL; /* 48MHz */
	  break;
	case GCLK_GENCTRL_SRC_DPLL0_Val:
	  mclk = 200000000UL; /* 200MHz */
	  break;
	case GCLK_GENCTRL_SRC_DPLL1_Val:
	  mclk = 200000000UL; /* 200MHz */
	  break;
	default:
	  mclk = 200000000UL; /* 200MHz */
	}

      mclk /= div;
      uint8_t crs[] = { 0, 1, 2, 3, 4, 5 }; // GMAC->NCFGR::CLK values
      uint8_t dividers[]
	  = { 8, 16, 32, 48, 64, 96 }; // Respective CLK dividers
      for (int i = 0; i < 6; i++)
	{
	  if (mclk / dividers[i] <= 2375000UL /* 2.5MHz - 5% */)
	    {
	      return crs[i];
	    }
	}

      return 5;
    }
}

static bool
mg_tcpip_driver_same54_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_same54_data *d
      = (struct mg_tcpip_driver_same54_data *) ifp->driver_data;
  s_ifp = ifp;

  MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_GMAC_Msk;
  MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_GMAC_Msk;
  GMAC_REGS->GMAC_NCFGR
      = GMAC_NCFGR_CLK (get_clock_rate (d)); // Set MDC divider
  GMAC_REGS->GMAC_NCR = 0;		     // Disable RX & TX
  GMAC_REGS->GMAC_NCR |= GMAC_NCR_MPE_Msk;   // Enable MDC & MDIO

  for (int i = 0; i < ETH_DESC_CNT; i++)
    {					      // Init TX descriptors
      s_txdesc[i][0] = (uint32_t) s_txbuf[i]; // Point to data buffer
      s_txdesc[i][1] = MG_BIT (31);	      // OWN bit
    }
  s_txdesc[ETH_DESC_CNT - 1][1] |= MG_BIT (30); // Last tx descriptor - wrap

  GMAC_REGS->GMAC_DCFGR = GMAC_DCFGR_DRBS (0x18) // DMA recv buf 1536
			  | GMAC_DCFGR_RXBMS (GMAC_DCFGR_RXBMS_FULL_Val)
			  | GMAC_DCFGR_TXPBMS (1); // See #2487
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {					      // Init RX descriptors
      s_rxdesc[i][0] = (uint32_t) s_rxbuf[i]; // Address of the data buffer
      s_rxdesc[i][1] = 0;		      // Clear status
    }
  s_rxdesc[ETH_DESC_CNT - 1][0] |= MG_BIT (1); // Last rx descriptor - wrap

  GMAC_REGS->GMAC_TBQB = (uint32_t) s_txdesc; // about the descriptor addresses
  GMAC_REGS->GMAC_RBQB = (uint32_t) s_rxdesc; // Let the controller know

  GMAC_REGS->SA[0].GMAC_SAB
      = MG_U32 (ifp->mac[3], ifp->mac[2], ifp->mac[1], ifp->mac[0]);
  GMAC_REGS->SA[0].GMAC_SAT = MG_U32 (0, 0, ifp->mac[5], ifp->mac[4]);

  GMAC_REGS->GMAC_UR &= ~GMAC_UR_MII_Msk; // Disable MII, use RMII
  GMAC_REGS->GMAC_NCFGR |= GMAC_NCFGR_MAXFS_Msk | GMAC_NCFGR_MTIHEN_Msk
			   | GMAC_NCFGR_EFRHD_Msk | GMAC_NCFGR_CAF_Msk;
  GMAC_REGS->GMAC_TSR = GMAC_TSR_HRESP_Msk | GMAC_TSR_UND_Msk
			| GMAC_TSR_TXCOMP_Msk | GMAC_TSR_TFC_Msk
			| GMAC_TSR_TXGO_Msk | GMAC_TSR_RLE_Msk
			| GMAC_TSR_COL_Msk | GMAC_TSR_UBR_Msk;
  GMAC_REGS->GMAC_RSR = GMAC_RSR_HNO_Msk | GMAC_RSR_RXOVR_Msk
			| GMAC_RSR_REC_Msk | GMAC_RSR_BNA_Msk;
  GMAC_REGS->GMAC_IDR = ~0U; // Disable interrupts, then enable required
  GMAC_REGS->GMAC_IER = GMAC_IER_HRESP_Msk | GMAC_IER_ROVR_Msk
			| GMAC_IER_TCOMP_Msk | GMAC_IER_TFC_Msk
			| GMAC_IER_RLEX_Msk | GMAC_IER_TUR_Msk
			| GMAC_IER_RXUBR_Msk | GMAC_IER_RCOMP_Msk;
  GMAC_REGS->GMAC_NCR |= GMAC_NCR_TXEN_Msk | GMAC_NCR_RXEN_Msk;
  NVIC_EnableIRQ (GMAC_IRQn);

  return true;
}

static size_t
mg_tcpip_driver_same54_tx (const void *buf, size_t len,
			   struct mg_tcpip_if *ifp)
{
  if (len > sizeof (s_txbuf[s_txno]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = 0; // Frame is too big
    }
  else if ((s_txdesc[s_txno][1] & MG_BIT (31)) == 0)
    {
      ifp->nerr++;
      MG_ERROR (("No free descriptors"));
      len = 0; // All descriptors are busy, fail
    }
  else
    {
      uint32_t status = len | MG_BIT (15); // Frame length, last chunk
      if (s_txno == ETH_DESC_CNT - 1)
	status |= MG_BIT (30);		  // wrap
      memcpy (s_txbuf[s_txno], buf, len); // Copy data
      s_txdesc[s_txno][1] = status;
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }
  __DSB (); // Ensure descriptors have been written
  GMAC_REGS->GMAC_NCR |= GMAC_NCR_TSTART_Msk; // Enable transmission
  return len;
}

static bool
mg_tcpip_driver_same54_up (struct mg_tcpip_if *ifp)
{
  uint16_t bsr = eth_read_phy (MG_PHY_ADDR, MG_PHYREG_BSR);
  bool up = bsr & MG_PHYREGBIT_BSR_LINK_STATUS ? 1 : 0;

  // If PHY is ready, update NCFGR accordingly
  if (ifp->state == MG_TCPIP_STATE_DOWN && up)
    {
      uint16_t bcr = eth_read_phy (MG_PHY_ADDR, MG_PHYREG_BCR);
      bool fd = bcr & MG_PHYREGBIT_BCR_DUPLEX_MODE ? 1 : 0;
      bool spd = bcr & MG_PHYREGBIT_BCR_SPEED ? 1 : 0;
      GMAC_REGS->GMAC_NCFGR
	  = (GMAC_REGS->GMAC_NCFGR
	     & ~(GMAC_NCFGR_SPD_Msk | MG_PHYREGBIT_BCR_SPEED))
	    | GMAC_NCFGR_SPD (spd) | GMAC_NCFGR_FD (fd);
    }

  return up;
}

void GMAC_Handler (void);
void
GMAC_Handler (void)
{
  uint32_t isr = GMAC_REGS->GMAC_ISR;
  uint32_t rsr = GMAC_REGS->GMAC_RSR;
  uint32_t tsr = GMAC_REGS->GMAC_TSR;
  if (isr & GMAC_ISR_RCOMP_Msk)
    {
      if (rsr & GMAC_ISR_RCOMP_Msk)
	{
	  for (uint8_t i = 0; i < ETH_DESC_CNT; i++)
	    {
	      if ((s_rxdesc[s_rxno][0] & MG_BIT (0)) == 0)
		break;
	      size_t len = s_rxdesc[s_rxno][1] & (MG_BIT (13) - 1);
	      mg_tcpip_qwrite (s_rxbuf[s_rxno], len, s_ifp);
	      s_rxdesc[s_rxno][0] &= ~MG_BIT (0); // Disown
	      if (++s_rxno >= ETH_DESC_CNT)
		s_rxno = 0;
	    }
	}
    }

  if ((tsr
       & (GMAC_TSR_HRESP_Msk | GMAC_TSR_UND_Msk | GMAC_TSR_TXCOMP_Msk
	  | GMAC_TSR_TFC_Msk | GMAC_TSR_TXGO_Msk | GMAC_TSR_RLE_Msk
	  | GMAC_TSR_COL_Msk | GMAC_TSR_UBR_Msk))
      != 0)
    {
      // MG_INFO((" --> %#x %#x", s_txdesc[s_txno][1], tsr));
      if (!(s_txdesc[s_txno][1] & MG_BIT (31)))
	s_txdesc[s_txno][1] |= MG_BIT (31);
    }

  GMAC_REGS->GMAC_RSR = rsr;
  GMAC_REGS->GMAC_TSR = tsr;
}

struct mg_tcpip_driver mg_tcpip_driver_same54
    = { mg_tcpip_driver_same54_init, mg_tcpip_driver_same54_tx, NULL,
	mg_tcpip_driver_same54_up };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/stm32f.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_STM32F)                       \
    && MG_ENABLE_DRIVER_STM32F
struct stm32f_eth
{
  volatile uint32_t MACCR, MACFFR, MACHTHR, MACHTLR, MACMIIAR, MACMIIDR,
      MACFCR, MACVLANTR, RESERVED0[2], MACRWUFFR, MACPMTCSR, RESERVED1,
      MACDBGR, MACSR, MACIMR, MACA0HR, MACA0LR, MACA1HR, MACA1LR, MACA2HR,
      MACA2LR, MACA3HR, MACA3LR, RESERVED2[40], MMCCR, MMCRIR, MMCTIR, MMCRIMR,
      MMCTIMR, RESERVED3[14], MMCTGFSCCR, MMCTGFMSCCR, RESERVED4[5], MMCTGFCR,
      RESERVED5[10], MMCRFCECR, MMCRFAECR, RESERVED6[10], MMCRGUFCR,
      RESERVED7[334], PTPTSCR, PTPSSIR, PTPTSHR, PTPTSLR, PTPTSHUR, PTPTSLUR,
      PTPTSAR, PTPTTHR, PTPTTLR, RESERVED8, PTPTSSR, PTPPPSCR, RESERVED9[564],
      DMABMR, DMATPDR, DMARPDR, DMARDLAR, DMATDLAR, DMASR, DMAOMR, DMAIER,
      DMAMFBOCR, DMARSWTR, RESERVED10[8], DMACHTDR, DMACHRDR, DMACHTBAR,
      DMACHRBAR;
};
#  undef ETH
#  define ETH ((struct stm32f_eth *) (uintptr_t) 0x40028000)

#  define ETH_PKT_SIZE 1540 // Max frame size
#  define ETH_DESC_CNT 4    // Descriptors count
#  define ETH_DS 4	    // Descriptor size (words)

static uint32_t s_rxdesc[ETH_DESC_CNT][ETH_DS];	    // RX descriptors
static uint32_t s_txdesc[ETH_DESC_CNT][ETH_DS];	    // TX descriptors
static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE]; // RX ethernet buffers
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE]; // TX ethernet buffers
static uint8_t s_txno;				    // Current TX descriptor
static uint8_t s_rxno;				    // Current RX descriptor

static struct mg_tcpip_if *s_ifp; // MIP interface

static uint16_t
eth_read_phy (uint8_t addr, uint8_t reg)
{
  ETH->MACMIIAR &= (7 << 2);
  ETH->MACMIIAR |= ((uint32_t) addr << 11) | ((uint32_t) reg << 6);
  ETH->MACMIIAR |= MG_BIT (0);
  while (ETH->MACMIIAR & MG_BIT (0))
    (void) 0;
  return ETH->MACMIIDR & 0xffff;
}

static void
eth_write_phy (uint8_t addr, uint8_t reg, uint16_t val)
{
  ETH->MACMIIDR = val;
  ETH->MACMIIAR &= (7 << 2);
  ETH->MACMIIAR
      |= ((uint32_t) addr << 11) | ((uint32_t) reg << 6) | MG_BIT (1);
  ETH->MACMIIAR |= MG_BIT (0);
  while (ETH->MACMIIAR & MG_BIT (0))
    (void) 0;
}

static uint32_t
get_hclk (void)
{
  struct rcc
  {
    volatile uint32_t CR, PLLCFGR, CFGR;
  } *rcc = (struct rcc *) 0x40023800;
  uint32_t clk = 0, hsi = 16000000 /* 16 MHz */, hse = 8000000 /* 8MHz */;

  if (rcc->CFGR & (1 << 2))
    {
      clk = hse;
    }
  else if (rcc->CFGR & (1 << 3))
    {
      uint32_t vco, m, n, p;
      m = (rcc->PLLCFGR & (0x3f << 0)) >> 0;
      n = (rcc->PLLCFGR & (0x1ff << 6)) >> 6;
      p = (((rcc->PLLCFGR & (3 << 16)) >> 16) + 1) * 2;
      clk = (rcc->PLLCFGR & (1 << 22)) ? hse : hsi;
      vco = (uint32_t) ((uint64_t) clk * n / m);
      clk = vco / p;
    }
  else
    {
      clk = hsi;
    }
  uint32_t hpre = (rcc->CFGR & (15 << 4)) >> 4;
  if (hpre < 8)
    return clk;

  uint8_t ahbptab[8] = { 1, 2, 3, 4, 6, 7, 8, 9 }; // log2(div)
  return ((uint32_t) clk) >> ahbptab[hpre - 8];
}

//  Guess CR from HCLK. MDC clock is generated from HCLK (AHB); as per 802.3,
//  it must not exceed 2.5MHz As the AHB clock can be (and usually is) derived
//  from the HSI (internal RC), and it can go above specs, the datasheets
//  specify a range of frequencies and activate one of a series of dividers to
//  keep the MDC clock safely below 2.5MHz. We guess a divider setting based on
//  HCLK with a +5% drift. If the user uses a different clock from our
//  defaults, needs to set the macros on top Valid for STM32F74xxx/75xxx
//  (38.8.1) and STM32F42xxx/43xxx (33.8.1) (both 4.5% worst case drift)
static int
guess_mdc_cr (void)
{
  uint8_t crs[] = { 2, 3, 0, 1, 4, 5 };		// ETH->MACMIIAR::CR values
  uint8_t div[] = { 16, 26, 42, 62, 102, 124 }; // Respective HCLK dividers
  uint32_t hclk = get_hclk ();			// Guess system HCLK
  int result = -1;				// Invalid CR value
  if (hclk < 25000000)
    {
      MG_ERROR (("HCLK too low"));
    }
  else
    {
      for (int i = 0; i < 6; i++)
	{
	  if (hclk / div[i] <= 2375000UL /* 2.5MHz - 5% */)
	    {
	      result = crs[i];
	      break;
	    }
	}
      if (result < 0)
	MG_ERROR (("HCLK too high"));
    }
  MG_DEBUG (("HCLK: %u, CR: %d", hclk, result));
  return result;
}

static bool
mg_tcpip_driver_stm32f_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_stm32f_data *d
      = (struct mg_tcpip_driver_stm32f_data *) ifp->driver_data;
  uint8_t phy_addr = d == NULL ? 0 : d->phy_addr;
  s_ifp = ifp;

  // Init RX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i][0] = MG_BIT (31); // Own
      s_rxdesc[i][1]
	  = sizeof (s_rxbuf[i]) | MG_BIT (14); // 2nd address chained
      s_rxdesc[i][2]
	  = (uint32_t) (uintptr_t) s_rxbuf[i]; // Point to data buffer
      s_rxdesc[i][3]
	  = (uint32_t) (uintptr_t) s_rxdesc[(i + 1) % ETH_DESC_CNT]; // Chain
    }

  // Init TX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_txdesc[i][2] = (uint32_t) (uintptr_t) s_txbuf[i]; // Buf pointer
      s_txdesc[i][3]
	  = (uint32_t) (uintptr_t) s_txdesc[(i + 1) % ETH_DESC_CNT]; // Chain
    }

  ETH->DMABMR |= MG_BIT (0); // Software reset
  while ((ETH->DMABMR & MG_BIT (0)) != 0)
    (void) 0; // Wait until done

  // Set MDC clock divider. If user told us the value, use it. Otherwise, guess
  int cr = (d == NULL || d->mdc_cr < 0) ? guess_mdc_cr () : d->mdc_cr;
  ETH->MACMIIAR = ((uint32_t) cr & 7) << 2;

  // NOTE(cpq): we do not use extended descriptor bit 7, and do not use
  // hardware checksum. Therefore, descriptor size is 4, not 8
  // ETH->DMABMR = MG_BIT(13) | MG_BIT(16) | MG_BIT(22) | MG_BIT(23) |
  // MG_BIT(25);
  ETH->MACIMR = MG_BIT (3) | MG_BIT (9); // Mask timestamp & PMT IT
  ETH->MACFCR = MG_BIT (7);		 // Disable zero quarta pause
  // ETH->MACFFR = MG_BIT(31);                            // Receive all
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  mg_phy_init (&phy, phy_addr, MG_PHY_CLOCKS_MAC);
  ETH->DMARDLAR = (uint32_t) (uintptr_t) s_rxdesc; // RX descriptors
  ETH->DMATDLAR = (uint32_t) (uintptr_t) s_txdesc; // RX descriptors
  ETH->DMAIER = MG_BIT (6) | MG_BIT (16);	   // RIE, NISE
  ETH->MACCR = MG_BIT (2) | MG_BIT (3) | MG_BIT (11)
	       | MG_BIT (14); // RE, TE, Duplex, Fast
  ETH->DMAOMR = MG_BIT (1) | MG_BIT (13) | MG_BIT (21)
		| MG_BIT (25); // SR, ST, TSF, RSF

  // MAC address filtering
  ETH->MACA0HR = ((uint32_t) ifp->mac[5] << 8U) | ifp->mac[4];
  ETH->MACA0LR = (uint32_t) (ifp->mac[3] << 24)
		 | ((uint32_t) ifp->mac[2] << 16)
		 | ((uint32_t) ifp->mac[1] << 8) | ifp->mac[0];
  return true;
}

static size_t
mg_tcpip_driver_stm32f_tx (const void *buf, size_t len,
			   struct mg_tcpip_if *ifp)
{
  if (len > sizeof (s_txbuf[s_txno]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = 0; // Frame is too big
    }
  else if ((s_txdesc[s_txno][0] & MG_BIT (31)))
    {
      ifp->nerr++;
      MG_ERROR (("No free descriptors"));
      // printf("D0 %lx SR %lx\n", (long) s_txdesc[0][0], (long) ETH->DMASR);
      len = 0; // All descriptors are busy, fail
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);   // Copy data
      s_txdesc[s_txno][1] = (uint32_t) len; // Set data len
      s_txdesc[s_txno][0]
	  = MG_BIT (20) | MG_BIT (28) | MG_BIT (29); // Chain,FS,LS
      s_txdesc[s_txno][0] |= MG_BIT (31); // Set OWN bit - let DMA take over
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }
  MG_DSB ();				// ensure descriptors have been written
  ETH->DMASR = MG_BIT (2) | MG_BIT (5); // Clear any prior TBUS/TUS
  ETH->DMATPDR = 0;			// and resume
  return len;
}

static bool
mg_tcpip_driver_stm32f_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_stm32f_data *d
      = (struct mg_tcpip_driver_stm32f_data *) ifp->driver_data;
  uint8_t phy_addr = d == NULL ? 0 : d->phy_addr;
  uint8_t speed = MG_PHY_SPEED_10M;
  bool up = false, full_duplex = false;
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  up = mg_phy_up (&phy, phy_addr, &full_duplex, &speed);
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      // tmp = reg with flags set to the most likely situation: 100M
      // full-duplex if(link is slow or half) set flags otherwise reg = tmp
      uint32_t maccr
	  = ETH->MACCR | MG_BIT (14) | MG_BIT (11); // 100M, Full-duplex
      if (speed == MG_PHY_SPEED_10M)
	maccr &= ~MG_BIT (14); // 10M
      if (full_duplex == false)
	maccr &= ~MG_BIT (11); // Half-duplex
      ETH->MACCR = maccr; // IRQ handler does not fiddle with this register
      MG_DEBUG (("Link is %uM %s-duplex", maccr & MG_BIT (14) ? 100 : 10,
		 maccr & MG_BIT (11) ? "full" : "half"));
    }
  return up;
}

#  ifdef __riscv
__attribute__ ((interrupt ())) // For RISCV CH32V307, which share the same MAC
#  endif
void
ETH_IRQHandler (void);
void
ETH_IRQHandler (void)
{
  if (ETH->DMASR & MG_BIT (6))
    {					     // Frame received, loop
      ETH->DMASR = MG_BIT (16) | MG_BIT (6); // Clear flag
      for (uint32_t i = 0; i < 10; i++)
	{ // read as they arrive but not forever
	  if (s_rxdesc[s_rxno][0] & MG_BIT (31))
	    break; // exit when done
	  if (((s_rxdesc[s_rxno][0] & (MG_BIT (8) | MG_BIT (9)))
	       == (MG_BIT (8) | MG_BIT (9)))
	      && !(s_rxdesc[s_rxno][0] & MG_BIT (15)))
	    { // skip partial/errored frames
	      uint32_t len = ((s_rxdesc[s_rxno][0] >> 16) & (MG_BIT (14) - 1));
	      //  printf("%lx %lu %lx %.8lx\n", s_rxno, len,
	      //  s_rxdesc[s_rxno][0], ETH->DMASR);
	      mg_tcpip_qwrite (s_rxbuf[s_rxno], len > 4 ? len - 4 : len,
			       s_ifp);
	    }
	  s_rxdesc[s_rxno][0] = MG_BIT (31);
	  if (++s_rxno >= ETH_DESC_CNT)
	    s_rxno = 0;
	}
    }
  // Cleanup flags
  ETH->DMASR = MG_BIT (16)   // NIS, normal interrupt summary
	       | MG_BIT (7); // Clear possible RBUS while processing
  ETH->DMARPDR = 0;	     // and resume RX
}

struct mg_tcpip_driver mg_tcpip_driver_stm32f
    = { mg_tcpip_driver_stm32f_init, mg_tcpip_driver_stm32f_tx, NULL,
	mg_tcpip_driver_stm32f_up };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/stm32h.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_STM32H)                       \
    && MG_ENABLE_DRIVER_STM32H
struct stm32h_eth
{
  volatile uint32_t MACCR, MACECR, MACPFR, MACWTR, MACHT0R, MACHT1R,
      RESERVED1[14], MACVTR, RESERVED2, MACVHTR, RESERVED3, MACVIR, MACIVIR,
      RESERVED4[2], MACTFCR, RESERVED5[7], MACRFCR, RESERVED6[7], MACISR,
      MACIER, MACRXTXSR, RESERVED7, MACPCSR, MACRWKPFR, RESERVED8[2], MACLCSR,
      MACLTCR, MACLETR, MAC1USTCR, RESERVED9[12], MACVR, MACDR, RESERVED10,
      MACHWF0R, MACHWF1R, MACHWF2R, RESERVED11[54], MACMDIOAR, MACMDIODR,
      RESERVED12[2], MACARPAR, RESERVED13[59], MACA0HR, MACA0LR, MACA1HR,
      MACA1LR, MACA2HR, MACA2LR, MACA3HR, MACA3LR, RESERVED14[248], MMCCR,
      MMCRIR, MMCTIR, MMCRIMR, MMCTIMR, RESERVED15[14], MMCTSCGPR, MMCTMCGPR,
      RESERVED16[5], MMCTPCGR, RESERVED17[10], MMCRCRCEPR, MMCRAEPR,
      RESERVED18[10], MMCRUPGR, RESERVED19[9], MMCTLPIMSTR, MMCTLPITCR,
      MMCRLPIMSTR, MMCRLPITCR, RESERVED20[65], MACL3L4C0R, MACL4A0R,
      RESERVED21[2], MACL3A0R0R, MACL3A1R0R, MACL3A2R0R, MACL3A3R0R,
      RESERVED22[4], MACL3L4C1R, MACL4A1R, RESERVED23[2], MACL3A0R1R,
      MACL3A1R1R, MACL3A2R1R, MACL3A3R1R, RESERVED24[108], MACTSCR, MACSSIR,
      MACSTSR, MACSTNR, MACSTSUR, MACSTNUR, MACTSAR, RESERVED25, MACTSSR,
      RESERVED26[3], MACTTSSNR, MACTTSSSR, RESERVED27[2], MACACR, RESERVED28,
      MACATSNR, MACATSSR, MACTSIACR, MACTSEACR, MACTSICNR, MACTSECNR,
      RESERVED29[4], MACPPSCR, RESERVED30[3], MACPPSTTSR, MACPPSTTNR, MACPPSIR,
      MACPPSWR, RESERVED31[12], MACPOCR, MACSPI0R, MACSPI1R, MACSPI2R, MACLMIR,
      RESERVED32[11], MTLOMR, RESERVED33[7], MTLISR, RESERVED34[55], MTLTQOMR,
      MTLTQUR, MTLTQDR, RESERVED35[8], MTLQICSR, MTLRQOMR, MTLRQMPOCR, MTLRQDR,
      RESERVED36[177], DMAMR, DMASBMR, DMAISR, DMADSR, RESERVED37[60], DMACCR,
      DMACTCR, DMACRCR, RESERVED38[2], DMACTDLAR, RESERVED39, DMACRDLAR,
      DMACTDTPR, RESERVED40, DMACRDTPR, DMACTDRLR, DMACRDRLR, DMACIER,
      DMACRIWTR, DMACSFCSR, RESERVED41, DMACCATDR, RESERVED42, DMACCARDR,
      RESERVED43, DMACCATBR, RESERVED44, DMACCARBR, DMACSR, RESERVED45[2],
      DMACMFCR;
};
#  undef ETH
#  define ETH                                                                 \
    ((struct stm32h_eth *) (uintptr_t) (0x40000000UL + 0x00020000UL           \
					+ 0x8000UL))

#  define ETH_PKT_SIZE 1540 // Max frame size
#  define ETH_DESC_CNT 4    // Descriptors count
#  define ETH_DS 4	    // Descriptor size (words)

static volatile uint32_t s_rxdesc[ETH_DESC_CNT][ETH_DS]; // RX descriptors
static volatile uint32_t s_txdesc[ETH_DESC_CNT][ETH_DS]; // TX descriptors
static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE];	 // RX ethernet buffers
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE];	 // TX ethernet buffers
static struct mg_tcpip_if *s_ifp;			 // MIP interface

static uint16_t
eth_read_phy (uint8_t addr, uint8_t reg)
{
  ETH->MACMDIOAR &= (0xF << 8);
  ETH->MACMDIOAR |= ((uint32_t) addr << 21) | ((uint32_t) reg << 16) | 3 << 2;
  ETH->MACMDIOAR |= MG_BIT (0);
  while (ETH->MACMDIOAR & MG_BIT (0))
    (void) 0;
  return (uint16_t) ETH->MACMDIODR;
}

static void
eth_write_phy (uint8_t addr, uint8_t reg, uint16_t val)
{
  ETH->MACMDIODR = val;
  ETH->MACMDIOAR &= (0xF << 8);
  ETH->MACMDIOAR |= ((uint32_t) addr << 21) | ((uint32_t) reg << 16) | 1 << 2;
  ETH->MACMDIOAR |= MG_BIT (0);
  while (ETH->MACMDIOAR & MG_BIT (0))
    (void) 0;
}

static uint32_t
get_hclk (void)
{
  struct rcc
  {
    volatile uint32_t CR, HSICFGR, CRRCR, CSICFGR, CFGR, RESERVED1, D1CFGR,
	D2CFGR, D3CFGR, RESERVED2, PLLCKSELR, PLLCFGR, PLL1DIVR, PLL1FRACR,
	PLL2DIVR, PLL2FRACR, PLL3DIVR, PLL3FRACR, RESERVED3, D1CCIPR, D2CCIP1R,
	D2CCIP2R, D3CCIPR, RESERVED4, CIER, CIFR, CICR, RESERVED5, BDCR, CSR,
	RESERVED6, AHB3RSTR, AHB1RSTR, AHB2RSTR, AHB4RSTR, APB3RSTR, APB1LRSTR,
	APB1HRSTR, APB2RSTR, APB4RSTR, GCR, RESERVED8, D3AMR, RESERVED11[9],
	RSR, AHB3ENR, AHB1ENR, AHB2ENR, AHB4ENR, APB3ENR, APB1LENR, APB1HENR,
	APB2ENR, APB4ENR, RESERVED12, AHB3LPENR, AHB1LPENR, AHB2LPENR,
	AHB4LPENR, APB3LPENR, APB1LLPENR, APB1HLPENR, APB2LPENR, APB4LPENR,
	RESERVED13[4];
  } *rcc = ((struct rcc *) (0x40000000 + 0x18020000 + 0x4400));
  uint32_t clk = 0, hsi = 64000000 /* 64 MHz */, hse = 8000000 /* 8MHz */,
	   csi = 4000000 /* 4MHz */;
  unsigned int sel = (rcc->CFGR & (7 << 3)) >> 3;

  if (sel == 1)
    {
      clk = csi;
    }
  else if (sel == 2)
    {
      clk = hse;
    }
  else if (sel == 3)
    {
      uint32_t vco, m, n, p;
      unsigned int src = (rcc->PLLCKSELR & (3 << 0)) >> 0;
      m = ((rcc->PLLCKSELR & (0x3F << 4)) >> 4);
      n = ((rcc->PLL1DIVR & (0x1FF << 0)) >> 0) + 1
	  + ((rcc->PLLCFGR & MG_BIT (0)) ? 1
					 : 0); // round-up in fractional mode
      p = ((rcc->PLL1DIVR & (0x7F << 9)) >> 9) + 1;
      if (src == 1)
	{
	  clk = csi;
	}
      else if (src == 2)
	{
	  clk = hse;
	}
      else
	{
	  clk = hsi;
	  clk >>= ((rcc->CR & 3) >> 3);
	}
      vco = (uint32_t) ((uint64_t) clk * n / m);
      clk = vco / p;
    }
  else
    {
      clk = hsi;
      clk >>= ((rcc->CR & 3) >> 3);
    }
  const uint8_t cptab[12] = { 1, 2, 3, 4, 6, 7, 8, 9 }; // log2(div)
  uint32_t d1cpre = (rcc->D1CFGR & (0x0F << 8)) >> 8;
  if (d1cpre >= 8)
    clk >>= cptab[d1cpre - 8];
  MG_DEBUG (("D1 CLK: %u", clk));
  uint32_t hpre = (rcc->D1CFGR & (0x0F << 0)) >> 0;
  if (hpre < 8)
    return clk;
  return ((uint32_t) clk) >> cptab[hpre - 8];
}

//  Guess CR from AHB1 clock. MDC clock is generated from the ETH peripheral
//  clock (AHB1); as per 802.3, it must not exceed 2. As the AHB clock can
//  be derived from HSI or CSI (internal RC) clocks, and those can go above
//  specs, the datasheets specify a range of frequencies and activate one of a
//  series of dividers to keep the MDC clock safely below 2.5MHz. We guess a
//  divider setting based on HCLK with some drift. If the user uses a different
//  clock from our defaults, needs to set the macros on top. Valid for
//  STM32H74xxx/75xxx (58.11.4)(4.5% worst case drift)(CSI clock has a 7.5 %
//  worst case drift @ max temp)
static int
guess_mdc_cr (void)
{
  const uint8_t crs[] = { 2, 3, 0, 1, 4, 5 }; // ETH->MACMDIOAR::CR values
  const uint8_t div[]
      = { 16, 26, 42, 62, 102, 124 }; // Respective HCLK dividers
  uint32_t hclk = get_hclk ();	      // Guess system HCLK
  int result = -1;		      // Invalid CR value
  for (int i = 0; i < 6; i++)
    {
      if (hclk / div[i] <= 2375000UL /* 2.5MHz - 5% */)
	{
	  result = crs[i];
	  break;
	}
    }
  if (result < 0)
    MG_ERROR (("HCLK too high"));
  MG_DEBUG (("HCLK: %u, CR: %d", hclk, result));
  return result;
}

static bool
mg_tcpip_driver_stm32h_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_stm32h_data *d
      = (struct mg_tcpip_driver_stm32h_data *) ifp->driver_data;
  s_ifp = ifp;
  uint8_t phy_addr = d == NULL ? 0 : d->phy_addr;
  uint8_t phy_conf = d == NULL ? MG_PHY_CLOCKS_MAC : d->phy_conf;

  // Init RX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i][0]
	  = (uint32_t) (uintptr_t) s_rxbuf[i]; // Point to data buffer
      s_rxdesc[i][3]
	  = MG_BIT (31) | MG_BIT (30) | MG_BIT (24); // OWN, IOC, BUF1V
    }

  // Init TX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_txdesc[i][0] = (uint32_t) (uintptr_t) s_txbuf[i]; // Buf pointer
    }

  ETH->DMAMR |= MG_BIT (0); // Software reset
  while ((ETH->DMAMR & MG_BIT (0)) != 0)
    (void) 0; // Wait until done

  // Set MDC clock divider. If user told us the value, use it. Otherwise, guess
  int cr = (d == NULL || d->mdc_cr < 0) ? guess_mdc_cr () : d->mdc_cr;
  ETH->MACMDIOAR = ((uint32_t) cr & 0xF) << 8;

  // NOTE(scaprile): We do not use timing facilities so the DMA engine does not
  // re-write buffer address
  ETH->DMAMR = 0 << 16;	       // use interrupt mode 0 (58.8.1) (reset value)
  ETH->DMASBMR |= MG_BIT (12); // AAL NOTE(scaprile): is this actually needed
  ETH->MACIER = 0; // Do not enable additional irq sources (reset value)
  ETH->MACTFCR = MG_BIT (7); // Disable zero-quanta pause
  // ETH->MACPFR = MG_BIT(31);  // Receive all
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  mg_phy_init (&phy, phy_addr, phy_conf);
  ETH->DMACRDLAR
      = (uint32_t) (uintptr_t) s_rxdesc; // RX descriptors start address
  ETH->DMACRDRLR = ETH_DESC_CNT - 1;	 // ring length
  ETH->DMACRDTPR
      = (uint32_t) (uintptr_t) &s_rxdesc[ETH_DESC_CNT
					 - 1]; // last valid descriptor address
  ETH->DMACTDLAR
      = (uint32_t) (uintptr_t) s_txdesc; // TX descriptors start address
  ETH->DMACTDRLR = ETH_DESC_CNT - 1;	 // ring length
  ETH->DMACTDTPR
      = (uint32_t) (uintptr_t) s_txdesc; // first available descriptor address
  ETH->DMACCR = 0; // DSL = 0 (contiguous descriptor table) (reset value)
  ETH->DMACIER = MG_BIT (6) | MG_BIT (15); // RIE, NIE
  ETH->MACCR = MG_BIT (0) | MG_BIT (1) | MG_BIT (13) | MG_BIT (14)
	       | MG_BIT (15);  // RE, TE, Duplex, Fast, Reserved
  ETH->MTLTQOMR |= MG_BIT (1); // TSF
  ETH->MTLRQOMR |= MG_BIT (5); // RSF
  ETH->DMACTCR |= MG_BIT (0);  // ST
  ETH->DMACRCR |= MG_BIT (0);  // SR

  // MAC address filtering
  ETH->MACA0HR = ((uint32_t) ifp->mac[5] << 8U) | ifp->mac[4];
  ETH->MACA0LR = (uint32_t) (ifp->mac[3] << 24)
		 | ((uint32_t) ifp->mac[2] << 16)
		 | ((uint32_t) ifp->mac[1] << 8) | ifp->mac[0];
  return true;
}

static uint32_t s_txno;
static size_t
mg_tcpip_driver_stm32h_tx (const void *buf, size_t len,
			   struct mg_tcpip_if *ifp)
{
  if (len > sizeof (s_txbuf[s_txno]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = 0; // Frame is too big
    }
  else if ((s_txdesc[s_txno][3] & MG_BIT (31)))
    {
      ifp->nerr++;
      MG_ERROR (("No free descriptors: %u %08X %08X %08X", s_txno,
		 s_txdesc[s_txno][3], ETH->DMACSR, ETH->DMACTCR));
      for (int i = 0; i < ETH_DESC_CNT; i++)
	MG_ERROR (("%08X", s_txdesc[i][3]));
      len = 0; // All descriptors are busy, fail
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);	       // Copy data
      s_txdesc[s_txno][2] = (uint32_t) len;	       // Set data len
      s_txdesc[s_txno][3] = MG_BIT (28) | MG_BIT (29); // FD, LD
      s_txdesc[s_txno][3] |= MG_BIT (31); // Set OWN bit - let DMA take over
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }
  ETH->DMACSR |= MG_BIT (2) | MG_BIT (1); // Clear any prior TBU, TPS
  ETH->DMACTDTPR = (uint32_t) (uintptr_t) &s_txdesc[s_txno]; // and resume
  return len;
  (void) ifp;
}

static bool
mg_tcpip_driver_stm32h_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_stm32h_data *d
      = (struct mg_tcpip_driver_stm32h_data *) ifp->driver_data;
  uint8_t phy_addr = d == NULL ? 0 : d->phy_addr;
  uint8_t speed = MG_PHY_SPEED_10M;
  bool up = false, full_duplex = false;
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  up = mg_phy_up (&phy, phy_addr, &full_duplex, &speed);
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      // tmp = reg with flags set to the most likely situation: 100M
      // full-duplex if(link is slow or half) set flags otherwise reg = tmp
      uint32_t maccr
	  = ETH->MACCR | MG_BIT (14) | MG_BIT (13); // 100M, Full-duplex
      if (speed == MG_PHY_SPEED_10M)
	maccr &= ~MG_BIT (14); // 10M
      if (full_duplex == false)
	maccr &= ~MG_BIT (13); // Half-duplex
      ETH->MACCR = maccr; // IRQ handler does not fiddle with this register
      MG_DEBUG (("Link is %uM %s-duplex", maccr & MG_BIT (14) ? 100 : 10,
		 maccr & MG_BIT (13) ? "full" : "half"));
    }
  return up;
}

void ETH_IRQHandler (void);
static uint32_t s_rxno;
void
ETH_IRQHandler (void)
{
  if (ETH->DMACSR & MG_BIT (6))
    {					      // Frame received, loop
      ETH->DMACSR = MG_BIT (15) | MG_BIT (6); // Clear flag
      for (uint32_t i = 0; i < 10; i++)
	{ // read as they arrive but not forever
	  if (s_rxdesc[s_rxno][3] & MG_BIT (31))
	    break; // exit when done
	  if (((s_rxdesc[s_rxno][3] & (MG_BIT (28) | MG_BIT (29)))
	       == (MG_BIT (28) | MG_BIT (29)))
	      && !(s_rxdesc[s_rxno][3] & MG_BIT (15)))
	    { // skip partial/errored frames
	      uint32_t len = s_rxdesc[s_rxno][3] & (MG_BIT (15) - 1);
	      // MG_DEBUG(("%lx %lu %lx %08lx", s_rxno, len,
	      // s_rxdesc[s_rxno][3], ETH->DMACSR));
	      mg_tcpip_qwrite (s_rxbuf[s_rxno], len > 4 ? len - 4 : len,
			       s_ifp);
	    }
	  s_rxdesc[s_rxno][3]
	      = MG_BIT (31) | MG_BIT (30) | MG_BIT (24); // OWN, IOC, BUF1V
	  if (++s_rxno >= ETH_DESC_CNT)
	    s_rxno = 0;
	}
    }
  ETH->DMACSR
      = MG_BIT (7) | MG_BIT (8); // Clear possible RBU RPS while processing
  ETH->DMACRDTPR
      = (uint32_t) (uintptr_t) &s_rxdesc[ETH_DESC_CNT - 1]; // and resume RX
}

struct mg_tcpip_driver mg_tcpip_driver_stm32h
    = { mg_tcpip_driver_stm32h_init, mg_tcpip_driver_stm32h_tx, NULL,
	mg_tcpip_driver_stm32h_up };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/tm4c.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_TM4C) && MG_ENABLE_DRIVER_TM4C
struct tm4c_emac
{
  volatile uint32_t EMACCFG, EMACFRAMEFLTR, EMACHASHTBLH, EMACHASHTBLL,
      EMACMIIADDR, EMACMIIDATA, EMACFLOWCTL, EMACVLANTG, RESERVED0, EMACSTATUS,
      EMACRWUFF, EMACPMTCTLSTAT, RESERVED1[2], EMACRIS, EMACIM, EMACADDR0H,
      EMACADDR0L, EMACADDR1H, EMACADDR1L, EMACADDR2H, EMACADDR2L, EMACADDR3H,
      EMACADDR3L, RESERVED2[31], EMACWDOGTO, RESERVED3[8], EMACMMCCTRL,
      EMACMMCRXRIS, EMACMMCTXRIS, EMACMMCRXIM, EMACMMCTXIM, RESERVED4,
      EMACTXCNTGB, RESERVED5[12], EMACTXCNTSCOL, EMACTXCNTMCOL, RESERVED6[4],
      EMACTXOCTCNTG, RESERVED7[6], EMACRXCNTGB, RESERVED8[4], EMACRXCNTCRCERR,
      EMACRXCNTALGNERR, RESERVED9[10], EMACRXCNTGUNI, RESERVED10[239],
      EMACVLNINCREP, EMACVLANHASH, RESERVED11[93], EMACTIMSTCTRL,
      EMACSUBSECINC, EMACTIMSEC, EMACTIMNANO, EMACTIMSECU, EMACTIMNANOU,
      EMACTIMADD, EMACTARGSEC, EMACTARGNANO, EMACHWORDSEC, EMACTIMSTAT,
      EMACPPSCTRL, RESERVED12[12], EMACPPS0INTVL, EMACPPS0WIDTH,
      RESERVED13[294], EMACDMABUSMOD, EMACTXPOLLD, EMACRXPOLLD, EMACRXDLADDR,
      EMACTXDLADDR, EMACDMARIS, EMACDMAOPMODE, EMACDMAIM, EMACMFBOC,
      EMACRXINTWDT, RESERVED14[8], EMACHOSTXDESC, EMACHOSRXDESC, EMACHOSTXBA,
      EMACHOSRXBA, RESERVED15[218], EMACPP, EMACPC, EMACCC, RESERVED16,
      EMACEPHYRIS, EMACEPHYIM, EMACEPHYIMSC;
};
#  undef EMAC
#  define EMAC ((struct tm4c_emac *) (uintptr_t) 0x400EC000)

#  define ETH_PKT_SIZE 1540 // Max frame size
#  define ETH_DESC_CNT 4    // Descriptors count
#  define ETH_DS 4	    // Descriptor size (words)

static uint32_t s_rxdesc[ETH_DESC_CNT][ETH_DS];	    // RX descriptors
static uint32_t s_txdesc[ETH_DESC_CNT][ETH_DS];	    // TX descriptors
static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE]; // RX ethernet buffers
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE]; // TX ethernet buffers
static struct mg_tcpip_if *s_ifp;		    // MIP interface
enum
{
  EPHY_ADDR = 0,
  EPHYBMCR = 0,
  EPHYBMSR = 1,
  EPHYSTS = 16
}; // PHY constants

static inline void
tm4cspin (volatile uint32_t count)
{
  while (count--)
    (void) 0;
}

static uint32_t
emac_read_phy (uint8_t addr, uint8_t reg)
{
  EMAC->EMACMIIADDR &= (0xf << 2);
  EMAC->EMACMIIADDR |= ((uint32_t) addr << 11) | ((uint32_t) reg << 6);
  EMAC->EMACMIIADDR |= MG_BIT (0);
  while (EMAC->EMACMIIADDR & MG_BIT (0))
    tm4cspin (1);
  return EMAC->EMACMIIDATA;
}

static void
emac_write_phy (uint8_t addr, uint8_t reg, uint32_t val)
{
  EMAC->EMACMIIDATA = val;
  EMAC->EMACMIIADDR &= (0xf << 2);
  EMAC->EMACMIIADDR
      |= ((uint32_t) addr << 11) | ((uint32_t) reg << 6) | MG_BIT (1);
  EMAC->EMACMIIADDR |= MG_BIT (0);
  while (EMAC->EMACMIIADDR & MG_BIT (0))
    tm4cspin (1);
}

static uint32_t
get_sysclk (void)
{
  struct sysctl
  {
    volatile uint32_t DONTCARE0[44], RSCLKCFG, DONTCARE1[43], PLLFREQ0,
	PLLFREQ1;
  } *sysctl = (struct sysctl *) 0x400FE000;
  uint32_t clk = 0, piosc = 16000000 /* 16 MHz */, mosc = 25000000 /* 25MHz */;
  if (sysctl->RSCLKCFG & (1 << 28))
    { // USEPLL
      uint32_t fin, vco, mdiv, n, q, psysdiv;
      uint32_t pllsrc = (sysctl->RSCLKCFG & (0xf << 24)) >> 24;
      if (pllsrc == 0)
	{
	  clk = piosc;
	}
      else if (pllsrc == 3)
	{
	  clk = mosc;
	}
      else
	{
	  MG_ERROR (("Unsupported clock source"));
	}
      q = (sysctl->PLLFREQ1 & (0x1f << 8)) >> 8;
      n = (sysctl->PLLFREQ1 & (0x1f << 0)) >> 0;
      fin = clk / ((q + 1) * (n + 1));
      mdiv = (sysctl->PLLFREQ0 & (0x3ff << 0))
	     >> 0; // mint + (mfrac / 1024); MFRAC not supported
      psysdiv = (sysctl->RSCLKCFG & (0x3f << 0)) >> 0;
      vco = (uint32_t) ((uint64_t) fin * mdiv);
      return vco / (psysdiv + 1);
    }
  uint32_t oscsrc = (sysctl->RSCLKCFG & (0xf << 20)) >> 20;
  if (oscsrc == 0)
    {
      clk = piosc;
    }
  else if (oscsrc == 3)
    {
      clk = mosc;
    }
  else
    {
      MG_ERROR (("Unsupported clock source"));
    }
  uint32_t osysdiv = (sysctl->RSCLKCFG & (0xf << 16)) >> 16;
  return clk / (osysdiv + 1);
}

//  Guess CR from SYSCLK. MDC clock is generated from SYSCLK (AHB); as per
//  802.3, it must not exceed 2.5MHz (also 20.4.2.6) As the AHB clock can be
//  derived from the PIOSC (internal RC), and it can go above  specs, the
//  datasheets specify a range of frequencies and activate one of a series of
//  dividers to keep the MDC clock safely below 2.5MHz. We guess a divider
//  setting based on SYSCLK with a +5% drift. If the user uses a different
//  clock from our defaults, needs to set the macros on top Valid for TM4C129x
//  (20.7) (4.5% worst case drift)
// The PHY receives the main oscillator (MOSC) (20.3.1)
static int
guess_mdc_cr (void)
{
  uint8_t crs[] = { 2, 3, 0, 1 };     // EMAC->MACMIIAR::CR values
  uint8_t div[] = { 16, 26, 42, 62 }; // Respective HCLK dividers
  uint32_t sysclk = get_sysclk ();    // Guess system SYSCLK
  int result = -1;		      // Invalid CR value
  if (sysclk < 25000000)
    {
      MG_ERROR (("SYSCLK too low"));
    }
  else
    {
      for (int i = 0; i < 4; i++)
	{
	  if (sysclk / div[i] <= 2375000UL /* 2.5MHz - 5% */)
	    {
	      result = crs[i];
	      break;
	    }
	}
      if (result < 0)
	MG_ERROR (("SYSCLK too high"));
    }
  MG_DEBUG (("SYSCLK: %u, CR: %d", sysclk, result));
  return result;
}

static bool
mg_tcpip_driver_tm4c_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_tm4c_data *d
      = (struct mg_tcpip_driver_tm4c_data *) ifp->driver_data;
  s_ifp = ifp;

  // Init RX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i][0] = MG_BIT (31); // Own
      s_rxdesc[i][1]
	  = sizeof (s_rxbuf[i]) | MG_BIT (14); // 2nd address chained
      s_rxdesc[i][2]
	  = (uint32_t) (uintptr_t) s_rxbuf[i]; // Point to data buffer
      s_rxdesc[i][3]
	  = (uint32_t) (uintptr_t) s_rxdesc[(i + 1) % ETH_DESC_CNT]; // Chain
      // MG_DEBUG(("%d %p", i, s_rxdesc[i]));
    }

  // Init TX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_txdesc[i][2] = (uint32_t) (uintptr_t) s_txbuf[i]; // Buf pointer
      s_txdesc[i][3]
	  = (uint32_t) (uintptr_t) s_txdesc[(i + 1) % ETH_DESC_CNT]; // Chain
    }

  EMAC->EMACDMABUSMOD |= MG_BIT (0); // Software reset
  while ((EMAC->EMACDMABUSMOD & MG_BIT (0)) != 0)
    tm4cspin (1); // Wait until done

  // Set MDC clock divider. If user told us the value, use it. Otherwise, guess
  int cr = (d == NULL || d->mdc_cr < 0) ? guess_mdc_cr () : d->mdc_cr;
  EMAC->EMACMIIADDR = ((uint32_t) cr & 0xf) << 2;

  // NOTE(cpq): we do not use extended descriptor bit 7, and do not use
  // hardware checksum. Therefore, descriptor size is 4, not 8
  // EMAC->EMACDMABUSMOD = MG_BIT(13) | MG_BIT(16) | MG_BIT(22) | MG_BIT(23) |
  // MG_BIT(25);
  EMAC->EMACIM = MG_BIT (3) | MG_BIT (9); // Mask timestamp & PMT IT
  EMAC->EMACFLOWCTL = MG_BIT (7);	  // Disable zero-quanta pause
  // EMAC->EMACFRAMEFLTR = MG_BIT(31);   // Receive all
  // EMAC->EMACPC defaults to internal PHY (EPHY) in MMI mode
  emac_write_phy (EPHY_ADDR, EPHYBMCR,
		  MG_BIT (15)); // Reset internal PHY (EPHY)
  emac_write_phy (EPHY_ADDR, EPHYBMCR, MG_BIT (12));	// Set autonegotiation
  EMAC->EMACRXDLADDR = (uint32_t) (uintptr_t) s_rxdesc; // RX descriptors
  EMAC->EMACTXDLADDR = (uint32_t) (uintptr_t) s_txdesc; // TX descriptors
  EMAC->EMACDMAIM = MG_BIT (6) | MG_BIT (16);		// RIE, NIE
  EMAC->EMACCFG = MG_BIT (2) | MG_BIT (3) | MG_BIT (11)
		  | MG_BIT (14); // RE, TE, Duplex, Fast
  EMAC->EMACDMAOPMODE = MG_BIT (1) | MG_BIT (13) | MG_BIT (21)
			| MG_BIT (25); // SR, ST, TSF, RSF
  EMAC->EMACADDR0H = ((uint32_t) ifp->mac[5] << 8U) | ifp->mac[4];
  EMAC->EMACADDR0L = (uint32_t) (ifp->mac[3] << 24)
		     | ((uint32_t) ifp->mac[2] << 16)
		     | ((uint32_t) ifp->mac[1] << 8) | ifp->mac[0];
  // NOTE(scaprile) There are 3 additional slots for filtering, disabled by
  // default. This also applies to the STM32 driver (at least for F7)
  return true;
}

static uint32_t s_txno;
static size_t
mg_tcpip_driver_tm4c_tx (const void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  if (len > sizeof (s_txbuf[s_txno]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = 0; // fail
    }
  else if ((s_txdesc[s_txno][0] & MG_BIT (31)))
    {
      ifp->nerr++;
      MG_ERROR (("No descriptors available"));
      // printf("D0 %lx SR %lx\n", (long) s_txdesc[0][0], (long)
      // EMAC->EMACDMARIS);
      len = 0; // fail
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);   // Copy data
      s_txdesc[s_txno][1] = (uint32_t) len; // Set data len
      s_txdesc[s_txno][0] = MG_BIT (20) | MG_BIT (28) | MG_BIT (29)
			    | MG_BIT (30); // Chain,FS,LS,IC
      s_txdesc[s_txno][0] |= MG_BIT (31);  // Set OWN bit - let DMA take over
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }
  EMAC->EMACDMARIS = MG_BIT (2) | MG_BIT (5); // Clear any prior TU/UNF
  EMAC->EMACTXPOLLD = 0;		      // and resume
  return len;
  (void) ifp;
}

static bool
mg_tcpip_driver_tm4c_up (struct mg_tcpip_if *ifp)
{
  uint32_t bmsr = emac_read_phy (EPHY_ADDR, EPHYBMSR);
  bool up = (bmsr & MG_BIT (2)) ? 1 : 0;
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      uint32_t sts = emac_read_phy (EPHY_ADDR, EPHYSTS);
      // tmp = reg with flags set to the most likely situation: 100M
      // full-duplex if(link is slow or half) set flags otherwise reg = tmp
      uint32_t emaccfg
	  = EMAC->EMACCFG | MG_BIT (14) | MG_BIT (11); // 100M, Full-duplex
      if (sts & MG_BIT (1))
	emaccfg &= ~MG_BIT (14); // 10M
      if ((sts & MG_BIT (2)) == 0)
	emaccfg &= ~MG_BIT (11); // Half-duplex
      EMAC->EMACCFG
	  = emaccfg; // IRQ handler does not fiddle with this register
      MG_DEBUG (("Link is %uM %s-duplex", emaccfg & MG_BIT (14) ? 100 : 10,
		 emaccfg & MG_BIT (11) ? "full" : "half"));
    }
  return up;
}

void EMAC0_IRQHandler (void);
static uint32_t s_rxno;
void
EMAC0_IRQHandler (void)
{
  if (EMAC->EMACDMARIS & MG_BIT (6))
    {						   // Frame received, loop
      EMAC->EMACDMARIS = MG_BIT (16) | MG_BIT (6); // Clear flag
      for (uint32_t i = 0; i < 10; i++)
	{ // read as they arrive but not forever
	  if (s_rxdesc[s_rxno][0] & MG_BIT (31))
	    break; // exit when done
	  if (((s_rxdesc[s_rxno][0] & (MG_BIT (8) | MG_BIT (9)))
	       == (MG_BIT (8) | MG_BIT (9)))
	      && !(s_rxdesc[s_rxno][0] & MG_BIT (15)))
	    { // skip partial/errored frames
	      uint32_t len = ((s_rxdesc[s_rxno][0] >> 16) & (MG_BIT (14) - 1));
	      //  printf("%lx %lu %lx %.8lx\n", s_rxno, len,
	      //  s_rxdesc[s_rxno][0], EMAC->EMACDMARIS);
	      mg_tcpip_qwrite (s_rxbuf[s_rxno], len > 4 ? len - 4 : len,
			       s_ifp);
	    }
	  s_rxdesc[s_rxno][0] = MG_BIT (31);
	  if (++s_rxno >= ETH_DESC_CNT)
	    s_rxno = 0;
	}
    }
  EMAC->EMACDMARIS = MG_BIT (7); // Clear possible RU while processing
  EMAC->EMACRXPOLLD = 0;	 // and resume RX
}

struct mg_tcpip_driver mg_tcpip_driver_tm4c
    = { mg_tcpip_driver_tm4c_init, mg_tcpip_driver_tm4c_tx, NULL,
	mg_tcpip_driver_tm4c_up };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/w5500.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_W5500)                        \
    && MG_ENABLE_DRIVER_W5500

enum
{
  W5500_CR = 0,
  W5500_S0 = 1,
  W5500_TX0 = 2,
  W5500_RX0 = 3
};

static void
w5500_txn (struct mg_tcpip_spi *s, uint8_t block, uint16_t addr, bool wr,
	   void *buf, size_t len)
{
  size_t i;
  uint8_t *p = (uint8_t *) buf;
  uint8_t cmd[] = { (uint8_t) (addr >> 8), (uint8_t) (addr & 255),
		    (uint8_t) ((block << 3) | (wr ? 4 : 0)) };
  s->begin (s->spi);
  for (i = 0; i < sizeof (cmd); i++)
    s->txn (s->spi, cmd[i]);
  for (i = 0; i < len; i++)
    {
      uint8_t r = s->txn (s->spi, p[i]);
      if (!wr)
	p[i] = r;
    }
  s->end (s->spi);
}

// clang-format off
static  void w5500_wn(struct mg_tcpip_spi *s, uint8_t block, uint16_t addr, void *buf, size_t len) { w5500_txn(s, block, addr, true, buf, len); }
static  void w5500_w1(struct mg_tcpip_spi *s, uint8_t block, uint16_t addr, uint8_t val) { w5500_wn(s, block, addr, &val, 1); }
static  void w5500_w2(struct mg_tcpip_spi *s, uint8_t block, uint16_t addr, uint16_t val) { uint8_t buf[2] = {(uint8_t) (val >> 8), (uint8_t) (val & 255)}; w5500_wn(s, block, addr, buf, sizeof(buf)); }
static  void w5500_rn(struct mg_tcpip_spi *s, uint8_t block, uint16_t addr, void *buf, size_t len) { w5500_txn(s, block, addr, false, buf, len); }
static  uint8_t w5500_r1(struct mg_tcpip_spi *s, uint8_t block, uint16_t addr) { uint8_t r = 0; w5500_rn(s, block, addr, &r, 1); return r; }
static  uint16_t w5500_r2(struct mg_tcpip_spi *s, uint8_t block, uint16_t addr) { uint8_t buf[2] = {0, 0}; w5500_rn(s, block, addr, buf, sizeof(buf)); return (uint16_t) ((buf[0] << 8) | buf[1]); }
// clang-format on

static size_t
w5500_rx (void *buf, size_t buflen, struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_spi *s = (struct mg_tcpip_spi *) ifp->driver_data;
  uint16_t r = 0, n = 0, len = (uint16_t) buflen, n2; // Read recv len
  while ((n2 = w5500_r2 (s, W5500_S0, 0x26)) > n)
    n = n2; // Until it is stable
  // printf("RSR: %d\n", (int) n);
  if (n > 0)
    {
      uint16_t ptr = w5500_r2 (s, W5500_S0, 0x28); // Get read pointer
      n = w5500_r2 (s, W5500_RX0, ptr);		   // Read frame length
      if (n <= len + 2 && n > 1)
	{
	  r = (uint16_t) (n - 2);
	  w5500_rn (s, W5500_RX0, (uint16_t) (ptr + 2), buf, r);
	}
      w5500_w2 (s, W5500_S0, 0x28,
		(uint16_t) (ptr + n)); // Advance read pointer
      w5500_w1 (s, W5500_S0, 1, 0x40); // Sock0 CR -> RECV
      // printf("  RX_RD: tot=%u n=%u r=%u\n", n2, n, r);
    }
  return r;
}

static size_t
w5500_tx (const void *buf, size_t buflen, struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_spi *s = (struct mg_tcpip_spi *) ifp->driver_data;
  uint16_t i, ptr, n = 0, len = (uint16_t) buflen;
  while (n < len)
    n = w5500_r2 (s, W5500_S0, 0x20);		   // Wait for space
  ptr = w5500_r2 (s, W5500_S0, 0x24);		   // Get write pointer
  w5500_wn (s, W5500_TX0, ptr, (void *) buf, len); // Write data
  w5500_w2 (s, W5500_S0, 0x24,
	    (uint16_t) (ptr + len)); // Advance write pointer
  w5500_w1 (s, W5500_S0, 1, 0x20);   // Sock0 CR -> SEND
  for (i = 0; i < 40; i++)
    {
      uint8_t ir = w5500_r1 (s, W5500_S0, 2); // Read S0 IR
      if (ir == 0)
	continue;
      // printf("IR %d, len=%d, free=%d, ptr %d\n", ir, (int) len, (int) n,
      // ptr);
      w5500_w1 (s, W5500_S0, 2, ir); // Write S0 IR: clear it!
      if (ir & 8)
	len = 0; // Timeout. Report error
      if (ir & (16 | 8))
	break; // Stop on SEND_OK or timeout
    }
  return len;
}

static bool
w5500_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_spi *s = (struct mg_tcpip_spi *) ifp->driver_data;
  s->end (s->spi);
  w5500_w1 (s, W5500_CR, 0, 0x80);    // Reset chip: CR -> 0x80
  w5500_w1 (s, W5500_CR, 0x2e, 0);    // CR PHYCFGR -> reset
  w5500_w1 (s, W5500_CR, 0x2e, 0xf8); // CR PHYCFGR -> set
  // w5500_wn(s, W5500_CR, 9, s->mac, 6);      // Set source MAC
  w5500_w1 (s, W5500_S0, 0x1e, 16);	    // Sock0 RX buf size
  w5500_w1 (s, W5500_S0, 0x1f, 16);	    // Sock0 TX buf size
  w5500_w1 (s, W5500_S0, 0, 4);		    // Sock0 MR -> MACRAW
  w5500_w1 (s, W5500_S0, 1, 1);		    // Sock0 CR -> OPEN
  return w5500_r1 (s, W5500_S0, 3) == 0x42; // Sock0 SR == MACRAW
}

static bool
w5500_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_spi *spi = (struct mg_tcpip_spi *) ifp->driver_data;
  uint8_t phycfgr = w5500_r1 (spi, W5500_CR, 0x2e);
  return phycfgr & 1; // Bit 0 of PHYCFGR is LNK (0 - down, 1 - up)
}

struct mg_tcpip_driver mg_tcpip_driver_w5500
    = { w5500_init, w5500_tx, w5500_rx, w5500_up };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/xmc.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_XMC) && MG_ENABLE_DRIVER_XMC

struct ETH_GLOBAL_TypeDef
{
  volatile uint32_t MAC_CONFIGURATION, MAC_FRAME_FILTER, HASH_TABLE_HIGH,
      HASH_TABLE_LOW, GMII_ADDRESS, GMII_DATA, FLOW_CONTROL, VLAN_TAG, VERSION,
      DEBUG, REMOTE_WAKE_UP_FRAME_FILTER, PMT_CONTROL_STATUS, RESERVED[2],
      INTERRUPT_STATUS, INTERRUPT_MASK, MAC_ADDRESS0_HIGH, MAC_ADDRESS0_LOW,
      MAC_ADDRESS1_HIGH, MAC_ADDRESS1_LOW, MAC_ADDRESS2_HIGH, MAC_ADDRESS2_LOW,
      MAC_ADDRESS3_HIGH, MAC_ADDRESS3_LOW, RESERVED1[40], MMC_CONTROL,
      MMC_RECEIVE_INTERRUPT, MMC_TRANSMIT_INTERRUPT,
      MMC_RECEIVE_INTERRUPT_MASK, MMC_TRANSMIT_INTERRUPT_MASK,
      TX_STATISTICS[26], RESERVED2, RX_STATISTICS_1[26], RESERVED3[6],
      MMC_IPC_RECEIVE_INTERRUPT_MASK, RESERVED4, MMC_IPC_RECEIVE_INTERRUPT,
      RESERVED5, RX_STATISTICS_2[30], RESERVED7[286], TIMESTAMP_CONTROL,
      SUB_SECOND_INCREMENT, SYSTEM_TIME_SECONDS, SYSTEM_TIME_NANOSECONDS,
      SYSTEM_TIME_SECONDS_UPDATE, SYSTEM_TIME_NANOSECONDS_UPDATE,
      TIMESTAMP_ADDEND, TARGET_TIME_SECONDS, TARGET_TIME_NANOSECONDS,
      SYSTEM_TIME_HIGHER_WORD_SECONDS, TIMESTAMP_STATUS, PPS_CONTROL,
      RESERVED8[564], BUS_MODE, TRANSMIT_POLL_DEMAND, RECEIVE_POLL_DEMAND,
      RECEIVE_DESCRIPTOR_LIST_ADDRESS, TRANSMIT_DESCRIPTOR_LIST_ADDRESS,
      STATUS, OPERATION_MODE, INTERRUPT_ENABLE,
      MISSED_FRAME_AND_BUFFER_OVERFLOW_COUNTER,
      RECEIVE_INTERRUPT_WATCHDOG_TIMER, RESERVED9, AHB_STATUS, RESERVED10[6],
      CURRENT_HOST_TRANSMIT_DESCRIPTOR, CURRENT_HOST_RECEIVE_DESCRIPTOR,
      CURRENT_HOST_TRANSMIT_BUFFER_ADDRESS,
      CURRENT_HOST_RECEIVE_BUFFER_ADDRESS, HW_FEATURE;
};

#  undef ETH0
#  define ETH0 ((struct ETH_GLOBAL_TypeDef *) 0x5000C000UL)

#  define ETH_PKT_SIZE 1536 // Max frame size
#  define ETH_DESC_CNT 4    // Descriptors count
#  define ETH_DS 4	    // Descriptor size (words)

static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE];
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE];
static uint32_t s_rxdesc[ETH_DESC_CNT][ETH_DS]; // RX descriptors
static uint32_t s_txdesc[ETH_DESC_CNT][ETH_DS]; // TX descriptors
static uint8_t s_txno;				// Current TX descriptor
static uint8_t s_rxno;				// Current RX descriptor

static struct mg_tcpip_if *s_ifp; // MIP interface
enum
{
  MG_PHY_ADDR = 0,
  MG_PHYREG_BCR = 0,
  MG_PHYREG_BSR = 1
};

static uint16_t
eth_read_phy (uint8_t addr, uint8_t reg)
{
  ETH0->GMII_ADDRESS = (ETH0->GMII_ADDRESS & 0x3c) | ((uint32_t) addr << 11)
		       | ((uint32_t) reg << 6) | 1;
  while ((ETH0->GMII_ADDRESS & 1) != 0)
    (void) 0;
  return (uint16_t) (ETH0->GMII_DATA & 0xffff);
}

static void
eth_write_phy (uint8_t addr, uint8_t reg, uint16_t val)
{
  ETH0->GMII_DATA = val;
  ETH0->GMII_ADDRESS = (ETH0->GMII_ADDRESS & 0x3c) | ((uint32_t) addr << 11)
		       | ((uint32_t) reg << 6) | 3;
  while ((ETH0->GMII_ADDRESS & 1) != 0)
    (void) 0;
}

static uint32_t
get_clock_rate (struct mg_tcpip_driver_xmc_data *d)
{
  if (d->mdc_cr == -1)
    {
      // assume ETH clock is 60MHz by default
      // then according to 13.2.8.1, we need to set value 3
      return 3;
    }

  return d->mdc_cr;
}

static bool
mg_tcpip_driver_xmc_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_xmc_data *d
      = (struct mg_tcpip_driver_xmc_data *) ifp->driver_data;
  s_ifp = ifp;

  // reset MAC
  ETH0->BUS_MODE |= 1;
  while (ETH0->BUS_MODE & 1)
    (void) 0;

  // set clock rate
  ETH0->GMII_ADDRESS = get_clock_rate (d) << 2;

  // init phy
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  mg_phy_init (&phy, d->phy_addr, MG_PHY_CLOCKS_MAC);

  // configure MAC: DO, DM, FES, TC
  ETH0->MAC_CONFIGURATION
      = MG_BIT (13) | MG_BIT (11) | MG_BIT (14) | MG_BIT (24);

  // set the MAC address
  ETH0->MAC_ADDRESS0_HIGH = MG_U32 (0, 0, ifp->mac[5], ifp->mac[4]);
  ETH0->MAC_ADDRESS0_LOW
      = MG_U32 (ifp->mac[3], ifp->mac[2], ifp->mac[1], ifp->mac[0]);

  // Configure the receive filter
  ETH0->MAC_FRAME_FILTER = MG_BIT (10) | MG_BIT (2); // HFP, HMC
  // Disable flow control
  ETH0->FLOW_CONTROL = 0;
  // Enable store and forward mode
  ETH0->OPERATION_MODE = MG_BIT (25) | MG_BIT (21); // RSF, TSF

  // Configure DMA bus mode (AAL, USP, RPBL, PBL)
  ETH0->BUS_MODE = MG_BIT (25) | MG_BIT (23) | (32 << 17) | (32 << 8);

  // init RX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i][0] = MG_BIT (31); // OWN descriptor
      s_rxdesc[i][1] = MG_BIT (14) | ETH_PKT_SIZE;
      s_rxdesc[i][2] = (uint32_t) s_rxbuf[i];
      if (i == ETH_DESC_CNT - 1)
	{
	  s_rxdesc[i][3] = (uint32_t) &s_rxdesc[0][0];
	}
      else
	{
	  s_rxdesc[i][3] = (uint32_t) &s_rxdesc[i + 1][0];
	}
    }
  ETH0->RECEIVE_DESCRIPTOR_LIST_ADDRESS = (uint32_t) &s_rxdesc[0][0];

  // init TX descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_txdesc[i][0] = MG_BIT (30) | MG_BIT (20);
      s_txdesc[i][2] = (uint32_t) s_txbuf[i];
      if (i == ETH_DESC_CNT - 1)
	{
	  s_txdesc[i][3] = (uint32_t) &s_txdesc[0][0];
	}
      else
	{
	  s_txdesc[i][3] = (uint32_t) &s_txdesc[i + 1][0];
	}
    }
  ETH0->TRANSMIT_DESCRIPTOR_LIST_ADDRESS = (uint32_t) &s_txdesc[0][0];

  // Clear interrupts
  ETH0->STATUS = 0xFFFFFFFF;

  // Disable MAC interrupts
  ETH0->MMC_TRANSMIT_INTERRUPT_MASK = 0xFFFFFFFF;
  ETH0->MMC_RECEIVE_INTERRUPT_MASK = 0xFFFFFFFF;
  ETH0->MMC_IPC_RECEIVE_INTERRUPT_MASK = 0xFFFFFFFF;
  ETH0->INTERRUPT_MASK = MG_BIT (9) | MG_BIT (3); // TSIM, PMTIM

  // Enable interrupts (NIE, RIE, TIE)
  ETH0->INTERRUPT_ENABLE = MG_BIT (16) | MG_BIT (6) | MG_BIT (0);

  // Enable MAC transmission and reception (TE, RE)
  ETH0->MAC_CONFIGURATION |= MG_BIT (3) | MG_BIT (2);
  // Enable DMA transmission and reception (ST, SR)
  ETH0->OPERATION_MODE |= MG_BIT (13) | MG_BIT (1);
  return true;
}

static size_t
mg_tcpip_driver_xmc_tx (const void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  if (len > sizeof (s_txbuf[s_txno]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = 0; // Frame is too big
    }
  else if ((s_txdesc[s_txno][0] & MG_BIT (31)))
    {
      ifp->nerr++;
      MG_ERROR (("No free descriptors"));
      len = 0; // All descriptors are busy, fail
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);
      s_txdesc[s_txno][1] = len;
      // Table 13-19 Transmit Descriptor Word 0 (IC, LS, FS, TCH)
      s_txdesc[s_txno][0]
	  = MG_BIT (30) | MG_BIT (29) | MG_BIT (28) | MG_BIT (20);
      s_txdesc[s_txno][0] |= MG_BIT (31); // OWN bit: handle control to DMA
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }

  // Resume processing
  ETH0->STATUS = MG_BIT (2); // clear Transmit unavailable
  ETH0->TRANSMIT_POLL_DEMAND = 0;
  return len;
}

static bool
mg_tcpip_driver_xmc_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_xmc_data *d
      = (struct mg_tcpip_driver_xmc_data *) ifp->driver_data;
  uint8_t speed = MG_PHY_SPEED_10M;
  bool up = false, full_duplex = false;
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  up = mg_phy_up (&phy, d->phy_addr, &full_duplex, &speed);
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      MG_DEBUG (("Link is %uM %s-duplex", speed == MG_PHY_SPEED_10M ? 10 : 100,
		 full_duplex ? "full" : "half"));
    }
  return up;
}

void ETH0_IRQHandler (void);
void
ETH0_IRQHandler (void)
{
  uint32_t irq_status = ETH0->STATUS;

  // check if a frame was received
  if (irq_status & MG_BIT (6))
    {
      for (uint8_t i = 0; i < ETH_DESC_CNT; i++)
	{
	  if ((s_rxdesc[s_rxno][0] & MG_BIT (31)) == 0)
	    {
	      size_t len = (s_rxdesc[s_rxno][0] & 0x3fff0000) >> 16;
	      mg_tcpip_qwrite (s_rxbuf[s_rxno], len, s_ifp);
	      s_rxdesc[s_rxno][0]
		  = MG_BIT (31); // OWN bit: handle control to DMA
	      // Resume processing
	      ETH0->STATUS = MG_BIT (7) | MG_BIT (6); // clear RU and RI
	      ETH0->RECEIVE_POLL_DEMAND = 0;
	      if (++s_rxno >= ETH_DESC_CNT)
		s_rxno = 0;
	    }
	}
      ETH0->STATUS = MG_BIT (6);
    }

  // clear Successful transmission interrupt
  if (irq_status & 1)
    {
      ETH0->STATUS = 1;
    }

  // clear normal interrupt
  if (irq_status & MG_BIT (16))
    {
      ETH0->STATUS = MG_BIT (16);
    }
}

struct mg_tcpip_driver mg_tcpip_driver_xmc
    = { mg_tcpip_driver_xmc_init, mg_tcpip_driver_xmc_tx, NULL,
	mg_tcpip_driver_xmc_up };
#endif

#ifdef MG_ENABLE_LINES
#  line 1 "src/drivers/xmc7.c"
#endif

#if MG_ENABLE_TCPIP && defined(MG_ENABLE_DRIVER_XMC7) && MG_ENABLE_DRIVER_XMC7

struct ETH_Type
{
  volatile uint32_t CTL, STATUS, RESERVED[1022], NETWORK_CONTROL,
      NETWORK_CONFIG, NETWORK_STATUS, USER_IO_REGISTER, DMA_CONFIG,
      TRANSMIT_STATUS, RECEIVE_Q_PTR, TRANSMIT_Q_PTR, RECEIVE_STATUS,
      INT_STATUS, INT_ENABLE, INT_DISABLE, INT_MASK, PHY_MANAGEMENT,
      PAUSE_TIME, TX_PAUSE_QUANTUM, PBUF_TXCUTTHRU, PBUF_RXCUTTHRU,
      JUMBO_MAX_LENGTH, EXTERNAL_FIFO_INTERFACE, RESERVED1, AXI_MAX_PIPELINE,
      RSC_CONTROL, INT_MODERATION, SYS_WAKE_TIME, RESERVED2[7], HASH_BOTTOM,
      HASH_TOP, SPEC_ADD1_BOTTOM, SPEC_ADD1_TOP, SPEC_ADD2_BOTTOM,
      SPEC_ADD2_TOP, SPEC_ADD3_BOTTOM, SPEC_ADD3_TOP, SPEC_ADD4_BOTTOM,
      SPEC_ADD4_TOP, SPEC_TYPE1, SPEC_TYPE2, SPEC_TYPE3, SPEC_TYPE4,
      WOL_REGISTER, STRETCH_RATIO, STACKED_VLAN, TX_PFC_PAUSE,
      MASK_ADD1_BOTTOM, MASK_ADD1_TOP, DMA_ADDR_OR_MASK, RX_PTP_UNICAST,
      TX_PTP_UNICAST, TSU_NSEC_CMP, TSU_SEC_CMP, TSU_MSB_SEC_CMP,
      TSU_PTP_TX_MSB_SEC, TSU_PTP_RX_MSB_SEC, TSU_PEER_TX_MSB_SEC,
      TSU_PEER_RX_MSB_SEC, DPRAM_FILL_DBG, REVISION_REG, OCTETS_TXED_BOTTOM,
      OCTETS_TXED_TOP, FRAMES_TXED_OK, BROADCAST_TXED, MULTICAST_TXED,
      PAUSE_FRAMES_TXED, FRAMES_TXED_64, FRAMES_TXED_65, FRAMES_TXED_128,
      FRAMES_TXED_256, FRAMES_TXED_512, FRAMES_TXED_1024, FRAMES_TXED_1519,
      TX_UNDERRUNS, SINGLE_COLLISIONS, MULTIPLE_COLLISIONS,
      EXCESSIVE_COLLISIONS, LATE_COLLISIONS, DEFERRED_FRAMES, CRS_ERRORS,
      OCTETS_RXED_BOTTOM, OCTETS_RXED_TOP, FRAMES_RXED_OK, BROADCAST_RXED,
      MULTICAST_RXED, PAUSE_FRAMES_RXED, FRAMES_RXED_64, FRAMES_RXED_65,
      FRAMES_RXED_128, FRAMES_RXED_256, FRAMES_RXED_512, FRAMES_RXED_1024,
      FRAMES_RXED_1519, UNDERSIZE_FRAMES, EXCESSIVE_RX_LENGTH, RX_JABBERS,
      FCS_ERRORS, RX_LENGTH_ERRORS, RX_SYMBOL_ERRORS, ALIGNMENT_ERRORS,
      RX_RESOURCE_ERRORS, RX_OVERRUNS, RX_IP_CK_ERRORS, RX_TCP_CK_ERRORS,
      RX_UDP_CK_ERRORS, AUTO_FLUSHED_PKTS, RESERVED3, TSU_TIMER_INCR_SUB_NSEC,
      TSU_TIMER_MSB_SEC, TSU_STROBE_MSB_SEC, TSU_STROBE_SEC, TSU_STROBE_NSEC,
      TSU_TIMER_SEC, TSU_TIMER_NSEC, TSU_TIMER_ADJUST, TSU_TIMER_INCR,
      TSU_PTP_TX_SEC, TSU_PTP_TX_NSEC, TSU_PTP_RX_SEC, TSU_PTP_RX_NSEC,
      TSU_PEER_TX_SEC, TSU_PEER_TX_NSEC, TSU_PEER_RX_SEC, TSU_PEER_RX_NSEC,
      PCS_CONTROL, PCS_STATUS, RESERVED4[2], PCS_AN_ADV, PCS_AN_LP_BASE,
      PCS_AN_EXP, PCS_AN_NP_TX, PCS_AN_LP_NP, RESERVED5[6], PCS_AN_EXT_STATUS,
      RESERVED6[8], TX_PAUSE_QUANTUM1, TX_PAUSE_QUANTUM2, TX_PAUSE_QUANTUM3,
      RESERVED7, RX_LPI, RX_LPI_TIME, TX_LPI, TX_LPI_TIME, DESIGNCFG_DEBUG1,
      DESIGNCFG_DEBUG2, DESIGNCFG_DEBUG3, DESIGNCFG_DEBUG4, DESIGNCFG_DEBUG5,
      DESIGNCFG_DEBUG6, DESIGNCFG_DEBUG7, DESIGNCFG_DEBUG8, DESIGNCFG_DEBUG9,
      DESIGNCFG_DEBUG10, RESERVED8[22], SPEC_ADD5_BOTTOM, SPEC_ADD5_TOP,
      RESERVED9[60], SPEC_ADD36_BOTTOM, SPEC_ADD36_TOP, INT_Q1_STATUS,
      INT_Q2_STATUS, INT_Q3_STATUS, RESERVED10[11], INT_Q15_STATUS, RESERVED11,
      TRANSMIT_Q1_PTR, TRANSMIT_Q2_PTR, TRANSMIT_Q3_PTR, RESERVED12[11],
      TRANSMIT_Q15_PTR, RESERVED13, RECEIVE_Q1_PTR, RECEIVE_Q2_PTR,
      RECEIVE_Q3_PTR, RESERVED14[3], RECEIVE_Q7_PTR, RESERVED15,
      DMA_RXBUF_SIZE_Q1, DMA_RXBUF_SIZE_Q2, DMA_RXBUF_SIZE_Q3, RESERVED16[3],
      DMA_RXBUF_SIZE_Q7, CBS_CONTROL, CBS_IDLESLOPE_Q_A, CBS_IDLESLOPE_Q_B,
      UPPER_TX_Q_BASE_ADDR, TX_BD_CONTROL, RX_BD_CONTROL, UPPER_RX_Q_BASE_ADDR,
      RESERVED17[2], HIDDEN_REG0, HIDDEN_REG1, HIDDEN_REG2, HIDDEN_REG3,
      RESERVED18[2], HIDDEN_REG4, HIDDEN_REG5;
};

#  define ETH0 ((struct ETH_Type *) 0x40490000)

#  define ETH_PKT_SIZE 1536 // Max frame size
#  define ETH_DESC_CNT 4    // Descriptors count
#  define ETH_DS 2	    // Descriptor size (words)

static uint8_t s_rxbuf[ETH_DESC_CNT][ETH_PKT_SIZE];
static uint8_t s_txbuf[ETH_DESC_CNT][ETH_PKT_SIZE];
static uint32_t s_rxdesc[ETH_DESC_CNT][ETH_DS]; // RX descriptors
static uint32_t s_txdesc[ETH_DESC_CNT][ETH_DS]; // TX descriptors
static uint8_t s_txno;				// Current TX descriptor
static uint8_t s_rxno;				// Current RX descriptor

static struct mg_tcpip_if *s_ifp; // MIP interface
enum
{
  MG_PHY_ADDR = 0,
  MG_PHYREG_BCR = 0,
  MG_PHYREG_BSR = 1
};

static uint16_t
eth_read_phy (uint8_t addr, uint8_t reg)
{
  // WRITE1, READ OPERATION, PHY, REG, WRITE10
  ETH0->PHY_MANAGEMENT = MG_BIT (30) | MG_BIT (29) | ((addr & 0xf) << 24)
			 | ((reg & 0x1f) << 18) | MG_BIT (17);
  while ((ETH0->NETWORK_STATUS & MG_BIT (2)) == 0)
    (void) 0;
  return ETH0->PHY_MANAGEMENT & 0xffff;
}

static void
eth_write_phy (uint8_t addr, uint8_t reg, uint16_t val)
{
  ETH0->PHY_MANAGEMENT = MG_BIT (30) | MG_BIT (28) | ((addr & 0xf) << 24)
			 | ((reg & 0x1f) << 18) | MG_BIT (17) | val;
  while ((ETH0->NETWORK_STATUS & MG_BIT (2)) == 0)
    (void) 0;
}

static uint32_t
get_clock_rate (struct mg_tcpip_driver_xmc7_data *d)
{
  // see ETH0 -> NETWORK_CONFIG register
  (void) d;
  return 3;
}

static bool
mg_tcpip_driver_xmc7_init (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_xmc7_data *d
      = (struct mg_tcpip_driver_xmc7_data *) ifp->driver_data;
  s_ifp = ifp;

  // enable controller, set RGMII mode
  ETH0->CTL = MG_BIT (31) | 2;

  uint32_t cr = get_clock_rate (d);
  // set NSP change, ignore RX FCS, data bus width, clock rate
  // frame length 1536, full duplex, speed
  ETH0->NETWORK_CONFIG = MG_BIT (29) | MG_BIT (26) | MG_BIT (21)
			 | ((cr & 7) << 18) | MG_BIT (8) | MG_BIT (4)
			 | MG_BIT (1) | MG_BIT (0);

  // config DMA settings: Force TX burst, Discard on Error, set RX buffer size
  // to 1536, TX_PBUF_SIZE, RX_PBUF_SIZE, AMBA_BURST_LENGTH
  ETH0->DMA_CONFIG
      = MG_BIT (26) | MG_BIT (24) | (0x18 << 16) | MG_BIT (10) | (3 << 8) | 4;

  // initialize descriptors
  for (int i = 0; i < ETH_DESC_CNT; i++)
    {
      s_rxdesc[i][0] = (uint32_t) s_rxbuf[i];
      if (i == ETH_DESC_CNT - 1)
	{
	  s_rxdesc[i][0] |= MG_BIT (1); // mark last descriptor
	}

      s_txdesc[i][0] = (uint32_t) s_txbuf[i];
      s_txdesc[i][1] = MG_BIT (31); // OWN descriptor
      if (i == ETH_DESC_CNT - 1)
	{
	  s_txdesc[i][1] |= MG_BIT (30); // mark last descriptor
	}
    }
  ETH0->RECEIVE_Q_PTR = (uint32_t) s_rxdesc;
  ETH0->TRANSMIT_Q_PTR = (uint32_t) s_txdesc;

  // disable other queues
  ETH0->TRANSMIT_Q2_PTR = 1;
  ETH0->TRANSMIT_Q1_PTR = 1;
  ETH0->RECEIVE_Q2_PTR = 1;
  ETH0->RECEIVE_Q1_PTR = 1;

  // enable interrupts (TX and RX complete)
  ETH0->INT_ENABLE = MG_BIT (7) | MG_BIT (1);

  // set MAC address
  ETH0->SPEC_ADD1_BOTTOM
      = ifp->mac[3] << 24 | ifp->mac[2] << 16 | ifp->mac[1] << 8 | ifp->mac[0];
  ETH0->SPEC_ADD1_TOP = ifp->mac[5] << 8 | ifp->mac[4];

  // enable MDIO, TX, RX
  ETH0->NETWORK_CONTROL = MG_BIT (4) | MG_BIT (3) | MG_BIT (2);

  // start transmission
  ETH0->NETWORK_CONTROL |= MG_BIT (9);

  // init phy
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  mg_phy_init (&phy, d->phy_addr, MG_PHY_CLOCKS_MAC);

  (void) d;
  return true;
}

static size_t
mg_tcpip_driver_xmc7_tx (const void *buf, size_t len, struct mg_tcpip_if *ifp)
{
  if (len > sizeof (s_txbuf[s_txno]))
    {
      MG_ERROR (("Frame too big, %ld", (long) len));
      len = 0; // Frame is too big
    }
  else if (((s_txdesc[s_txno][1] & MG_BIT (31)) == 0))
    {
      ifp->nerr++;
      MG_ERROR (("No free descriptors"));
      len = 0; // All descriptors are busy, fail
    }
  else
    {
      memcpy (s_txbuf[s_txno], buf, len);
      s_txdesc[s_txno][1] = (s_txno == ETH_DESC_CNT - 1 ? MG_BIT (30) : 0)
			    | MG_BIT (15) | len; // Last buffer and length

      ETH0->NETWORK_CONTROL |= MG_BIT (9); // enable transmission
      if (++s_txno >= ETH_DESC_CNT)
	s_txno = 0;
    }

  MG_DSB ();
  ETH0->TRANSMIT_STATUS = ETH0->TRANSMIT_STATUS;
  ETH0->NETWORK_CONTROL |= MG_BIT (9); // enable transmission

  return len;
}

static bool
mg_tcpip_driver_xmc7_up (struct mg_tcpip_if *ifp)
{
  struct mg_tcpip_driver_xmc7_data *d
      = (struct mg_tcpip_driver_xmc7_data *) ifp->driver_data;
  uint8_t speed = MG_PHY_SPEED_10M;
  bool up = false, full_duplex = false;
  struct mg_phy phy = { eth_read_phy, eth_write_phy };
  up = mg_phy_up (&phy, d->phy_addr, &full_duplex, &speed);
  if ((ifp->state == MG_TCPIP_STATE_DOWN) && up)
    { // link state just went up
      if (speed == MG_PHY_SPEED_1000M)
	{
	  ETH0->NETWORK_CONFIG |= MG_BIT (10);
	}
      MG_DEBUG (("Link is %uM %s-duplex",
		 speed == MG_PHY_SPEED_10M
		     ? 10
		     : (speed == MG_PHY_SPEED_100M ? 100 : 1000),
		 full_duplex ? "full" : "half"));
    }
  (void) d;
  return up;
}

void
ETH_IRQHandler (void)
{
  uint32_t irq_status = ETH0->INT_STATUS;
  if (irq_status & MG_BIT (1))
    {
      for (uint8_t i = 0; i < ETH_DESC_CNT; i++)
	{
	  if (s_rxdesc[s_rxno][0] & MG_BIT (0))
	    {
	      size_t len = s_rxdesc[s_rxno][1] & (MG_BIT (13) - 1);
	      // MG_INFO(("Receive complete: %ld bytes", len));
	      mg_tcpip_qwrite (s_rxbuf[s_rxno], len, s_ifp);
	      s_rxdesc[s_rxno][0]
		  &= ~MG_BIT (0); // OWN bit: handle control to DMA
	      if (++s_rxno >= ETH_DESC_CNT)
		s_rxno = 0;
	    }
	}
    }

  ETH0->INT_STATUS = irq_status;
}

struct mg_tcpip_driver mg_tcpip_driver_xmc7
    = { mg_tcpip_driver_xmc7_init, mg_tcpip_driver_xmc7_tx, NULL,
	mg_tcpip_driver_xmc7_up };
#endif
