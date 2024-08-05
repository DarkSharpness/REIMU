#include <interpreter/interpreter.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>
#include <simulation/icache.h>
#include <linker/layout.h>
#include <config/config.h>
#include <libc/libc.h>
#include <map>

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
    (RegisterFile &rf, Memory &mem, Device &dev, std::size_t timeout) {
    ICache icache { mem };
    try {
        Hint hint {};
        while (rf.advance() && timeout --> 0) {
            auto &exe = icache.ifetch(rf.get_pc(), hint);
            hint = exe(rf, mem, dev);
        }
        panic_if(timeout + 1 == 0, "Time Limit Exceeded");
    } catch (FailToInterpret &e) {
        panic("{}", e.what(rf, mem, dev));
    } catch (std::exception &e) {
        unreachable(std::format("std::exception caught: {}\n", e.what()));
    } catch (...) {
        unreachable(std::format("unexpected exception caught\n"));
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

    libc::libc_init(regfile, memory, device);

    if (config.has_option("debug")) {
        simulate_debug(regfile, memory, device, config.get_timeout());
    } else {
        simulate_normal(regfile, memory, device, config.get_timeout());
    }

    console::profile << '\n';

    bool enable_detail = config.has_option("detail");
    regfile.print_details(enable_detail);
    memory.print_details(enable_detail);
    device.print_details(enable_detail);
}

static void simulate_debug
    (RegisterFile &, Memory &, Device &, std::size_t) {
    panic("Debug Mode is not implemented yet");
}

} // namespace dark
