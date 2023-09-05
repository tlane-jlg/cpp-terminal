#include "cpp-terminal/platforms/conversion.hpp"

#include "cpp-terminal/exception.hpp"

#if defined(_WIN32)
  #include <windows.h>
#endif

namespace Term
{
namespace Private
{

#if defined(_WIN32)
std::string to_utf8(LPCWCH utf16Str)
{
  int         size_needed = WideCharToMultiByte(CP_UTF8, 0, utf16Str, -1, nullptr, 0, nullptr, nullptr);
  std::string ret(size_needed, '\0');
  WideCharToMultiByte(CP_UTF8, 0, utf16Str, wcslen(utf16Str), &ret[0], size_needed, nullptr, nullptr);
  return ret.c_str();
}
#endif

static constexpr std::uint8_t UTF8_ACCEPT = 0;
static constexpr std::uint8_t UTF8_REJECT = 0xf;

std::uint8_t utf8_decode_step(std::uint8_t state, std::uint8_t octet, std::uint32_t* cpp)
{
  static const std::uint32_t utf8ClassTab[0x10] = {
    0x88888888UL, 0x88888888UL, 0x99999999UL, 0x99999999UL, 0xaaaaaaaaUL, 0xaaaaaaaaUL, 0xaaaaaaaaUL, 0xaaaaaaaaUL, 0x222222ffUL, 0x22222222UL, 0x22222222UL, 0x22222222UL, 0x3333333bUL, 0x33433333UL, 0xfff5666cUL, 0xffffffffUL,
  };

  static const std::uint32_t utf8StateTab[0x10] = {
    0xfffffff0UL, 0xffffffffUL, 0xfffffff1UL, 0xfffffff3UL, 0xfffffff4UL, 0xfffffff7UL, 0xfffffff6UL, 0xffffffffUL, 0x33f11f0fUL, 0xf3311f0fUL, 0xf33f110fUL, 0xfffffff2UL, 0xfffffff5UL, 0xffffffffUL, 0xffffffffUL, 0xffffffffUL,
  };

  const std::uint8_t reject = (state >> 3), nonAscii = (octet >> 7);
  const std::uint8_t class_ = (!nonAscii ? 0 : (0xf & (utf8ClassTab[(octet >> 3) & 0xf] >> (4 * (octet & 7)))));

  *cpp = (state == UTF8_ACCEPT ? (octet & (0xffU >> class_)) : ((octet & 0x3fU) | (*cpp << 6)));

  return (reject ? 0xf : (0xf & (utf8StateTab[class_] >> (4 * (state & 7)))));
}

void codepoint_to_utf8(std::string& s, char32_t c)
{
  if(c > 0x0010FFFF) { throw Term::Exception("Invalid UTF32 codepoint."); }
  char     bytes[4];
  int      nbytes = 1;
  char32_t d      = c;
  if(c >= 0x10000)
  {
    nbytes++;
    bytes[3] = ((d | 0x80) & 0xBF);
    d >>= 6;
  }
  if(c >= 0x800)
  {
    nbytes++;
    bytes[2] = ((d | 0x80) & 0xBF);
    d >>= 6;
  }
  if(c >= 0x80)
  {
    nbytes++;
    bytes[1] = ((d | 0x80) & 0xBF);
    d >>= 6;
  }
  static const unsigned char mask[4] = {0x00, 0xC0, 0xE0, 0xF0};
  bytes[0]                           = static_cast<char>(d | mask[nbytes - 1]);
  s.append(bytes, nbytes);
}

std::u32string utf8_to_utf32(const std::string& s)
{
  std::uint32_t  codepoint{};
  std::uint8_t   state = UTF8_ACCEPT;
  std::u32string r{};
  for(char i: s)
  {
    state = utf8_decode_step(state, i, &codepoint);
    if(state == UTF8_ACCEPT) { r.push_back(codepoint); }
    else if(state == UTF8_REJECT) { throw Term::Exception("Invalid byte in UTF8 encoded string"); }
  }
  if(state != UTF8_ACCEPT) { throw Term::Exception("Expected more bytes in UTF8 encoded string"); }
  return r;
}

std::string utf32_to_utf8(const std::u32string& s)
{
  std::string r{};
  for(char32_t i: s) { codepoint_to_utf8(r, i); }
  return r;
}

std::string vector_to_string(const std::vector<char>& vector)
{
  std::string string;
  for(char i: vector) { string.push_back(i); }
  return string;
}

bool is_valid_utf8_code_unit(const std::string& s)
{
  static const constexpr int b1OOOOOOO{128};
  static const constexpr int b11OOOOOO{192};
  static const constexpr int b111OOOOO{224};
  static const constexpr int b1111OOOO{240};
  static const constexpr int b11111OOO{248};
  switch(s.size())
  {
    case 1: return ((s[0] & b1OOOOOOO) == 0) ? true : false;
    case 2: return ((s[0] & b111OOOOO) == b11OOOOOO) && ((s[1] & b11OOOOOO) == b1OOOOOOO) ? true : false;
    case 3: return ((s[0] & b1111OOOO) == b111OOOOO) && ((s[1] & b11OOOOOO) == b1OOOOOOO) && ((s[2] & b11OOOOOO) == b1OOOOOOO) ? true : false;
    case 4: return ((s[0] & b11111OOO) == b1111OOOO) && ((s[1] & b11OOOOOO) == b1OOOOOOO) && ((s[2] & b11OOOOOO) == b1OOOOOOO) && ((s[3] & b11OOOOOO) == b1OOOOOOO) ? true : false;
    default: return false;
  }
}

}  // namespace Private
}  // namespace Term