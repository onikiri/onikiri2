// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>

#include "Emu/Utility/System/Loader/ElfReader.h"

#include <ios>
#include <algorithm>
#include <cstring>
#include <limits>

#include "SysDeps/Endian.h"

using namespace std;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;

ElfReader::ElfReader()
{
    m_sectionNameTable = 0;
    m_bigEndian = false;
}

ElfReader::~ElfReader()
{
    delete[] m_sectionNameTable;
}

void ElfReader::Open(const char *name)
{
    m_file.open(name, ios_base::in | ios_base::binary);
    if (m_file.fail()) {
        stringstream ss;
        ss << "'" << name << "' : cannot open file";
        throw runtime_error(ss.str());
    }

    try {
        ReadELFHeader();
        ReadSectionHeaders();
        ReadProgramHeaders();
        ReadSectionNameTable();
    }
    catch (runtime_error& e) {
        m_file.close();

        stringstream ss;
        ss << "'" << name << "' : " << e.what();
        throw runtime_error(ss.str());
    }
}

void ElfReader::ReadELFHeader()
{
    m_file.read((char *)&m_elfHeader, sizeof(m_elfHeader));
    if (m_file.fail())
        throw runtime_error("cannot read ELF header");

    // ELF64かどうかチェック
    static const int CLASS_ELF64 = 2;
    if (!equal(&m_elfHeader.e_ident[0], &m_elfHeader.e_ident[3], "\177ELF") && m_elfHeader.e_ident[4] != CLASS_ELF64)
        throw runtime_error("not a valid ELF file");

    // エンディアンの検出
    static const int DATA_LE = 1;
    static const int DATA_BE = 2;
    switch (m_elfHeader.e_ident[5]) {
    case DATA_LE:
        m_bigEndian = false;
        break;
    case DATA_BE:
        m_bigEndian = true;
        break;
    default:
        throw runtime_error("unknown endian type");
    }

    EndianSpecifiedToHostInPlace(m_elfHeader, m_bigEndian);

    // ヘッダのサイズが想定値に一致しないならエラーにしてしまう手抜き
    if (m_elfHeader.e_ehsize != sizeof(m_elfHeader) ||
        m_elfHeader.e_shentsize != sizeof(Elf_Shdr) ||
        m_elfHeader.e_phentsize != sizeof(Elf_Phdr))
    {
        throw runtime_error("invalid header size");
    }
}

void ElfReader::ReadSectionHeaders()
{
    const char* readError = "cannot read section headers";

    // セクションヘッダがない
    if (m_elfHeader.e_shoff == 0)
        return;

    m_file.seekg(static_cast<istream::off_type>(m_elfHeader.e_shoff), ios_base::beg);
    if (m_file.fail())
        throw runtime_error(readError);


    m_elfSectionHeaders.clear();
    for (int i = 0; i < m_elfHeader.e_shnum; i ++) {
        Elf_Shdr sh;

        m_file.read((char*)&sh, sizeof(sh));
        EndianSpecifiedToHostInPlace(sh, m_bigEndian);
        m_elfSectionHeaders.push_back(sh);
    }

    if (m_file.fail())
        throw runtime_error(readError);
}

void ElfReader::ReadProgramHeaders()
{
    const char* readError = "cannot read program headers";

    // プログラムヘッダがない
    if (m_elfHeader.e_phoff == 0)
        return;

    m_file.seekg(static_cast<istream::off_type>(m_elfHeader.e_phoff), ios_base::beg);
    if (m_file.fail())
        throw runtime_error(readError);

    m_elfProgramHeaders.clear();
    for (int i = 0; i < m_elfHeader.e_phnum; i ++) {
        Elf_Phdr ph;

        m_file.read((char*)&ph, sizeof(ph));
        EndianSpecifiedToHostInPlace(ph, m_bigEndian);
        m_elfProgramHeaders.push_back(ph);
    }

    if (m_file.fail())
        throw runtime_error(readError);
}


void ElfReader::ReadSectionNameTable()
{
    if (m_elfHeader.e_shstrndx >= GetSectionHeaderCount())
        throw runtime_error("no section name table found");

    int shstrndx = m_elfHeader.e_shstrndx;
    int shstrsize = static_cast<int>( GetSectionHeader(shstrndx).sh_size );

    delete[] m_sectionNameTable;
    m_sectionNameTable = new char[ shstrsize ];
    ReadSectionBody(shstrndx, m_sectionNameTable, shstrsize);
}

void ElfReader::Close()
{
    m_file.close();
}

void ElfReader::ReadSectionBody(int index, char *buf, size_t buf_size) const
{
    const Elf_Shdr &sh = GetSectionHeader(index);
    if (buf_size != sh.sh_size)
        throw runtime_error("not enough buffer");

    ReadRange((size_t)sh.sh_offset, buf, (size_t)sh.sh_size);
}

void ElfReader::ReadRange(size_t offset, char *buf, size_t buf_size) const
{
    m_file.seekg(static_cast<istream::off_type>(offset), ios_base::beg);
    m_file.read(buf, static_cast<std::streamsize>(buf_size));

    if (m_file.fail())
        throw runtime_error("read error");
}

const char *ElfReader::GetSectionName(int index) const
{
    const Elf_Shdr &sh = GetSectionHeader(index);

    return m_sectionNameTable + sh.sh_name;
}

int ElfReader::FindSection(const char *name) const
{
    for (int i = 0; i < GetSectionHeaderCount(); i++) {
        if (strcmp(GetSectionName(i), name) == 0)
            return i;
    }

    return -1;
}

streamsize ElfReader::GetImageSize() const
{
    m_file.seekg(0, ios_base::end);
    return m_file.tellg();
}

void ElfReader::ReadImage(char *buf, size_t buf_size) const
{
    streamsize size = GetImageSize();
    if ((streamsize)buf_size < size)
        throw runtime_error("read error");

    m_file.seekg(0, ios_base::beg);
    m_file.read(buf, static_cast<std::streamoff>(size));

    if (m_file.fail())
        throw runtime_error("read error");
}


int ElfReader::GetClass() const
{
    return m_elfHeader.e_ident[EI_CLASS];
}

int ElfReader::GetDataEncoding() const
{
    return m_elfHeader.e_ident[EI_DATA];
}

int ElfReader::GetVersion() const
{
    return m_elfHeader.e_ident[EI_VERSION];
}

u16 ElfReader::GetMachine() const
{
    return m_elfHeader.e_machine;
}

bool ElfReader::IsBigEndian() const
{
    return m_bigEndian;
}

ElfReader::Elf_Addr ElfReader::GetEntryPoint() const
{
    return m_elfHeader.e_entry;
}

ElfReader::Elf_Off ElfReader::GetSectionHeaderOffset() const
{
    return m_elfHeader.e_shoff;
}

ElfReader::Elf_Off ElfReader::GetProgramHeaderOffset() const
{
    return m_elfHeader.e_phoff;
}

int ElfReader::GetSectionHeaderCount() const
{
    // 元々Elf32_Half = 16bitなので
    return (int)m_elfSectionHeaders.size();
}

int ElfReader::GetProgramHeaderCount() const
{
    return (int)m_elfProgramHeaders.size();
}

const ElfReader::Elf_Shdr &ElfReader::GetSectionHeader(int index) const
{
    return m_elfSectionHeaders[index];
}

const ElfReader::Elf_Phdr &ElfReader::GetProgramHeader(int index) const
{
    return m_elfProgramHeaders[index];
}
