#include "firmware/fls_structure.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    const auto path = std::filesystem::temp_directory_path() / "c3350emu_structure_test.fls";
    try {
        std::vector<unsigned char> b(0xE0 + 12 * 0x20, 0);
        b[0] = 0xE8; b[1] = 0x03;
        b[0xC8] = 11; // observed highest-index candidate => 12 records
        const char* names[] = {"SLB","CODE","DYN_EEP","SLB","SLB","PSI_RAM",
                               "EBL","SLB","PSI_FLASH","CUST_DISK","CODE","CDS"};
        for (std::size_t i=0;i<12;++i) {
            const auto p=0xE0+i*0x20;
            for (std::size_t j=0;names[i][j];++j)
                b[p+4+j]=static_cast<unsigned char>(names[i][j]);
            b[p+20]=static_cast<unsigned char>(i);
        }
        { std::ofstream o(path,std::ios::binary); o.write(reinterpret_cast<const char*>(b.data()),b.size()); }
        const auto report=firmware::decode_fls_structure(path);
        if(report.find("records parsed = 12")==std::string::npos ||
           report.find("name=PSI_FLASH")==std::string::npos ||
           report.find("name=CDS")==std::string::npos ||
           report.find("tail0=0x0000000B")==std::string::npos)
            throw std::runtime_error("Structural FLS decoder output mismatch.");
        std::filesystem::remove(path);
        return 0;
    } catch (...) {
        std::error_code ec; std::filesystem::remove(path,ec); throw;
    }
}
