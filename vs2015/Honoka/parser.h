#pragma once

#include "stdafx.h"

#define QWORD DWORD64

#pragma comment(lib, "fpu.lib")
extern "C" void ExtendedToDouble(void* float80, double* pOut);


enum ErrorType {
	ERROR_NONE,
	ERROR_DEVIDEZERO = 1,
	ERROR_PAGEFAULT = 2,
	ERROR_TYPE = 4,
	ERROR_PARSE = 8,
};

enum DataType {
	TYPE_NONE = 0,
	TYPE_BIT,
	TYPE_BYTE,
	TYPE_WORD,
	TYPE_DWORD,
	TYPE_QWORD,
	TYPE_FLOAT,
	TYPE_DOUBLE
};

struct ParseInfo {

	static BOOL bX86;

	const WCHAR* ptr;
	DWORD Error;
	DataType Type;
	BOOL bSigned;

	union {
		INT64 s64Value;
		UINT64 u64Value;
		INT32 s32Value;
		UINT32 u32Value;
		FLOAT fValue;
		DOUBLE dValue;
		INT16 s16Value;
		UINT16 u16Value;
		INT8 s8Value;
		UINT8 u8Value;
		BOOL bValue : 1;
	};

	BOOL IsFloating() const { return Type == TYPE_FLOAT || Type == TYPE_DOUBLE; }
	BOOL IsFloat() const { return Type == TYPE_FLOAT; }
	BOOL IsDouble() const { return Type == TYPE_DOUBLE; }

	//BOOL IsParsable() const {return (*ptr) && (!IsError());}
	BOOL IsError() const { return (Error & ERROR_TYPE) || (Error & ERROR_PARSE);}

	ParseInfo::ParseInfo(const WCHAR* ptr, DWORD Error) : ptr(ptr), Error(Error) {}
	ParseInfo::ParseInfo(const WCHAR* ptr, DataType Type, DWORD Error) : ptr(ptr), Type(Type), Error(Error) {}

	ParseInfo::ParseInfo(const WCHAR* ptr, DataType Type, UINT64 value, BOOL bSigned, DWORD Error) :
		ptr(ptr), Type(Type), bSigned(bSigned), Error(Error) {

		switch (Type) {
		case TYPE_BIT:
			bSigned = FALSE;
			u64Value = !!value;
			break;
		case TYPE_BYTE:
			if (bSigned)	s64Value = (INT8)value;
			else			u64Value = (UINT8)value;
			break;
		case TYPE_WORD:
			if (bSigned)	s64Value = (INT16)value;
			else			u64Value = (UINT16)value;
			break;
		case TYPE_DWORD:
			if (bSigned)	s64Value = (INT32)value;
			else			u64Value = (UINT32)value;
			break;
		case TYPE_QWORD:
			if (bSigned)	s64Value = (INT64)value;
			else			u64Value = (UINT64)value;
			break;
		case TYPE_FLOAT:
			bSigned = TRUE;
			u64Value = (UINT32)value;
			break;
		case TYPE_DOUBLE:
			bSigned = TRUE;
			u64Value = (UINT64)value;
			break;
		}
	}
	ParseInfo::ParseInfo(const WCHAR* ptr, float f, DWORD Error) : ptr(ptr), Type(TYPE_FLOAT), fValue(f), bSigned(TRUE), Error(Error) {}
	ParseInfo::ParseInfo(const WCHAR* ptr, double d, DWORD Error) : ptr(ptr), Type(TYPE_DOUBLE), dValue(d), bSigned(TRUE), Error(Error) {}

