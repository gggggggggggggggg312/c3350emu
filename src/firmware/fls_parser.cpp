#include "fls_parser.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace firmware {
namespace {
std::vector<std::uint8_t> read_all(const std::filesystem::path& path,
                                   std::uintmax_t limit) {
    if (!std::filesystem::exists(path))
        throw std::runtime_error("Firmware file does not exist: " + path.string());
    if (!std::filesystem::is_regular_file(path))
        throw std::runtime_error("Firmware path is not a regular file: " + path.string());
    const auto size = std::filesystem::file_size(path);
    if (size == 0) throw std::runtime_error("Firmware file is empty.");
    if (size > limit) throw std::runtime_error("Firmware file exceeds the configured safety limit.");
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Unable to open firmware file: " + path.string());
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!in) throw std::runtime_error("Failed while reading firmware file.");
    return data;
}
std::uint32_t crc32(const std::vector<std::uint8_t>& d) {
    std::uint32_t c = 0xffffffffu;
    for (auto b : d) {
        c ^= b;
        for (int i=0;i<8;++i) c = (c >> 1) ^ (0xedb88320u & (0u-(c&1u)));
    }
    return ~c;
}
std::uint64_t fnv1a(const std::vector<std::uint8_t>& d) {
    std::uint64_t h=14695981039346656037ull;
    for (auto b:d) { h ^= b; h *= 1099511628211ull; }
    return h;
}
std::string hex_u64(std::uint64_t v, int width=16) {
    std::ostringstream s; s << "0x" << std::hex << std::uppercase
                            << std::setfill('0') << std::setw(width) << v; return s.str();
}
double entropy(const std::vector<std::uint8_t>& d, std::size_t begin, std::size_t end) {
    std::array<std::size_t,256> counts{};
    for (std::size_t i=begin;i<end;++i) ++counts[d[i]];
    const double n=static_cast<double>(end-begin); double e=0;
    for (auto c:counts) if(c) { const double p=c/n; e-=p*std::log2(p); }
    return e;
}
bool printable(std::uint8_t c) { return c>=32 && c<=126; }
}

std::string FlsParser::inspect_directory(const std::filesystem::path& path,
                                             std::uintmax_t max_file_size) {
    const auto data = read_all(path, max_file_size);
    std::ostringstream out;
    out << "C3350EMU candidate FLS directory scan\\n"
        << "======================================\\n"
        << "Input: " << path.string() << "\\n"
        << "Scan: 32-byte aligned records in the first 0x1000 bytes\\n"
        << "Pattern: four zero bytes followed by a short uppercase/underscore name\\n"
        << "NOTE: This pattern is inferred from one BSY sample, not a confirmed format specification.\\n\\n";

    std::size_t found = 0;
    const std::size_t scan_end = std::min<std::size_t>(data.size(), 0x1000);
    for (std::size_t offset = 0xE0; offset + 32 <= scan_end; offset += 0x20) {
        if (data[offset] != 0 || data[offset + 1] != 0 ||
            data[offset + 2] != 0 || data[offset + 3] != 0) {
            continue;
        }

        std::string name;
        bool valid = true;
        bool terminated = false;
        for (std::size_t i = 0; i < 12; ++i) {
            const auto ch = data[offset + 4 + i];
            if (ch == 0) {
                terminated = true;
                continue;
            }
            if (terminated || !((ch >= 'A' && ch <= 'Z') ||
                                (ch >= '0' && ch <= '9') || ch == '_')) {
                valid = false;
                break;
            }
            name.push_back(static_cast<char>(ch));
        }
        if (!valid || name.empty()) continue;

        out << "Record @ " << hex_u64(offset, 8) << "  name=" << name << "\\n"
            << "  raw little-endian words:";
        for (std::size_t word = 0; word < 8; ++word) {
            const std::size_t pos = offset + word * 4;
            const std::uint32_t value =
                static_cast<std::uint32_t>(data[pos]) |
                (static_cast<std::uint32_t>(data[pos + 1]) << 8) |
                (static_cast<std::uint32_t>(data[pos + 2]) << 16) |
                (static_cast<std::uint32_t>(data[pos + 3]) << 24);
            out << (word == 0 ? " " : ", ") << "W" << word << "=" << hex_u64(value, 8);
        }
        out << "\\n";
        ++found;
    }

    out << "\\nCandidate records found: " << found << "\\n"
        << "Word values are raw observations; no offsets, sizes, checksums, or load addresses are assigned meanings.\\n";
    return out.str();
}

FlsInfo FlsParser::inspect(const std::filesystem::path& path,
                           std::size_t header_size,
                           std::uintmax_t max_file_size) {
    auto data=read_all(path,max_file_size);
    const auto count=std::min(header_size,data.size());
    FlsInfo result;
    result.path=path; result.file_size=data.size();
    result.header.assign(data.begin(),data.begin()+static_cast<std::ptrdiff_t>(count));
    result.classification="Unclassified FLS-like binary";
    result.note="Only raw observations are reported; FLS records and addresses are not yet validated.";
    return result;
}

