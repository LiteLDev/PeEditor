#include "pe_editor/FakeSymbol.h"

#include "demangler/MicrosoftDemangle.h"

namespace pe_editor::FakeSymbol {

inline demangler::ms_demangle::SpecialIntrinsicKind consumeSpecialIntrinsicKind(StringView& MangledName) {
    using namespace demangler::ms_demangle;
    using namespace demangler;
    if (MangledName.consumeFront("?_7"))
        return SpecialIntrinsicKind::Vftable;
    if (MangledName.consumeFront("?_8"))
        return SpecialIntrinsicKind::Vbtable;
    if (MangledName.consumeFront("?_9"))
        return SpecialIntrinsicKind::VcallThunk;
    if (MangledName.consumeFront("?_A"))
        return SpecialIntrinsicKind::Typeof;
    if (MangledName.consumeFront("?_B"))
        return SpecialIntrinsicKind::LocalStaticGuard;
    if (MangledName.consumeFront("?_C"))
        return SpecialIntrinsicKind::StringLiteralSymbol;
    if (MangledName.consumeFront("?_P"))
        return SpecialIntrinsicKind::UdtReturning;
    if (MangledName.consumeFront("?_R0"))
        return SpecialIntrinsicKind::RttiTypeDescriptor;
    if (MangledName.consumeFront("?_R1"))
        return SpecialIntrinsicKind::RttiBaseClassDescriptor;
    if (MangledName.consumeFront("?_R2"))
        return SpecialIntrinsicKind::RttiBaseClassArray;
    if (MangledName.consumeFront("?_R3"))
        return SpecialIntrinsicKind::RttiClassHierarchyDescriptor;
    if (MangledName.consumeFront("?_R4"))
        return SpecialIntrinsicKind::RttiCompleteObjLocator;
    if (MangledName.consumeFront("?_S"))
        return SpecialIntrinsicKind::LocalVftable;
    if (MangledName.consumeFront("?__E"))
        return SpecialIntrinsicKind::DynamicInitializer;
    if (MangledName.consumeFront("?__F"))
        return SpecialIntrinsicKind::DynamicAtexitDestructor;
    if (MangledName.consumeFront("?__J"))
        return SpecialIntrinsicKind::LocalStaticThreadGuard;
    return SpecialIntrinsicKind::None;
}

// generate fakeSymbol for virtual functions
std::optional<std::string> getFakeSymbol(const std::string& fn, bool removeVirtual) {
    using namespace demangler::ms_demangle;
    using namespace demangler;
    Demangler  demangler;
    StringView name(fn.c_str());

    if (!name.consumeFront('?'))
        return std::nullopt;

    SpecialIntrinsicKind specialIntrinsicKind = consumeSpecialIntrinsicKind(name);

    if (specialIntrinsicKind != SpecialIntrinsicKind::None)
        return std::nullopt;

    SymbolNode* symbolNode = demangler.demangleDeclarator(name);
    if (symbolNode == nullptr
        || (symbolNode->kind() != NodeKind::FunctionSymbol && symbolNode->kind() != NodeKind::VariableSymbol)
        || demangler.Error)
        return std::nullopt;

    if (symbolNode->kind() == NodeKind::FunctionSymbol) {
        auto&  funcNode     = reinterpret_cast<FunctionSymbolNode*>(symbolNode)->Signature->FunctionClass;
        bool   modified     = false;
        size_t funcNodeSize = funcNode.toString().size();
        if (removeVirtual) {
            if (funcNode.has(FC_Virtual)) {
                funcNode.remove(FC_Virtual);
                modified = true;
            } else {
                return std::nullopt;
            }
        }
        if (funcNode.has(FC_Protected)) {
            funcNode.remove(FC_Protected);
            funcNode.add(FC_Public);
            modified = true;
        }
        if (funcNode.has(FC_Private)) {
            funcNode.remove(FC_Private);
            funcNode.add(FC_Public);
            modified = true;
        }
        if (modified) {
            std::string fakeSymbol   = fn;
            std::string fakeFuncNode = funcNode.toString();
            size_t      offset       = fn.size() - funcNode.pos->size();
            fakeSymbol.erase(offset, funcNodeSize);
            fakeSymbol.insert(offset, fakeFuncNode);
            return fakeSymbol;
        }
    } else if (symbolNode->kind() == NodeKind::VariableSymbol) {
        auto& storageClass = reinterpret_cast<VariableSymbolNode*>(symbolNode)->SC;
        bool  modified     = false;
        if (storageClass == StorageClass::PrivateStatic) {
            storageClass.set(StorageClass::PublicStatic);
            modified = true;
        }
        if (storageClass == StorageClass::ProtectedStatic) {
            storageClass.set(StorageClass::PublicStatic);
            modified = true;
        }
        if (modified) {
            std::string fakeSymbol       = fn;
            char        fakeStorageClass = storageClass.toChar();
            size_t      offset           = fn.size() - storageClass.pos->size();
            fakeSymbol[offset]           = fakeStorageClass;
            return fakeSymbol;
        }
    }

    return std::nullopt;
}

} // namespace pe_editor::FakeSymbol
