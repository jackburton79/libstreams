#include "Stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>


class SubStreamAdapter : public Stream {
public:
	SubStreamAdapter(Stream* stream, off_t offset, size_t length)
		:
		fParent(stream),
		fOffset(offset),
		fSize(length)
	{
		assert(fOffset >= 0);
		assert(size_t(fOffset) < fParent->Size());
		assert(fSize <= fParent->Size() - fOffset);
		// TODO: The parent stream should be aware,
		// otherwise deleting it could cause leaks or memory corruption
	};
	virtual ssize_t ReadAt(off_t pos, void *dst, size_t size)
	{
		return fParent->ReadAt(fOffset + pos, dst, size);
	};
	virtual off_t Seek(off_t where, int whence)
	{
		return fParent->Seek(where + fOffset, whence) - fOffset;
	};
	virtual off_t Position() const
	{
		return fParent->Position() - fOffset;
	};

	virtual size_t Size() const
	{
		return fSize;
	}
private:
	Stream* fParent;
	off_t fOffset;
	size_t fSize;
};


// Stream
Stream::Stream()
{
}


Stream::~Stream()
{
}

	
ssize_t
Stream::Read(void *dst, size_t size)
{
	off_t curPos = Position();
	ssize_t read = ReadAt(curPos, dst, size);
	if (read > 0)
		Seek(read, SEEK_CUR);
	return read;
}


char*
Stream::ReadLine(char *buffer, size_t maxSize, char endLine)
{
	maxSize--;

	char *ptr = buffer;
	try {
		while ((*ptr = ReadByte()) != endLine
				&& (size_t)(ptr - buffer) < maxSize) {
			ptr++;
		}
	} catch (std::exception& e) {
		// eof
	}

	if (ptr > buffer) {
		*(ptr - 1) = '\0';
		return buffer;
	}

	return NULL;
}


ssize_t
Stream::Write(const void *src, size_t size)
{
	off_t curPos = Position();
	ssize_t wrote = WriteAt(curPos, src, size);
	if (wrote > 0)
		Seek(wrote, SEEK_CUR);
	return wrote;
}


uint8
Stream::ReadByte()
{
	uint8 byte;
	ssize_t read = Read(&byte, sizeof(byte));
	if (read != sizeof(byte))
		throw std::runtime_error("Stream::ReadByte(): tried to read uint8 but got less !");
	return byte;
}


uint16
Stream::ReadWordBE()
{
	uint8 result1 = ReadByte();
	uint8 result2 = ReadByte();

	return (uint16)((result1 << 8) | (result2));
}


uint16
Stream::ReadWordLE()
{
	uint16 result;
	if (Read(&result, sizeof(result)) != sizeof(result))
		throw std::runtime_error("ReadWordLE() read error");
	return result;
}


uint32
Stream::ReadDWordBE()
{
	uint16 result1 = ReadWordBE();
	uint16 result2 = ReadWordBE();

	return (uint32)((result1 << 16) | (result2));
}


uint32
Stream::ReadDWordLE()
{
	uint32 result;
	if (Read(&result, sizeof(result)) != sizeof(result))
		throw std::runtime_error("ReadDWordLE() read error");
	return result;
}


/* virtual */
Stream*
Stream::Clone() const
{
	std::cerr << "Stream::Clone() not implemented!";
	throw std::runtime_error("Stream::Clone() not implemented!");
	return NULL;
}


/* virtual */
Stream*
Stream::Adopt()
{
	std::cerr << "Stream::Adopt() not implemented!";
	throw std::runtime_error("Stream::Adopt() not implemented!");
	return NULL;
}


Stream*
Stream::SubStream(off_t offset, size_t size)
{
	return new SubStreamAdapter(this, offset, size);
}


void
Stream::Dump()
{
	off_t oldPos = Position();
	Seek(0, SEEK_SET);
	uint8 byte;
	try {
		while (true) {
			byte = ReadByte();
			std::cout << byte;
		}
	} catch (std::exception& e) {
		// eof
	}

	Seek(oldPos, SEEK_SET);
}


void
Stream::DumpToFile(const char *fileName)
{
	FILE *file = fopen(fileName, "wb");
	if (file) {
		off_t oldPos = Position();
		Seek(0, SEEK_SET);
		ssize_t read;
		char buffer[1024];
		while ((read = Read(buffer, 1024)) > 0)
			fwrite(buffer, read, 1, file);
		fclose(file);
		Seek(oldPos, SEEK_SET);
	}
}
