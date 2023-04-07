#pragma once

namespace nkgt::error {

enum class breakpoint {
    peek_address_fail,
    poke_address_fail,
};

enum class registers {
    getregs_fail,
    setregs_fail,
    unknown_dwarf_number,
    unknown_reg_name,
};

enum class address {
    malformed_register,
};

enum class debug_symbols {
    load_fail,
};

}
