//
// AuthenticodeBase.cs: Authenticode signature base class
//
// Author:
//	Sebastien Pouliot <sebastien@ximian.com>
//
// (C) 2003 Motus Technologies Inc. (http://www.motus.com)
// Copyright (C) 2004, 2006 Novell, Inc (http://www.novell.com)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

using System;
using System.IO;
using System.Security.Cryptography;

/* FIXME

This code has a number of structural and correctness problems.

1. The original code was structured to read 4K blocks and assume
the headers are within the first 4K block. This is an invalid assumption,
but usually works, i.e. as long as there isn't a creative MS-DOS header.
The right design is to allow a few exclusions anywhere in the file, at
arbitrary offset and spanning arbitrary offsets.
To make a smaller change, the code now grows the block size to accomodate
the headers, and then resets it back to the original 4K.

2. It is likely that 4K is too small to be performant on modern systems.

3. The handling of coffSymbolTableOffset is seems bogus and incorrect.
It should be removed.

4. The code ignores this part of the specification:
	"The area past the last section (defined by highest offset) is not hashed."
It is concievable the author thought coffSymbolTableOffset is related
to this, but it is not.
*/

namespace Mono.Security.Authenticode {

	// References:
	// a.	http://www.cs.auckland.ac.nz/~pgut001/pubs/authenticode.txt

#if INSIDE_CORLIB
	internal
#else
	public
#endif
	enum Authority {
		Individual,
		Commercial,
		Maximum
	}

#if INSIDE_CORLIB
	internal
#else
	public
#endif
	class AuthenticodeBase {

		public const string spcIndirectDataContext = "1.3.6.1.4.1.311.2.1.4";

		private byte[] fileblock;
		private FileStream fs;
		private int blockNo;
		private int blockLength;
		private int peOffset;
		private int dirSecurityOffset;
		private int dirSecuritySize;
		private int coffSymbolTableOffset;

		void ResetBlockSize ()
		{
			fileblock = new byte [4096];
		}

		bool IncreaseBlockSize ()
		{
			// FIXME
			// Given the design, don't consume too much
			// memory for rare files. There is a design
			// that accomodates them better, but 4K will usually work here.
			if (BlockSize > (64 * 1024 * 1024))
				return false;
			fileblock = new byte [BlockSize * 2];
			return true;
		}

		internal int BlockSize => fileblock.Length;

		public AuthenticodeBase ()
		{
			ResetBlockSize ();
		}

		internal int PEOffset {
			get {
				if (blockNo < 1)
					ReadFirstBlock ();
				return peOffset;
			}
		}

		internal int CoffSymbolTableOffset {
			get {
				if (blockNo < 1)
					ReadFirstBlock ();
				return coffSymbolTableOffset;
			}
		}

		internal int SecurityOffset {
			get {
				if (blockNo < 1)
					ReadFirstBlock ();
				return dirSecurityOffset;
			}
		}

		internal void Open (string filename)
		{
			if (fs != null)
				Close ();
			fs = new FileStream (filename, FileMode.Open, FileAccess.Read, FileShare.Read);
			blockNo = 0;
		}

		internal void Close ()
		{
			if (fs != null) {
				fs.Close ();
				fs = null;
			}
		}

		internal void ReadFirstBlock ()
		{
			int error = ProcessFirstBlock ();
			if (error != 0) {
				string msg = Locale.GetText ("Cannot sign non PE files, e.g. .CAB or .MSI files (error {0}).", 
					error);
				throw new NotSupportedException (msg);
			}
		}

		internal int ProcessFirstBlock ()
		// Wrapper around ProcessFirstBlockHelper to void
		// indenting it.
		{
			while (true)
			{
				int error = ProcessFirstBlockHelper();
				if (error != -1)
					return error;
				if (!IncreaseBlockSize())
					return -1;
			}
		}

