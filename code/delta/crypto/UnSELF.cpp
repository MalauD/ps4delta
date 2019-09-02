
// Copyright (C) Force67 2019

#include <vector>
#include "UnSELF.h"

namespace crypto
{
	using namespace loaders;

	class ElfBuilder
	{
		// in file
		utl::FileHandle &file;

		ELFHeader elf{};
		std::vector<ELFPgHeader> sections;
		std::vector<uint8_t> data;

	public:

		explicit ElfBuilder(utl::FileHandle &f) :
			file(f)
		{}

		bool LoadHeaders(SELFHeader &self)
		{
			size_t headerDelta = sizeof(SELFHeader) + (sizeof(SELFSegmentTable) * self.numSegments);

			// get elf
			file->Seek(headerDelta, utl::seekMode::seek_set);
			if (!file->Read(elf))
				return false;

			// valid elf?
			if (elf.magic != 0x464C457F)
				return false;

			// read section info
			sections.resize(elf.phnum);
			return file->Read(sections);
		}

		bool LoadData()
		{
			// align position
			uint64_t pos = (file->Tell() + 0xF) & ~0xF;
	
			// todo: check is always 65?
			uint64_t offset = file->Seek(pos + 0x41, utl::seekMode::seek_set);

#ifdef DELTA_DBG
			std::printf(__FUNCTION__ " offset %llx\n", offset);
#endif

			if (offset < file->GetSize()) {

				size_t delta = file->GetSize() - offset;
				data.resize(delta);
			
				return file->Read(data);
			}

			return false;
		}

		// find a better way
		void ExportBuffer(std::vector<uint8_t> &out) {
			
			// the delta to reserve
			size_t delta = sizeof(ELFHeader) + (elf.phnum * sizeof(ELFPgHeader)) + data.size();
			out.reserve(delta);

			//std::memcpy()
		}

		bool MakeELFFile(const std::wstring &name)
		{
			utl::File file(name, utl::fileMode::write);
			if (file.IsOpen()) {
				file.Write(elf);
				file.Write(sections);
				file.Write(data);

				return true;
			}

			return false;
		}
	};

	// todo: make less shit
	bool convert_self(utl::FileHandle &file, std::vector<uint8_t> &&out)
	{
		// reset
		file->Seek(0, utl::seekMode::seek_set);

		SELFHeader self{};
		file->Read(self);

		if (self.magic == SELF_MAGIC /*&& NotDebugging()*/) {
			ElfBuilder builder(file);

			if (!builder.LoadHeaders(self)) {
				std::puts("Unable to parse SELF headers!");
				return false;
			}

			if (!builder.LoadData()) {
				std::puts("Unable to gather ELF data!");
				return false;
			}

			// export it for us
			builder.ExportBuffer(std::move(out));

			return true;
		}

		return false;
	}

	bool convert_self(utl::FileHandle& file, const std::wstring& to)
	{
		// reset
		file->Seek(0, utl::seekMode::seek_set);

		SELFHeader self{};
		file->Read(self);

		if (self.magic == SELF_MAGIC) {
			ElfBuilder builder(file);

			if (!builder.LoadHeaders(self)) {
				std::puts("Unable to parse SELF headers!");
				return false;
			}

			if (!builder.LoadData()) {
				std::puts("Unable to gather ELF data!");
				return false;
			}

			return builder.MakeELFFile(to);
		}

		return false;
	}
}