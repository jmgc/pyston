// Copyright (c) 2014-2016 Dropbox, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PYSTON_CORE_AST_H
#define PYSTON_CORE_AST_H

#include <cassert>
#include <cstdlib>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/raw_ostream.h"

#include "analysis/scoping_analysis.h"
#include "core/common.h"
#include "core/stringpool.h"

namespace pyston {

namespace AST_TYPE {
// These are in a pretty random order (started off alphabetical but then I had to add more).
// These can be changed freely as long as parse_ast.py is also updated
#define FOREACH_TYPE(X)                                                                                                \
    X(alias, 1)                                                                                                        \
    X(arguments, 2)                                                                                                    \
    X(Assert, 3)                                                                                                       \
    X(Assign, 4)                                                                                                       \
    X(Attribute, 5)                                                                                                    \
    X(AugAssign, 6)                                                                                                    \
    X(BinOp, 7)                                                                                                        \
    X(BoolOp, 8)                                                                                                       \
    X(Call, 9)                                                                                                         \
    X(ClassDef, 10)                                                                                                    \
    X(Compare, 11)                                                                                                     \
    X(comprehension, 12)                                                                                               \
    X(Delete, 13)                                                                                                      \
    X(Dict, 14)                                                                                                        \
    X(Exec, 16)                                                                                                        \
    X(ExceptHandler, 17)                                                                                               \
    X(ExtSlice, 18)                                                                                                    \
    X(Expr, 19)                                                                                                        \
    X(For, 20)                                                                                                         \
    X(FunctionDef, 21)                                                                                                 \
    X(GeneratorExp, 22)                                                                                                \
    X(Global, 23)                                                                                                      \
    X(If, 24)                                                                                                          \
    X(IfExp, 25)                                                                                                       \
    X(Import, 26)                                                                                                      \
    X(ImportFrom, 27)                                                                                                  \
    X(Index, 28)                                                                                                       \
    X(keyword, 29)                                                                                                     \
    X(Lambda, 30)                                                                                                      \
    X(List, 31)                                                                                                        \
    X(ListComp, 32)                                                                                                    \
    X(Module, 33)                                                                                                      \
    X(Num, 34)                                                                                                         \
    X(Name, 35)                                                                                                        \
    X(Pass, 37)                                                                                                        \
    X(Pow, 38)                                                                                                         \
    X(Print, 39)                                                                                                       \
    X(Raise, 40)                                                                                                       \
    X(Repr, 41)                                                                                                        \
    X(Return, 42)                                                                                                      \
    X(Slice, 44)                                                                                                       \
    X(Str, 45)                                                                                                         \
    X(Subscript, 46)                                                                                                   \
    X(TryExcept, 47)                                                                                                   \
    X(TryFinally, 48)                                                                                                  \
    X(Tuple, 49)                                                                                                       \
    X(UnaryOp, 50)                                                                                                     \
    X(With, 51)                                                                                                        \
    X(While, 52)                                                                                                       \
    X(Yield, 53)                                                                                                       \
    X(Store, 54)                                                                                                       \
    X(Load, 55)                                                                                                        \
    X(Param, 56)                                                                                                       \
    X(Not, 57)                                                                                                         \
    X(In, 58)                                                                                                          \
    X(Is, 59)                                                                                                          \
    X(IsNot, 60)                                                                                                       \
    X(Or, 61)                                                                                                          \
    X(And, 62)                                                                                                         \
    X(Eq, 63)                                                                                                          \
    X(NotEq, 64)                                                                                                       \
    X(NotIn, 65)                                                                                                       \
    X(GtE, 66)                                                                                                         \
    X(Gt, 67)                                                                                                          \
    X(Mod, 68)                                                                                                         \
    X(Add, 69)                                                                                                         \
    X(Continue, 70)                                                                                                    \
    X(Lt, 71)                                                                                                          \
    X(LtE, 72)                                                                                                         \
    X(Break, 73)                                                                                                       \
    X(Sub, 74)                                                                                                         \
    X(Del, 75)                                                                                                         \
    X(Mult, 76)                                                                                                        \
    X(Div, 77)                                                                                                         \
    X(USub, 78)                                                                                                        \
    X(BitAnd, 79)                                                                                                      \
    X(BitOr, 80)                                                                                                       \
    X(BitXor, 81)                                                                                                      \
    X(RShift, 82)                                                                                                      \
    X(LShift, 83)                                                                                                      \
    X(Invert, 84)                                                                                                      \
    X(UAdd, 85)                                                                                                        \
    X(FloorDiv, 86)                                                                                                    \
    X(DictComp, 15)                                                                                                    \
    X(Set, 43)                                                                                                         \
    X(Ellipsis, 87)                                                                                                    \
    /* like Module, but used for eval. */                                                                              \
    X(Expression, 88)                                                                                                  \
    X(SetComp, 89)                                                                                                     \
    X(Suite, 90)                                                                                                       \
                                                                                                                       \
    /* Pseudo-nodes that are specific to this compiler: */                                                             \
    X(Branch, 200)                                                                                                     \
    X(Jump, 201)                                                                                                       \
    X(ClsAttribute, 202)                                                                                               \
    X(AugBinOp, 203)                                                                                                   \
    X(Invoke, 204)                                                                                                     \
    X(LangPrimitive, 205)                                                                                              \
    /* wraps a ClassDef to make it an expr */                                                                          \
    X(MakeClass, 206)                                                                                                  \
    /* wraps a FunctionDef to make it an expr */                                                                       \
    X(MakeFunction, 207)                                                                                               \
                                                                                                                       \
    /* These aren't real AST types, but since we use AST types to represent binexp types */                            \
    /* and divmod+truediv are essentially types of binops, we add them here (at least for now): */                     \
    X(DivMod, 250)                                                                                                     \
    X(TrueDiv, 251)

#define GENERATE_ENUM(ENUM, N) ENUM = N,
#define GENERATE_STRING(STRING, N) m[N] = #STRING;

enum AST_TYPE : unsigned char { FOREACH_TYPE(GENERATE_ENUM) };

static const char* stringify(int n) {
    static std::map<int, const char*> m;
    FOREACH_TYPE(GENERATE_STRING)
    return m[n];
}

#undef FOREACH_TYPE
#undef GENERATE_ENUM
#undef GENERATE_STRING
};

class ASTVisitor;
class ASTStmtVisitor;
class AST_keyword;
class AST_stmt;

class ASTAllocator {
private:
    template <int slab_size, int alignment = 8> class ASTAllocatorSlab {
        unsigned char data[slab_size];
        int num_bytes_used;

    public:
        ASTAllocatorSlab() : num_bytes_used(0) {}
        ~ASTAllocatorSlab();

        int numBytesFree() const { return slab_size - num_bytes_used; }
        void* alloc(int num_bytes) {
            assert(num_bytes_used % alignment == 0);
            assert(numBytesFree() >= num_bytes);
            void* ptr = &data[num_bytes_used];
            num_bytes_used += llvm::RoundUpToAlignment(num_bytes, alignment);
            return ptr;
        }
    };