		// constants in ProcessFirstBlockHelper
		const int MsdosHeaderSize = 64;
		const int PESignatureSize = 4;
		const int FileHeaderSize = 20;
		const int OptionalHeader = PESignatureSize + FileHeaderSize; // not optional
		const int IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b;
		const int IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20b;
		const uint IMAGE_DIRECTORY_ENTRY_SECURITY = 4;

		// FIXME locals in ProcessFirstBlockHelper, captured by reference in MinSize
		int offset_NumberOfRvaAndSizes = 92; // or 108
		int offset_SecurityDirectory = 128; // or 144

		int MinSize() // FIXME lambda in ProcessFirstBlockHelper with reference capture
		{
			return peOffset + OptionalHeader + offset_SecurityDirectory + 8;
		}

		internal int ProcessFirstBlockHelper ()
		// 0: success; some historical values are preserved
		// >0: error (historical)
		// <0: internal: grow buffer and retry (mitigate historical design problem)
		{
/*
portable executable (PE) file is structured partly as follows:

typedef uint16_t WORD;
typedef uint32_t DWORD;

64 bytes {
  2 byte MZ
  ...
  4 byte offset to NT headers
}
NT headers {                            offset size
  DWORD                 Signature;           0 4     PE\0\0
  IMAGE_FILE_HEADER     FileHeader;          4 20
  IMAGE_OPTIONAL_HEADER OptionalHeader;     24 varies
}
The "optional" header is not actually optional.

 IMAGE_FILE_HEADER {          offset size
  WORD  Machine;		   0 2 x86, amd64, etc.
  WORD  NumberOfSections;          2 2 typically a small number like 5
  DWORD TimeDateStamp;             4 4 Unix time or a hash
  DWORD PointerToSymbolTable;      8 4 usually zero esp. in executables
  DWORD NumberOfSymbols;          12 4 usually zero esp. in executables
  WORD  SizeOfOptionalHeader;     16 2 very important
  WORD  Characteristics;          18 2 flags
                                  20
}

IMAGE_SECTION_HEADER {			offset size
  BYTE  Name[IMAGE_SIZEOF_SHORT_NAME];       0 8 .text .data .rdata, .reloc etc.
  union {
    DWORD PhysicalAddress;                   8 4 unused
    DWORD VirtualSize;                       8 4 0 or memory-relative
  } Misc;
  DWORD VirtualAddress;                     12 4 (RVA memory-relative)
  DWORD SizeOfRawData;                      16 4 (file-relative)
  DWORD PointerToRawData;                   20 4 (file-relative)
  DWORD PointerToRelocations;               24 4 (object only)
  DWORD PointerToLinenumbers;               28 4 (object only)
  WORD  NumberOfRelocations;                32 2 (object only)
  WORD  NumberOfLinenumbers;                34 2 (unused/object only)
  DWORD Characteristics;                    36 4 flags, page protection etc.
                                            40
SizeOfRawData is less than VirtualSize to mean uninitialized tail (zeroed)
SizeOfRawData is zero for pure uninitialized data
}
*/
			// locals in ProcessFirstBlockHelper
			peOffset = MsdosHeaderSize; // minimum, will typically increase
			offset_NumberOfRvaAndSizes = 92; // or 108
			offset_SecurityDirectory = 128; // or 144

			//System.Console.WriteLine("ProcessFirstBlock 1");
			if (fs == null)
				return 1;

			//System.Console.WriteLine("ProcessFirstBlock 2");

			fs.Position = 0;
			// read first block - it will include (100% sure) 
			// the MZ header and (99.9% sure) the PE header
			blockLength = fs.Read (fileblock, 0, fileblock.Length);
			blockNo = 1;
			if (blockLength < 64)
				return 2;	// invalid PE file

			//System.Console.WriteLine("ProcessFirstBlock 3");

			// 1. Validate the MZ header informations
			// 1.1. Check for magic MZ at start of header
			if (BitConverterLE.ToUInt16 (fileblock, 0) != 0x5A4D)
				return 3;

			// 1.2. Find the offset of the PE header
			peOffset = BitConverterLE.ToInt32 (fileblock, 60);
			if (peOffset > fs.Length)
				return 4;

			// File big enough?
			if (fs.Length < MinSize()) {
				System.Console.WriteLine($"ProcessFirstBlock3 smallFile:0x{fs.Length:X} MinSize:0x{MinSize():X}");
				return -3;
			}

			// Buffer big enough? If not, grow it and retry.
			if (BlockSize < MinSize()) {
				System.Console.WriteLine($"ProcessFirstBlock4 smallBuffer BlockSize:0x{BlockSize:X} MinSize:0x{MinSize():X}");
				return -1;
			}

			// Successful read big enough? (How to get coverage?)
			if (blockLength < MinSize()) {
				System.Console.WriteLine($"ProcessFirstBlock5 smallRead:0x{blockLength:X} MinSize:0x{MinSize():X}");
				return -5;
			}

			uint magic = BitConverterLE.ToUInt16 (fileblock, peOffset + OptionalHeader);
			if (magic ==  IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
				// nothing, ok
			}
			else if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
				offset_NumberOfRvaAndSizes = 108;
				offset_SecurityDirectory = 144;
			} else {
				System.Console.WriteLine ($"ProcessFirstBlock6 invalid PE/10b/20b magic number magic:0x{magic:X}");
				return -6;
			}

