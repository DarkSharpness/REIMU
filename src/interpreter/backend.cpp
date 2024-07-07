#include <interpreter/interpreter.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>
#include <interpreter/register.h>
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

    auto get(target_size_t pc) {
        struct Result {
            std::string_view label;
            target_size_t offset;
        };
        auto pos = labels.upper_bound(pc);
        if (pos == labels.begin()) return Result { "", pc };
        pos--; return Result { pos->second, pc - pos->first };
    }
};

RegisterFile::RegisterFile(target_size_t pc, const Config &config)
    : regs(), pc(), new_pc(pc) {
    (*this)[Register::sp] = config.get_stack_top();
    (*this)[Register::ra] = this->end_pc;
}

static void interpret_normal
    (RegisterFile &regfile, Memory &memory, Device &device, std::size_t timeout) {
    while (regfile.advance() && timeout --> 0) {
        auto &exe = memory.fetch_executable(regfile.get_pc());
        exe(regfile, memory, device);
    }
    panic_if(timeout + 1 == std::size_t{}, "Time Limit Exceeded");
}

void Interpreter::interpret(const Config &config) {
    auto device_ptr = Device::create(config);
    auto memory_ptr = Memory::create(config, this->layout);

    auto &device = *device_ptr;
    auto &memory = *memory_ptr;

    auto regfile = RegisterFile { layout.position_table.at("main"), config };

    if (!config.has_option("debug")) {
        interpret_normal(regfile, memory, device, config.get_timeout());
    } else {
        panic("Debug Mode is not implemented yet");
    }

    if (config.has_option("detail")) {
        std::cout << std::format("Program exited normally with code {}\n", regfile[Register::a0]);
    }
}

} // namespace dark