	ParseInfo SoftCast(DataType nType, BOOL bSigned) {

		switch (Type) {

		case TYPE_FLOAT:
			switch (nType) {
			case TYPE_FLOAT:
				return *this;
			case TYPE_DOUBLE:
				return ParseInfo(ptr, (double)fValue, Error);
			default:
				switch (bSigned) {
				case TRUE:	return ParseInfo(ptr, nType, (INT64)fValue, TRUE, Error);
				default:	return ParseInfo(ptr, nType, (UINT64)fValue, TRUE, Error);
				}
			}
			break;

		case TYPE_DOUBLE:
			switch (nType) {
			case TYPE_FLOAT:
				return ParseInfo(ptr, (float)dValue, Error);
			case TYPE_DOUBLE:
				return *this;
			default:
				switch (bSigned) {
				case TRUE:	return ParseInfo(ptr, nType, (INT64)dValue, TRUE, Error);
				default:	return ParseInfo(ptr, nType, (UINT64)dValue, TRUE, Error);
				}
			}
			break;

		default:
			switch (nType) {
			case TYPE_FLOAT:
				switch (bSigned) {
				case TRUE:	return ParseInfo(ptr, (float)s64Value, Error);
				default:	return ParseInfo(ptr, (float)u64Value, Error);
				}
			case TYPE_DOUBLE:
				switch (bSigned) {
				case TRUE:	return ParseInfo(ptr, (double)s64Value, Error);
				default:	return ParseInfo(ptr, (double)u64Value, Error);
				}
			}

			return ParseInfo(ptr, nType, u64Value, bSigned, Error);
		}
	}

	ParseInfo ReadPtr(HANDLE hProcess, DataType nPtrType, BOOL bSigned) {
		if (IsFloating())
			return SoftCast(TYPE_QWORD, FALSE).ReadPtr(hProcess, nPtrType, bSigned);

		SIZE_T nSize;

		switch (nPtrType) {
		case TYPE_BIT:		nSize = 1; break;
		case TYPE_BYTE:		nSize = 1; break;
		case TYPE_WORD:		nSize = 2; break;
		case TYPE_DWORD:	nSize = 4; break;
		case TYPE_FLOAT:	nSize = 4; break;
		case TYPE_QWORD:	nSize = 8; break;
		case TYPE_DOUBLE:	nSize = 8; break;
		default: return ParseInfo(ptr, ERROR_TYPE);
		}

		SIZE_T nRead;
		QWORD NewValue = 0;
		BOOL bResult = CE_ReadProcessMemory(hProcess, (LPVOID)u64Value, &NewValue, nSize, &nRead);

		if (bResult && nRead == nSize)
			return ParseInfo(ptr, nPtrType, NewValue, bSigned, Error);

		return ParseInfo(ptr, nPtrType, ERROR_PAGEFAULT);
	}

	//Unary operators
	ParseInfo operator+() {
		//convert unsigned int to signed int
		ParseInfo r(ptr, Type, 0, TRUE, Error);
		switch (Type) {
		case TYPE_FLOAT:	r.fValue = +fValue;		break;
		case TYPE_DOUBLE:	r.dValue = +dValue;		break;
		default:			r.s64Value = +s64Value;	break;
		}
		return r;
	}
	ParseInfo operator-() {
		//convert unsigned int to signed int
		ParseInfo r(ptr, Type, 0, TRUE, Error);
		switch (Type) {
		case TYPE_FLOAT:	r.fValue = -fValue;		break;
		case TYPE_DOUBLE:	r.dValue = -dValue;		break;
		default:			r.s64Value = -s64Value;	break;
		}
		return r;
	}
	ParseInfo operator~() {
		if (IsFloating()) return ParseInfo(ptr, ERROR_TYPE);
		return ParseInfo(ptr, Type, ~s64Value, bSigned, Error);
	}
	ParseInfo operator!() {
		if (IsFloating()) return ParseInfo(ptr, ERROR_TYPE);
		return ParseInfo(ptr, Type, !s64Value, FALSE, Error);
	}
	ParseInfo operator*() {
		return ReadPtr(*CheatEngine.OpenedProcessHandle, ParseInfo::bX86 ? TYPE_DWORD : TYPE_QWORD, FALSE);
	}

	//Binary operators
	//Term
	ParseInfo operator*(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, max(a.Type, b.Type), a.s64Value * b.s64Value, a.bSigned && b.bSigned, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;
		if (a.IsDouble() || b.IsDouble())
			return ParseInfo(b.ptr, v1 * v2, a.Error | b.Error);

		return ParseInfo(b.ptr, float(v1 * v2), a.Error | b.Error);
	}
	ParseInfo operator/(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating()) {
			if (b.s64Value == 0)
				return ParseInfo(b.ptr, max(a.Type, b.Type), ERROR_DEVIDEZERO);
			return ParseInfo(b.ptr, max(a.Type, b.Type), a.s64Value / b.s64Value, a.bSigned && b.bSigned, a.Error | b.Error);
		}

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;
		if (a.IsDouble() || b.IsDouble()) {
			if (v2 == 0)
				return ParseInfo(b.ptr, TYPE_DOUBLE, ERROR_DEVIDEZERO);
			return ParseInfo(b.ptr, v1 / v2, a.Error | b.Error);
		}