			// File big enough?
			if (fs.Length < MinSize()) {
				System.Console.WriteLine($"ProcessFirstBlock8 smallFile:0x{fs.Length:X} MinSize:0x{MinSize():X}");
				return -8;
			}

			// Buffer big enough? If not, grow it and retry.
			if (BlockSize < MinSize()) {
				System.Console.WriteLine($"ProcessFirstBlock9 smallBuffer BlockSize:0x{BlockSize:X} MinSize:0x{MinSize():X}");
				return -1;
			}

			// Successful read big enough? (How to get coverage?)
			if (blockLength < MinSize()) {
				System.Console.WriteLine($"ProcessFirstBlock10 smallRead:0x{blockLength:X} MinSize:0x{MinSize():X}");
				return -10;
			}

			uint numberOfRvaAndSizes = BitConverterLE.ToUInt32 (fileblock, peOffset + OptionalHeader + offset_NumberOfRvaAndSizes);
			if (numberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_SECURITY) {
				System.Console.WriteLine($"ProcessFirstBlock11 numberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_SECURITY 0x{numberOfRvaAndSizes:X}");
				return -11;
			}

			// 2. Read between DOS header and first part of PE header
			// 2.1. Check for magic PE at start of header
			//	PE - NT header ('P' 'E' 0x00 0x00)
			if (BitConverterLE.ToUInt32 (fileblock, peOffset) != 0x4550)
				return 5;

			// 2.2. Locate IMAGE_DIRECTORY_ENTRY_SECURITY (offset and size)
			dirSecurityOffset = BitConverterLE.ToInt32 (fileblock, peOffset + OptionalHeader + offset_SecurityDirectory);
			dirSecuritySize = BitConverterLE.ToInt32 (fileblock, peOffset + OptionalHeader + offset_SecurityDirectory + 4);

			if (dirSecurityOffset + dirSecuritySize > fs.Length) {
				System.Console.WriteLine($"ProcessFirstBlock13 securityDirectotyOutOfRange fs.Length:0x{fs.Length:X} dirSecurityOffset:{dirSecurityOffset} dirSecuritySize:{dirSecuritySize}");
				return -13;
			}

			// COFF symbol tables are deprecated - we'll strip them if we see them!
			// (otherwise the signature won't work on MS and we don't want to support COFF for that)
			coffSymbolTableOffset = BitConverterLE.ToInt32 (fileblock, peOffset + 12);

			// FIXME validation and testing of coffSymbolTableOffset != 0
			// I don't see why this code is even here.

			return 0;
		}

