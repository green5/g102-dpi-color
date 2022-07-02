#ifndef STD_H_
#define STD_H_

#define LINE_(...) Line(__FILE__,__LINE__,__FUNCTION__,"" __VA_ARGS__)
#define plog  LINE_().plog_
#define perr  LINE_("ERR",GetLastError()).plog_
#define pexit LINE_("EXIT",GetLastError()).plog_
#define PLOG  plog()
#define PERR  perr()
#define PEXIT pexit()

#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef __unix
#pragma GCC diagnostic ignored "-Wunused-function"
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#else
#include <windows.h>
#include <winerror.h>
#include <io.h>
#include <fcntl.h>
#endif

#if defined(__unix) || defined(__MINGW_GCC_VERSION)
#include <sys/time.h>
#endif
#include <time.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <algorithm>

#include <type_traits>
#include <chrono>
#include <ctime>
#include <codecvt>

#if !defined(__BORLANDC__)
#include <thread>
#include <atomic>
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW_GCC_VERSION)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#define ATTR_PRINTF_(...)
#endif

using namespace std;

#if defined(_MSC_VER) || defined(__BORLANDC__)
namespace std
{
  static string vformat_(string& out, const char* fmt, va_list arg)
  {
    if (fmt)
    {
      va_list arg2;
      va_copy(arg2, arg);
      size_t size = _vsnprintf(NULL, 0, fmt, arg);
      //void* a1 = va_arg(arg, char*);
      if (size != -1) /// A return value of -1 indicates that an encoding error has occurred.
      {
        size_t old = out.size();
        out.resize(old + size);
        _vsnprintf((char*)out.c_str() + old, size + 1, fmt ? fmt : "NULL", arg2);
      }
    }
    return out;
  }
  static wstring vformat_(wstring& out, const wchar_t* wfmt, va_list arg)
  {
    if (wfmt)
    {
      va_list arg2;
      va_copy(arg2, arg);
      size_t size = _vsnwprintf(NULL, 0, wfmt, arg); /// wchars
      if (size != -1)
      {
        size_t old = out.size(); // wchars
        out.resize(old + size);
        _vsnwprintf_s((wchar_t*)out.c_str() + old, size + 1, size + 1, wfmt ? wfmt : L"NULL", arg2); //vsnwprintf((wchar_t*)out.c_str() + old, size + 1, wfmt ? wfmt : L"NULL", arg2);
      }
    }
    return out;
  }
}
#else
namespace std
{
  static string vformat_(string& out, const char* fmt, va_list arg)
  {
    if (fmt)
    {
      va_list arg2;
      va_copy(arg2, arg);
      int size = vsnprintf(NULL, 0, fmt, arg); // vswprintf /// -fshort-wchar problem
      size_t old = out.size();
      out.resize(old + size);
      vsnprintf((char*)out.c_str() + old, size + 1, fmt ? fmt : "NULL", arg2);
    }
    return out;
  }
  static wstring vformat_(wstring& out, const wchar_t* wfmt, va_list arg)
  {
    if (wfmt)
    {
      va_list arg2;
      va_copy(arg2, arg);
      int size = vswprintf(NULL, (size_t)0, wfmt, arg);
      size_t old = out.size();
      out.resize(old + size);
      vswprintf((wchar_t*)out.c_str() + old, size + 1, wfmt ? wfmt : L"NULL", arg2);
      /*
          string fmt = wide_utf(wfmt);
          int size = vsnprintf(NULL, 0, fmt.c_str(), arg);
          string aout;
          aout.resize(size);
          vsnprintf((char*)aout.c_str(), size + 1, wfmt ? fmt.c_str() : "NULL", arg2);
          out += utf_wide(aout);
      */
    }
    return out;
  }
}
#endif

namespace std
{
  template<typename Char> inline static basic_string<Char,char_traits<Char>,allocator<Char>> format(basic_string<Char,char_traits<Char>,allocator<Char>>& out, const Char* fmt, ...)
  {
    va_list arg;
    va_start(arg, fmt);
    vformat_(out, fmt, arg);
    va_end(arg);
    return out;
  }
  template<typename Char> ATTR_PRINTF_(1) inline static basic_string<Char,char_traits<Char>,allocator<Char>> format(const Char* fmt, ...)
  {
    basic_string<Char,char_traits<Char>,allocator<Char>> out;
    va_list arg;
    va_start(arg, fmt);
    vformat_(out, fmt, arg);
    va_end(arg);
    return out;
  }
  template<typename Char> inline static basic_string<Char,char_traits<Char>,allocator<Char>> nformat(const Char* fmt, ...)
  {
    basic_string<Char,char_traits<Char>,allocator<Char>> out;
    va_list arg;
    va_start(arg, fmt);
    vformat_(out, fmt, arg);
    va_end(arg);
    out += "\n"; ///
    return out;
  }
  template<typename Char> inline static basic_string<Char,char_traits<Char>,allocator<Char>> vformat(const Char* fmt, va_list arg)
  {
    basic_string<Char,char_traits<Char>,allocator<Char>> out;
    vformat_(out, fmt, arg);
    return out;
  }
}

namespace std {
  void (*pline_)(const string &action, string& log) = 0;
}

struct Line {
  int code_;
  string action_;
  string out_;
  Line(const char* file, int line, const char* func, const char *action="", int code=-1) : code_(code), action_(action) {
    out_ = string(file) + "." + format("%d",line) + "." + func + ": ";
  }
  int plog_(const char* fmt = 0, ...) ATTR_PRINTF_(2) {
    va_list arg;
    va_start(arg, fmt);
    out_ += vformat(fmt, arg);
    va_end(arg);
    if(pline_) pline_(action_, out_);    
    if (action_ == "exit") exit(code_);
    return code_ ? code_ : -1;
  }
};

#endif
