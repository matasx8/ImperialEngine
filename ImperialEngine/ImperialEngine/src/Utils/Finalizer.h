#pragma once

template<class Func>
class Finalizer
{
public:
    Finalizer(Func&& func) :
        m_Func(std::move(func))
    {}

    ~Finalizer()
    {
        m_Func();
    }

private:
    Func m_Func;
};

template<class Func>
Finalizer<Func> MakeFinalizer(Func&& func)
{
    return Finalizer<Func>(std::move(func));
}