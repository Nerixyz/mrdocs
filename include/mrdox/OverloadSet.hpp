//
// This is a derivative work. originally part of the LLVM Project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdox
//

#include <functional>

namespace clang {
namespace mrdox {

class Corpus;
struct FunctionInfo;

//------------------------------------------------

struct OverloadSet
{
    llvm::StringRef name;
    std::vector<FunctionInfo const*> list;
};

std::vector<OverloadSet>
makeOverloadSet(
    Corpus const& corpus,
    Scope const& scope,
    std::function<bool(FunctionInfo const&)> filter);

} // mrdox
} // clang
