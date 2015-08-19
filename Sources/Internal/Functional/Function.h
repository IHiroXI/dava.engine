/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#ifndef __DAVA_FUNCTION_H__
#define __DAVA_FUNCTION_H__

#include <new>
#include <memory>
#include <array>
#include <type_traits>
#include <functional>

namespace DAVA {
namespace Fn11 {

template<typename T, typename... Other>
struct first_pointer_class {
    typedef typename std::remove_pointer<T>::type type;
};

template<typename... Args>
using first_pointer_class_t = typename first_pointer_class<Args...>::type;

template<typename T>
struct is_best_argument_type : std::integral_constant<bool,
        std::is_fundamental<T>::value || std::is_pointer<T>::value ||
        (sizeof(T) <= sizeof(void*) && (std::is_standard_layout<T>::value || std::is_pod<T>::value))>
{ };

class Closure
{
public:
    // this is internal storage for pointer on function or functional objects (lambda/bind)
    // if it fits Storage size and has trivial constructor|destructor it can be stored
    // in-place in Storage, otherwise 'new' should be used.
    using Storage = std::array<void *, 4>;

    enum StorageType
    {
        TRIVIAL,
        SHARED
    };

    Closure() = default;

    ~Closure()
    {
        Clear();
    }

    // Store Hldr class in-place
    template<typename Hldr, typename Fn, typename... Prms>
    void BindTrivialHolder(const Fn& fn, Prms... params)
    {
        Clear();
        static_assert(sizeof(Hldr) <= sizeof(Storage), "Fn can't be trivially bind");
        new (storage.data()) Hldr(fn, std::forward<Prms>(params)...);
    }

    // Store Hldr class using std::shared_ptr
    template<typename Hldr, typename Fn, typename... Prms>
    void BindSharedHolder(const Fn& fn, Prms... params)
    {
        Clear();
        Hldr *holder = new Hldr(fn, std::forward<Prms>(params)...);
        new (storage.data()) std::shared_ptr<void>(holder);
        storageType = SHARED;
    }

    template<typename Hldr>
    inline Hldr* GetSharedHolder() const
    {
        return static_cast<Hldr*>(SharedPtr()->get());
    }

    template<typename Hldr>
    inline Hldr* GetTrivialHolder() const
    {
        static_assert(sizeof(Hldr) <= sizeof(Storage), "Fn can't be trivially get");
        return reinterpret_cast<Hldr *>(const_cast<void **>(storage.data()));
    }

    Closure(const Closure& c)
    {
        Copy(c);
    }

    Closure(Closure&& c)
    {
        Swap(c);
    }

    Closure& operator=(const Closure &c)
    {
        if (this != &c)
        {
            Clear();
            Copy(c);
        }

        return *this;
    }

    Closure& operator=(Closure &&c)
    {
        Clear();
        Swap(c);

        return *this;
    }

    Closure& operator=(std::nullptr_t)
    {
        Clear();
        return *this;
    }

    bool operator==(const Closure&) const = delete;
    bool operator!=(const Closure&) const = delete;

    void Swap(Closure& c)
    {
        std::swap(storageType, c.storageType);
        std::swap(storage, c.storage);
    }

    const Storage& GetStorage() const
    {
        return storage;
    }

    StorageType GetStorageType() const
    {
        return storageType;
    }

protected:
    Storage storage{ };
    StorageType storageType{ TRIVIAL };

    inline std::shared_ptr<void>* SharedPtr() const
    {
        return reinterpret_cast<std::shared_ptr<void>*>(const_cast<void**>(storage.data()));
    }

    void Clear()
    {
        if (SHARED == storageType)
        {
            SharedPtr()->reset();
            storageType = TRIVIAL;
        }
        storage.fill(nullptr);
    }

    void Copy(const Closure& c)
    {
        storageType = c.storageType;
        if (SHARED == storageType)
            new (storage.data()) std::shared_ptr<void>(*c.SharedPtr());
        else
            storage = c.storage;
    }
};

template<typename Fn, typename Ret, typename... Args>
class HolderFree
{
public:
    HolderFree(const Fn& _fn)
        : fn(_fn)
    { }

    static Ret invokeTrivial(const Closure &c, typename std::conditional<is_best_argument_type<Args>::value, Args, Args&&>::type... args)
    {
        HolderFree *holder = c.GetTrivialHolder<HolderFree>();
        return (Ret)holder->fn(std::forward<Args>(args)...);
    }

