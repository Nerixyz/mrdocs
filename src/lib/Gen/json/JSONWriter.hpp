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

#ifndef MRDOCS_LIB_GEN_XML_JSONWriter_HPP
#define MRDOCS_LIB_GEN_XML_JSONWriter_HPP

#include "lib/Support/YamlFwd.hpp"
#include "mrdocs/ADT/PolymorphicValue.hpp"
#include "mrdocs/Metadata/Info/Record.hpp"
#include "mrdocs/Metadata/Javadoc.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/JSON.h>
#include <mrdocs/Corpus.hpp>
#include <mrdocs/Metadata.hpp>
#include <mrdocs/Support/Error.hpp>

namespace clang {
namespace mrdocs {
namespace json {

/** A writer which outputs JSON.
 */
class JSONWriter
{
    template <class T> friend struct llvm::yaml::MappingTraits;

  protected:
    llvm::json::OStream json_;
    Corpus const & corpus_;

  public:
    JSONWriter(llvm::raw_ostream& os, Corpus const & corpus) noexcept;

    Expected<void> build();

  private:
    void writeIndex();

    void writeInfo(const Info& I);

#define INFO(Type) void write##Type(Type##Info const &);
#include <mrdocs/Metadata/InfoNodesPascal.inc>

    void writeLocation(Location const & loc);
    void writeJavadoc(Javadoc const & javadoc);

    template <typename T>
    void writeBlocks(const std::vector<PolymorphicValue<T>>& blocks);

    void writeNode(doc::Node const & node);

    void writeAdmonition(doc::Admonition const & node);
    void writeBrief(doc::Brief const & node);
    void writeCode(doc::Code const & node);
    void writeHeading(doc::Heading const & node);
    void writeLink(doc::Link const & node);
    void writeListItem(doc::ListItem const & node);
    void writeUnorderedList(doc::UnorderedList const & node);
    void writeParagraph(doc::Paragraph const & node);
    void writeJParam(doc::Param const & node);
    void writeReturns(doc::Returns const & node);
    void writeStyledText(doc::Styled const & node);
    void writeJTParam(doc::TParam const & node);
    void writeReference(doc::Reference const & node);
    void writeCopied(doc::Copied const & node);
    void writeThrows(doc::Throws const & node);
    void writeDetails(doc::Details const & node);
    void writeSee(doc::See const & node);
    void writePrecondition(doc::Precondition const & node);
    void writePostcondition(doc::Postcondition const & node);

    void writeText(doc::Text const & node);
    void writeInnerReference(doc::Reference const & node);
    void writeInnerListItem(doc::ListItem const & node);

    void taggedValueBegin(llvm::StringRef key);
    void taggedValueEnd();

    void taggedObjectBegin(llvm::StringRef key);
    void taggedObjectEnd();

    void taggedArrayBegin(llvm::StringRef key);
    void taggedArrayEnd();

    void writeSymbolIDs(std::span<const SymbolID> IDs);

    void writeNoexcept(const NoexceptInfo& N);
    void writeExplicit(const ExplicitInfo& E);

    void writeNameInfo(const NameInfo& N);
    void appendSpecializationNameInfo(const SpecializationNameInfo& N);

    void writeParams(std::span<const Param> Params);

    void writeType(const PolymorphicValue<TypeInfo>& T);
    void writeParam(const Param& P);

    void writeTParam(const TParam& T);
    void writeTypeTParam(const TypeTParam& T);
    void writeNonTypeTParam(const NonTypeTParam& T);
    void writeTemplateTParam(const TemplateTParam& T);

    void appendTParam(const TParam& T);

    void writeTArg(const TArg& T);
    void writeTypeTArg(const TypeTArg& T);
    void writeNonTypeTArg(const NonTypeTArg& T);
    void writeTemplateTArg(const TemplateTArg& T);

    void writeRecordInterface(const RecordInterface& I);
    void writeRecordTranche(const RecordTranche& T);

    void tryAppendTemplate(const std::optional<TemplateInfo>& T);

    void boolAttr(llvm::StringRef name, bool value);
};

} // namespace json
} // namespace mrdocs
} // namespace clang

#endif