    // we subtract the size of the "num_bytes_used" field to generate a power of two allocation which I guess is more
    // what the allocator is optimized for.
    static constexpr int slab_size = 4096 - sizeof(int);
    static_assert(sizeof(ASTAllocatorSlab<slab_size>) == 4096, "");

    llvm::SmallVector<std::unique_ptr<ASTAllocatorSlab<slab_size>>, 4> slabs;

public:
    ASTAllocator() = default;
    ASTAllocator(ASTAllocator&&) = delete;

    void* allocate(int num_bytes) {
        if (slabs.empty() || slabs.back()->numBytesFree() < num_bytes)
            slabs.emplace_back(llvm::make_unique<ASTAllocatorSlab<slab_size>>());
        return slabs.back()->alloc(num_bytes);
    }
};

class AST {
public:
    virtual ~AST() {}

    const AST_TYPE::AST_TYPE type;
    uint32_t lineno, col_offset;

    virtual void accept(ASTVisitor* v) = 0;
    virtual int getSize() const = 0; // returns size of AST node

// #define DEBUG_LINE_NUMBERS 1
#ifdef DEBUG_LINE_NUMBERS
private:
    // Initialize lineno to something unique, so that if we see something ridiculous
    // appear in the traceback, we can isolate the allocation which created it.
    static int next_lineno;

public:
    AST(AST_TYPE::AST_TYPE type);
#else
    AST(AST_TYPE::AST_TYPE type) : type(type), lineno(0), col_offset(0) {}
#endif
    AST(AST_TYPE::AST_TYPE type, uint32_t lineno, uint32_t col_offset = 0)
        : type(type), lineno(lineno), col_offset(col_offset) {}


    static void* operator new(size_t count, ASTAllocator& allocator) { return allocator.allocate(count); }
    static void operator delete(void*) { RELEASE_ASSERT(0, "use the ASTAllocator instead"); }

    // These could be virtual methods, but since we already keep track of the type use a switch statement
    // like everywhere else.
    InternedStringPool& getStringpool();
    llvm::ArrayRef<AST_stmt*> getBody();
    BORROWED(BoxedString*) getName() noexcept;
};
Box* getDocString(llvm::ArrayRef<AST_stmt*> body);

template <int slab_size, int alignment> ASTAllocator::ASTAllocatorSlab<slab_size, alignment>::~ASTAllocatorSlab() {
    // find all AST* nodes and call the virtual destructor
    for (int current_pos = 0; current_pos < num_bytes_used;) {
        AST* node = (AST*)&data[current_pos];
        int node_size = node->getSize();
        node->~AST();
        current_pos += llvm::RoundUpToAlignment(node_size, alignment);
    }
}


#define DEFINE_AST_NODE(name)                                                                                          \
    static const AST_TYPE::AST_TYPE TYPE = AST_TYPE::name;                                                             \
    virtual int getSize() const override { return sizeof(*this); }


class AST_expr : public AST {
public:
    AST_expr(AST_TYPE::AST_TYPE type) : AST(type) {}
    AST_expr(AST_TYPE::AST_TYPE type, uint32_t lineno, uint32_t col_offset = 0) : AST(type, lineno, col_offset) {}
};

class AST_stmt : public AST {
public:
    virtual void accept_stmt(ASTStmtVisitor* v) = 0;

    AST_stmt(AST_TYPE::AST_TYPE type) : AST(type) {}
};

class AST_slice : public AST {
public:
    AST_slice(AST_TYPE::AST_TYPE type) : AST(type) {}
    AST_slice(AST_TYPE::AST_TYPE type, uint32_t lineno, uint32_t col_offset = 0) : AST(type, lineno, col_offset) {}
};

class AST_alias : public AST {
public:
    InternedString name, asname;

    virtual void accept(ASTVisitor* v) override;

    AST_alias(InternedString name, InternedString asname) : AST(AST_TYPE::alias), name(name), asname(asname) {}

    DEFINE_AST_NODE(alias)
};

class AST_Name;

class AST_arguments : public AST {
public:
    // no lineno, col_offset attributes
    std::vector<AST_expr*> args, defaults;

