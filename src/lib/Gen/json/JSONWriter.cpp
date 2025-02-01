//
// This is a derivative work. originally part of the LLVM Project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdocs
//

#include "JSONWriter.hpp"
#include "lib/Support/Radix.hpp"
#include "mrdocs/Metadata/ExtractionMode.hpp"
#include "mrdocs/Metadata/Info.hpp"
#include "mrdocs/Metadata/Info/Overloads.hpp"
#include "mrdocs/Metadata/Info/Record.hpp"
#include "mrdocs/Metadata/Javadoc.hpp"
#include "mrdocs/Metadata/Symbols.hpp"
#include "mrdocs/Support/TypeTraits.hpp"
#include <llvm/ADT/StringRef.h>
#include <mrdocs/Platform.hpp>
#include <vector>

//------------------------------------------------
//
// JSON
//
//------------------------------------------------

namespace clang {
namespace mrdocs {
namespace json {

JSONWriter::JSONWriter(
    llvm::raw_ostream& os,
    Corpus const & corpus) noexcept
    : json_(os, corpus.config->prettyOutput ? 2 : 0), corpus_(corpus)
{
}

Expected<void>
JSONWriter::build()
{
    json_.objectBegin();
    json_.attribute("root", toBase64(corpus_.globalNamespace().id));

    json_.attributeBegin("index");
    writeIndex();
    json_.attributeEnd();

    json_.objectEnd();

    return {};
}

void
JSONWriter::writeIndex()
{
    json_.objectBegin();
    for (auto& I : corpus_)
    {
        json_.attributeBegin(toBase64(I.id));
        writeInfo(I);
        json_.attributeEnd();
    }
    json_.objectEnd();
}

void
JSONWriter::writeInfo(
    const Info& I)
{
    json_.objectBegin();
    json_.attribute("id", toBase64(I.id));
    json_.attribute("name", I.Name);

    if (I.Access != AccessKind::None)
        json_.attribute("access", llvm::StringRef(toString(I.Access)));
    if (I.Extraction != ExtractionMode::Regular)
        json_.attribute("extraction", llvm::StringRef(toString(I.Extraction)));
    if (I.Parent)
        json_.attribute("parent", toBase64(I.Parent));

    if (I.javadoc)
    {
        json_.attributeBegin("javadoc");
        writeJavadoc(*I.javadoc);
        json_.attributeEnd();
    }

    if (I.DefLoc)
    {
        json_.attributeBegin("def_loc");
        writeLocation(*I.DefLoc);
        json_.attributeEnd();
    }

    if (! I.Loc.empty())
    {
        json_.attributeBegin("locations");
        json_.arrayBegin();
        for (auto const & loc : I.Loc)
            writeLocation(loc);
        json_.arrayEnd();
        json_.attributeEnd(); // locations
    }

    json_.attributeBegin("inner");

    visit(I,
          [this]<typename T>(const T& I)
          {
#define INFO(Type) else if constexpr (T::is##Type()) write##Type(I);
              if (false)
              {
              }
#include <mrdocs/Metadata/InfoNodesPascal.inc>
              else
                  static_assert(false);
          });

    json_.attributeEnd();

    json_.objectEnd();
}

//------------------------------------------------

void
JSONWriter::writeNamespace(
    NamespaceInfo const & I)
{
    taggedObjectBegin("namespace");
    boolAttr("is_inline", I.IsInline);
    boolAttr("is_anonymous", I.IsAnonymous);

    if (! I.UsingDirectives.empty())
    {
        json_.attributeBegin("using_directives");
        writeSymbolIDs(I.UsingDirectives);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeEnum(
    EnumInfo const & I)
{
    taggedObjectBegin("enum");
    if (I.Scoped)
        json_.attribute("scoped", I.Scoped);

    json_.attributeBegin("type");
    writeType(I.UnderlyingType);
    json_.attributeEnd();

    json_.attributeBegin("constants");
    writeSymbolIDs(I.Constants);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeEnumConstant(
    EnumConstantInfo const & I)
{
    taggedObjectBegin("enum_constant");

    if (I.Initializer.Value)
        json_.attribute("value", *I.Initializer.Value);
    json_.attribute("written", I.Initializer.Written);

    taggedObjectEnd();
}

void
JSONWriter::writeFriend(
    FriendInfo const & I)
{
    taggedObjectBegin("friend");

    if (I.FriendSymbol)
        json_.attribute("id", toBase64(I.FriendSymbol));

    if (I.FriendType)
    {
        json_.attributeBegin("type");
        writeType(I.FriendType);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeFunction(
    FunctionInfo const & I)
{
    taggedObjectBegin("function");

    json_.attributeBegin("return_type");
    writeType(I.ReturnType);
    json_.attributeEnd();

    json_.attributeBegin("params");
    writeParams(I.Params);
    json_.attributeEnd();

    tryAppendTemplate(I.Template);

    if (I.Class != FunctionClass::Normal)
        json_.attribute("class", llvm::StringRef(toString(I.Class)));

    if (I.Noexcept.Kind != NoexceptKind::False)
    {
        json_.attributeBegin("noexcept");
        writeNoexcept(I.Noexcept);
        json_.attributeEnd();
    }

    if (! I.Requires.Written.empty())
        json_.attribute("requires", I.Requires.Written);

    boolAttr("is_variadic", I.IsVariadic);
    boolAttr("is_defaulted", I.IsDefaulted);
    boolAttr("is_explicitly_defaulted", I.IsExplicitlyDefaulted);
    boolAttr("is_deleted", I.IsDeleted);
    boolAttr("is_deleted_as_written", I.IsDeletedAsWritten);
    boolAttr("is_no_return", I.IsNoReturn);
    boolAttr("has_override_attr", I.HasOverrideAttr);
    boolAttr("has_trailing_return", I.HasTrailingReturn);
    boolAttr("is_nodiscard", I.IsNodiscard);
    boolAttr("is_explicit_object_member_function",
             I.IsExplicitObjectMemberFunction);

    if (I.Constexpr != ConstexprKind::None)
        json_.attribute("constexpr_kind",
                        llvm::StringRef(toString(I.Constexpr)));

    if (I.OverloadedOperator != OperatorKind::None)
        json_.attribute("overloaded_operator",
                        to_underlying(I.OverloadedOperator));

    if (I.StorageClass != StorageClassKind::None)
        json_.attribute("storage_class",
                        llvm::StringRef(toString(I.StorageClass)));

    if (! I.Attributes.empty())
    {
        json_.attributeBegin("attributes");
        json_.arrayBegin();
        for (const auto& attr : I.Attributes)
        {
            json_.value(attr);
        }
        json_.arrayEnd();
        json_.attributeEnd();
    }

    boolAttr("is_virtual", I.IsVirtual);
    boolAttr("is_virtual_as_written", I.IsVirtualAsWritten);
    boolAttr("is_pure", I.IsPure);
    boolAttr("is_const", I.IsConst);
    boolAttr("is_volatile", I.IsVolatile);
    boolAttr("is_final", I.IsFinal);

    if (I.RefQualifier != ReferenceKind::None)
    {
        json_.attribute("ref_qualifier", to_underlying(I.RefQualifier));
    }

    if (I.Explicit.Kind != ExplicitKind::False)
    {
        json_.attributeBegin("explicit");
        writeExplicit(I.Explicit);
        json_.attributeEnd();
    }

    taggedObjectEnd(); // function
}

void
JSONWriter::writeGuide(
    GuideInfo const & I)
{
    taggedObjectBegin("guide");

    json_.attributeBegin("deduced");
    writeType(I.Deduced);
    json_.attributeEnd();

    tryAppendTemplate(I.Template);

    json_.attributeBegin("params");
    writeParams(I.Params);
    json_.attributeEnd();

    if (I.Explicit.Kind != ExplicitKind::False)
    {
        json_.attributeBegin("explicit");
        writeExplicit(I.Explicit);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeConcept(
    ConceptInfo const & I)
{
    taggedObjectBegin("concept");

    tryAppendTemplate(I.Template);
    json_.attribute("constraint", I.Constraint.Written);

    taggedObjectEnd();
}

void
JSONWriter::writeNamespaceAlias(
    NamespaceAliasInfo const & I)
{
    taggedObjectBegin("namespace_alias");

    json_.attributeBegin("symbol");
    writeNameInfo(*I.AliasedSymbol);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeUsing(
    UsingInfo const & I)
{
    taggedObjectBegin("using");
    json_.attribute("class", llvm::StringRef(toString(I.Class)));

    json_.attributeBegin("symbols");
    writeSymbolIDs(I.UsingSymbols);
    json_.attributeEnd();

    if (I.Qualifier)
    {
        json_.attributeBegin("qualifier");
        writeNameInfo(*I.Qualifier);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeRecord(
    RecordInfo const & I)
{
    taggedObjectBegin("record");

    json_.attribute("kind", llvm::StringRef(toString(I.KeyKind)));
    tryAppendTemplate(I.Template);

    boolAttr("is_type_def", I.IsTypeDef);
    boolAttr("is_final", I.IsFinal);
    boolAttr("is_final_destructor", I.IsFinalDestructor);

    if (! I.Bases.empty())
    {
        json_.attributeBegin("bases");
        json_.arrayBegin();
        for (const auto& base : I.Bases)
        {
            json_.objectBegin();

            json_.attributeBegin("type");
            writeType(base.Type);
            json_.attributeEnd();

            json_.attribute("access", llvm::StringRef(toString(base.Access)));
            if (base.IsVirtual)
                json_.attribute("is_virtual", base.IsVirtual);

            json_.objectEnd();
        }
        json_.arrayEnd();
        json_.attributeEnd();
    }

    json_.attributeBegin("interface");
    writeRecordInterface(I.Interface);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeRecordInterface(
    const RecordInterface& I)
{
    json_.objectBegin();
    if (I.Public)
    {
        json_.attributeBegin("public");
        writeRecordTranche(I.Public);
        json_.attributeEnd();
    }
    if (I.Protected)
    {
        json_.attributeBegin("protected");
        writeRecordTranche(I.Protected);
        json_.attributeEnd();
    }
    if (I.Private)
    {
        json_.attributeBegin("private");
        writeRecordTranche(I.Private);
        json_.attributeEnd();
    }
    json_.objectEnd();
}

void
JSONWriter::writeRecordTranche(
    const RecordTranche& T)
{
    auto write = [&](llvm::StringRef name, const std::vector<SymbolID>& syms)
    {
        if (! syms.empty())
        {
            json_.attributeBegin(name);
            writeSymbolIDs(syms);
            json_.attributeEnd();
        }
    };

    json_.objectBegin();
    write("namespace_aliases", T.NamespaceAliases);
    write("typedefs", T.Typedefs);
    write("records", T.Records);
    write("enums", T.Enums);
    write("functions", T.Functions);
    write("static_functions", T.StaticFunctions);
    write("variables", T.Variables);
    write("static_variables", T.StaticVariables);
    write("concepts", T.Concepts);
    write("guides", T.Guides);
    write("friends", T.Friends);
    write("usings", T.Usings);
    json_.objectEnd();
}

void
JSONWriter::writeTypedef(
    TypedefInfo const & I)
{
    taggedObjectBegin("typedef");

    json_.attributeBegin("type");
    writeType(I.Type);
    json_.attributeEnd();

    json_.attribute("is_using", I.IsUsing);

    tryAppendTemplate(I.Template);

    taggedObjectEnd();
}

void
JSONWriter::writeField(
    const FieldInfo& I)
{
    taggedObjectBegin("field");

    json_.attributeBegin("type");
    writeType(I.Type);
    json_.attributeEnd();

    if (! I.Default.Written.empty())
        json_.attribute("default", I.Default.Written);

    boolAttr("is_variant", I.IsVariant);
    boolAttr("is_mutable", I.IsMutable);
    boolAttr("is_bitfield", I.IsBitfield);

    if (I.BitfieldWidth.Value)
    {
        json_.attributeBegin("bitfield_width");
        json_.objectBegin();
        json_.attribute("value", *I.BitfieldWidth.Value);
        json_.attribute("written", I.BitfieldWidth.Written);
        json_.objectEnd();
        json_.attributeEnd();
    }

    boolAttr("is_maybe_unused", I.IsMaybeUnused);
    boolAttr("is_deprecated", I.IsDeprecated);
    boolAttr("has_no_unique_address", I.HasNoUniqueAddress);

    if (! I.Attributes.empty())
    {
        json_.attributeBegin("attributes");
        json_.arrayBegin();
        for (const auto& attr : I.Attributes)
        {
            json_.value(attr);
        }
        json_.arrayEnd();
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeVariable(
    VariableInfo const & I)
{
    taggedObjectBegin("variable");

    json_.attributeBegin("type");
    writeType(I.Type);
    json_.attributeEnd();

    tryAppendTemplate(I.Template);

    if (! I.Initializer.Written.empty())
        json_.attribute("initializer", I.Initializer.Written);

    if (I.StorageClass != StorageClassKind::None)
        json_.attribute("storage_class",
                        llvm::StringRef(toString(I.StorageClass)));

    boolAttr("is_inline", I.IsInline);
    boolAttr("is_constexpr", I.IsConstexpr);
    boolAttr("is_constinit", I.IsConstinit);
    boolAttr("is_thread_local", I.IsThreadLocal);

    if (! I.Attributes.empty())
    {
        json_.attributeBegin("attributes");
        json_.arrayBegin();
        for (const auto& attr : I.Attributes)
        {
            json_.value(attr);
        }
        json_.arrayEnd();
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeOverloads(
    OverloadsInfo const & O)
{
    taggedObjectBegin("overloads");

    if (O.Class != FunctionClass::Normal)
        json_.attribute("class", llvm::StringRef(toString(O.Class)));
    if (O.OverloadedOperator != OperatorKind::None)
        json_.attribute("operator", to_underlying(O.OverloadedOperator));

    json_.attributeBegin("members");
    writeSymbolIDs(O.Members);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeLocation(
    Location const & loc)
{
    json_.objectBegin();
    if (corpus_.config->fullPaths)
        json_.attribute("full_path", loc.FullPath);
    json_.attribute("short_path", loc.ShortPath);
    json_.attribute("source_path", loc.SourcePath);
    json_.attribute("line", loc.LineNumber);
    boolAttr("documented", loc.Documented);
    json_.objectEnd();
}

void
JSONWriter::writeSpecialization(
    const SpecializationInfo& I)
{
    taggedObjectBegin("specialization");

    json_.attributeBegin("args");
    json_.arrayBegin();
    for (const auto& targ : I.Args)
        writeTArg(*targ);
    json_.arrayEnd();
    json_.attributeEnd(); // args

    json_.attribute("primary", toBase64(I.Primary));

    taggedObjectEnd();
}

//------------------------------------------------

void
JSONWriter::writeJavadoc(
    Javadoc const & javadoc)
{
    writeBlocks(javadoc.getBlocks());
}

template <typename T>
void
JSONWriter::writeBlocks(
    const std::vector<PolymorphicValue<T>>& blocks)
{
    json_.arrayBegin();
    for (auto const & block : blocks)
        writeNode(*block);
    json_.arrayEnd();
}

void
JSONWriter::writeNode(
    doc::Node const & node)
{
    switch (node.kind)
    {
    case doc::Kind::text:
        writeText(dynamic_cast<doc::Text const &>(node));
        break;
    case doc::Kind::admonition:
        writeAdmonition(dynamic_cast<doc::Admonition const &>(node));
        break;
    case doc::Kind::brief:
        writeBrief(dynamic_cast<doc::Brief const &>(node));
        break;
    case doc::Kind::code:
        writeCode(dynamic_cast<doc::Code const &>(node));
        break;
    case doc::Kind::heading:
        writeHeading(dynamic_cast<doc::Heading const &>(node));
        break;
    case doc::Kind::link:
        writeLink(dynamic_cast<doc::Link const &>(node));
        break;
    case doc::Kind::list_item:
        writeListItem(dynamic_cast<doc::ListItem const &>(node));
        break;
    case doc::Kind::unordered_list:
        writeUnorderedList(dynamic_cast<doc::UnorderedList const &>(node));
        break;
    case doc::Kind::paragraph:
        writeParagraph(dynamic_cast<doc::Paragraph const &>(node));
        break;
    case doc::Kind::param:
        writeJParam(dynamic_cast<doc::Param const &>(node));
        break;
    case doc::Kind::returns:
        writeReturns(dynamic_cast<doc::Returns const &>(node));
        break;
    case doc::Kind::styled:
        writeStyledText(dynamic_cast<doc::Styled const &>(node));
        break;
    case doc::Kind::tparam:
        writeJTParam(dynamic_cast<doc::TParam const &>(node));
        break;
    case doc::Kind::reference:
        writeReference(dynamic_cast<doc::Reference const &>(node));
        break;
    case doc::Kind::copied:
        writeCopied(dynamic_cast<doc::Copied const &>(node));
        break;
    case doc::Kind::throws:
        writeThrows(dynamic_cast<doc::Throws const &>(node));
        break;
    case doc::Kind::details:
        writeDetails(dynamic_cast<doc::Details const &>(node));
        break;
    case doc::Kind::see:
        writeSee(dynamic_cast<doc::See const &>(node));
        break;
    case doc::Kind::precondition:
        writePrecondition(dynamic_cast<doc::Precondition const &>(node));
        break;
    case doc::Kind::postcondition:
        writePostcondition(dynamic_cast<doc::Postcondition const &>(node));
        break;
    default:
        // unknown kind
        MRDOCS_UNREACHABLE();
    }
}

void
JSONWriter::writeReference(
    doc::Reference const & node)
{
    taggedValueBegin("reference");
    writeInnerReference(node);
    taggedValueEnd();
}

void
JSONWriter::writeInnerReference(
    doc::Reference const & node)
{
    json_.objectBegin();
    json_.attribute("id", toBase64(node.id));
    json_.attribute("text", node.string);
    json_.objectEnd();
}

void
JSONWriter::writeCopied(
    doc::Copied const & node)
{
    llvm::StringRef parts;
    switch (node.parts)
    {
    case doc::Parts::all:
        parts = "all";
        break;
    case doc::Parts::brief:
        parts = "brief";
        break;
    case doc::Parts::description:
        parts = "description";
        break;
    default:
        MRDOCS_UNREACHABLE();
    }

    taggedObjectBegin("copied");
    json_.attribute("parts", parts);
    json_.attribute("id", toBase64(node.id));
    json_.attribute("text", node.string);
    taggedObjectEnd();
}

void
JSONWriter::writeLink(
    doc::Link const & node)
{
    taggedObjectBegin("link");
    json_.attribute("href", node.href);
    json_.attribute("text", node.string);
    taggedObjectEnd();
}

void
JSONWriter::writeListItem(
    doc::ListItem const & node)
{
    taggedValueBegin("list_item");
    writeInnerListItem(node);
    taggedValueEnd();
}

void
JSONWriter::writeInnerListItem(
    doc::ListItem const & node)
{
    writeBlocks(node.children);
}

void
JSONWriter::writeUnorderedList(
    doc::UnorderedList const & node)
{
    taggedObjectBegin("unordered_list");

    json_.attributeBegin("items");
    json_.arrayBegin();
    for (const auto& item : node.items)
    {
        writeInnerListItem(item);
    }
    json_.arrayEnd();
    json_.attributeEnd();

    if (! node.children.empty())
    {
        json_.attributeBegin("children");
        writeBlocks(node.children);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeBrief(
    doc::Brief const & node)
{
    taggedValueBegin("brief");
    writeBlocks(node.children);
    taggedValueEnd();
}

void
JSONWriter::writeText(
    doc::Text const & node)
{
    taggedValueBegin("text");
    json_.value(node.string);
    taggedValueEnd();
}

void
JSONWriter::writeStyledText(
    doc::Styled const & node)
{
    taggedObjectBegin("styled");
    json_.attribute("style", llvm::StringRef(toString(node.style)));
    json_.attribute("text", node.string);
    taggedObjectEnd();
}

void
JSONWriter::writeHeading(
    doc::Heading const & heading)
{
    taggedObjectBegin("heading");

    json_.attribute("text", heading.string);

    if (! heading.children.empty())
    {
        json_.attributeBegin("children");
        writeBlocks(heading.children);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeParagraph(
    doc::Paragraph const & para)
{
    taggedValueBegin("paragraph");
    writeBlocks(para.children);
    taggedValueEnd();
}

void
JSONWriter::writeDetails(
    doc::Details const & node)
{
    taggedValueBegin("details");
    writeBlocks(node.children);
    taggedValueEnd();
}

void
JSONWriter::writeSee(
    doc::See const & node)
{
    taggedValueBegin("see");
    writeBlocks(node.children);
    taggedValueEnd();
}

void
JSONWriter::writePrecondition(
    doc::Precondition const & node)
{
    taggedValueBegin("precondition");
    writeBlocks(node.children);
    taggedValueEnd();
}

void
JSONWriter::writePostcondition(
    doc::Postcondition const & node)
{
    taggedValueBegin("postcondition");
    writeBlocks(node.children);
    taggedValueEnd();
}

void
JSONWriter::writeAdmonition(
    doc::Admonition const & node)
{
    llvm::StringRef kind;
    switch (node.admonish)
    {
    case doc::Admonish::note:
        kind = "note";
        break;
    case doc::Admonish::tip:
        kind = "tip";
        break;
    case doc::Admonish::important:
        kind = "important";
        break;
    case doc::Admonish::caution:
        kind = "caution";
        break;
    case doc::Admonish::warning:
        kind = "warning";
        break;
    default:
        // unknown style
        MRDOCS_UNREACHABLE();
    }
    taggedObjectBegin("admonition");
    json_.attribute("kind", kind);

    json_.attributeBegin("content");
    writeBlocks(node.children);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeCode(
    doc::Code const & code)
{
    taggedValueBegin("code");
    writeBlocks(code.children);
    taggedValueEnd();
}

void
JSONWriter::writeReturns(
    doc::Returns const & node)
{
    taggedValueBegin("returns");
    writeBlocks(node.children);
    taggedValueEnd();
}

void
JSONWriter::writeThrows(
    doc::Throws const & node)
{
    taggedObjectBegin("throws");
    json_.attributeBegin("exception");
    writeInnerReference(node.exception);
    json_.attributeEnd();

    json_.attributeBegin("content");
    writeBlocks(node.children);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeJParam(
    doc::Param const & param)
{
    llvm::StringRef direction;
    switch (param.direction)
    {
    case doc::ParamDirection::none:
        break; // empty
    case doc::ParamDirection::in:
        direction = "in";
        break;
    case doc::ParamDirection::out:
        direction = "out";
        break;
    case doc::ParamDirection::inout:
        direction = "inout";
        break;
    default:
        MRDOCS_UNREACHABLE();
    }

    taggedObjectBegin("param");

    if (! direction.empty())
        json_.attribute("direction", direction);

    json_.attribute("name", param.name);

    json_.attributeBegin("content");
    writeBlocks(param.children);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeJTParam(
    doc::TParam const & tparam)
{
    taggedObjectBegin("tparam");

    json_.attribute("name", tparam.name);

    json_.attributeBegin("content");
    writeBlocks(tparam.children);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::taggedValueBegin(
    llvm::StringRef key)
{
    json_.objectBegin();
    json_.attributeBegin(key);
}

void
JSONWriter::taggedValueEnd()
{
    json_.attributeEnd();
    json_.objectEnd();
}

void
JSONWriter::taggedObjectBegin(
    llvm::StringRef key)
{
    taggedValueBegin(key);
    json_.objectBegin();
}

void
JSONWriter::taggedObjectEnd()
{
    json_.objectEnd();
    taggedValueEnd();
}

void
JSONWriter::taggedArrayBegin(
    llvm::StringRef key)
{
    taggedValueBegin(key);
    json_.arrayBegin();
}

void
JSONWriter::taggedArrayEnd()
{
    json_.arrayEnd();
    taggedValueEnd();
}

void
JSONWriter::writeSymbolIDs(
    std::span<const SymbolID> IDs)
{
    json_.arrayBegin();
    for (const auto& I : IDs)
    {
        json_.value(toBase64(I));
    }
    json_.arrayEnd();
}

void
JSONWriter::writeNoexcept(
    const NoexceptInfo& I)
{
    json_.objectBegin();

    json_.attribute("implicit", I.Implicit);
    json_.attribute("kind", llvm::StringRef(toString(I.Kind)));
    if (! I.Operand.empty())
        json_.attribute("operand", I.Operand);

    json_.objectEnd();
}

void
JSONWriter::writeExplicit(
    const ExplicitInfo& I)
{
    json_.objectBegin();

    json_.attribute("implicit", I.Implicit);
    json_.attribute("kind", llvm::StringRef(toString(I.Kind)));
    if (! I.Operand.empty())
        json_.attribute("operand", I.Operand);

    json_.objectEnd();
}

void
JSONWriter::writeNameInfo(
    const NameInfo& I)
{
    json_.objectBegin();

    json_.attribute("kind", llvm::StringRef(toString(I.Kind)));
    json_.attribute("name", I.Name);

    if (I.id)
        json_.attribute("id", toBase64(I.id));

    if (I.Prefix)
    {
        json_.attributeBegin("prefix");
        writeNameInfo(*I.Prefix);
        json_.attributeEnd();
    }

    visit(I,
          [this](const auto& I)
          {
              if constexpr (requires { I.TemplateArgs; })
              {
                  json_.attributeBegin("template_args");
                  json_.arrayBegin();
                  for (const auto& arg : I.TemplateArgs)
                      writeTArg(*arg);
                  json_.arrayEnd();
                  json_.attributeEnd();
              }
          });

    json_.objectEnd();
}

void
JSONWriter::writeParams(
    std::span<const Param> Params)
{
    json_.arrayBegin();
    for (const auto& P : Params)
    {
        json_.objectBegin();
        json_.attribute("name", P.Name);
        json_.attributeBegin("type");
        writeType(P.Type);
        json_.attributeEnd();
        if (! P.Default.empty())
        {
            json_.attribute("default", P.Default);
        }
        json_.objectEnd();
    }
    json_.arrayEnd();
}

void
JSONWriter::writeType(
    const PolymorphicValue<TypeInfo>& T)
{
    if (! T)
    {
        json_.value(nullptr);
        return;
    }
    llvm::StringRef kind;
    switch (T->Kind)
    {

    case TypeKind::Named:
        kind = "named";
        break;
    case TypeKind::Decltype:
        kind = "decltype";
        break;
    case TypeKind::Auto:
        kind = "auto";
        break;
    case TypeKind::LValueReference:
        kind = "lvalue_reference";
        break;
    case TypeKind::RValueReference:
        kind = "rvalue_reference";
        break;
    case TypeKind::Pointer:
        kind = "pointer";
        break;
    case TypeKind::MemberPointer:
        kind = "member_pointer";
        break;
    case TypeKind::Array:
        kind = "array";
        break;
    case TypeKind::Function:
        kind = "function";
        break;
    default:
        MRDOCS_UNREACHABLE();
    }

    taggedObjectBegin(kind);

    visit(
        *T,
        [this]<typename T>(const T& t)
        {
            if constexpr (requires { t.ParentType; })
            {
                if (t.ParentType)
                {
                    json_.attributeBegin("parent");
                    writeType(t.ParentType);
                    json_.attributeEnd();
                }
            }

            if constexpr (T::isNamed())
            {
                if (t.Name)
                {
                    json_.attributeBegin("name");
                    writeNameInfo(*t.Name);
                    json_.attributeEnd();
                }
            }

            if constexpr (requires { t.CVQualifiers; })
            {
                if (t.CVQualifiers != QualifierKind::None)
                    json_.attribute("cv_qualifiers",
                                    llvm::StringRef(toString(t.CVQualifiers)));
            }

            if constexpr (T::isArray())
            {
                json_.attributeBegin("element_type");
                writeType(t.ElementType);
                json_.attributeEnd();

                json_.attributeBegin("bounds");
                json_.objectBegin();
                if (t.Bounds.Value)
                    json_.attribute("value", *t.Bounds.Value);
                json_.attribute("written", t.Bounds.Written);
                json_.objectEnd();
                json_.attributeEnd(); // bounds
            }

            if constexpr (T::isDecltype())
            {
                json_.attribute("operand", t.Operand.Written);
            }

            if constexpr (T::isAuto())
            {
                json_.attribute("keyword",
                                llvm::StringRef(toString(t.Keyword)));
                if (t.Constraint)
                {
                    json_.attributeBegin("constraint");
                    writeNameInfo(*t.Constraint);
                    json_.attributeEnd();
                }
            }

            if constexpr (T::isFunction())
            {
                if (t.IsVariadic)
                    json_.attribute("is_variadic", t.IsVariadic);

                if (t.RefQualifier != ReferenceKind::None)
                    json_.attribute("ref_qualifier",
                                    llvm::StringRef(toString(t.RefQualifier)));

                if (t.ExceptionSpec.Kind != NoexceptKind::False)
                {
                    json_.attributeBegin("exception_spec");
                    writeNoexcept(t.ExceptionSpec);
                    json_.attributeEnd();
                }

                json_.attributeBegin("return");
                writeType(t.ReturnType);
                json_.attributeEnd();

                json_.attributeBegin("params");
                json_.arrayBegin();
                for (const auto& p : t.ParamTypes)
                    writeType(p);
                json_.arrayEnd();
                json_.attributeEnd(); // params
            }

            if constexpr (requires { t.PointeeType; })
            {
                json_.attributeBegin("pointee");
                writeType(t.PointeeType);
                json_.attributeEnd();
            }
        });

    taggedObjectEnd();
}

void
JSONWriter::tryAppendTemplate(
    const std::optional<TemplateInfo>& T)
{
    if (! T)
    {
        return;
    }

    json_.attributeBegin("template");
    json_.objectBegin();

    json_.attribute("kind", llvm::StringRef(toString(T->specializationKind())));

    if (! T->Params.empty())
    {
        json_.attributeBegin("params");
        json_.arrayBegin();
        for (const auto& P : T->Params)
        {
            writeTParam(*P);
        }
        json_.arrayEnd();
        json_.attributeEnd(); // params
    }

    if (! T->Args.empty())
    {
        json_.attributeBegin("args");
        json_.arrayBegin();
        for (const auto& A : T->Args)
        {
            writeTArg(*A);
        }
        json_.arrayEnd();
        json_.attributeEnd(); // args
    }

    if (! T->Requires.Written.empty())
        json_.attribute("requires", T->Requires.Written);
    if (T->Primary)
        json_.attribute("primary", toBase64(T->Primary));

    json_.objectEnd();
    json_.attributeEnd(); // template
}

void
JSONWriter::writeTParam(
    const TParam& T)
{
    visit(T,
          [this](const auto& T)
          {
              if constexpr (T.isType())
                  writeTypeTParam(T);
              else if constexpr (T.isNonType())
                  writeNonTypeTParam(T);
              else if constexpr (T.isTemplate())
                  writeTemplateTParam(T);
              else
                  static_assert(false);
          });
}

void
JSONWriter::writeTypeTParam(
    const TypeTParam& T)
{
    taggedObjectBegin("type");
    appendTParam(T);

    json_.attribute("key", llvm::StringRef(toString(T.KeyKind)));
    if (T.Constraint)
    {
        json_.attributeBegin("constraint");
        writeNameInfo(*T.Constraint);
        json_.attributeEnd();
    }

    taggedObjectEnd();
}

void
JSONWriter::writeNonTypeTParam(
    const NonTypeTParam& T)
{
    taggedObjectBegin("non_type");
    appendTParam(T);

    json_.attributeBegin("type");
    writeType(T.Type);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeTemplateTParam(
    const TemplateTParam& T)
{
    taggedObjectBegin("template");
    appendTParam(T);

    json_.attributeBegin("params");
    json_.arrayBegin();
    for (const auto& p : T.Params)
        writeTParam(*p);
    json_.arrayEnd();
    json_.attributeEnd(); // params

    taggedObjectEnd();
}

void
JSONWriter::appendTParam(
    const TParam& T)
{
    json_.attribute("name", T.Name);
    if (T.IsParameterPack)
        json_.attribute("is_parameter_pack", T.IsParameterPack);
    if (T.Default)
    {
        json_.attributeBegin("default");
        writeTArg(*T.Default);
        json_.attributeEnd();
    }
}

void
JSONWriter::writeTArg(
    const TArg& T)
{
    visit(T,
          [this](const auto& T)
          {
              if constexpr (T.isType())
                  writeTypeTArg(T);
              else if constexpr (T.isNonType())
                  writeNonTypeTArg(T);
              else if constexpr (T.isTemplate())
                  writeTemplateTArg(T);
              else
                  static_assert(false);
          });
}

void
JSONWriter::writeTypeTArg(
    const TypeTArg& T)
{
    taggedObjectBegin("type");

    if (T.IsPackExpansion)
        json_.attribute("is_pack_expansion", T.IsPackExpansion);

    json_.attributeBegin("type");
    writeType(T.Type);
    json_.attributeEnd();

    taggedObjectEnd();
}

void
JSONWriter::writeNonTypeTArg(
    const NonTypeTArg& T)
{
    taggedObjectBegin("non_type");

    if (T.IsPackExpansion)
        json_.attribute("is_pack_expansion", T.IsPackExpansion);

    json_.attribute("value", T.Value.Written);

    taggedObjectEnd();
}

void
JSONWriter::writeTemplateTArg(
    const TemplateTArg& T)
{
    taggedObjectBegin("template");

    if (T.IsPackExpansion)
        json_.attribute("is_pack_expansion", T.IsPackExpansion);

    json_.attribute("template", toBase64(T.Template));
    json_.attribute("name", T.Name);

    taggedObjectEnd();
}

void
JSONWriter::boolAttr(
    llvm::StringRef name,
    bool value)
{
    if (value)
        json_.attribute(name, value);
}

} // namespace json
} // namespace mrdocs
} // namespace clang
