#pragma once
#include <RHI/RHIDLL.h>

class PL_RHI_DLL plRHIQueryInterface
{
public:
    virtual ~plRHIQueryInterface() = default;

    template<typename T>
    T& As()
    {
        return static_cast<T&>(*this);
    }

    template<typename T>
    const T& As() const
    {
        return static_cast<T&>(*this);
    }
};
