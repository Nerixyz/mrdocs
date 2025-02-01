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

#include "JSONGenerator.hpp"
#include "JSONWriter.hpp"
#include "lib/Support/Radix.hpp"
#include "lib/Support/RawOstream.hpp"
#include <mrdocs/Metadata.hpp>
#include <mrdocs/Support/Error.hpp>


namespace clang {
namespace mrdocs {
namespace json {

Expected<void>
JSONGenerator::buildOne(
    std::ostream& os,
    Corpus const & corpus) const
{
    RawOstream raw_os(os);
    return JSONWriter(raw_os, corpus).build();
}

} // namespace json

//------------------------------------------------

std::unique_ptr<Generator>
makeJSONGenerator()
{
    return std::make_unique<json::JSONGenerator>();
}

} // namespace mrdocs
} // namespace clang