		internal byte[] GetSecurityEntry () 
		{
			if (blockNo < 1)
				ReadFirstBlock ();

			if (dirSecuritySize > 8) {
				// remove header from size (not ASN.1 based)
				byte[] secEntry = new byte [dirSecuritySize - 8];
				// position after header and read entry
				fs.Position = dirSecurityOffset + 8;
				fs.Read (secEntry, 0, secEntry.Length);
				return secEntry;
			}
			return null;
		}

		internal byte[] GetHash (HashAlgorithm hash)
		{
			if (blockNo < 1)
				ReadFirstBlock ();
			fs.Position = blockLength;

			// hash the rest of the file
			long n;
			int addsize = 0;
			// minus any authenticode signature (with 8 bytes header)
			if (dirSecurityOffset > 0) {
				// FIXME This assumes if the signature
				// starts in the first block, that it does
				// not span blocks. Typically the signature
				// is at the end.
				if (dirSecurityOffset < blockLength) {
					blockLength = dirSecurityOffset;
					n = 0;
				} else {
					n = dirSecurityOffset - blockLength;
				}
			} else if (coffSymbolTableOffset > 0) {
				// FIXME This is messed up but mitigated.
				fileblock[PEOffset + 12] = 0;
				fileblock[PEOffset + 13] = 0;
				fileblock[PEOffset + 14] = 0;
				fileblock[PEOffset + 15] = 0;
				fileblock[PEOffset + 16] = 0;
				fileblock[PEOffset + 17] = 0;
				fileblock[PEOffset + 18] = 0;
				fileblock[PEOffset + 19] = 0;

				// FIXME This is also incorrect but rarely/never occurs.
				if (coffSymbolTableOffset < blockLength) {
					blockLength = coffSymbolTableOffset;
					n = 0;
				} else {
					n = coffSymbolTableOffset - blockLength;
				}
			} else {
				addsize = (int) (fs.Length & 7);
				if (addsize > 0)
					addsize = 8 - addsize;
				
				n = fs.Length - blockLength;
			}

			// Skip the checksum and the security directory.
			int pe = peOffset + OptionalHeader + 64;
			hash.TransformBlock (fileblock, 0, pe, fileblock, 0);
			// then skip 4 for checksum
			pe += 4;
			hash.TransformBlock (fileblock, pe, peOffset + OptionalHeader + offset_SecurityDirectory - pe, fileblock, pe);
			// then skip 8 bytes for IMAGE_DIRECTORY_ENTRY_SECURITY
			pe = peOffset + OptionalHeader + offset_SecurityDirectory;
			pe += 8;

			// everything is present so start the hashing
			if (n == 0) {
				// hash the (only) block
				hash.TransformFinalBlock (fileblock, pe, blockLength - pe);
			}
			else {
				// hash the last part of the first (already in memory) block
				hash.TransformBlock (fileblock, pe, blockLength - pe, fileblock, pe);

				ResetBlockSize ();

				// hash by blocks of 4096 bytes
				long blocks = (n >> 12);
				int remainder = (int)(n - (blocks << 12));
				if (remainder == 0) {
					blocks--;
					remainder = 4096;
				}
				// blocks
				while (blocks-- > 0) {
					fs.Read (fileblock, 0, fileblock.Length);
					hash.TransformBlock (fileblock, 0, fileblock.Length, fileblock, 0);
				}
				// remainder
				if (fs.Read (fileblock, 0, remainder) != remainder)
					return null;

				if (addsize > 0) {
					hash.TransformBlock (fileblock, 0, remainder, fileblock, 0);
					hash.TransformFinalBlock (new byte [addsize], 0, addsize);
				} else {
					hash.TransformFinalBlock (fileblock, 0, remainder);
				}
			}
			return hash.Hash;
		}

		// for compatibility only
		protected byte[] HashFile (string fileName, string hashName) 
		{
			try {
				Open (fileName);
				HashAlgorithm hash = HashAlgorithm.Create (hashName);
				byte[] result = GetHash (hash);
				Close ();
				return result;
			}
			catch {
				return null;
			}
		}
	}
}
