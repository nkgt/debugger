#include "nkgt/registers.hpp"
#include "nkgt/error_codes.hpp"
#include "nkgt/util.hpp"

#include <cerrno>
#include <cstdint>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "fmt/core.h"
#include "tl/expected.hpp"

namespace {

[[nodiscard]]
auto get_register_value_from_user_regs(
    const user_regs_struct& regs,
    nkgt::registers::reg r
) -> uint64_t {
    using namespace nkgt::registers;

    switch (r) {
    case reg::rax:      return regs.rax;
    case reg::rdx:      return regs.rdx;
    case reg::rcx:      return regs.rcx;
    case reg::rbx:      return regs.rbx;
    case reg::rsi:      return regs.rsi;
    case reg::rdi:      return regs.rdi;
    case reg::rbp:      return regs.rbp;
    case reg::rsp:      return regs.rsp;
    case reg::r8:       return regs.r8;
    case reg::r9:       return regs.r9;
    case reg::r10:      return regs.r10;
    case reg::r11:      return regs.r11;
    case reg::r12:      return regs.r12;
    case reg::r13:      return regs.r13;
    case reg::r14:      return regs.r14;
    case reg::r15:      return regs.r15;
    case reg::eflags:   return regs.eflags;
    case reg::es:       return regs.es;
    case reg::cs:       return regs.cs;
    case reg::ss:       return regs.ss;
    case reg::ds:       return regs.ds;
    case reg::fs:       return regs.fs;
    case reg::gs:       return regs.gs;
    case reg::fs_base:  return regs.fs_base;
    case reg::gs_base:  return regs.gs_base;
    case reg::orig_rax: return regs.orig_rax;
    case reg::rip:      return regs.rip;
    }
}

[[nodiscard]]
auto get_register_value_from_user_regs(
    const user_regs_struct& regs,
    unsigned dwarf_number
) -> tl::expected<uint64_t, nkgt::error::registers> {
    switch (dwarf_number) {
    case 0:  return regs.rax;
    case 1:  return regs.rdx;
    case 2:  return regs.rcx;
    case 3:  return regs.rbx;
    case 4:  return regs.rsi;
    case 5:  return regs.rdi;
    case 6:  return regs.rbp;
    case 7:  return regs.rsp;
    case 8:  return regs.r8;
    case 9:  return regs.r9;
    case 10: return regs.r10;
    case 11: return regs.r11;
    case 12: return regs.r12;
    case 13: return regs.r13;
    case 14: return regs.r14;
    case 15: return regs.r15;
    case 49: return regs.eflags;
    case 50: return regs.es;
    case 51: return regs.cs;
    case 52: return regs.ss;
    case 53: return regs.ds;
    case 54: return regs.fs;
    case 55: return regs.gs;
    case 58: return regs.fs_base;
    case 59: return regs.gs_base;
    default: return tl::make_unexpected(nkgt::error::registers::unknown_dwarf_number);
    }
}

auto set_register_value_to_user_regs(
    user_regs_struct& regs,
    nkgt::registers::reg r,
    uint64_t value
) -> void {
    using namespace nkgt::registers;

    switch (r) {
    case reg::rax:      regs.rax = value; return;
    case reg::rdx:      regs.rdx = value; return;
    case reg::rcx:      regs.rcx = value; return;
    case reg::rbx:      regs.rbx = value; return;
    case reg::rsi:      regs.rsi = value; return;
    case reg::rdi:      regs.rdi = value; return;
    case reg::rbp:      regs.rbp = value; return;
    case reg::rsp:      regs.rsp = value; return;
    case reg::r8:       regs.r8 = value; return;
    case reg::r9:       regs.r9 = value; return;
    case reg::r10:      regs.r10 = value; return;
    case reg::r11:      regs.r11 = value; return;
    case reg::r12:      regs.r12 = value; return;
    case reg::r13:      regs.r13 = value; return;
    case reg::r14:      regs.r14 = value; return;
    case reg::r15:      regs.r15 = value; return;
    case reg::eflags:   regs.eflags = value; return;
    case reg::es:       regs.es = value; return;
    case reg::cs:       regs.cs = value; return;
    case reg::ss:       regs.ss = value; return;
    case reg::ds:       regs.ds = value; return;
    case reg::fs:       regs.fs = value; return;
    case reg::gs:       regs.gs = value; return;
    case reg::fs_base:  regs.fs_base = value; return;
    case reg::gs_base:  regs.gs_base = value; return;
    case reg::orig_rax: regs.orig_rax = value; return;
    case reg::rip:      regs.rip = value; return;
    }
}

std::array<std::pair<nkgt::registers::reg, std::string>, 27> reg_name_lookup = {{
    {nkgt::registers::reg::rax,      "rax"},
    {nkgt::registers::reg::rdx,      "rdx"},
    {nkgt::registers::reg::rcx,      "rcx"},
    {nkgt::registers::reg::rbx,      "rbx"},
    {nkgt::registers::reg::rsi,      "rsi"},
    {nkgt::registers::reg::rdi,      "rdi"},
    {nkgt::registers::reg::rbp,      "rbp"},
    {nkgt::registers::reg::rsp,      "rsp"},
    {nkgt::registers::reg::r8,       "r8"},
    {nkgt::registers::reg::r9,       "r9"},
    {nkgt::registers::reg::r10,      "r10"},
    {nkgt::registers::reg::r11,      "r11"},
    {nkgt::registers::reg::r12,      "r12"},
    {nkgt::registers::reg::r13,      "r13"},
    {nkgt::registers::reg::r14,      "r14"},
    {nkgt::registers::reg::r15,      "r15"},
    {nkgt::registers::reg::eflags,   "eflags"},
    {nkgt::registers::reg::es,       "es"},
    {nkgt::registers::reg::cs,       "cs"},
    {nkgt::registers::reg::ss,       "ss"},
    {nkgt::registers::reg::ds,       "ds"},
    {nkgt::registers::reg::fs,       "fs"},
    {nkgt::registers::reg::gs,       "gs"},
    {nkgt::registers::reg::fs_base,  "fs_base"},
    {nkgt::registers::reg::gs_base,  "gs_base"},
    {nkgt::registers::reg::orig_rax, "orig_rax"},
    {nkgt::registers::reg::rip,      "rip"},
}};

[[nodiscard]]
auto query_user_regs(
    pid_t pid
) -> tl::expected<user_regs_struct, nkgt::error::registers> {
    user_regs_struct regs;

    if(ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        nkgt::util::print_error_message("ptrace", errno);
        return tl::make_unexpected(nkgt::error::registers::getregs_fail);
    }

    return regs;
}

}

