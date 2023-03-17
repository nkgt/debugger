#pragma once
#include "nkgt/error_codes.hpp"

#include <tl/expected.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace nkgt::registers {

enum class reg {
    rax, rbx, rcx, rdx,
    rdi, rsi, rbp, rsp,
    r8,  r9,  r10, r11,
    r12, r13, r14, r15,
    rip, eflags,    cs,
    orig_rax, fs_base,
    gs_base,
    fs, gs, ss, ds, es
};

struct reg_descriptor {
    reg r;
    int dwarf_r;
};

[[nodiscard]]
auto get_register_value(
    pid_t pid,
    reg r
) -> tl::expected<uint64_t, error::registers>;

[[nodiscard]]
auto get_register_value_from_dwarf_number(
    pid_t pid,
    unsigned dwarf_number
) -> tl::expected<uint64_t, error::registers>;

[[nodiscard]]
auto set_register_value(
    pid_t pid,
    reg r,
    uint64_t value
) -> tl::expected<void, error::registers>;

[[nodiscard]]
auto to_string(reg r) -> std::string;

[[nodiscard]]
auto from_string(std::string_view r) -> tl::expected<reg, error::registers>;

auto dump_registers(pid_t pid) -> void;

}
