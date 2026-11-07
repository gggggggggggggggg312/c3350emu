#include "boot_plan.hpp"
#include "memory/memory.hpp"
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace firmware {
namespace {
std::uint64_t number(const std::string& value, const char* label) {
    if (value.empty() || value.front() == '-') throw std::runtime_error(std::string("Invalid ") + label);
    std::size_t used = 0;
    const auto result = std::stoull(value, &used, 0);
    if (used != value.size()) throw std::runtime_error(std::string("Invalid ") + label);
    return result;
}
}
BootPlan parse_boot_plan(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open boot plan: " + path.string());
    BootPlan plan;
    bool have_ram=false, have_entry=false, have_limit=false;
    std::string line;
    std::size_t line_number=0;
    const auto base_dir=std::filesystem::absolute(path).parent_path();
    while (std::getline(input,line)) {
        ++line_number;
        const auto comment=line.find('#');
        if(comment!=std::string::npos)line.erase(comment);
        std::istringstream row(line);
        std::string command;
        if(!(row>>command))continue;
        try {
            if(command=="RAM") {
                std::string base,size,extra;
                if(!(row>>base>>size)||row>>extra)throw std::runtime_error("RAM requires exactly: base size");
                const auto b=number(base,"RAM base"),s=number(size,"RAM size");
                if(b>0xffffffffu||s==0||s>std::numeric_limits<std::size_t>::max()||b+s>(std::uint64_t{1}<<32))
                    throw std::runtime_error("RAM range is invalid");
                plan.ram_base=static_cast<std::uint32_t>(b);plan.ram_size=static_cast<std::size_t>(s);have_ram=true;
            } else if(command=="ENTRY") {
                std::string value,extra;if(!(row>>value)||row>>extra)throw std::runtime_error("ENTRY requires one address");
                const auto v=number(value,"entry point");if(v>0xffffffffu)throw std::runtime_error("Entry exceeds 32 bits");
                plan.entry=static_cast<std::uint32_t>(v);have_entry=true;
            } else if(command=="LIMIT") {
                std::string value,extra;if(!(row>>value)||row>>extra)throw std::runtime_error("LIMIT requires one instruction count");
                plan.instruction_limit=number(value,"instruction limit");if(!plan.instruction_limit)throw std::runtime_error("Instruction limit must be nonzero");have_limit=true;
            } else if(command=="COPY") {
                std::string source,destination,length,extra;
                if(!(row>>source>>destination>>length)||row>>extra)throw std::runtime_error("COPY requires: source-address destination-address length");
                const auto src=number(source,"copy source"),dst=number(destination,"copy destination"),n=number(length,"copy length");
                if(src>0xffffffffu||dst>0xffffffffu||n==0||n>std::numeric_limits<std::size_t>::max())throw std::runtime_error("COPY range is invalid");
                plan.memory_initialization.push_back({MemoryInitKind::Copy,static_cast<std::uint32_t>(src),static_cast<std::uint32_t>(dst),static_cast<std::size_t>(n)});
            } else if(command=="ZERO") {
                std::string destination,length,extra;
                if(!(row>>destination>>length)||row>>extra)throw std::runtime_error("ZERO requires: destination-address length");
                const auto dst=number(destination,"zero destination"),n=number(length,"zero length");
                if(dst>0xffffffffu||n==0||n>std::numeric_limits<std::size_t>::max())throw std::runtime_error("ZERO range is invalid");
                plan.memory_initialization.push_back({MemoryInitKind::Zero,0,static_cast<std::uint32_t>(dst),static_cast<std::size_t>(n)});
            } else if(command=="BREAK") {
                std::string value,extra;
                if(!(row>>value)||row>>extra)throw std::runtime_error("BREAK requires one address");
                const auto address=number(value,"breakpoint");
                if(address>0xffffffffu)throw std::runtime_error("Breakpoint exceeds 32 bits");
                plan.breakpoints.push_back(static_cast<std::uint32_t>(address));
            } else if(command=="TRACE") {
                std::string file,extra;
                if(!(row>>std::quoted(file))||row>>extra)throw std::runtime_error("TRACE requires one quoted output path");
                std::filesystem::path target(file);
                if(target.is_relative())target=base_dir/target;
                plan.trace_file=target.lexically_normal();
            } else if(command=="SEGMENT") {
                std::string file,offset,length,load,extra;
                if(!(row>>std::quoted(file)>>offset>>length>>load)||row>>extra)
                    throw std::runtime_error("SEGMENT requires: quoted-file offset length load-address");
                const auto o=number(offset,"file offset"),n=number(length,"segment length"),a=number(load,"load address");
                if(!n||n>std::numeric_limits<std::size_t>::max()||a>0xffffffffu)
                    throw std::runtime_error("Segment length or load address is invalid");
                std::filesystem::path source(file);
                if(source.is_relative())source=base_dir/source;
                plan.segments.push_back({source.lexically_normal(),o,static_cast<std::size_t>(n),static_cast<std::uint32_t>(a)});
            } else throw std::runtime_error("Unknown directive: "+command);
        } catch(const std::exception& e) {
            throw std::runtime_error("Boot plan line "+std::to_string(line_number)+": "+e.what());
        }
    }
    if(!input.eof())throw std::runtime_error("Error reading boot plan");
    if(!have_ram||!have_entry||!have_limit||plan.segments.empty())
        throw std::runtime_error("Plan must contain RAM, ENTRY, LIMIT, and at least one SEGMENT");
    const std::uint64_t ram_end=static_cast<std::uint64_t>(plan.ram_base)+plan.ram_size;
    bool entry_covered=false;
    for(std::size_t i=0;i<plan.segments.size();++i) {
        const auto& s=plan.segments[i];
        const std::uint64_t end=static_cast<std::uint64_t>(s.load_address)+s.length;
        if(s.load_address<plan.ram_base||end>ram_end||end>(std::uint64_t{1}<<32))
            throw std::runtime_error("A segment does not fit inside configured RAM: "+s.file.string());
        for(std::size_t j=0;j<i;++j) {
            const auto& p=plan.segments[j];
            const std::uint64_t p_end=static_cast<std::uint64_t>(p.load_address)+p.length;
            if(s.load_address<p_end&&p.load_address<end)
                throw std::runtime_error("Boot-plan segments overlap in RAM");
        }
        const auto aligned=plan.entry&~1u;
        const auto entry_bytes=(plan.entry&1u)?2u:4u;
        if(aligned>=s.load_address&&static_cast<std::uint64_t>(aligned)+entry_bytes<=end)
            entry_covered=true;
    }
    if(!entry_covered)throw std::runtime_error("Entry point is not inside any loaded segment");
    for(const auto& op:plan.memory_initialization) {
        const auto destination_end=static_cast<std::uint64_t>(op.destination)+op.length;
        if(op.destination<plan.ram_base||destination_end>ram_end)
            throw std::runtime_error("Memory initialization destination is outside configured RAM");
        if(op.kind==MemoryInitKind::Copy) {
            const auto source_end=static_cast<std::uint64_t>(op.source)+op.length;
            if(op.source<plan.ram_base||source_end>ram_end)
                throw std::runtime_error("Memory initialization source is outside configured RAM");
        }
    }
    return plan;
}

void apply_memory_initialization(const BootPlan& plan, Memory& memory) {
    for(const auto& op:plan.memory_initialization) {
        if(op.kind==MemoryInitKind::Zero) {
            for(std::size_t i=0;i<op.length;++i)
                memory.write8(static_cast<std::uint32_t>(static_cast<std::uint64_t>(op.destination)+i),0);
        } else {
            std::vector<std::uint8_t> snapshot(op.length);
            for(std::size_t i=0;i<op.length;++i)
                snapshot[i]=memory.read8(static_cast<std::uint32_t>(static_cast<std::uint64_t>(op.source)+i));
            memory.write_block(op.destination,snapshot);
        }
    }
}
}
