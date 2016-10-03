#pragma once

#include "Base/BaseTypes.h"
#include "Base/Any.h"
#include "Scripting/LuaException.h"

namespace DAVA
{
struct ScriptState;

/// \brief Class for Lua script.
class LuaScript final
{
public:
    /// \brief Default constructor.
    LuaScript();

    /// \brief Constructor with flag for loading default Lua libs in state.
    LuaScript(bool initDefaultLibs);

    /// \brief Default move constructor.
    LuaScript(LuaScript&&);

    /// \brief Delete copy constructor. Lua state is non-copyable.
    LuaScript(const LuaScript&) = delete;

    /// \brief Destroys the object and free Lua state
    ~LuaScript();

    /// \brief Delete assign operator. Lua state is non-copyable.
    LuaScript& operator=(const LuaScript&) = delete;

    /// \brief Load script from string, run it and return number of results.
    ///        Throw LuaException on error.
    ///        Lua stack changes [-0, +nresult, -]
    int32 ExecuteString(const String& script);

    /// \brief Load script from string, run it and return number of results.
    ///        Return -1 on error.
    ///        Lua stack changes [-0, +nresult, -]
    int32 ExecuteStringSafe(const String& script);

    /// \brief Run fName() function and return number of results.
    ///        Throw LuaException on error.
    ///        Lua stack changes [-0, +nresult, -]
    int32 CallFunction(const String& fName);

    /// \brief Run fName() function and return number of results.
    ///        Return -1 on error.
    ///        Lua stack changes [-0, +nresult, -]
    int32 CallFunctionSafe(const String& fName);

    /// \brief Run fName(...) function with arguments and return number of
    ///        results. Throw LuaException on error.
    ///        Lua stack changes [-0, +nresult, -]
    template <typename... T>
    int32 CallFunction(const String& fName, T... args);

    /// \brief Run fName(...) function with arguments return number of results.
    ///        Return -1 on error.
    ///        Lua stack changes [-0, +nresult, -]
    template <typename... T>
    int32 CallFunctionSafe(const String& fName, T... args);

    /// \brief Return value from top of the stack as Any and pop it.
    ///        Throw LuaException or error.
    ///        Lua stack changes [-(top-fromIndex), +0, v].
    Any PopAny();

    /// \brief Return value from top of the stack as Any and pop it.
    ///        Return false on error.
    ///        Lua stack changes [-(top-fromIndex), +0, v].
    bool PopAnySafe(Any& any);

    /// \brief Set variable to global table with name vName
    void SetGlobalVariable(const String& vName, const Any& value);

    //TODO: Create new LuaScript object with lua_State is new thread of current lua_State
    // LuaScript CreateNewThread();

private:
    /// \brief Internal state
    ScriptState* state = nullptr;

    /// \brief Find function with name fName and put it into stack
    ///        Lua stack changes [-0, +1, -]
    void BeginCallFunction(const String& fName);

    /// \brief Push any value into stack
    ///        Lua stack changes [-0, +1, -]
    void PushArg(const Any& any);

    /// \brief Call Lua function with nargs arguments on top of stack, pop they
    ///        and return number of function results in stack.
    ///        Throw LuaException on error.
    ///        Lua stack changes [-(nargs+1), +nresults, v]
    int32 EndCallFunction(int32 nargs);
};

template <typename... T>
inline int32 LuaScript::CallFunction(const String& fName, T... args)
{
    BeginCallFunction(fName);
    const int32 size = sizeof...(args);
    bool vargs[] = { (PushArg(Any(args)), true)... };
    return EndCallFunction(size);
}

template <typename... T>
inline int32 LuaScript::CallFunctionSafe(const String& fName, T... args)
{
    try
    {
        return CallFunction(fName, args...);
    }
    catch (const LuaException& e)
    {
        Logger::Error(Format("LuaException: %s", e.what()).c_str());
        return -1;
    }
}
}