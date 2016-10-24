#ifndef __ZERO_NEEDLE_HPP__
#define __ZERO_NEEDLE_HPP__ 1
#include <zero/frame.hpp>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace zero
{
  struct needle
  {
    explicit needle(std::uint8_t *P) : pointer(P) {}
    explicit needle(frame &F) : pointer(F.data()) {}

    unsigned put(std::uint8_t v)
    {
      assert(pointer != NULL);
      *pointer = v;
      pointer++;
      return 1;
    }

    unsigned put(std::uint16_t v)
    {
      assert(pointer != NULL);
      pointer [0] = std::uint8_t(((v >> 8)  & 0xFF));
      pointer [1] = std::uint8_t(((v)       & 0xFF)) ;
      pointer += 2;
      return 2;
    }

    unsigned put(std::uint32_t v)
    {
      assert(pointer != NULL);
      pointer [0] = std::uint8_t(((v >> 24) & 0xFF));
      pointer [1] = std::uint8_t(((v >> 16) & 0xFF));
      pointer [2] = std::uint8_t(((v >> 8)  & 0xFF));
      pointer [3] = std::uint8_t(((v)       & 0xFF));
      pointer += 4;
      return 4;
    }

    unsigned put(std::uint64_t v)
    {
      assert(pointer != NULL);
      pointer [0] = std::uint8_t(((v >> 56) & 0xFF));
      pointer [1] = std::uint8_t(((v >> 48) & 0xFF));
      pointer [2] = std::uint8_t(((v >> 40) & 0xFF));
      pointer [3] = std::uint8_t(((v >> 32) & 0xFF));
      pointer [4] = std::uint8_t(((v >> 24) & 0xFF));
      pointer [5] = std::uint8_t(((v >> 16) & 0xFF));
      pointer [6] = std::uint8_t(((v >> 8)  & 0xFF));
      pointer [7] = std::uint8_t(((v)       & 0xFF));
      pointer += 8;
      return 8;
    }

    unsigned get(std::uint8_t & v)
    {
      assert(pointer != NULL);
      v = *pointer;
      pointer++;
      return 1;
    }

    unsigned get(std::uint16_t & v)
    {
      assert(pointer != NULL);
      v = ((std::uint16_t)(pointer [0]) << 8)
    + ((std::uint16_t)(pointer [1])) ;
      pointer += 2;
      return 2;
    }

    unsigned get(std::uint32_t & v)
    {
      assert(pointer != NULL);
      v = ((std::uint32_t)(pointer [0]) << 24)
    + ((std::uint32_t)(pointer [1]) << 16)
    + ((std::uint32_t)(pointer [2]) << 8)
    + ((std::uint32_t)(pointer [3]) ) ;
      pointer += 4;
      return 4;
    }

    unsigned get(std::uint64_t & v)
    {
      assert(pointer != NULL);
      v = ((std::uint64_t)(pointer [0]) << 56)
    + ((std::uint64_t)(pointer [1]) << 48)
    + ((std::uint64_t)(pointer [2]) << 40)
    + ((std::uint64_t)(pointer [3]) << 32)
    + ((std::uint64_t)(pointer [4]) << 24)
    + ((std::uint64_t)(pointer [5]) << 16)
    + ((std::uint64_t)(pointer [6]) << 8)
    +  (std::uint64_t)(pointer [7]) ;
      pointer += 8;
      return 8;
    }

    unsigned put(std::int8_t  v)  { return put(static_cast<std::uint8_t> (v)); }
    unsigned put(std::int16_t v)  { return put(static_cast<std::uint16_t>(v)); }
    unsigned put(std::int32_t v)  { return put(static_cast<std::uint32_t>(v)); }
    unsigned put(std::int64_t v)  { return put(static_cast<std::uint64_t>(v)); }
    unsigned get(std::int8_t  &v) { return get(*reinterpret_cast<std::uint8_t*> (&v)); }
    unsigned get(std::int16_t &v) { return get(*reinterpret_cast<std::uint16_t*>(&v)); }
    unsigned get(std::int32_t &v) { return get(*reinterpret_cast<std::uint32_t*>(&v)); }
    unsigned get(std::int64_t &v) { return get(*reinterpret_cast<std::uint64_t*>(&v)); }

    template <class SizeType>
    unsigned put_string(const std::string & s)
    {
      assert(pointer != NULL);
      std::size_t size = s.size();
      auto n = put(SizeType(size));
      std::memcpy(pointer, &s[0], size);
      pointer += size;
      return n + size;
    }

    template <class SizeType>
    unsigned get_string(std::string & s)
    {
      assert(pointer != NULL);
      SizeType size;
      auto n = get(size);
      s.resize(size);
      std::memcpy(&s[0], pointer, size);
      pointer += size;
      return n + size;
    }

    unsigned put(std::uint8_t *data, std::size_t size)
    {
      assert(pointer != NULL);
      std::memcpy(pointer, data, size);
      pointer += size;
      return size;
    }

    unsigned get(std::uint8_t *data, std::size_t size)
    {
      assert(pointer != NULL);
      std::memcpy(data, pointer, size);
      pointer += size;
      return size;
    }

    operator std::uint8_t *() const { return pointer; }

    needle & operator =(std::uint8_t *p) { pointer  = p; return *this; }
    needle & operator+=(unsigned n) { pointer += n; return *this; }
 
  private:
    std::uint8_t *pointer;

    needle(const needle &) = delete;
    void operator=(const needle &) = delete;
  };
}//namespace zero
#endif//__ZERO_NEEDLE_HPP__