    AST_Name* kwarg = NULL, * vararg = NULL;

    virtual void accept(ASTVisitor* v) override;

    AST_arguments() : AST(AST_TYPE::arguments) {}

    DEFINE_AST_NODE(arguments)
};

class AST_Assert : public AST_stmt {
public:
    AST_expr* msg, *test;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Assert() : AST_stmt(AST_TYPE::Assert) {}

    DEFINE_AST_NODE(Assert)
};

class AST_Assign : public AST_stmt {
public:
    std::vector<AST_expr*> targets;
    AST_expr* value;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Assign() : AST_stmt(AST_TYPE::Assign) {}

    DEFINE_AST_NODE(Assign)
};

class AST_AugAssign : public AST_stmt {
public:
    AST_expr* value;
    AST_expr* target;
    AST_TYPE::AST_TYPE op_type;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_AugAssign() : AST_stmt(AST_TYPE::AugAssign) {}

    DEFINE_AST_NODE(AugAssign)
};

class AST_AugBinOp : public AST_expr {
public:
    AST_TYPE::AST_TYPE op_type;
    AST_expr* left, *right;

    virtual void accept(ASTVisitor* v) override;

    AST_AugBinOp() : AST_expr(AST_TYPE::AugBinOp) {}

    DEFINE_AST_NODE(AugBinOp)
};

class AST_Attribute : public AST_expr {
public:
    AST_expr* value;
    AST_TYPE::AST_TYPE ctx_type;
    InternedString attr;

    virtual void accept(ASTVisitor* v) override;

    AST_Attribute() : AST_expr(AST_TYPE::Attribute) {}

    AST_Attribute(AST_expr* value, AST_TYPE::AST_TYPE ctx_type, InternedString attr)
        : AST_expr(AST_TYPE::Attribute), value(value), ctx_type(ctx_type), attr(attr) {}

    DEFINE_AST_NODE(Attribute)
};

class AST_BinOp : public AST_expr {
public:
    AST_TYPE::AST_TYPE op_type;
    AST_expr* left, *right;

    virtual void accept(ASTVisitor* v) override;

    AST_BinOp() : AST_expr(AST_TYPE::BinOp) {}

    DEFINE_AST_NODE(BinOp)
};

class AST_BoolOp : public AST_expr {
public:
    AST_TYPE::AST_TYPE op_type;
    std::vector<AST_expr*> values;

    virtual void accept(ASTVisitor* v) override;

    AST_BoolOp() : AST_expr(AST_TYPE::BoolOp) {}

    DEFINE_AST_NODE(BoolOp)
};

class AST_Break : public AST_stmt {
public:
    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Break() : AST_stmt(AST_TYPE::Break) {}

    DEFINE_AST_NODE(Break)
};

class AST_Call : public AST_expr {
public:
    AST_expr* starargs, *kwargs, *func;
    std::vector<AST_expr*> args;
    std::vector<AST_keyword*> keywords;

    virtual void accept(ASTVisitor* v) override;

    AST_Call() : AST_expr(AST_TYPE::Call) {}

    DEFINE_AST_NODE(Call)
};

class AST_Compare : public AST_expr {
public:
    std::vector<AST_TYPE::AST_TYPE> ops;
    std::vector<AST_expr*> comparators;
    AST_expr* left;

    virtual void accept(ASTVisitor* v) override;

    AST_Compare() : AST_expr(AST_TYPE::Compare) {}

    DEFINE_AST_NODE(Compare)
};

class AST_comprehension : public AST {
public:
    AST_expr* target;
    AST_expr* iter;
    std::vector<AST_expr*> ifs;

    virtual void accept(ASTVisitor* v) override;

    AST_comprehension() : AST(AST_TYPE::comprehension) {}

    DEFINE_AST_NODE(comprehension)
};

class AST_ClassDef : public AST_stmt {
public:
    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    std::vector<AST_expr*> bases, decorator_list;
    std::vector<AST_stmt*> body;
    InternedString name;

    AST_ClassDef() : AST_stmt(AST_TYPE::ClassDef) {}

    DEFINE_AST_NODE(ClassDef)
};

class AST_Continue : public AST_stmt {
public:
    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Continue() : AST_stmt(AST_TYPE::Continue) {}

    DEFINE_AST_NODE(Continue)
};

class AST_Dict : public AST_expr {
public:
    std::vector<AST_expr*> keys, values;

    virtual void accept(ASTVisitor* v) override;

    AST_Dict() : AST_expr(AST_TYPE::Dict) {}

    DEFINE_AST_NODE(Dict)
};

class AST_DictComp : public AST_expr {
public:
    std::vector<AST_comprehension*> generators;
    AST_expr* key, *value;

    virtual void accept(ASTVisitor* v) override;

    AST_DictComp() : AST_expr(AST_TYPE::DictComp) {}

    DEFINE_AST_NODE(DictComp)
};

class AST_Delete : public AST_stmt {
public:
    std::vector<AST_expr*> targets;
    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Delete() : AST_stmt(AST_TYPE::Delete) {}

    DEFINE_AST_NODE(Delete)
};

class AST_Ellipsis : public AST_slice {
public:
    virtual void accept(ASTVisitor* v) override;

    AST_Ellipsis() : AST_slice(AST_TYPE::Ellipsis) {}

    DEFINE_AST_NODE(Ellipsis)
};

class AST_Expr : public AST_stmt {
public:
    AST_expr* value;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Expr() : AST_stmt(AST_TYPE::Expr) {}
    AST_Expr(AST_expr* value) : AST_stmt(AST_TYPE::Expr), value(value) {}