    static Ret invokeShared(const Closure &c, typename std::conditional<is_best_argument_type<Args>::value, Args, Args&&>::type... args)
    {
        HolderFree *holder = c.GetSharedHolder<HolderFree>();
        return (Ret)holder->fn(std::forward<Args>(args)...);
    }

protected:
    Fn fn;
};

template<typename Obj, typename Ret, typename Cls, typename... ClsArgs>
class HolderClass
{
    static_assert(std::is_base_of<Cls, Obj>::value, "Specified class doesn't match class method");

public:
    using Fn = Ret(Cls::*)(ClsArgs...);

    HolderClass(const Fn& _fn)
        : fn(_fn)
    { }

    static Ret invokeTrivial(const Closure &c, Obj* cls, typename std::conditional<is_best_argument_type<ClsArgs>::value, ClsArgs, ClsArgs&&>::type... args)
    {
        HolderClass *holder = c.GetTrivialHolder<HolderClass>();
        return (Ret)(static_cast<Cls*>(cls)->*holder->fn)(std::forward<ClsArgs>(args)...);
    }

    static Ret invokeShared(const Closure &c, Obj* cls, typename std::conditional<is_best_argument_type<ClsArgs>::value, ClsArgs, ClsArgs&&>::type... args)
    {
        HolderClass *holder = c.GetSharedHolder<HolderClass>();
        return (Ret)(static_cast<Cls*>(cls)->*holder->fn)(std::forward<ClsArgs>(args)...);
    }

protected:
    Fn fn;
};

template<typename Obj, typename Ret, typename Cls, typename... ClsArgs>
class HolderObject
{
    static_assert(std::is_base_of<Cls, Obj>::value, "Specified class doesn't match class method");

public:
    using Fn = Ret(Cls::*)(ClsArgs...);

    HolderObject(const Fn& _fn, Obj* _obj)
        : fn(_fn)
        , obj(_obj)
    { }

    static Ret invokeTrivial(const Closure &c, typename std::conditional<is_best_argument_type<ClsArgs>::value, ClsArgs, ClsArgs&&>::type... args)
    {
        HolderObject *holder = c.GetTrivialHolder<HolderObject>();
        return (Ret)(static_cast<Cls *>(holder->obj)->*holder->fn)(std::forward<ClsArgs>(args)...);
    }

    static Ret invokeShared(const Closure &c, typename std::conditional<is_best_argument_type<ClsArgs>::value, ClsArgs, ClsArgs&&>::type... args)
    {
        HolderObject *holder = c.GetSharedHolder<HolderObject>();
        return (Ret)(static_cast<Cls *>(holder->obj)->*holder->fn)(std::forward<ClsArgs>(args)...);
    }

protected:
    Fn fn;
    Obj *obj;
};

template<typename Obj, typename Ret, typename Cls, typename... ClsArgs>
class HolderSharedObject
{
    static_assert(std::is_base_of<Cls, Obj>::value, "Specified class doesn't match class method");

public:
    using Fn = Ret(Cls::*)(ClsArgs...);

    HolderSharedObject(const Fn& _fn, const std::shared_ptr<Obj> _obj)
        : fn(_fn)
        , obj(_obj)
    { }

    static Ret invokeShared(const Closure &c, typename std::conditional<is_best_argument_type<ClsArgs>::value, ClsArgs, ClsArgs&&>::type... args)
    {
        HolderSharedObject *holder = c.GetSharedHolder<HolderSharedObject>();
        return (static_cast<Cls*>(holder->obj.get())->*holder->fn)(std::forward<ClsArgs>(args)...);
    }

protected:
    Fn fn;
    std::shared_ptr<Obj> obj;
};

} // namespace Fn11

template<typename Fn>
class Function;

template<typename Ret, typename... Args>
class Function<Ret(Args...)>
{
    template<typename>
    friend class Function;

public:
    Function()
    { }

    Function(std::nullptr_t)
    { }

    template<typename Fn>
    Function(const Fn& fn)
    {
        static_assert(!std::is_member_function_pointer<Fn>::value, "There is no appropriate constructor for such Fn type.");
        using Holder = Fn11::HolderFree<Fn, Ret, Args...>;
        Init<Holder, Fn>(fn);
    }