namespace nkgt::registers {

auto get_register_value(
    pid_t pid,
    reg r
) -> tl::expected<uint64_t, error::registers> {
    const auto regs = query_user_regs(pid);

    if(!regs) {
        return tl::make_unexpected(regs.error());
    }

    return get_register_value_from_user_regs(*regs, r);
}

auto get_register_value_from_dwarf_number(
    pid_t pid,
    unsigned dwarf_number
) -> tl::expected<uint64_t, error::registers> {
    const auto regs = query_user_regs(pid);

    if(!regs) {
        return tl::make_unexpected(regs.error());
    }

    return get_register_value_from_user_regs(*regs, dwarf_number);
}

auto set_register_value(
    pid_t pid,
    reg r,
    uint64_t value
) -> tl::expected<void, error::registers> {
    auto regs = query_user_regs(pid);

    if(!regs) {
        return tl::make_unexpected(regs.error());
    }

    set_register_value_to_user_regs(*regs, r, value);

    if(ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        util::print_error_message("ptrace", errno);
        return tl::make_unexpected(error::registers::setregs_fail);
    }

    return {};
}

auto to_string(reg r) -> std::string {
    switch (r) {
    case reg::rax:      return "rax";
    case reg::rdx:      return "rdx";
    case reg::rcx:      return "rcx";
    case reg::rbx:      return "rbx";
    case reg::rsi:      return "rsi";
    case reg::rdi:      return "rdi";
    case reg::rbp:      return "rbp";
    case reg::rsp:      return "rsp";
    case reg::r8:       return "r8";
    case reg::r9:       return "r9";
    case reg::r10:      return "r10";
    case reg::r11:      return "r11";
    case reg::r12:      return "r12";
    case reg::r13:      return "r13";
    case reg::r14:      return "r14";
    case reg::r15:      return "r15";
    case reg::eflags:   return "eflags";
    case reg::es:       return "es";
    case reg::cs:       return "cs";
    case reg::ss:       return "ss";
    case reg::ds:       return "ds";
    case reg::fs:       return "fs";
    case reg::gs:       return "gs";
    case reg::fs_base:  return "fs_base";
    case reg::gs_base:  return "gs_base";
    case reg::orig_rax: return "orig_rax";
    case reg::rip:      return "rip";
    }
}

auto from_string(
    std::string_view reg_str
) -> tl::expected<reg, error::registers> {
    for(const auto& [r, name] : reg_name_lookup) {
        if(name == reg_str) {
            return r;
        }
    }

    return tl::make_unexpected(error::registers::unknown_reg_name);
}

auto dump_registers(pid_t pid) -> void {
    const auto regs = query_user_regs(pid);

    if(!regs) {
        fmt::print("Unable to retrieve register values\n");
        return;
    }

    fmt::print("rax:      {:#018x}\n"
               "rdx:      {:#018x}\n"
               "rcx:      {:#018x}\n"
               "rbx:      {:#018x}\n"
               "rsi:      {:#018x}\n"
               "rdi:      {:#018x}\n"
               "rbp:      {:#018x}\n"
               "rsp:      {:#018x}\n"
               "r8:       {:#018x}\n"
               "r9:       {:#018x}\n"
               "r10:      {:#018x}\n"
               "r11:      {:#018x}\n"
               "r12:      {:#018x}\n"
               "r13:      {:#018x}\n"
               "r14:      {:#018x}\n"
               "r15:      {:#018x}\n"
               "eflags:   {:#018x}\n"
               "es:       {:#018x}\n"
               "cs:       {:#018x}\n"
               "ss:       {:#018x}\n"
               "ds:       {:#018x}\n"
               "fs:       {:#018x}\n"
               "gs:       {:#018x}\n"
               "fs_base:  {:#018x}\n"
               "gs_base:  {:#018x}\n"
               "orig_rax: {:#018x}\n"
               "rip:      {:#018x}\n",
               regs->rax, regs->rdx, regs->rcx, regs->rbx,
               regs->rsi, regs->rdi, regs->rbp, regs->rsp,
               regs->r8, regs->r9, regs->r10, regs->r11,
               regs->r12, regs->r13, regs->r14, regs->r15,
               regs->eflags, regs->es, regs->cs, regs->ss,
               regs->ds, regs->fs, regs->gs, regs->fs_base,
               regs->gs_base, regs->orig_rax, regs->rip
    );
}

}