		if (v2 == 0)
			return ParseInfo(b.ptr, TYPE_FLOAT, ERROR_DEVIDEZERO);
		return ParseInfo(b.ptr, float(v1 / v2), a.Error | b.Error);
	}
	ParseInfo operator%(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (a.IsFloating() || b.IsFloating())
			return ParseInfo(b.ptr, ERROR_TYPE);

		if (b.u64Value == 0)
			return ParseInfo(b.ptr, max(a.Type, b.Type), ERROR_DEVIDEZERO);

		if (a.bSigned && b.bSigned)
			return ParseInfo(b.ptr, max(a.Type, b.Type), a.s64Value % b.s64Value, TRUE, a.Error | b.Error);

		return ParseInfo(b.ptr, max(a.Type, b.Type), a.u64Value % b.u64Value, FALSE, a.Error | b.Error);
	}
	//Add
	ParseInfo operator+(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, max(a.Type, b.Type), a.s64Value + b.s64Value, a.bSigned && b.bSigned, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;
		if (a.IsDouble() || b.IsDouble())
			return ParseInfo(b.ptr, v1 + v2, a.Error | b.Error);

		return ParseInfo(b.ptr, float(v1 + v2), a.Error | b.Error);
	}
	ParseInfo operator-(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, max(a.Type, b.Type), a.s64Value - b.s64Value, a.bSigned && b.bSigned, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;
		if (a.IsDouble() || b.IsDouble())
			return ParseInfo(b.ptr, v1 - v2, a.Error | b.Error);

		return ParseInfo(b.ptr, float(v1 - v2), a.Error | b.Error);
	}
	//Shift
	ParseInfo operator<<(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (a.IsFloating() || b.IsFloating())
			return ParseInfo(b.ptr, ERROR_TYPE);

		return ParseInfo(b.ptr, a.Type, a.u64Value << b.u64Value, a.bSigned, a.Error | b.Error);
	}
	ParseInfo operator >> (const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (a.IsFloating() || b.IsFloating())
			return ParseInfo(b.ptr, ERROR_TYPE);

		if (a.bSigned)
			return ParseInfo(b.ptr, a.Type, a.s64Value >> b.u64Value, TRUE, a.Error | b.Error);

		return ParseInfo(b.ptr, a.Type, a.u64Value >> b.u64Value, FALSE, a.Error | b.Error);
	}
	//Compare
	ParseInfo operator<(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, TYPE_BIT, a.s64Value < b.s64Value, FALSE, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;

		return ParseInfo(b.ptr, TYPE_BIT, v1 < v2, FALSE, a.Error | b.Error);
	}
	ParseInfo operator>(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, TYPE_BIT, a.s64Value > b.s64Value, FALSE, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;

		return ParseInfo(b.ptr, TYPE_BIT, v1 > v2, FALSE, a.Error | b.Error);
	}
	ParseInfo operator<=(const ParseInfo& b) {
		return !(*this > b);
	}
	ParseInfo operator>=(const ParseInfo& b) {
		return !(*this < b);
	}
	//Equal
	ParseInfo operator==(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, TYPE_BIT, a.s64Value == b.s64Value, FALSE, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;

		return ParseInfo(b.ptr, TYPE_BIT, v1 == v2, FALSE, a.Error | b.Error);
	}
	ParseInfo operator!=(const ParseInfo& b) {
		return !(*this == b);
	}
	//Bit operators
	ParseInfo operator&(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (a.IsFloating() || b.IsFloating())
			return ParseInfo(b.ptr, ERROR_TYPE);

		return ParseInfo(b.ptr, a.Type, a.u64Value & b.u64Value, a.bSigned && b.bSigned, a.Error | b.Error);
	}
	ParseInfo operator^(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (a.IsFloating() || b.IsFloating())
			return ParseInfo(b.ptr, ERROR_TYPE);

		return ParseInfo(b.ptr, a.Type, a.u64Value ^ b.u64Value, a.bSigned && b.bSigned, a.Error | b.Error);
	}
	ParseInfo operator|(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (a.IsFloating() || b.IsFloating())
			return ParseInfo(b.ptr, ERROR_TYPE);

		return ParseInfo(b.ptr, a.Type, a.u64Value | b.u64Value, a.bSigned && b.bSigned, a.Error | b.Error);
	}
	//Logic operators
	ParseInfo operator&&(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, TYPE_BIT, a.s64Value && b.s64Value, FALSE, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;

		return ParseInfo(b.ptr, TYPE_BIT, v1 && v2, FALSE, a.Error | b.Error);
	}
	ParseInfo operator||(const ParseInfo& b) {
		const ParseInfo& a = *this;

		if (!a.IsFloating() && !b.IsFloating())
			return ParseInfo(b.ptr, TYPE_BIT, a.s64Value || b.s64Value, FALSE, a.Error | b.Error);

		double v1 = a.IsDouble() ? a.dValue : a.IsFloat() ? a.fValue : a.bSigned ? a.s64Value : a.u64Value;
		double v2 = b.IsDouble() ? b.dValue : b.IsFloat() ? b.fValue : b.bSigned ? b.s64Value : b.u64Value;

		return ParseInfo(b.ptr, TYPE_BIT, v1 || v2, FALSE, a.Error | b.Error);
	}
};