    template<typename Cls, typename... ClsArgs>
    Function(Ret(Cls::* const &fn)(ClsArgs...))
    {
        using Holder = Fn11::HolderClass<Fn11::first_pointer_class_t<Args...>, Ret, Cls, ClsArgs...>;
        using Fn = Ret(Cls::*)(ClsArgs...);
        Init<Holder, Fn>(fn);
    }

    template<typename Cls, typename... ClsArgs>
    Function(Ret(Cls::* const &fn)(ClsArgs...) const)
    {
        using Holder = Fn11::HolderClass<Fn11::first_pointer_class_t<Args...>, Ret, Cls, ClsArgs...>;
        using Fn = Ret(Cls::*)(ClsArgs...);
        Init<Holder, Fn>(reinterpret_cast<Fn>(fn));
    }

    template<typename Obj, typename Cls, typename... ClsArgs>
    Function(Obj *obj, Ret(Cls::* const &fn)(ClsArgs...))
    {
        using Holder = Fn11::HolderObject<Obj, Ret, Cls, ClsArgs...>;
        using Fn = Ret(Cls::*)(ClsArgs...);
        Init<Holder, Fn>(fn, obj);
    }

    template<typename Obj, typename Cls, typename... ClsArgs>
    Function(Obj *obj, Ret(Cls::* const &fn)(ClsArgs...) const)
    {
        using Holder = Fn11::HolderObject<Obj, Ret, Cls, ClsArgs...>;
        using Fn = Ret(Cls::*)(ClsArgs...);
        Init<Holder, Fn>(reinterpret_cast<Fn>(fn), obj);
    }

    template<typename Obj, typename Cls, typename... ClsArgs>
    Function(const std::shared_ptr<Obj> &obj, Ret(Cls::* const &fn)(ClsArgs...))
    {
        using Holder = Fn11::HolderSharedObject<Obj, Ret, Cls, ClsArgs...>;
        using Fn = Ret(Cls::*)(ClsArgs...);

        // always use BindShared
        closure.template BindSharedHolder<Holder, Fn>(fn, obj);
        invoker = &Holder::invokeShared;
    }

    template<typename Obj, typename Cls, typename... ClsArgs>
    Function(const std::shared_ptr<Obj> &obj, Ret(Cls::* const &fn)(ClsArgs...) const)
    {
        using Holder = Fn11::HolderSharedObject<Obj, Ret, Cls, ClsArgs...>;
        using Fn = Ret(Cls::*)(ClsArgs...);

        // always use BindShared
        closure.template BindSharedHolder<Holder, Fn>(reinterpret_cast<Fn>(fn), obj);
        invoker = &Holder::invokeShared;
    }

    Function(const Function& fn)
        : invoker(fn.invoker)
        , closure(fn.closure)
    { }

    Function(Function&& fn)
        : invoker(fn.invoker)
        , closure(std::move(fn.closure))
    {
        fn.invoker = nullptr;
    }

    template<typename AnotherRet>
    Function(const Function<AnotherRet(Args...)> &fn)
        : invoker(fn.invoker)
        , closure(fn.closure)
    { }

    Function& operator=(std::nullptr_t)
    {
        invoker = nullptr;
        closure = nullptr;
        return *this;
    }

    Function& operator=(const Function &fn)
    {
        if (this != &fn)
        {
            closure = fn.closure;
            invoker = fn.invoker;
        }
        return *this;
    }

    Function& operator=(Function&& fn)
    {
        closure = std::move(fn.closure);
        invoker = fn.invoker;
        fn.invoker = nullptr;
        return *this;
    }

    bool operator==(const Function&) const = delete;
    bool operator!=(const Function&) const = delete;

    operator bool() const { return (invoker != nullptr); }

    friend bool operator<(const Function& fnL, const Function& fnR) { return (fnL.invoker < fnR.invoker); }
    friend bool operator==(std::nullptr_t, const Function &fn) { return (nullptr == fn.invoker); }
    friend bool operator==(const Function &fn, std::nullptr_t) { return (nullptr == fn.invoker); }
    friend bool operator!=(std::nullptr_t, const Function &fn) { return (nullptr != fn.invoker); }
    friend bool operator!=(const Function &fn, std::nullptr_t) { return (nullptr != fn.invoker); }

    Ret operator()(Args... args) const
    {
        return invoker(closure, std::forward<Args>(args)...);
    }

