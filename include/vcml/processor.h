/******************************************************************************
 *                                                                            *
 * Copyright 2018 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#ifndef VCML_PROCESSOR_H
#define VCML_PROCESSOR_H

#include "vcml/common/includes.h"
#include "vcml/common/types.h"
#include "vcml/common/utils.h"
#include "vcml/common/report.h"

#include "vcml/logging/logger.h"
#include "vcml/backends/backend.h"
#include "vcml/properties/property.h"

#include "vcml/elf.h"
#include "vcml/range.h"
#include "vcml/ports.h"
#include "vcml/component.h"
#include "vcml/master_socket.h"

namespace vcml {

    struct irq_stats {
        unsigned int irq;
        unsigned int irq_count;
        bool         irq_status;
        sc_time      irq_last;
        sc_time      irq_uptime;
        sc_time      irq_longest;
    };

    class processor: public component
    {
    private:
        double m_run_time;
        u64    m_num_cycles;
        elf*   m_symbols;

        std::map<unsigned int, irq_stats> m_irq_stats;
        std::vector<u64> m_breakpoints;

        bool cmd_dump(const vector<string>& args, ostream& os);
        bool cmd_reset(const vector<string>& args, ostream& os);
        bool cmd_read(const vector<string>& args, ostream& os);
        bool cmd_symbols(const vector<string>& args, ostream& os);
        bool cmd_lsym(const vector<string>& args, ostream& os);
        bool cmd_bp(const vector<string>& args, ostream& os);
        bool cmd_lsbp(const vector<string>& args, ostream& os);
        bool cmd_rmbp(const vector<string>& args, ostream& os);
        bool cmd_disas(const vector<string>& args, ostream& os);

        void processor_thread();
        void irq_handler(unsigned int irq);

        processor();
        processor(const processor&);

    public:
        property<clock_t> clock;
        property<string> symbols;

        in_port_list  IRQ;
        master_socket INSN;
        master_socket DATA;

        processor(const sc_module_name& name, clock_t clk);
        virtual ~processor();

        VCML_KIND(processor);

        virtual bool insert_breakpoint(u64 address) { return false; }
        virtual bool remove_breakpoint(u64 address) { return false; }

        virtual bool virt_to_phys(u64 vaddr, u64& paddr);
        virtual string disassemble(u64& addr, unsigned char* insn);

        virtual u64 get_program_counter() { return 0; }
        virtual u64 get_stack_pointer()   { return 0; }
        virtual u64 get_core_id()         { return 0; }

        virtual void set_program_counter(u64 val) {}
        virtual void set_stack_pointer(u64 val)   {}
        virtual void set_core_id(u64 val)         {}

        u64 get_num_cycles()  const { return m_num_cycles; }
        double get_run_time() const { return m_run_time; }
        double get_cps()      const { return m_num_cycles / m_run_time; }

        inline void reset();

        bool get_irq_stats(unsigned int irq, irq_stats& stats);

        virtual void end_of_elaboration();

        virtual void interrupt(unsigned int irq, bool set);
        virtual void simulate(unsigned int&) = 0;

        template <typename T>
        inline tlm_response_status fetch (u64 addr, T& data);

        template <typename T>
        inline tlm_response_status read  (u64 addr, T& data);

        template <typename T>
        inline tlm_response_status write (u64 addr, const T& data);

    };

    inline bool processor::virt_to_phys(u64 vaddr, u64& paddr) {
        paddr = vaddr;
        return true;
    }

    inline string processor::disassemble(u64& addr, unsigned char* insn) {
        addr += 4;
        return "n/a";
    }

    inline void processor::reset() {
        m_num_cycles = 0;
        m_run_time = 0.0;
    }

    template <typename T>
    inline tlm_response_status processor::fetch(u64 addr, T& data) {
        tlm_response_status rs = INSN.read(addr, data);
        if (failed(rs)) {
            string status = tlm_response_to_str(rs);
            log_error("detected bus error during fetch operation");
            log_error("  addr = 0x%08" PRIx32, addr);
            log_error("  pc   = 0x%08" PRIx32, get_program_counter());
            log_error("  sp   = 0x%08" PRIx32, get_stack_pointer());
            log_error("  size = %d bytes", sizeof(data));
            log_error("  port = %s", INSN.name());
            log_error("  code = %s", status.c_str());
        }
        return rs;
    }

    template <typename T>
    inline tlm_response_status processor::read(u64 addr, T& data) {
        tlm_response_status rs = DATA.read(addr, data);
        if (failed(rs)) {
            string status = tlm_response_to_str(rs);
            log_error("detected bus error during read operation");
            log_error("  addr = 0x%08" PRIx32, addr);
            log_error("  pc   = 0x%08" PRIx32, get_program_counter());
            log_error("  sp   = 0x%08" PRIx32, get_stack_pointer());
            log_error("  size = %d bytes", sizeof(data));
            log_error("  port = %s", DATA.name());
            log_error("  code = %s", status.c_str());
        }
        return rs;
    }

    template <typename T>
    inline tlm_response_status processor::write(u64 addr, const T& data) {
        tlm_response_status rs = DATA.write(addr, data);
        if (failed(rs)) {
            string status = tlm_response_to_str(rs);
            log_error("detected bus error during write operation");
            log_error("  addr = 0x%08" PRIx32, addr);
            log_error("  pc   = 0x%08" PRIx32, get_program_counter());
            log_error("  sp   = 0x%08" PRIx32, get_stack_pointer());
            log_error("  size = %d bytes", sizeof(data));
            log_error("  port = %s", DATA.name());
            log_error("  code = %s", status.c_str());
        }
        return rs;
    }

}

#endif