class Parser {
private:
	CONTEXT ctx;
public:
	static int ParseSymbol(const WCHAR* ptr) {
		if (!ptr)
			return 0;
		int i = 0;
		for (i = 0; isalnum(ptr[i]) || ptr[i] == '_' || ptr[i] == '.'; i++);
		return i;
	}

	static BOOL IsPrefix(const CString& str, LPCWSTR prefix) {
		size_t len = wcslen(prefix);
		for (int i = 0; i < len; i++) {
			if (str[i] != prefix[i])
				return FALSE;
		}
		return TRUE;
	}

	Parser(const CONTEXT& ctx) : ctx(ctx) {}

	ParseInfo Token(const WCHAR* ptr) {
		int i = ParseSymbol(ptr);
		CString szSymbol(ptr, i);
		ptr += i;

		if (szSymbol.GetLength() == 0)
			return ParseInfo(ptr, ERROR_PARSE);

		//Rax
		if (szSymbol.CompareNoCase(L"rax") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rax, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eax") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rax, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ax") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rax, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ah") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rax), FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"al") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rax, FALSE, ERROR_NONE);

		//Rbx
		if (szSymbol.CompareNoCase(L"rbx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rbx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ebx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rbx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"bx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rbx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"bh") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rbx), FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"bl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rbx, FALSE, ERROR_NONE);

		//Rcx
		if (szSymbol.CompareNoCase(L"rcx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rcx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ecx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rcx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"cx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rcx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ch") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rcx), FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"cl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rcx, FALSE, ERROR_NONE);

		//Rdx
		if (szSymbol.CompareNoCase(L"rdx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rdx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"edx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rdx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rdx, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dh") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rdx), FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rdx, FALSE, ERROR_NONE);

		//Rsi
		if (szSymbol.CompareNoCase(L"rsi") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rsi, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"esi") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rsi, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"si") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rsi, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"sil") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rsi, FALSE, ERROR_NONE);

		//Rdi
		if (szSymbol.CompareNoCase(L"rdi") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rdi, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"edi") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rdi, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"di") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rdi, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dil") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rdi, FALSE, ERROR_NONE);

		//Rbp
		if (szSymbol.CompareNoCase(L"rbp") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rbp, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ebp") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rbp, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"bp") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rbp, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"bpl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rbp, FALSE, ERROR_NONE);