    void Swap(Function& fn)
    {
        std::swap(invoker, fn.invoker);
        closure.Swap(fn.closure);
    }

    const Fn11::Closure::Storage& Target() const
    {
        return closure.GetStorage();
    }

    bool IsTrivialTarget() const
    {
        return (Fn11::Closure::TRIVIAL == closure.GetStorageType());
    }

private:
    using Invoker = Ret(*)(const Fn11::Closure &, typename std::conditional<Fn11::is_best_argument_type<Args>::value, Args, Args&&>::type...);

    Invoker invoker = nullptr;
    Fn11::Closure closure;

    template<typename Hldr, typename Fn, typename... Prms, bool trivial = true>
    void Init(const Fn& fn, Prms&&... params)
    {
        Detail<
                trivial && sizeof(Hldr) <= sizeof(Fn11::Closure::Storage)
                && std::is_trivially_destructible<Fn>::value
#if defined(__GLIBCXX__) && __GLIBCXX__ <= 20141030
                // android old-style way
                && std::has_trivial_copy_constructor<Fn>::value
                && std::has_trivial_copy_assign<Fn>::value
#else
                // standard c++14 way
                && std::is_trivially_copy_constructible<Fn>::value
                && std::is_trivially_copy_assignable<Fn>::value
#endif
            , Hldr, Fn, Prms... >::Init(this, fn, std::forward<Prms>(params)...);
    }

private:
    template<bool trivial, typename Hldr, typename... Prms>
    struct Detail;

    // trivial specialization
    template<typename Hldr, typename Fn, typename... Prms>
    struct Detail<true, Hldr, Fn, Prms...>
    {
        static void Init(Function *that, const Fn& fn, Prms&&... params)
        {
            that->closure.template BindTrivialHolder<Hldr, Fn>(fn, std::forward<Prms>(params)...);
            that->invoker = &Hldr::invokeTrivial;
        }
    };

    // shared specialization
    template<typename Hldr, typename Fn, typename... Prms>
    struct Detail<false, Hldr, Fn, Prms...>
    {
        static void Init(Function *that, const Fn& fn, Prms&&... params)
        {
            that->closure.template BindSharedHolder<Hldr, Fn>(fn, std::forward<Prms>(params)...);
            that->invoker = &Hldr::invokeShared;
        }
    };
};

template<typename Ret, typename... Args>
Function<Ret(Args...)> MakeFunction(Ret(*const &fn)(Args...)) { return Function<Ret(Args...)>(fn); }

template<typename Cls, typename Ret, typename... Args>
Function<Ret(Cls*, Args...)> MakeFunction(Ret(Cls::* const &fn)(Args...)) { return Function<Ret(Cls*, Args...)>(fn); }

template<typename Cls, typename Ret, typename... Args>
Function<Ret(Cls*, Args...)> MakeFunction(Ret(Cls::* const &fn)(Args...) const) { return Function<Ret(Cls*, Args...)>(fn); }

template<typename Obj, typename Cls, typename Ret, typename... Args>
Function<Ret(Args...)> MakeFunction(Obj *obj, Ret(Cls::* const &fn)(Args...)) { return Function<Ret(Args...)>(obj, fn); }

template<typename Obj, typename Cls, typename Ret, typename... Args>
Function<Ret(Args...)> MakeFunction(Obj *obj, Ret(Cls::* const &fn)(Args...) const) { return Function<Ret(Args...)>(obj, fn); }

template<typename Obj, typename Cls, typename Ret, typename... Args>
Function<Ret(Args...)> MakeFunction(const std::shared_ptr<Obj> &obj, Ret(Cls::* const &fn)(Args...)) { return Function<Ret(Args...)>(obj, fn); }

template<typename Obj, typename Cls, typename Ret, typename... Args>
Function<Ret(Args...)> MakeFunction(const std::shared_ptr<Obj> &obj, Ret(Cls::* const &fn)(Args...) const) { return Function<Ret(Args...)>(obj, fn); }

template<typename Ret, typename... Args>
Function<Ret(Args...)> MakeFunction(const Function<Ret(Args...)>& fn) { return Function<Ret(Args...)>(fn); }

} // namespace DAVA

namespace std
{
    template<typename F>
    void swap(DAVA::Function<F> &lf, DAVA::Function<F> &rf)
    {
        lf.Swap(rf);
    }

} // namespace std

#endif // __DAVA_FUNCTION_H__