    DEFINE_AST_NODE(Expr)
};

class AST_ExceptHandler : public AST {
public:
    std::vector<AST_stmt*> body;
    AST_expr* type; // can be NULL for a bare "except:" clause
    AST_expr* name; // can be NULL if the exception doesn't get a name

    virtual void accept(ASTVisitor* v) override;

    AST_ExceptHandler() : AST(AST_TYPE::ExceptHandler) {}

    DEFINE_AST_NODE(ExceptHandler)
};

class AST_Exec : public AST_stmt {
public:
    AST_expr* body;
    AST_expr* globals;
    AST_expr* locals;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Exec() : AST_stmt(AST_TYPE::Exec) {}

    DEFINE_AST_NODE(Exec)
};

// (Alternative to AST_Module, used for, e.g., eval)
class AST_Expression : public AST {
public:
    std::unique_ptr<InternedStringPool> interned_strings;

    // this should be an expr but we convert it into a AST_Return(AST_expr) to make the code simpler
    AST_stmt* body;

    virtual void accept(ASTVisitor* v) override;

    AST_Expression(std::unique_ptr<InternedStringPool> interned_strings)
        : AST(AST_TYPE::Expression), interned_strings(std::move(interned_strings)) {}

    DEFINE_AST_NODE(Expression)
};

class AST_ExtSlice : public AST_slice {
public:
    std::vector<AST_slice*> dims;

    virtual void accept(ASTVisitor* v) override;

    AST_ExtSlice() : AST_slice(AST_TYPE::ExtSlice) {}

    DEFINE_AST_NODE(ExtSlice)
};

class AST_For : public AST_stmt {
public:
    std::vector<AST_stmt*> body, orelse;
    AST_expr* target, *iter;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_For() : AST_stmt(AST_TYPE::For) {}

    DEFINE_AST_NODE(For)
};

class AST_FunctionDef : public AST_stmt {
public:
    std::vector<AST_stmt*> body;
    std::vector<AST_expr*> decorator_list;
    InternedString name; // if the name is not set this is a lambda
    AST_arguments* args;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_FunctionDef() : AST_stmt(AST_TYPE::FunctionDef) {}

    DEFINE_AST_NODE(FunctionDef)
};

class AST_GeneratorExp : public AST_expr {
public:
    std::vector<AST_comprehension*> generators;
    AST_expr* elt;

    virtual void accept(ASTVisitor* v) override;

    AST_GeneratorExp() : AST_expr(AST_TYPE::GeneratorExp) {}

    DEFINE_AST_NODE(GeneratorExp)
};

class AST_Global : public AST_stmt {
public:
    std::vector<InternedString> names;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Global() : AST_stmt(AST_TYPE::Global) {}

    DEFINE_AST_NODE(Global)
};

class AST_If : public AST_stmt {
public:
    std::vector<AST_stmt*> body, orelse;
    AST_expr* test;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_If() : AST_stmt(AST_TYPE::If) {}

    DEFINE_AST_NODE(If)
};

class AST_IfExp : public AST_expr {
public:
    AST_expr* body, *test, *orelse;

    virtual void accept(ASTVisitor* v) override;

    AST_IfExp() : AST_expr(AST_TYPE::IfExp) {}

    DEFINE_AST_NODE(IfExp)
};

class AST_Import : public AST_stmt {
public:
    std::vector<AST_alias*> names;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Import() : AST_stmt(AST_TYPE::Import) {}

    DEFINE_AST_NODE(Import)
};

class AST_ImportFrom : public AST_stmt {
public:
    InternedString module;
    std::vector<AST_alias*> names;
    int level;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_ImportFrom() : AST_stmt(AST_TYPE::ImportFrom) {}

    DEFINE_AST_NODE(ImportFrom)
};

class AST_Index : public AST_slice {
public:
    AST_expr* value;

    virtual void accept(ASTVisitor* v) override;

    AST_Index() : AST_slice(AST_TYPE::Index) {}

    DEFINE_AST_NODE(Index)
};

class AST_keyword : public AST {
public:
    // no lineno, col_offset attributes
    AST_expr* value;
    InternedString arg;

    virtual void accept(ASTVisitor* v) override;

    AST_keyword() : AST(AST_TYPE::keyword) {}

    DEFINE_AST_NODE(keyword)
};

class AST_Lambda : public AST_expr {
public:
    AST_arguments* args;
    AST_expr* body;

    virtual void accept(ASTVisitor* v) override;

    AST_Lambda() : AST_expr(AST_TYPE::Lambda) {}

    DEFINE_AST_NODE(Lambda)
};

class AST_List : public AST_expr {
public:
    std::vector<AST_expr*> elts;
    AST_TYPE::AST_TYPE ctx_type;

    virtual void accept(ASTVisitor* v) override;

    AST_List() : AST_expr(AST_TYPE::List) {}

    DEFINE_AST_NODE(List)
};

class AST_ListComp : public AST_expr {
public:
    std::vector<AST_comprehension*> generators;
    AST_expr* elt;

    virtual void accept(ASTVisitor* v) override;

    AST_ListComp() : AST_expr(AST_TYPE::ListComp) {}

    DEFINE_AST_NODE(ListComp)
};

class AST_Module : public AST {
public:
    std::unique_ptr<InternedStringPool> interned_strings;

    // no lineno, col_offset attributes
    std::vector<AST_stmt*> body;

    virtual void accept(ASTVisitor* v) override;

