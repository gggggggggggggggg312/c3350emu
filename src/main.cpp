#include "cpu/cpu.hpp"
#include "firmware/fls_parser.hpp"
#include "firmware/boot_plan.hpp"
#include "platform/platform.hpp"
#include "platform/battery.hpp"
#include "memory/memory.hpp"
#include "memory/lpddr1.hpp"

#include <exception>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>
#include <vector>

static void print_help(const char* program) {
    std::cout
        << "C3350EMU - Samsung GT-C3350 research emulator\n\n"
        << "Usage:\n"
        << "  " << program << " --help\n"
        << "  " << program << " --platform-info\n"
        << "  " << program << " --lpddr1-sequence\n"
        << "  " << program << " --run-raw <file> <ram-base> <ram-size> <entry> <max-instructions>\n"
        << "  " << program << " --run-fls-segment <file.fls> <offset> <length> <ram-base> <ram-size> <load-address> <entry> <max-instructions>\n"
        << "  " << program << " --inspect-firmware <file.fls>\n"
        << "  " << program << " --analyze-firmware <file.fls> [report.txt]\n"
        << "  " << program << " --inspect-fls-directory <file.fls>\n"
        << "  " << program << " --compare-firmware-set <directory> [bytes]\n"
        << "  " << program << " --load-raw-segment <file> <offset> <length> <ram-base> <ram-size> <load-address>\n\n"
        << "The native analyzer creates a text report without Python or extra dependencies.\n"
        << "The directory scanner reports candidate 32-byte records; its layout is not yet validated.\n"
        << "Raw execution uses a user-supplied map and entry point; it is not a verified phone boot.\n";
}
static void print_platform_info() {
    std::cout << "GT-C3350 provisional hardware research profile\n"
              << "==============================================\n";
    for (const auto& fact : platform::gt_c3350_research_profile())
        std::cout << fact.label << ": " << fact.value << " [" << fact.confidence << "]\n";
    std::cout << "\nNo physical memory addresses or peripheral register offsets are assumed.\n";
}
static std::uint64_t parse_number(const char* value, const char* label) {
    const std::string text(value);
    if (text.empty() || text.front() == '-')
        throw std::runtime_error(std::string("Invalid ") + label + ".");
    std::size_t used = 0;
    const auto parsed = std::stoull(text, &used, 0);
    if (used != text.size())
        throw std::runtime_error(std::string("Invalid ") + label + ".");
    return parsed;
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { print_help(argv[0]); return 0; }
    if (argc == 2 && std::string(argv[1]) == "--platform-info") { print_platform_info(); return 0; }
    if (argc == 2 && std::string(argv[1]) == "--lpddr1-sequence") {
        memory::Lpddr1Protocol lpddr;
        const auto trace = lpddr.initialize();
        std::cout << "Generic LPDDR1 protocol trace (NOT XMM2150 register programming)\n";
        for (std::size_t i = 0; i < trace.size(); ++i)
            std::cout << (i + 1) << ". " << trace[i] << "\n";
        std::cout << "Protocol trace complete: " << (lpddr.ready() ? "ready" : "not ready") << "\n";
        std::cout << "No controller registers, physical addresses, or real delays were used.\n";
        return 0;
    }

    if (argc == 5 && std::string(argv[1]) == "--battery-demo") {
        try {
            const auto percent = parse_number(argv[2], "battery percentage");
            const auto seconds = parse_number(argv[4], "elapsed seconds");
            if (percent > 100u || seconds > 86400000u)
                throw std::runtime_error("Battery percentage must be 0-100 and time <= 24 days.");
            const std::string mode(argv[3]);
            if (mode != "charge" && mode != "discharge")
                throw std::runtime_error("Mode must be charge or discharge.");
            platform::Battery battery(1000u, static_cast<std::uint8_t>(percent));
            battery.set_charging(mode == "charge");
            battery.advance_seconds(static_cast<std::uint32_t>(seconds));
            std::cout << "C3350EMU simulated battery\\n"
                      << "Charge: " << unsigned(battery.percentage()) << "%\\n"
                      << "Voltage estimate: " << battery.voltage_mv() << " mV\\n"
                      << "Remaining: " << battery.remaining_mah() << " / "
                      << battery.capacity_mah() << " mAh\\n"
                      << "State: " << (battery.charging() ? "charging" :
                          (battery.full() ? "full" : "discharging")) << "\\n"
                      << "Model: provisional 1000 mAh, 300 mA charge / 180 mA discharge.\\n"
                      << "This is simulated telemetry, not measured GT-C3350 hardware data.\\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Battery simulation failed: " << error.what() << "\\n";
            return 1;
        }
    }

    if (argc == 3 && std::string(argv[1]) == "--run-boot-plan") {
        try {
            const auto plan = firmware::parse_boot_plan(argv[2]);
            Memory memory;
            memory.map(plan.ram_base, plan.ram_size, true, "boot-plan shared RAM");
            std::cout << "C3350EMU MULTI-REGION FIRMWARE EXECUTION\\n"
                      << "RAM: 0x" << std::hex << plan.ram_base
                      << " - 0x" << (static_cast<std::uint64_t>(plan.ram_base)+plan.ram_size-1u)
                      << "\\n";
            for (std::size_t i=0;i<plan.segments.size();++i) {
                const auto& segment=plan.segments[i];
                const auto loaded=firmware::FlsParser::load_raw_segment(
                    segment.file,segment.offset,segment.length,segment.load_address,memory);
                std::cout << "Loaded region " << std::dec << (i+1) << ": "
                          << loaded.source.filename().string()
                          << " offset=0x" << std::hex << loaded.file_offset
                          << " load=0x" << loaded.load_address
                          << " bytes=0x" << loaded.bytes_loaded
                          << " CRC32=0x" << loaded.crc32 << "\\n";
            }
            firmware::apply_memory_initialization(plan,memory);
            std::cout << "Runtime memory operations applied: " << std::dec << plan.memory_initialization.size() << "\n";
            CPU cpu(memory);
            cpu.reset(plan.entry);
            std::cout << "Starting CPU at 0x" << std::hex << plan.entry
                      << (plan.entry&1u?" (Thumb)":" (ARM)")
                      << "\\nInstruction/step limit: " << std::dec << plan.instruction_limit << "\\n";
            std::ofstream trace;
            if(!plan.trace_file.empty()) {
                trace.open(plan.trace_file,std::ios::binary);
                if(!trace)throw std::runtime_error("Cannot create execution trace: "+plan.trace_file.string());
                trace << "step,pc,state,opcode,stage,source_offset,next_pc,cpsr\n";
            }
            std::string reason="execution limit reached";
            std::uint64_t steps=0;
            while(steps<plan.instruction_limit &&
                  cpu.instructions_executed()<plan.instruction_limit) {
                const auto pc=cpu.getPC();
                if(std::find(plan.breakpoints.begin(),plan.breakpoints.end(),pc)!=plan.breakpoints.end()) {
                    reason="breakpoint reached at 0x"+[&](){std::ostringstream s;s<<std::hex<<pc;return s.str();}();
                    break;
                }
                const auto cpsr=cpu.get_cpsr();
                const bool thumb=(cpsr&(1u<<5))!=0;
                std::uint32_t opcode=0;
                try { opcode=thumb?(static_cast<std::uint32_t>(memory.read8(pc)) |
                    (static_cast<std::uint32_t>(memory.read8(pc+1u))<<8)):memory.read32le(pc); }
                catch(const std::exception&) {}
                std::string stage="UNMAPPED";std::uint64_t source_offset=0;
                for(const auto& segment:plan.segments) {
                    const auto address=pc&~1u;
                    if(address>=segment.load_address && static_cast<std::uint64_t>(address)<
                       static_cast<std::uint64_t>(segment.load_address)+segment.length) {
                        stage=segment.file.filename().string();
                        source_offset=segment.offset+(address-segment.load_address);break;
                    }
                }
                ++steps;std::string step_error;
                try { cpu.step(); }
                catch(const std::exception& error) { reason=error.what();step_error=reason; }
                if(trace) {
                    trace<<steps<<",0x"<<std::hex<<pc<<","<<(thumb?"THUMB":"ARM")
                         <<",0x"<<opcode<<",\""<<stage<<"\",0x"<<source_offset
                         <<",0x"<<cpu.getPC()<<",0x"<<cpu.get_cpsr()<<std::dec<<"\n";
                    if(!trace)throw std::runtime_error("Failed writing execution trace.");
                }
                if(!step_error.empty())break;
            }
            if(trace) {
                trace.flush();if(!trace)throw std::runtime_error("Failed flushing execution trace.");
                std::cout<<"Execution trace: "<<plan.trace_file.string()<<"\n";
            }
            std::cout << "\nExecution stopped: " << reason
                      << "\nSteps: " << steps
                      << "\nInstructions completed: " << cpu.instructions_executed()
                      << "\nPC: 0x" << std::hex << cpu.getPC() << "\n";
            for(unsigned i=0;i<16;++i)
                std::cout << "R" << std::dec << i << "=0x" << std::hex
                          << cpu.get_register(i) << ((i%4==3)?"\\n":"  ");
            std::cout << "CPSR=0x" << std::hex << cpu.get_cpsr() << std::dec << "\\n"
                      << "Data fault: address=0x" << std::hex << cpu.get_data_fault_address()
                      << " status=0x" << cpu.get_data_fault_status()
                      << "\\nPrefetch fault: address=0x" << cpu.get_instruction_fault_address()
                      << " status=0x" << cpu.get_instruction_fault_status() << std::dec << "\\n"
                      << "NOTE: this executes explicitly selected FLS byte ranges in shared user-configured RAM.\\n"
                      << "No automatic FLS decoding or verified C3350 physical memory map is implied.\\n";
            return 0;
        } catch(const std::exception& error) {
            std::cerr << "Boot-plan execution failed: " << error.what() << "\\n";
            return 1;
        }
    }

    if ((argc == 3 || argc == 4) && std::string(argv[1]) == "--prepare-boot") {
        try {
            const std::filesystem::path directory(argv[2]);
            if (!std::filesystem::is_directory(directory))
                throw std::runtime_error("Firmware directory does not exist.");
            std::vector<std::filesystem::path> files;
            for (const auto& item : std::filesystem::directory_iterator(directory))
                if (item.is_regular_file() && item.path().extension()==".fls")
                    files.push_back(item.path());
            std::sort(files.begin(),files.end());
            const auto find_component=[&](const std::string& key){
                std::vector<std::filesystem::path> found;
                for(const auto& path:files){
                    std::string name=path.filename().string();
                    std::transform(name.begin(),name.end(),name.begin(),
                        [](unsigned char ch){return static_cast<char>(std::toupper(ch));});
                    if(name.find(key)!=std::string::npos)found.push_back(path);
                }
                return found;
            };
            const std::vector<std::pair<std::string,std::string>> order={
                {"PSI","Downloader sequence step 1"},
                {"SLB","Downloader sequence step 2"},
                {"BSY","Downloader sequence steps 3 and 4 (two reported regions)"},
                {"CDS","Downloader sequence step 5"}
            };
            std::ostringstream report;
            report<<"C3350EMU firmware boot-preparation report\\n"
                  <<"========================================\\n"
                  <<"Directory: "<<directory.string()<<"\\n\\n";
            bool complete=true;
            for(const auto& item:order){
                const auto matches=find_component(item.first);
                report<<item.first<<" - "<<item.second<<"\\n";
                if(matches.empty()){report<<"  MISSING: no matching .fls filename\\n";complete=false;}
                for(const auto& path:matches){
                    const auto info=firmware::FlsParser::inspect(path);
                    report<<"  File: "<<path.filename().string()<<"\\n"
                          <<"  Size: "<<info.file_size<<" bytes\\n"
                          <<"  Classification: "<<info.classification<<"\\n"
                          <<"  Header: "<<firmware::hex_bytes(info.header)<<"\\n"
                          <<"  Note: "<<info.note<<"\\n";
                }
                report<<"\\n";
            }
            report<<"Firmware-set inventory: "<<(complete?"all four component families present":"INCOMPLETE")<<"\\n"
                  <<"Known downloader order: PSI -> SLB -> BSY region 0 -> BSY region 1 -> CDS\\n"
                  <<"BOOT STATUS: PREPARATION ONLY. No component was decoded, relocated, or executed.\\n"
                  <<"FLS offsets, runtime load addresses, checksums, and entry points remain unverified.\\n";
            if(argc==4){
                std::ofstream out(argv[3],std::ios::binary);
                if(!out)throw std::runtime_error("Cannot create boot-preparation report.");
                out<<report.str();
                if(!out)throw std::runtime_error("Failed writing boot-preparation report.");
                std::cout<<"Report saved: "<<argv[3]<<"\\n";
            }
            std::cout<<report.str();
            return complete?0:2;
        } catch(const std::exception& e) {
            std::cerr<<"Boot preparation failed: "<<e.what()<<"\\n";return 1;
        }
    }

    if (argc == 7 && std::string(argv[1]) == "--run-raw") {
        try {
            const auto ram_base = parse_number(argv[3], "RAM base");
            const auto ram_size = parse_number(argv[4], "RAM size");
            const auto entry = parse_number(argv[5], "entry point");
            const auto instruction_limit = parse_number(argv[6], "instruction limit");
            if (ram_base > 0xffffffffu || ram_size == 0 ||
                ram_size > std::numeric_limits<std::size_t>::max() ||
                ram_size > (std::uint64_t{1} << 32) - ram_base ||
                entry > 0xffffffffu || instruction_limit == 0)
                throw std::runtime_error("RAM, entry point, or instruction limit is out of range.");

            std::ifstream input(argv[2], std::ios::binary | std::ios::ate);
            if (!input) throw std::runtime_error("Cannot open firmware file.");
            const auto end = input.tellg();
            if (end <= 0) throw std::runtime_error("Firmware file is empty or unreadable.");
            const auto file_size = static_cast<std::uint64_t>(end);
            if (file_size > ram_size || file_size > std::numeric_limits<std::size_t>::max())
                throw std::runtime_error("Firmware file does not fit in the configured memory region.");
            if (entry < ram_base || entry + 4 > ram_base + file_size || (entry & 3u))
                throw std::runtime_error("ARM entry point must be aligned and inside the loaded file.");

            std::vector<std::uint8_t> image(static_cast<std::size_t>(file_size));
            input.seekg(0, std::ios::beg);
            if (!input.read(reinterpret_cast<char*>(image.data()),
                            static_cast<std::streamsize>(image.size())))
                throw std::runtime_error("Failed to read the complete firmware file.");

            Memory memory;
            memory.map(static_cast<std::uint32_t>(ram_base),
                       static_cast<std::size_t>(ram_size), true,
                       "user-configured raw firmware memory");
            memory.write_block(static_cast<std::uint32_t>(ram_base), image);
            CPU cpu(memory);
            cpu.reset(static_cast<std::uint32_t>(entry));

            std::cout << "Raw ARM execution started\n"
                      << "File: " << argv[2] << "\n"
                      << "Loaded bytes: " << image.size() << "\n"
                      << "RAM base: 0x" << std::hex << ram_base
                      << "  size: 0x" << ram_size << "\n"
                      << "Entry: 0x" << entry << std::dec
                      << "  instruction limit: " << instruction_limit << "\n";
            std::string stop_reason = "instruction limit reached";
            while (cpu.instructions_executed() < instruction_limit) {
                try {
                    cpu.step();
                } catch (const std::exception& error) {
                    stop_reason = error.what();
                    break;
                }
            }
            std::cout << "\nExecution stopped: " << stop_reason
                      << "\nInstructions: " << cpu.instructions_executed()
                      << "\nPC: 0x" << std::hex << cpu.getPC() << "\n";
            for (unsigned i = 0; i < 16; ++i)
                std::cout << "R" << std::dec << i << "=0x" << std::hex
                          << cpu.get_register(i) << ((i % 4 == 3) ? "\n" : "  ");
            std::cout << "CPSR=0x" << std::hex << cpu.get_cpsr() << std::dec << "\n"
                      << "NOTE: user-configured memory only; not a verified GT-C3350 boot.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Raw execution failed: " << error.what() << "\n";
            return 1;
        }
    }


    if (argc == 10 && std::string(argv[1]) == "--run-fls-segment") {
        try {
            const auto offset = parse_number(argv[3], "FLS file offset");
            const auto length = parse_number(argv[4], "segment length");
            const auto ram_base = parse_number(argv[5], "RAM base");
            const auto ram_size = parse_number(argv[6], "RAM size");
            const auto load_address = parse_number(argv[7], "load address");
            const auto entry = parse_number(argv[8], "entry point");
            const auto limit = parse_number(argv[9], "instruction limit");
            if (!length || ram_base > 0xffffffffu || load_address > 0xffffffffu ||
                entry > 0xffffffffu || !ram_size ||
                ram_size > std::numeric_limits<std::size_t>::max() ||
                length > std::numeric_limits<std::size_t>::max() ||
                ram_base + ram_size > (std::uint64_t{1} << 32) ||
                load_address < ram_base || load_address > ram_base + ram_size ||
                length > ram_base + ram_size - load_address || !limit)
                throw std::runtime_error("Address, size, or instruction limit is out of range.");
            const bool thumb = (entry & 1u) != 0;
            const auto aligned_entry = entry & ~std::uint64_t{1};
            const auto instruction_size = thumb ? 2u : 4u;
            if (aligned_entry < load_address ||
                aligned_entry + instruction_size > load_address + length ||
                (!thumb && (entry & 3u)))
                throw std::runtime_error("Entry point must be aligned and inside the loaded segment.");

            Memory memory;
            memory.map(static_cast<std::uint32_t>(ram_base),
                       static_cast<std::size_t>(ram_size), true,
                       "user-configured FLS staging RAM");
            const auto loaded = firmware::FlsParser::load_raw_segment(
                argv[2], offset, static_cast<std::size_t>(length),
                static_cast<std::uint32_t>(load_address), memory);
            CPU cpu(memory);
            cpu.reset(static_cast<std::uint32_t>(entry));
            std::cout << "Executing selected FLS byte range\n"
                      << "File: " << loaded.source.string() << "\n"
                      << "File offset: 0x" << std::hex << loaded.file_offset
                      << "  load address: 0x" << loaded.load_address
                      << "  entry: 0x" << entry << "\n"
                      << "CRC-32: 0x" << loaded.crc32 << std::dec
                      << "  bytes loaded: " << loaded.bytes_loaded
                      << "  mode: " << (thumb ? "Thumb" : "ARM") << "\n";
            std::string reason = "instruction limit reached";
            while (cpu.instructions_executed() < limit) {
                try { cpu.step(); }
                catch (const std::exception& error) { reason = error.what(); break; }
            }
            std::cout << "Stopped: " << reason
                      << "\nInstructions executed: " << cpu.instructions_executed()
                      << "\nPC: 0x" << std::hex << cpu.getPC() << "\n";
            for (unsigned i = 0; i < 16; ++i)
                std::cout << "R" << std::dec << i << "=0x" << std::hex
                          << cpu.get_register(i) << ((i % 4 == 3) ? "\n" : "  ");
            std::cout << "CPSR=0x" << std::hex << cpu.get_cpsr() << std::dec << "\n"
                      << "This runs a chosen byte range, not an automatically decoded FLS segment.\n"
                      << "Memory addresses and entry point are user-supplied, not verified C3350 hardware values.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "FLS segment execution failed: " << error.what() << "\n";
            return 1;
        }
    }

    if (argc == 8 && std::string(argv[1]) == "--load-raw-segment") {
        try {
            const auto offset = parse_number(argv[3], "file offset");
            const auto length64 = parse_number(argv[4], "segment length");
            const auto ram_base64 = parse_number(argv[5], "RAM base");
            const auto ram_size64 = parse_number(argv[6], "RAM size");
            const auto load_address64 = parse_number(argv[7], "load address");
            if (length64 > std::numeric_limits<std::size_t>::max() ||
                ram_size64 == 0 || ram_size64 > std::numeric_limits<std::size_t>::max() ||
                ram_base64 > 0xffffffffu || load_address64 > 0xffffffffu)
                throw std::runtime_error("One or more address/size values are out of range.");

            Memory memory;
            memory.map(static_cast<std::uint32_t>(ram_base64),
                       static_cast<std::size_t>(ram_size64), true,
                       "user-configured staging RAM");
            const auto result = firmware::FlsParser::load_raw_segment(
                argv[2], offset, static_cast<std::size_t>(length64),
                static_cast<std::uint32_t>(load_address64), memory);
            std::cout << "Raw firmware segment staged successfully\n"
                      << "Source: " << result.source.string() << "\n"
                      << "File offset: 0x" << std::hex << result.file_offset << "\n"
                      << "Load address: 0x" << result.load_address << "\n"
                      << std::dec << "Bytes loaded: " << result.bytes_loaded << "\n"
                      << "Calculated CRC-32: 0x" << std::hex << result.crc32 << "\n"
                      << "RAM: user-configured only; not a verified GT-C3350 physical map.\n"
                      << "FLS segment boundaries and true load address remain unverified.\n";
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Firmware staging failed: " << e.what() << "\n";
            return 1;
        }
    }

    if ((argc == 3 || argc == 4) && std::string(argv[1]) == "--compare-firmware-set") {
        try {
            std::size_t bytes = 4096;
            if (argc == 4) bytes = static_cast<std::size_t>(parse_number(argv[3], "comparison length"));
            std::cout << firmware::FlsParser::compare_headers_in_directory(argv[2], bytes);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Header comparison failed: " << e.what() << "\n";
            return 1;
        }
    }
    if (argc == 3 && std::string(argv[1]) == "--inspect-fls-directory") {
        try {
            std::cout << firmware::FlsParser::inspect_directory(argv[2]);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Directory inspection failed: " << e.what() << "\n";
            return 1;
        }
    }
    if ((argc == 3 || argc == 4) && std::string(argv[1]) == "--analyze-firmware") {
        try {
            const auto report=firmware::FlsParser::analyze(argv[2], argc==4 ? argv[3] : "");
            std::cout << "Analysis complete. Report: " << report.string() << "\n"
                      << "This is heuristic analysis, not a confirmed FLS decode.\n";
            return 0;
        } catch (const std::exception& e) { std::cerr << "Analysis failed: " << e.what() << "\n"; return 1; }
    }
    if (argc == 3 && std::string(argv[1]) == "--inspect-firmware") {
        try {
            const auto info=firmware::FlsParser::inspect(argv[2]);
            std::cout<<"File: "<<info.path.string()<<"\nSize: "<<info.file_size
                     <<" bytes\nClassification: "<<info.classification
                     <<"\nHeader: "<<firmware::hex_bytes(info.header)
                     <<"\nNote: "<<info.note<<"\n";
            return 0;
        } catch(const std::exception& e) { std::cerr<<"Firmware inspection failed: "<<e.what()<<"\n"; return 1; }
    }
    if(argc!=1) { print_help(argv[0]); return 2; }
    std::cout<<"C3350EMU research emulator\nARM-state interpreter: initial instruction subset implemented\n"
             <<"Firmware boot: not implemented\nUse --platform-info, --analyze-firmware, or --run-raw.\n";
    return 0;
}