		//Rsp
		if (szSymbol.CompareNoCase(L"rsp") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rsp, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"esp") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rsp, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"sp") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rsp, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"spl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rsp, FALSE, ERROR_NONE);

		//R8
		if (szSymbol.CompareNoCase(L"r8") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R8, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r8d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R8, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r8w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R8, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r8b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R8, FALSE, ERROR_NONE);

		//R9
		if (szSymbol.CompareNoCase(L"r9") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R9, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r9d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R9, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r9w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R9, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r9b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R9, FALSE, ERROR_NONE);

		//R10
		if (szSymbol.CompareNoCase(L"r10") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R10, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r10d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R10, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r10w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R10, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r10b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R10, FALSE, ERROR_NONE);

		//R11
		if (szSymbol.CompareNoCase(L"r11") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R11, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r11d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R11, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r11w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R11, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r11b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R11, FALSE, ERROR_NONE);

		//R12
		if (szSymbol.CompareNoCase(L"r12") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R12, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r12d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R12, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r12w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R12, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r12b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R12, FALSE, ERROR_NONE);

		//R13
		if (szSymbol.CompareNoCase(L"r13") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R13, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r13d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R13, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r13w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R13, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r13b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R13, FALSE, ERROR_NONE);

		//R14
		if (szSymbol.CompareNoCase(L"r14") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R14, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r14d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R14, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r14w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R14, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r14b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R14, FALSE, ERROR_NONE);

		//R15
		if (szSymbol.CompareNoCase(L"r15") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R15, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r15d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R15, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r15w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R15, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"r15b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R15, FALSE, ERROR_NONE);

		//Rip
		if (szSymbol.CompareNoCase(L"rip") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rip, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eip") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rip, FALSE, ERROR_NONE);

		//Segment registers
		if (szSymbol.CompareNoCase(L"cs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegCs, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ss") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegSs, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"ds") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegDs, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"es") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegEs, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"fs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegFs, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"gs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegGs, FALSE, ERROR_NONE);

		EFlags eflags;
		eflags.dwValue = ctx.EFlags;

		//EFlags
		if (szSymbol.CompareNoCase(L"eflags") == 0)
			return ParseInfo(ptr, TYPE_DWORD, eflags.dwValue, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.cf") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.CF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.pf") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.PF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.af") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.AF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.zf") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.ZF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.sf") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.SF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.tf") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.TF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.if") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.IF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.df") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.DF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.of") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.OF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.iopl0") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.IOPL0, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.iopl1") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.IOPL1, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.nt") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.NT, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.rf") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.RF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.vm") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.VM, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.ac") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.AC, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.vif") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.VIF, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.vip") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.VIP, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"eflags.id") == 0)
			return ParseInfo(ptr, TYPE_BIT, eflags.ID, FALSE, ERROR_NONE);

		//Debug registers
		if (szSymbol.CompareNoCase(L"dr0") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr0, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dr1") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr1, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dr2") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr2, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dr3") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr3, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dr6") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr6, FALSE, ERROR_NONE);
		if (szSymbol.CompareNoCase(L"dr7") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr7, FALSE, ERROR_NONE);

		//st register
		if (szSymbol.Left(2).CompareNoCase(L"st") == 0) {
			int stIndex = szSymbol[2] - L'0';
			if (stIndex >= 0 && stIndex <= 7) {
				double dValue;
				ExtendedToDouble(&ctx.FltSave.FloatRegisters[stIndex], &dValue);
				if (szSymbol[3] == 0)
					return ParseInfo(ptr, (double)dValue, ERROR_NONE);
				if (szSymbol[4] == 0) {
					if (szSymbol[3] == L'f' || szSymbol[3] == L'F')
						return ParseInfo(ptr, (float)dValue, ERROR_NONE);
					if (szSymbol[3] == L'q' || szSymbol[3] == L'Q') {
						UINT64 uValue = *(PUINT64)&ctx.FltSave.FloatRegisters[stIndex];
						return ParseInfo(ptr, TYPE_QWORD, uValue, FALSE, ERROR_NONE);
					}
				}

			}
		}

		//xmm register 0~9
		int j = 3;
		if (szSymbol.Left(j).CompareNoCase(L"xmm") == 0) {
			int xmmIndex = szSymbol[j] - L'0';
			if (xmmIndex >= 0 && xmmIndex <= 9) {
				LARGE_INTEGER lvalue; lvalue.QuadPart = ctx.FltSave.XmmRegisters[xmmIndex].Low;
				LARGE_INTEGER hvalue; hvalue.QuadPart = ctx.FltSave.XmmRegisters[xmmIndex].High;
				if (szSymbol[j + 1] == L'0') {
					if (szSymbol[j + 2] == 0)
						return ParseInfo(ptr, *(double*)&lvalue, ERROR_NONE);
					else if (szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'q' || szSymbol[j + 2] == 'Q')
							return ParseInfo(ptr, TYPE_QWORD, lvalue.QuadPart, FALSE, ERROR_NONE);
						else if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&lvalue.LowPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, lvalue.LowPart, FALSE, ERROR_NONE);
					}
				}
				else if (szSymbol[j + 1] == L'1') {
					if (szSymbol[j + 2] == 0)
						return ParseInfo(ptr, *(double*)&hvalue, ERROR_NONE);
					else if (szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'q' || szSymbol[j + 2] == 'Q')
							return ParseInfo(ptr, TYPE_QWORD, hvalue.QuadPart, FALSE, ERROR_NONE);
						else if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&lvalue.HighPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, lvalue.HighPart, FALSE, ERROR_NONE);
					}
				}
				else if (szSymbol[j + 1] == L'2') {
					if (szSymbol[j + 2] && szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&hvalue.LowPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, hvalue.LowPart, FALSE, ERROR_NONE);
					}
				}
				else if (szSymbol[j + 1] == L'3') {
					if (szSymbol[j + 2] && szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&hvalue.HighPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, hvalue.HighPart, FALSE, ERROR_NONE);
					}
				}
			}
		}

		//xmm register 10~15
		j = 4;
		if (szSymbol.Left(j).CompareNoCase(L"xmm1") == 0) {
			int xmmIndex = szSymbol[j] - L'0' + 10;
			if (xmmIndex >= 10 && xmmIndex <= 15) {
				LARGE_INTEGER lvalue; lvalue.QuadPart = ctx.FltSave.XmmRegisters[xmmIndex].Low;
				LARGE_INTEGER hvalue; hvalue.QuadPart = ctx.FltSave.XmmRegisters[xmmIndex].High;
				if (szSymbol[j + 1] == L'0') {
					if (szSymbol[j + 2] == 0)
						return ParseInfo(ptr, *(double*)&lvalue, ERROR_NONE);
					else if (szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'q' || szSymbol[j + 2] == 'Q')
							return ParseInfo(ptr, TYPE_QWORD, lvalue.QuadPart, FALSE, ERROR_NONE);
						else if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&lvalue.LowPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, lvalue.LowPart, FALSE, ERROR_NONE);
					}
				}
				else if (szSymbol[j + 1] == L'1') {
					if (szSymbol[j + 2] == 0)
						return ParseInfo(ptr, *(double*)&hvalue, ERROR_NONE);
					else if (szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'q' || szSymbol[j + 2] == 'Q')
							return ParseInfo(ptr, TYPE_QWORD, hvalue.QuadPart, FALSE, ERROR_NONE);
						else if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&lvalue.HighPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, lvalue.HighPart, FALSE, ERROR_NONE);
					}
				}
				else if (szSymbol[j + 1] == L'2') {
					if (szSymbol[j + 2] && szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&hvalue.LowPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, hvalue.LowPart, FALSE, ERROR_NONE);
					}
				}
				else if (szSymbol[j + 1] == L'3') {
					if (szSymbol[j + 2] && szSymbol[j + 3] == 0) {
						if (szSymbol[j + 2] == 'f' || szSymbol[j + 2] == 'F')
							return ParseInfo(ptr, *(float*)&hvalue.HighPart, ERROR_NONE);
						else if (szSymbol[j + 2] == 'd' || szSymbol[j + 2] == 'D')
							return ParseInfo(ptr, TYPE_DWORD, hvalue.HighPart, FALSE, ERROR_NONE);
					}
				}
			}
		}

		//Decimal constant
		WCHAR* pEnd;
		DWORD64 Number = _wcstoui64(szSymbol, &pEnd, 10);
		if (errno == 0 && *pEnd == 0) {
			if(Number < 0x100000000)
				return ParseInfo(ptr, TYPE_DWORD, Number, FALSE, ERROR_NONE);
			return ParseInfo(ptr, TYPE_QWORD, Number, FALSE, ERROR_NONE);
		}

		//Float constant
		if (szSymbol.Find(L'.') != -1) {
			DOUBLE dNumber = wcstod(szSymbol, &pEnd);
			if (errno == 0) {
				if (*pEnd == 0) return ParseInfo(ptr, dNumber, ERROR_NONE);
				if ((*pEnd == L'f' || *pEnd == L'F') && pEnd[1] == 0) return ParseInfo(ptr, (float)dNumber, ERROR_NONE);
			}
		}

		//Handle to cheat engine
		UINT64 Address;

		char szBuffer[0x100];
		WideCharToMultiByte(CP_ACP, 0, szSymbol, -1, szBuffer, sizeof(szBuffer), NULL, NULL);

		if (CheatEngine.sym_nameToAddress(szBuffer, &Address)) {
			if (ParseInfo::bX86)
				return ParseInfo(ptr, TYPE_DWORD, Address, FALSE, ERROR_NONE);
			return ParseInfo(ptr, TYPE_QWORD, Address, FALSE, ERROR_NONE);
		}

		return ParseInfo(ptr, ERROR_PARSE);
	}

	ParseInfo Bracket(const WCHAR* ptr) {
		int i = ParseSymbol(ptr);
		CString szSymbol(ptr, i);

		BOOL bSigned = FALSE;
		DataType nType = TYPE_NONE;

		if (i) {
			if (szSymbol.CompareNoCase(L"int64") == 0) {
				nType = TYPE_QWORD;
				bSigned = TRUE;
			}
			else if (szSymbol.CompareNoCase(L"int32") == 0) {
				nType = TYPE_DWORD;
				bSigned = TRUE;
			}
			else if (szSymbol.CompareNoCase(L"int") == 0) {
				nType = TYPE_DWORD;
				bSigned = TRUE;
			}
			else if (szSymbol.CompareNoCase(L"int16") == 0) {
				nType = TYPE_WORD;
				bSigned = TRUE;
			}
			else if (szSymbol.CompareNoCase(L"int8") == 0) {
				nType = TYPE_BYTE;
				bSigned = TRUE;
			}
			else if (szSymbol.CompareNoCase(L"uint64") == 0) {
				nType = TYPE_QWORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"uint32") == 0) {
				nType = TYPE_DWORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"uint") == 0) {
				nType = TYPE_DWORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"uint16") == 0) {
				nType = TYPE_WORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"uint8") == 0) {
				nType = TYPE_BYTE;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"qword") == 0) {
				nType = TYPE_QWORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"dword") == 0) {
				nType = TYPE_DWORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"word") == 0) {
				nType = TYPE_WORD;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"byte") == 0) {
				nType = TYPE_BYTE;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"bool") == 0) {
				nType = TYPE_BIT;
				bSigned = FALSE;
			}
			else if (szSymbol.CompareNoCase(L"float") == 0) {
				nType = TYPE_FLOAT;
				bSigned = TRUE;
			}
			else if (szSymbol.CompareNoCase(L"double") == 0) {
				nType = TYPE_DOUBLE;
				bSigned = TRUE;
			}

			if (nType != TYPE_NONE)
				ptr += i;
		}

		switch (*ptr) {
		case L'[': {
			ParseInfo info = Expression(ptr + 1);
			if (!info.ptr || *info.ptr != L']')
				return ParseInfo(info.ptr, ERROR_PARSE);
			info.ptr++;
			if (nType != TYPE_NONE)
				return info.ReadPtr(*CheatEngine.OpenedProcessHandle, nType, bSigned);
			return info.ReadPtr(*CheatEngine.OpenedProcessHandle, ParseInfo::bX86 ? TYPE_DWORD : TYPE_QWORD, FALSE);
		}
		case L'(': {
			ParseInfo info = Expression(ptr + 1);
			if (!info.ptr || *info.ptr != L')')
				return ParseInfo(info.ptr, ERROR_PARSE);
			info.ptr++;
			if (nType != TYPE_NONE)
				return info.SoftCast(nType, bSigned);
			return info;
		}
		case L'{': {
			ParseInfo info = Expression(ptr + 1);
			if (!info.ptr || *info.ptr != L'}')
				return ParseInfo(info.ptr, ERROR_PARSE);
			info.ptr++;
			if (nType != TYPE_NONE)
				return ParseInfo(info.ptr, nType, info.u64Value, bSigned, info.Error);
			return info;
		}
		}

		if (nType != TYPE_NONE)
			return ParseInfo(ptr, ERROR_PARSE);

		return Token(ptr);
	}

	ParseInfo Unary(const WCHAR* ptr) {
		ParseInfo info(ptr, ERROR_PARSE);
		if (*info.ptr == 0)
			return info;

		CString szStr(ptr);

		if (IsPrefix(szStr, L"*"))
			info = *Unary(ptr + 1);
		else if (IsPrefix(szStr, L"+"))
			info = +Unary(ptr + 1);
		else if (IsPrefix(szStr, L"-"))
			info = -Unary(ptr + 1);
		else if (IsPrefix(szStr, L"~"))
			info = ~Unary(ptr + 1);
		else if (IsPrefix(szStr, L"!"))
			info = !Unary(ptr + 1);

		if (!info.IsError())
			return info;

		return Bracket(ptr);
	}

	ParseInfo Term(const WCHAR* ptr) {
		ParseInfo info = Unary(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"*"))
				temp = info * Unary(info.ptr + 1);
			else if (IsPrefix(szStr, L"/"))
				temp = info / Unary(info.ptr + 1);
			else if (IsPrefix(szStr, L"%"))
				temp = info % Unary(info.ptr + 1);
			else 
				break;
		}
		return info;
	}

	ParseInfo Add(const WCHAR* ptr) {
		ParseInfo info = Term(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"+"))
				temp = info + Term(info.ptr + 1);
			else if (IsPrefix(szStr, L"-"))
				temp = info - Term(info.ptr + 1);
			else
				break;
		}
		return info;
	}

	ParseInfo Shift(const WCHAR* ptr) {
		ParseInfo info = Add(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"<<"))
				temp = info << Add(info.ptr + 2);
			else if (IsPrefix(szStr, L">>"))
				temp = info >> Add(info.ptr + 2);
			else
				break;
		}
		return info;
	}

	ParseInfo Compare(const WCHAR* ptr) {
		ParseInfo info = Shift(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"<="))
				temp = info <= Shift(info.ptr + 2);
			else if (IsPrefix(szStr, L">>"))
				temp = info >= Shift(info.ptr + 2);
			else if (IsPrefix(szStr, L"<"))
				temp = info < Shift(info.ptr + 1);
			else if (IsPrefix(szStr, L">"))
				temp = info > Shift(info.ptr + 1);
			else
				break;
		}
		return info;
	}

	ParseInfo Equal(const WCHAR* ptr) {
		ParseInfo info = Compare(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"=="))
				temp = info == Compare(info.ptr + 2);
			else if (IsPrefix(szStr, L"!="))
				temp = info != Compare(info.ptr + 2);
			else
				break;
		}
		return info;
	}

	ParseInfo BitAnd(const WCHAR* ptr) {
		ParseInfo info = Equal(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"&"))
				temp = info & Equal(info.ptr + 1);
			else
				break;
		}
		return info;
	}

	ParseInfo BitXor(const WCHAR* ptr) {
		ParseInfo info = BitAnd(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"^"))
				temp = info ^ BitAnd(info.ptr + 1);
			else
				break;
		}
		return info;
	}

	ParseInfo BitOr(const WCHAR* ptr) {
		ParseInfo info = BitXor(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"|"))
				temp = info | BitXor(info.ptr + 1);
			else
				break;
		}
		return info;
	}

	ParseInfo LogicAnd(const WCHAR* ptr) {
		ParseInfo info = BitOr(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"&&"))
				temp = info && BitOr(info.ptr + 2);
			else
				break;
		}
		return info;
	}

	ParseInfo LogicOr(const WCHAR* ptr) {
		ParseInfo info = LogicAnd(ptr);
		ParseInfo temp = info;

		while (!temp.IsError()) {
			info = temp;
			CString szStr(info.ptr);
			if (IsPrefix(szStr, L"||"))
				temp = info || LogicAnd(info.ptr + 2);
			else
				break;
		}
		return info;
	}

	ParseInfo Expression(const WCHAR* ptr) {
		return LogicOr(ptr);
	}

	ParseInfo Parse(const WCHAR* szString) {
		CString szCopy = szString;
		szCopy.Remove(L' ');
		szCopy.Remove(L'\t');
		ParseInfo result = Expression(szCopy);
		if (*result.ptr)
			result.Error |= ERROR_PARSE;
		return result;
	}
};