    AST_Module(std::unique_ptr<InternedStringPool> interned_strings)
        : AST(AST_TYPE::Module), interned_strings(std::move(interned_strings)) {}

    DEFINE_AST_NODE(Module)
};

class AST_Suite : public AST {
public:
    std::unique_ptr<InternedStringPool> interned_strings;

    std::vector<AST_stmt*> body;

    virtual void accept(ASTVisitor* v) override;

    AST_Suite(std::unique_ptr<InternedStringPool> interned_strings)
        : AST(AST_TYPE::Suite), interned_strings(std::move(interned_strings)) {}

    DEFINE_AST_NODE(Suite)
};

class AST_Name : public AST_expr {
public:
    AST_TYPE::AST_TYPE ctx_type;
    InternedString id;

    // The resolved scope of this name.  Kind of hacky to be storing it in the AST node;
    // in CPython it ends up getting "cached" by being translated into one of a number of
    // different bytecodes.
    ScopeInfo::VarScopeType lookup_type;

    virtual void accept(ASTVisitor* v) override;

    AST_Name(InternedString id, AST_TYPE::AST_TYPE ctx_type, int lineno, int col_offset = 0)
        : AST_expr(AST_TYPE::Name, lineno, col_offset),
          ctx_type(ctx_type),
          id(id),
          lookup_type(ScopeInfo::VarScopeType::UNKNOWN) {}

    DEFINE_AST_NODE(Name)
};

class AST_Num : public AST_expr {
public:
    enum NumType : unsigned char {
        // These values must correspond to the values in parse_ast.py
        INT = 0x10,
        FLOAT = 0x20,
        LONG = 0x30,

        // for COMPLEX, n_float is the imaginary part, real part is 0
        COMPLEX = 0x40,
    } num_type;

    union {
        int64_t n_int;
        double n_float;
    };
    std::string n_long;

    virtual void accept(ASTVisitor* v) override;

    AST_Num() : AST_expr(AST_TYPE::Num) {}

    DEFINE_AST_NODE(Num)
};

class AST_Repr : public AST_expr {
public:
    AST_expr* value;

    virtual void accept(ASTVisitor* v) override;

    AST_Repr() : AST_expr(AST_TYPE::Repr) {}

    DEFINE_AST_NODE(Repr)
};

class AST_Pass : public AST_stmt {
public:
    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Pass() : AST_stmt(AST_TYPE::Pass) {}

    DEFINE_AST_NODE(Pass)
};

class AST_Print : public AST_stmt {
public:
    AST_expr* dest;
    bool nl;
    std::vector<AST_expr*> values;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Print() : AST_stmt(AST_TYPE::Print) {}

    DEFINE_AST_NODE(Print)
};

class AST_Raise : public AST_stmt {
public:
    // In the python ast module, these are called "type", "inst", and "tback", respectively.
    // Renaming to arg{0..2} since I find that confusing, since they are filled in
    // sequentially rather than semantically.
    // Ie "raise Exception()" will have type==Exception(), inst==None, tback==None
    AST_expr* arg0, *arg1, *arg2;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Raise() : AST_stmt(AST_TYPE::Raise), arg0(NULL), arg1(NULL), arg2(NULL) {}

    DEFINE_AST_NODE(Raise)
};

class AST_Return : public AST_stmt {
public:
    AST_expr* value;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Return() : AST_stmt(AST_TYPE::Return) {}

    DEFINE_AST_NODE(Return)
};

class AST_Set : public AST_expr {
public:
    std::vector<AST_expr*> elts;

    virtual void accept(ASTVisitor* v) override;

    AST_Set() : AST_expr(AST_TYPE::Set) {}

    DEFINE_AST_NODE(Set)
};

class AST_SetComp : public AST_expr {
public:
    std::vector<AST_comprehension*> generators;
    AST_expr* elt;

    virtual void accept(ASTVisitor* v) override;

    AST_SetComp() : AST_expr(AST_TYPE::SetComp) {}

    DEFINE_AST_NODE(SetComp)
};

class AST_Slice : public AST_slice {
public:
    AST_expr* lower, *upper, *step;

    virtual void accept(ASTVisitor* v) override;

    AST_Slice() : AST_slice(AST_TYPE::Slice) {}

    DEFINE_AST_NODE(Slice)
};

class AST_Str : public AST_expr {
public:
    enum StrType : unsigned char {
        UNSET = 0x00,
        STR = 0x10,
        UNICODE = 0x20,
    } str_type;

    // The meaning of str_data depends on str_type.  For STR, it's just the bytes value.
    // For UNICODE, it's the utf-8 encoded value.
    std::string str_data;

    virtual void accept(ASTVisitor* v) override;

    AST_Str() : AST_expr(AST_TYPE::Str), str_type(UNSET) {}
    AST_Str(std::string s) : AST_expr(AST_TYPE::Str), str_type(STR), str_data(std::move(s)) {}

    DEFINE_AST_NODE(Str)
};

class AST_Subscript : public AST_expr {
public:
    AST_expr* value;
    AST_slice* slice;
    AST_TYPE::AST_TYPE ctx_type;

    virtual void accept(ASTVisitor* v) override;

    AST_Subscript() : AST_expr(AST_TYPE::Subscript) {}

    DEFINE_AST_NODE(Subscript)
};

class AST_TryExcept : public AST_stmt {
public:
    std::vector<AST_stmt*> body, orelse;
    std::vector<AST_ExceptHandler*> handlers;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_TryExcept() : AST_stmt(AST_TYPE::TryExcept) {}

