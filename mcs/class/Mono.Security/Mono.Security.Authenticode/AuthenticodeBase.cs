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

internal class PeHeader
{
	internal byte[] bytes;

	internal PeHeader(int size)
	{
		bytes = new byte[size];
	}

	internal int get2(int offset)
	{
		return bytes[offset] | (bytes[offset] << 8);
	}

	internal int get4(int offset)
	{
		return get2(offset) | (get2(offset + 2) << 16);
	}

	internal void copy(byte[] source, int offset)
	{
		for (int i = 0; i < bytes.Length; ++i)
			bytes[i] = source[offset + i];
	}
}

// FIXME not used
internal class DosHeader : PeHeader
{
	internal DosHeader() : base(size) { }

	internal const int size = 64;
	internal int e_magic => get2(0);
	internal int e_lfanew => get4(60);
}

class FileHeader : PeHeader
{
	internal FileHeader() : base(size) { }
	internal const int size = 20;
	internal int SizeOfOptionalHeader => get2(16);
	internal int NumberOfSections => get2(2);
}

internal class SectionHeader : PeHeader
{
	SectionHeader() : base(size) { }

	internal const int size = 40;
	internal int VirtualSize => get4(8);
	internal int SizeOfRawData => get4(16);
	internal int PointerToRawData => get4(20);
};

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
			fileblock = new byte [4096];
		}

		internal int PEOffset {
			get {
				if (blockNo < 1)
					ReadFirstBlock ();
				return peOffset;
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
		internal const int MsdosHeaderSize = 64;
		internal const int PESignatureSize = 4;
		internal const int FileHeaderSize = 20;
		internal const int OptionalHeader = PESignatureSize + FileHeaderSize; // not optional
		internal const uint IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b;
		internal const uint IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20b;
		internal const int IMAGE_DIRECTORY_ENTRY_SECURITY = 4;

		// FIXME locals in ProcessFirstBlockHelper, captured by reference in MinSize
		internal int offset_NumberOfRvaAndSizes = 92; // or 108 for PE32+
		internal int offset_SecurityDirectory = 128; // or 144 for PE32+
		internal int numberOfSections = 0; // usually changed later
		internal int sizeOfOptionalHeader = 0; // changed later
		internal int endOfSections = 0; // usually changed later

		int MinSize() // FIXME lambda in ProcessFirstBlockHelper with reference capture
		{
			return peOffset + OptionalHeader
				+ offset_SecurityDirectory + 8
				+ numberOfSections * SectionHeader.size
				+ sizeOfOptionalHeader;

		}

		internal int ProcessFirstBlockHelper ()
		// 0: success; some historical values are preserved
		// >0: error (historical)
		// <0: internal: grow buffer and retry (mitigate historical design problem)
		{
			// locals in ProcessFirstBlockHelper
			peOffset = MsdosHeaderSize; // minimum, will typically increase
			offset_NumberOfRvaAndSizes = 92; // or 108 for 64bit
			offset_SecurityDirectory = 128; // or 144 for 64bit

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

			// Now read the section headers, find the last, and
			// laster trim file size at its end.

			var file_header = new FileHeader();
			file_header.copy (fileblock, peOffset + PESignatureSize);
			numberOfSections = file_header.NumberOfSections;
			sizeOfOptionalHeader = file_header.SizeOfOptionalHeader;

			if (numberOfSections == 0)
			{
				// No sections is unusual and nearly useless,
				// but valid, you can still sign the headers.

				// It is not clear how much to hash in the absence
				// of sections -- optional header, entire file?

				endOfSections = fs.Length;
			}
			else
			{
				// File big enough?
				if (fs.Length < MinSize()) {
					System.Console.WriteLine($"ProcessFirstBlock14 smallFile:0x{fs.Length:X} MinSize:0x{MinSize():X}");
					return -14;
				}

				// Buffer big enough? If not, grow it and retry.
				if (BlockSize < MinSize()) {
					System.Console.WriteLine($"ProcessFirstBlock15 smallBuffer BlockSize:0x{BlockSize:X} MinSize:0x{MinSize():X}");
					return -1;
				}

				// Successful read big enough? (How to get coverage?)
				if (blockLength < MinSize()) {
					System.Console.WriteLine($"ProcessFirstBlock16 smallRead:0x{blockLength:X} MinSize:0x{MinSize():X}");
					return -16;
				}

				var section = new SectionHeader();
				for (int i = 0; i < numberOfSections; ++i)
				{
					section.copy (fileblock, sizeOfOptionalHeader + i * section.size);
					endOfSections = uint.Max (endOfSections, section.SizeOfRawData + section.PointerToRawData);
				}
			}

			// If the file fits in the first block, truncate
			// the first block appropriately.

			if (endOfSections > blockLength)
				blockLength = endOfSections;

			return 0;
		}

		internal byte[] GetHash (HashAlgorithm hash)
		{
			if (blockNo < 1)
				ReadFirstBlock ();
			fs.Position = blockLength;

			// Hash the rest of the file, up to the last section.
			// It is not clear how much to hash in the absence
			// of sections -- optional header, entire file?

			long n = endOfSections - blockLength;

			// Pad out to a multiple of 8
			int addsize = (n & 7);
			if (addsize > 0)
				addsize = 8 - addsize;

			// Skip the checksum and the security directory.
			int pe = peOffset + OptionalHeader + 64;
			hash.TransformBlock (fileblock, 0, pe, fileblock, 0);
			// then skip 4 for checksum
			pe += 4;
			hash.TransformBlock (fileblock, pe, peOffset + OptionalHeader + offset_SecurityDirectory - pe, fileblock, pe);
			// then skip 8 bytes for IMAGE_DIRECTORY_ENTRY_SECURITY
			pe = peOffset + OptionalHeader + offset_SecurityDirectory;
			pe += 8;

			// hash the last part of the first (already in memory) block
			hash.TransformBlock (fileblock, pe, blockLength - pe, fileblock, pe);

			while (n > 0) {
				int m = (n > fileblock.Length) ? fileblock.Length : (int)n;
				if (fs.Read (fileblock, 0, m) != m)
					return null;
				hash.TransformBlock (fileblock, 0, m, fileblock, 0);
				n -= m;
			}

			if (addsize > 0)
				hash.TransformBlock (new byte [addsize], 0, addsize, fileblock, 0);

			hash.TransformFinalBlock (new byte [0], 0, 0);
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
