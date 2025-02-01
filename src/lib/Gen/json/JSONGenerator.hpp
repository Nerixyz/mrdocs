//
// This is a derivative work. originally part of the LLVM Project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdocs
//

#ifndef MRDOCS_LIB_GEN_JSON_JSONGENERATOR_HPP
#define MRDOCS_LIB_GEN_JSON_JSONGENERATOR_HPP

#include <mrdocs/Generator.hpp>

namespace clang {
namespace mrdocs {
namespace json {

//------------------------------------------------

struct JSONGenerator : Generator
{
    std::string_view id() const noexcept override { return "json"; }

    std::string_view displayName() const noexcept override
    {
        return "JavaScript Object Notation (JSON)";
    }

    std::string_view fileExtension() const noexcept override { return "json"; }

    Expected<void> buildOne(std::ostream& os,
                            Corpus const & corpus) const override;
};

} // namespace json
} // namespace mrdocs
} // namespace clang

#endif