    DEFINE_AST_NODE(TryExcept)
};

class AST_TryFinally : public AST_stmt {
public:
    std::vector<AST_stmt*> body, finalbody;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_TryFinally() : AST_stmt(AST_TYPE::TryFinally) {}

    DEFINE_AST_NODE(TryFinally)
};

class AST_Tuple : public AST_expr {
public:
    std::vector<AST_expr*> elts;
    AST_TYPE::AST_TYPE ctx_type;

    virtual void accept(ASTVisitor* v) override;

    AST_Tuple() : AST_expr(AST_TYPE::Tuple) {}

    DEFINE_AST_NODE(Tuple)
};

class AST_UnaryOp : public AST_expr {
public:
    AST_expr* operand;
    AST_TYPE::AST_TYPE op_type;

    virtual void accept(ASTVisitor* v) override;

    AST_UnaryOp() : AST_expr(AST_TYPE::UnaryOp) {}

    DEFINE_AST_NODE(UnaryOp)
};

class AST_While : public AST_stmt {
public:
    AST_expr* test;
    std::vector<AST_stmt*> body, orelse;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_While() : AST_stmt(AST_TYPE::While) {}

    DEFINE_AST_NODE(While)
};

class AST_With : public AST_stmt {
public:
    AST_expr* optional_vars, *context_expr;
    std::vector<AST_stmt*> body;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_With() : AST_stmt(AST_TYPE::With) {}

    DEFINE_AST_NODE(With)
};

class AST_Yield : public AST_expr {
public:
    AST_expr* value;

    virtual void accept(ASTVisitor* v) override;

    AST_Yield() : AST_expr(AST_TYPE::Yield) {}

    DEFINE_AST_NODE(Yield)
};


// AST pseudo-nodes that will get added during CFG-construction.  These don't exist in the input AST, but adding them in
// lets us avoid creating a completely new IR for this phase

class CFGBlock;

class AST_ClsAttribute : public AST_expr {
public:
    AST_expr* value;
    InternedString attr;

    virtual void accept(ASTVisitor* v) override;

    AST_ClsAttribute() : AST_expr(AST_TYPE::ClsAttribute) {}

    DEFINE_AST_NODE(ClsAttribute)
};

class AST_Invoke : public AST_stmt {
public:
    AST_stmt* stmt;

    CFGBlock* normal_dest, *exc_dest;

    virtual void accept(ASTVisitor* v) override;
    virtual void accept_stmt(ASTStmtVisitor* v) override;

    AST_Invoke(AST_stmt* stmt) : AST_stmt(AST_TYPE::Invoke), stmt(stmt) {}

    DEFINE_AST_NODE(Invoke)
};

// "LangPrimitive" represents operations that "primitive" to the language,
// but aren't directly *exactly* representable as normal Python.
// ClsAttribute would fall into this category.
// These are basically bytecodes, framed as pseudo-AST-nodes.
class AST_LangPrimitive : public AST_expr {
public:
    enum Opcodes : unsigned char {
        LANDINGPAD, // grabs the info about the last raised exception
        LOCALS,
        GET_ITER,
        IMPORT_FROM,
        IMPORT_NAME,
        IMPORT_STAR,
        NONE,
        NONZERO, // determines whether something is "true" for purposes of `if' and so forth
        CHECK_EXC_MATCH,
        SET_EXC_INFO,
        UNCACHE_EXC_INFO,
        HASNEXT,
        PRINT_EXPR,
    } opcode;
    std::vector<AST_expr*> args;

    virtual void accept(ASTVisitor* v) override;

    AST_LangPrimitive(Opcodes opcode) : AST_expr(AST_TYPE::LangPrimitive), opcode(opcode) {}

    DEFINE_AST_NODE(LangPrimitive)
};

template <typename T> T* ast_cast(AST* node) {
    ASSERT(!node || node->type == T::TYPE, "%d", node ? node->type : 0);
    return static_cast<T*>(node);
}

class ASTVisitor {
protected:
public:
    virtual ~ASTVisitor() {}

