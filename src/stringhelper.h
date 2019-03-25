#pragma once

#include <cstdio>
#include <cstdarg>
#include <cwchar>
#include <cctype>
#include <string>
#include <algorithm>
#include <functional>
#include <locale>
#include <codecvt>

#define TRIMCHAR " \r\n\t"
#define TRIMWCHAR L" \r\n\t"

namespace StringHelper
{
	template <typename T>
	inline T& ReplaceAll(T& s, const T& src, const T& des)
	{
		typename T::size_type pos = 0;
		typename T::size_type srcLen = src.size();
		typename T::size_type desLen = des.size();
		pos = s.find(src, pos);
		while ((pos != T::npos))
		{
			s.replace(pos, srcLen, des);
			pos = s.find(src, (pos + desLen));
		}
		return s;
	}

	inline std::string& TrimLeft(std::string& s)
	{
		if (s.empty()) return s;
		return s.erase(0, s.find_first_not_of(TRIMCHAR));
	}

	inline std::string& TrimRight(std::string& s)
	{
		if (s.empty()) return s;
		return s.erase(s.find_last_not_of(TRIMCHAR) + 1);
	}

	inline std::wstring& TrimLeft(std::wstring& s)
	{
		if (s.empty()) return s;
		return s.erase(0, s.find_first_not_of(TRIMWCHAR));
	}

	inline std::wstring& TrimRight(std::wstring& s)
	{
		if (s.empty()) return s;
		return s.erase(s.find_last_not_of(TRIMWCHAR) + 1);
	}

	template <typename T>
	inline T& Trim(T& s)
	{
		return TrimRight(TrimLeft(s));
	}

	template <typename T>
	inline T ToLower_Copy(const T& s)
	{
		T ss(s.c_str());
		std::transform(s.begin(), s.end(), ss.begin(), static_cast<int(*)(int)>(tolower));
		return ss;
	}

	template <typename T>
	inline T ToUpper_Copy(const T& s)
	{
		T ss(s.c_str());
		std::transform(s.begin(), s.end(), ss.begin(), static_cast<int(*)(int)>(toupper));
		return ss;
	}

	template <typename T>
	inline int CompareIgnoreCase(const T& s1, const T& s2)
	{
		T ss1 = ToLower_Copy(s1);
		T ss2 = ToLower_Copy(s2);
		return ss1.compare(ss2);
	}

	inline std::string FormatString(const char* s, ...)
	{
		char strBuffer[4096] = { 0 };
		va_list vlArgs;
		va_start(vlArgs, s);
		vsnprintf(strBuffer, 4095, s, vlArgs);
		va_end(vlArgs);
		return std::string(strBuffer);
	}

	inline std::wstring FormatString(const wchar_t* s, ...)
	{
		wchar_t strBuffer[4096] = { 0 };
		va_list vlArgs;
		va_start(vlArgs, s);
		vswprintf(strBuffer, 4095, s, vlArgs);
		va_end(vlArgs);
		return std::wstring(strBuffer);
	}

	inline std::wstring A2W(const std::string& s, const std::locale& loc = std::locale(""))
	{
		typedef std::codecvt<wchar_t, char, mbstate_t> facet_type;
		mbstate_t mbst = mbstate_t();

		const char* frombegin = s.data();
		const char* fromend = frombegin + s.size();
		const char* fromnext = nullptr;
		
		std::vector<wchar_t> tobuf(s.size() + 1);
		wchar_t* tobegin = tobuf.data();
		wchar_t* toend = tobegin + tobuf.size();
		wchar_t* tonext = nullptr;

		if (std::use_facet<facet_type>(loc).in(mbst, frombegin, fromend, fromnext, tobegin, toend, tonext) != facet_type::ok)
		{
			throw std::runtime_error("转换编码失败");
		}
		return tobuf.data();
	}

	inline std::string W2A(const std::wstring& s, const std::locale& loc = std::locale(""))
	{
		typedef std::codecvt<wchar_t, char, mbstate_t> facet_type;
		mbstate_t mbst = mbstate_t();

		const wchar_t* frombegin = s.c_str();
		const wchar_t* fromend = frombegin + s.size();
		const wchar_t* fromnext = nullptr;

		std::vector<char> tobuf(s.size() * 4 + 1);
		char* tobegin = tobuf.data();
		char* toend = tobegin + tobuf.size();
		char* tonext = nullptr;

		if (std::use_facet<facet_type>(loc).out(mbst, frombegin, fromend, fromnext, tobegin, toend, tonext) != facet_type::ok)
		{
			throw std::runtime_error("转换编码失败");
		}
		return tobuf.data();
	}

	inline std::string T2A(const std::wstring& s, const std::locale& loc = std::locale(""))
	{
		return W2A(s, loc);
	}

	inline std::string T2A(const std::string& s, const std::locale& loc = std::locale(""))
	{
		return s;
	}

	inline std::locale GetGBKLocale()
	{
		std::locale loc;
		try
		{
			loc = std::locale("zh_CN.GBK");  // linux格式
		}
		catch (...)
		{
			try
			{
				loc = std::locale("zh-CN");  // Windows格式
			}
			catch (...)
			{
				throw std::runtime_error("当前系统不支持GBK编码");
			}

		}
		return std::move(loc);
	}

	inline std::string A2GBK(const std::string& s)
	{
		return W2A(A2W(s), GetGBKLocale());
	}

	inline std::string GBK2A(const std::string& s)
	{
		return W2A(A2W(s, GetGBKLocale()));
	}
}