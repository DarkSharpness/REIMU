#include <interpreter/interpreter.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>
#include <linker/layout.h>
#include <config/config.h>
#include <utility.h>
#include <map>
#include <fstream>

namespace dark {

struct LabelMap {
  private:
    std::map <target_size_t, std::string_view> labels;

  public:
    void add(target_size_t pc, std::string_view label) {
        labels[pc] = label;
    }

    auto get(target_size_t pc) const {
        struct Result {
            std::string_view label;
            target_size_t offset;
        };
        auto pos = labels.upper_bound(pc);
        if (pos == labels.begin()) return Result { "", pc };
        pos--; return Result { pos->second, pc - pos->first };
    }
};

static void simulate_normal
    (RegisterFile &regfile, Memory &memory, Device &device, std::size_t timeout) {
    try {
        while (regfile.advance() && timeout --> 0) {
            auto &exe = memory.fetch_executable(regfile.get_pc());
            exe(regfile, memory, device);
        }
        panic_if(timeout + 1 == std::size_t{}, "Time Limit Exceeded");
    } catch (FailToInterpret &e) {
        panic("{}", e.what(regfile, memory, device));
    } catch (std::exception &e) {
        std::cerr << std::format("std::exception caught: {}\n", e.what());
        unreachable();
    } catch (...) {
        std::cerr << "unexpected exception caught\n";
        unreachable();
    }
}

static void simulate_debug
    (RegisterFile &regfile, Memory &memory, Device &device, std::size_t timeout);

void Interpreter::simulate() {
    auto &layout = this->memory_layout.get <MemoryLayout &>();

    auto device_ptr = Device::create(config);
    auto memory_ptr = Memory::create(config, layout);

    auto &device = *device_ptr;
    auto &memory = *memory_ptr;

    auto regfile = RegisterFile { layout.position_table.at("main"), config };

    if (config.has_option("debug")) {
        simulate_debug(regfile, memory, device, config.get_timeout());
    } else {
        simulate_normal(regfile, memory, device, config.get_timeout());
    }

    if (config.has_option("silent")) return;
    bool enable_detail = config.has_option("detail");
    regfile.print_details(enable_detail);
    memory.print_details(enable_detail);
    device.print_details(enable_detail);
}

static void simulate_debug
    (RegisterFile &regfile, Memory &memory, Device &device, std::size_t timeout) {
    panic("Debug Mode is not implemented yet");
}

} // namespace dark