    virtual bool visit_alias(AST_alias* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_arguments(AST_arguments* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_assert(AST_Assert* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_assign(AST_Assign* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_augassign(AST_AugAssign* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_augbinop(AST_AugBinOp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_attribute(AST_Attribute* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_binop(AST_BinOp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_boolop(AST_BoolOp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_break(AST_Break* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_call(AST_Call* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_clsattribute(AST_ClsAttribute* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_compare(AST_Compare* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_comprehension(AST_comprehension* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_classdef(AST_ClassDef* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_continue(AST_Continue* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_delete(AST_Delete* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_dict(AST_Dict* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_dictcomp(AST_DictComp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_ellipsis(AST_Ellipsis* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_excepthandler(AST_ExceptHandler* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_exec(AST_Exec* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_expr(AST_Expr* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_expression(AST_Expression* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_suite(AST_Suite* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_extslice(AST_ExtSlice* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_for(AST_For* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_functiondef(AST_FunctionDef* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_generatorexp(AST_GeneratorExp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_global(AST_Global* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_if(AST_If* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_ifexp(AST_IfExp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_import(AST_Import* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_importfrom(AST_ImportFrom* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_index(AST_Index* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_invoke(AST_Invoke* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_keyword(AST_keyword* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_lambda(AST_Lambda* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_langprimitive(AST_LangPrimitive* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_list(AST_List* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_listcomp(AST_ListComp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_module(AST_Module* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_name(AST_Name* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_num(AST_Num* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_pass(AST_Pass* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_print(AST_Print* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_raise(AST_Raise* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_repr(AST_Repr* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_return(AST_Return* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_set(AST_Set* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_setcomp(AST_SetComp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_slice(AST_Slice* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_str(AST_Str* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_subscript(AST_Subscript* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_tryexcept(AST_TryExcept* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_tryfinally(AST_TryFinally* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_tuple(AST_Tuple* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_unaryop(AST_UnaryOp* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_while(AST_While* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_with(AST_With* node) { RELEASE_ASSERT(0, ""); }
    virtual bool visit_yield(AST_Yield* node) { RELEASE_ASSERT(0, ""); }
};

class NoopASTVisitor : public ASTVisitor {
protected:
public:
    virtual ~NoopASTVisitor() {}

    virtual bool visit_alias(AST_alias* node) { return false; }
    virtual bool visit_arguments(AST_arguments* node) { return false; }
    virtual bool visit_assert(AST_Assert* node) { return false; }
    virtual bool visit_assign(AST_Assign* node) { return false; }
    virtual bool visit_augassign(AST_AugAssign* node) { return false; }
    virtual bool visit_augbinop(AST_AugBinOp* node) { return false; }
    virtual bool visit_attribute(AST_Attribute* node) { return false; }
    virtual bool visit_binop(AST_BinOp* node) { return false; }
    virtual bool visit_boolop(AST_BoolOp* node) { return false; }
    virtual bool visit_break(AST_Break* node) { return false; }
    virtual bool visit_call(AST_Call* node) { return false; }
    virtual bool visit_clsattribute(AST_ClsAttribute* node) { return false; }
    virtual bool visit_compare(AST_Compare* node) { return false; }
    virtual bool visit_comprehension(AST_comprehension* node) { return false; }
    virtual bool visit_classdef(AST_ClassDef* node) { return false; }
    virtual bool visit_continue(AST_Continue* node) { return false; }
    virtual bool visit_delete(AST_Delete* node) { return false; }
    virtual bool visit_dict(AST_Dict* node) { return false; }
    virtual bool visit_dictcomp(AST_DictComp* node) { return false; }
    virtual bool visit_ellipsis(AST_Ellipsis* node) { return false; }
    virtual bool visit_excepthandler(AST_ExceptHandler* node) { return false; }
    virtual bool visit_exec(AST_Exec* node) { return false; }
    virtual bool visit_expr(AST_Expr* node) { return false; }
    virtual bool visit_expression(AST_Expression* node) { return false; }
    virtual bool visit_suite(AST_Suite* node) { return false; }
    virtual bool visit_extslice(AST_ExtSlice* node) { return false; }
    virtual bool visit_for(AST_For* node) { return false; }
    virtual bool visit_functiondef(AST_FunctionDef* node) { return false; }
    virtual bool visit_generatorexp(AST_GeneratorExp* node) { return false; }
    virtual bool visit_global(AST_Global* node) { return false; }
    virtual bool visit_if(AST_If* node) { return false; }
    virtual bool visit_ifexp(AST_IfExp* node) { return false; }
    virtual bool visit_import(AST_Import* node) { return false; }
    virtual bool visit_importfrom(AST_ImportFrom* node) { return false; }
    virtual bool visit_index(AST_Index* node) { return false; }
    virtual bool visit_invoke(AST_Invoke* node) { return false; }
    virtual bool visit_keyword(AST_keyword* node) { return false; }
    virtual bool visit_lambda(AST_Lambda* node) { return false; }
    virtual bool visit_langprimitive(AST_LangPrimitive* node) { return false; }
    virtual bool visit_list(AST_List* node) { return false; }
    virtual bool visit_listcomp(AST_ListComp* node) { return false; }
    virtual bool visit_module(AST_Module* node) { return false; }
    virtual bool visit_name(AST_Name* node) { return false; }
    virtual bool visit_num(AST_Num* node) { return false; }
    virtual bool visit_pass(AST_Pass* node) { return false; }
    virtual bool visit_print(AST_Print* node) { return false; }
    virtual bool visit_raise(AST_Raise* node) { return false; }
    virtual bool visit_repr(AST_Repr* node) { return false; }
    virtual bool visit_return(AST_Return* node) { return false; }
    virtual bool visit_set(AST_Set* node) { return false; }
    virtual bool visit_setcomp(AST_SetComp* node) { return false; }
    virtual bool visit_slice(AST_Slice* node) { return false; }
    virtual bool visit_str(AST_Str* node) { return false; }
    virtual bool visit_subscript(AST_Subscript* node) { return false; }
    virtual bool visit_tryexcept(AST_TryExcept* node) { return false; }
    virtual bool visit_tryfinally(AST_TryFinally* node) { return false; }
    virtual bool visit_tuple(AST_Tuple* node) { return false; }
    virtual bool visit_unaryop(AST_UnaryOp* node) { return false; }
    virtual bool visit_while(AST_While* node) { return false; }
    virtual bool visit_with(AST_With* node) { return false; }
    virtual bool visit_yield(AST_Yield* node) { return false; }
};

class ASTStmtVisitor {
protected:
public:
    virtual ~ASTStmtVisitor() {}

    virtual void visit_assert(AST_Assert* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_assign(AST_Assign* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_augassign(AST_AugAssign* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_break(AST_Break* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_classdef(AST_ClassDef* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_delete(AST_Delete* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_continue(AST_Continue* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_exec(AST_Exec* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_expr(AST_Expr* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_for(AST_For* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_functiondef(AST_FunctionDef* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_global(AST_Global* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_if(AST_If* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_import(AST_Import* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_importfrom(AST_ImportFrom* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_invoke(AST_Invoke* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_pass(AST_Pass* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_print(AST_Print* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_raise(AST_Raise* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_return(AST_Return* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_tryexcept(AST_TryExcept* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_tryfinally(AST_TryFinally* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_while(AST_While* node) { RELEASE_ASSERT(0, ""); }
    virtual void visit_with(AST_With* node) { RELEASE_ASSERT(0, ""); }
};

void print_ast(AST* ast);
class ASTPrintVisitor : public ASTVisitor {
private:
    llvm::raw_ostream& stream;
    int indent;
    void printIndent();
    void printOp(AST_TYPE::AST_TYPE op_type);

public:
    ASTPrintVisitor(int indent = 0, llvm::raw_ostream& stream = llvm::outs()) : stream(stream), indent(indent) {}
    virtual ~ASTPrintVisitor() {}
    void flush() { stream.flush(); }

    virtual bool visit_alias(AST_alias* node);
    virtual bool visit_arguments(AST_arguments* node);
    virtual bool visit_assert(AST_Assert* node);
    virtual bool visit_assign(AST_Assign* node);
    virtual bool visit_augassign(AST_AugAssign* node);
    virtual bool visit_augbinop(AST_AugBinOp* node);
    virtual bool visit_attribute(AST_Attribute* node);
    virtual bool visit_binop(AST_BinOp* node);
    virtual bool visit_boolop(AST_BoolOp* node);
    virtual bool visit_break(AST_Break* node);
    virtual bool visit_call(AST_Call* node);
    virtual bool visit_compare(AST_Compare* node);
    virtual bool visit_comprehension(AST_comprehension* node);
    virtual bool visit_classdef(AST_ClassDef* node);
    virtual bool visit_clsattribute(AST_ClsAttribute* node);
    virtual bool visit_continue(AST_Continue* node);
    virtual bool visit_delete(AST_Delete* node);
    virtual bool visit_dict(AST_Dict* node);
    virtual bool visit_dictcomp(AST_DictComp* node);
    virtual bool visit_ellipsis(AST_Ellipsis* node);
    virtual bool visit_excepthandler(AST_ExceptHandler* node);
    virtual bool visit_exec(AST_Exec* node);
    virtual bool visit_expr(AST_Expr* node);
    virtual bool visit_expression(AST_Expression* node);
    virtual bool visit_suite(AST_Suite* node);
    virtual bool visit_extslice(AST_ExtSlice* node);
    virtual bool visit_for(AST_For* node);
    virtual bool visit_functiondef(AST_FunctionDef* node);
    virtual bool visit_generatorexp(AST_GeneratorExp* node);
    virtual bool visit_global(AST_Global* node);
    virtual bool visit_if(AST_If* node);
    virtual bool visit_ifexp(AST_IfExp* node);
    virtual bool visit_import(AST_Import* node);
    virtual bool visit_importfrom(AST_ImportFrom* node);
    virtual bool visit_index(AST_Index* node);
    virtual bool visit_invoke(AST_Invoke* node);
    virtual bool visit_keyword(AST_keyword* node);
    virtual bool visit_lambda(AST_Lambda* node);
    virtual bool visit_langprimitive(AST_LangPrimitive* node);
    virtual bool visit_list(AST_List* node);
    virtual bool visit_listcomp(AST_ListComp* node);
    virtual bool visit_module(AST_Module* node);
    virtual bool visit_name(AST_Name* node);
    virtual bool visit_num(AST_Num* node);
    virtual bool visit_pass(AST_Pass* node);
    virtual bool visit_print(AST_Print* node);
    virtual bool visit_raise(AST_Raise* node);
    virtual bool visit_repr(AST_Repr* node);
    virtual bool visit_return(AST_Return* node);
    virtual bool visit_set(AST_Set* node);
    virtual bool visit_setcomp(AST_SetComp* node);
    virtual bool visit_slice(AST_Slice* node);
    virtual bool visit_str(AST_Str* node);
    virtual bool visit_subscript(AST_Subscript* node);
    virtual bool visit_tuple(AST_Tuple* node);
    virtual bool visit_tryexcept(AST_TryExcept* node);
    virtual bool visit_tryfinally(AST_TryFinally* node);
    virtual bool visit_unaryop(AST_UnaryOp* node);
    virtual bool visit_while(AST_While* node);
    virtual bool visit_with(AST_With* node);
    virtual bool visit_yield(AST_Yield* node);
};

// Given an AST node, return a vector of the node plus all its descendents.
// This is useful for analyses that care more about the constituent nodes than the
// exact tree structure; ex, finding all "global" directives.
void flatten(llvm::ArrayRef<AST_stmt*> roots, std::vector<AST*>& output, bool expand_scopes);
void flatten(AST_expr* root, std::vector<AST*>& output, bool expand_scopes);
// Similar to the flatten() function, but filters for a specific type of ast nodes:
template <class T, class R> void findNodes(const R& roots, std::vector<T*>& output, bool expand_scopes) {
    std::vector<AST*> flattened;
    flatten(roots, flattened, expand_scopes);
    for (AST* n : flattened) {
        if (n->type == T::TYPE)
            output.push_back(reinterpret_cast<T*>(n));
    }
}

llvm::StringRef getOpSymbol(int op_type);
BORROWED(BoxedString*) getOpName(int op_type);
int getReverseCmpOp(int op_type, bool& success);
BoxedString* getReverseOpName(int op_type);
BoxedString* getInplaceOpName(int op_type);
std::string getInplaceOpSymbol(int op_type);
};

#endif
