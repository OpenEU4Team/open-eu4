#pragma once

namespace boost {
namespace noncopyable_ {
// boost::noncopyable_::noncopyable - _ZN13CContextOwnerC1ERK7CStringS2_ @ 0x01fc8ce8
//      -> abstract
// A base that prevents copying.
class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;

private:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete; // _ZN13CContextOwnerC1ERK7CStringS2_ @ 0x01fc8ce8
};
}
}