std::filesystem::path FlsParser::analyze(const std::filesystem::path& path,
                                         const std::filesystem::path& requested,
                                         std::uintmax_t max_file_size) {
    const auto d=read_all(path,max_file_size);
    auto outpath=requested;
    if(outpath.empty()) outpath=std::filesystem::path(path.string()+".analysis.txt");
    std::ofstream out(outpath);
    if(!out) throw std::runtime_error("Unable to create report: "+outpath.string());
    out << "C3350EMU NATIVE FIRMWARE ANALYSIS\n"
        << "=================================\n"
        << "Input: "<<path.string()<<"\nSize: "<<d.size()<<" bytes\n"
        << "CRC-32/ISO-HDLC (whole file): "<<hex_u64(crc32(d),8)<<"\n"
        << "FNV-1a 64-bit (whole file): "<<hex_u64(fnv1a(d))<<"\n"
        << "NOTE: hashes identify this sample; they are not evidence of an embedded FLS checksum.\n\n";
    out << "[FIRST 256 BYTES]\n";
    for(std::size_t i=0;i<std::min<std::size_t>(256,d.size());i+=16) {
        out<<hex_u64(i,8)<<"  ";
        for(std::size_t j=i;j<std::min(i+16,d.size());++j)
            out<<std::hex<<std::uppercase<<std::setw(2)<<std::setfill('0')<<(unsigned)d[j]<<' ';
        out<<std::dec<<"\n";
    }
    out << "\n[ENTROPY BY 4 KiB BLOCK]\n";
    for(std::size_t i=0;i<d.size();i+=4096) {
        const auto end=std::min(i+4096,d.size());
        out<<hex_u64(i,8)<<"-"<<hex_u64(end-1,8)<<"  "<<std::fixed<<std::setprecision(3)
           <<entropy(d,i,end)<<" bits/byte";
        if(entropy(d,i,end)>7.5) out<<"  (high entropy: compressed/encrypted/data candidate)";
        out<<"\n";
    }
    out << "\n[PRINTABLE STRINGS, MINIMUM 5 CHARACTERS]\n";
    std::size_t shown=0;
    for(std::size_t i=0;i<d.size() && shown<500;) {
        if(!printable(d[i])) { ++i; continue; }
        const auto start=i;
        while(i<d.size() && printable(d[i])) ++i;
        if(i-start>=5) {
            out<<hex_u64(start,8)<<"  ";
            for(auto k=start;k<i;++k) out<<static_cast<char>(d[k]);
            out<<"\n"; ++shown;
        }
    }
    if(shown==0) out<<"No qualifying ASCII strings found.\n";
    out << "\n[REPEATED BYTE RUNS, 32 BYTES OR LONGER]\n";
    bool runs=false;
    for(std::size_t i=0;i<d.size();) {
        std::size_t j=i+1; while(j<d.size() && d[j]==d[i]) ++j;
        if(j-i>=32) { out<<hex_u64(i,8)<<" length="<<(j-i)<<" value="<<hex_u64(d[i],2)<<"\n"; runs=true; }
        i=j;
    }
    if(!runs) out<<"None found.\n";
    out << "\n[ALIGNED 32-BIT WORDS THAT LOOK LIKE RAM/ROM POINTERS]\n"
        << "Heuristic ranges only; these are not confirmed address mappings.\n";
    std::size_t pointers=0;
    for(std::size_t i=0;i+3<d.size() && pointers<300;i+=4) {
        const std::uint32_t v=(std::uint32_t)d[i]|((std::uint32_t)d[i+1]<<8)|
            ((std::uint32_t)d[i+2]<<16)|((std::uint32_t)d[i+3]<<24);
        if((v>=0x00010000u && v<0x20000000u) || (v>=0x80000000u && v<0xC0000000u)) {
            out<<hex_u64(i,8)<<" -> "<<hex_u64(v,8)<<"\n"; ++pointers;
        }
    }
    if(!pointers) out<<"No candidates under the broad heuristic.\n";
    out << "\n[ARM-STATE BRANCH CANDIDATES]\n"
        << "Word scan only. A matching bit pattern does not prove executable code.\n";
    std::size_t branches=0;
    for(std::size_t i=0;i+3<d.size() && branches<300;i+=4) {
        const std::uint32_t w=(std::uint32_t)d[i]|((std::uint32_t)d[i+1]<<8)|
            ((std::uint32_t)d[i+2]<<16)|((std::uint32_t)d[i+3]<<24);
        if(((w>>25)&7u)==5u) {
            const std::int32_t imm=static_cast<std::int32_t>(w<<8)>>6;
            const std::uint32_t target=static_cast<std::uint32_t>(i+8+imm);
            out<<hex_u64(i,8)<<" instruction="<<hex_u64(w,8)
               <<" branch-target-if-file-offset="<<hex_u64(target,8)<<"\n"; ++branches;
        }
    }
    if(!branches) out<<"No ARM branch-shaped words found.\n";
    out << "\n[FORMAT CONCLUSIONS]\n"
        << "- This report does not decode the Samsung FLS container specification.\n"
        << "- No segment boundaries, load addresses, or entry points are asserted.\n"
        << "- CRC-32 above is calculated over the entire input; embedded checksum fields remain unknown.\n"
        << "- ARM-looking words and pointer-like values are leads for manual/repeated-sample analysis only.\n"
        << "- Compare reports from PSI, SLB, BSY region files, and CDS to identify common structures.\n";
    if(!out) throw std::runtime_error("Failed while writing report: "+outpath.string());
    return outpath;
}

std::string hex_bytes(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output; output<<std::hex<<std::setfill('0');
    for(std::size_t i=0;i<bytes.size();++i) { if(i) output<<' '; output<<std::setw(2)<<(unsigned)bytes[i]; }
    return output.str();
}
} // namespace firmware
