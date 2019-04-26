#pragma once

#include "stdafx.h"

#define QWORD DWORD64

enum ParseType {
	TYPE_UNKNOWN = -1,
	TYPE_BOOL,
	TYPE_BYTE, 
	TYPE_WORD, 
	TYPE_DWORD, 
	TYPE_QWORD,
};

struct ParseInfo {
	static BOOL bX86;

	const WCHAR* ptr;
	ParseType type;
	__int64 value;
	BOOL bValidPtr;

	ParseInfo::ParseInfo() : ptr(0), type(TYPE_QWORD), value(0), bValidPtr(FALSE) {}

	ParseInfo::ParseInfo(const WCHAR* ptr, ParseType type, QWORD value, BOOL bValidPtr) :
		ptr(ptr), type(type), value(value), bValidPtr(bValidPtr) 
	{
		this->ptr = ptr;

		switch(type) {
		case TYPE_BOOL: this->value = !!value; break;
		case TYPE_BYTE: this->value = (BYTE)value; break;
		case TYPE_WORD: this->value = (WORD)value; break;
		case TYPE_DWORD:this->value = (DWORD)value;break;
		case TYPE_QWORD:this->value = (QWORD)value;break;
		case TYPE_UNKNOWN:
			if(bX86) {
				this->type = TYPE_DWORD;
				this->value = (DWORD)value;
			}
			else {
				this->type = TYPE_QWORD;
				this->value = (QWORD)value;
			}
			break;
		}
	}

	ParseInfo ReadPtr(HANDLE hProcess, ParseType nPtrType) {
		SIZE_T nSize;

		switch(nPtrType) {
		case TYPE_BYTE: nSize = 1; break;
		case TYPE_WORD: nSize = 2; break;
		case TYPE_DWORD: nSize = 4; break;
		case TYPE_QWORD: nSize = 8; break;
		case TYPE_UNKNOWN: 
			if(bX86)	nSize = 4;
			else		nSize = 8;
			break;
		default: return ParseInfo(0, nPtrType, 0, FALSE);
		}

		SIZE_T nRead;
		QWORD NewValue = 0;
		BOOL bResult = CE_ReadProcessMemory(hProcess,(void*)value, &NewValue, nSize, &nRead);

		if(bResult && nRead == nSize)
			return ParseInfo(ptr, nPtrType, NewValue, TRUE);

		return ParseInfo(ptr, nPtrType, 0, FALSE);
	}

	//Unary operators
	ParseInfo operator+() {return ParseInfo(ptr, type, +value, bValidPtr);}
	ParseInfo operator-() {return ParseInfo(ptr, type, -value, bValidPtr);}
	ParseInfo operator~() {return ParseInfo(ptr, type, ~value, bValidPtr);}
	ParseInfo operator!() {return ParseInfo(ptr, TYPE_BOOL, !value, bValidPtr);}
	ParseInfo operator*() {return ReadPtr(*CheatEngine.OpenedProcessHandle, TYPE_UNKNOWN);}

	//Binary operators
	//Term
	ParseInfo operator*(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value * b.value, bValidPtr && b.bValidPtr);}
	ParseInfo operator/(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value / b.value, bValidPtr && b.bValidPtr);}
	ParseInfo operator%(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value % b.value, bValidPtr && b.bValidPtr);}
	//Add
	ParseInfo operator+(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value + b.value, bValidPtr && b.bValidPtr);}
	ParseInfo operator-(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value - b.value, bValidPtr && b.bValidPtr);}
	//Shift
	ParseInfo operator<<(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value << b.value, bValidPtr && b.bValidPtr); }
	ParseInfo operator>>(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value >> b.value, bValidPtr && b.bValidPtr); }
	//Compare
	ParseInfo operator<(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value < b.value, bValidPtr && b.bValidPtr); }
	ParseInfo operator<=(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value <= b.value, bValidPtr && b.bValidPtr); }
	ParseInfo operator>(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value > b.value, bValidPtr && b.bValidPtr); }
	ParseInfo operator>=(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value >= b.value, bValidPtr && b.bValidPtr); }
	//Equal
	ParseInfo operator==(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value == b.value, bValidPtr && b.bValidPtr); }
	ParseInfo operator!=(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value != b.value, bValidPtr && b.bValidPtr); }
	//Bit operators
	ParseInfo operator&(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value & b.value, bValidPtr && b.bValidPtr);}
	ParseInfo operator^(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value ^ b.value, bValidPtr && b.bValidPtr);}
	ParseInfo operator|(const ParseInfo& b) {return ParseInfo(b.ptr, max(type, b.type), value | b.value, bValidPtr && b.bValidPtr);}
	//Logic operators
	ParseInfo operator&&(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value && b.value, bValidPtr && b.bValidPtr); }
	ParseInfo operator||(const ParseInfo& b) {return ParseInfo(b.ptr, TYPE_BOOL, value || b.value, bValidPtr && b.bValidPtr); }
};

class Parser {
private:
	CONTEXT ctx;
public:
	static void RemoveSpace(WCHAR* s) {
		WCHAR* i = s;
		WCHAR* j = s;
		do { 
			while(isspace(*i)) {
				i++;
			} 
		} while(*j++ = *i++);
	}

	static int ParseSymbol(const WCHAR* ptr) {
		if(!ptr)
			return 0;
		int i = 0;
		for(i = 0 ; isalnum(ptr[i]) || ptr[i] == '_' || ptr[i] == '.' ; i++);
		return i;
	}

	Parser(const CONTEXT& ctx) : ctx(ctx) {}

	ParseInfo Token(const WCHAR* ptr) {
		int i = ParseSymbol(ptr);
		CString szSymbol(ptr, i);
		ptr += i;

		//Rax
		if(szSymbol.CompareNoCase(L"rax") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rax, TRUE);
		if(szSymbol.CompareNoCase(L"eax") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rax, TRUE);
		if(szSymbol.CompareNoCase(L"ax") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rax, TRUE);
		if(szSymbol.CompareNoCase(L"ah") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rax), TRUE);
		if(szSymbol.CompareNoCase(L"al") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rax, TRUE);

		//Rbx
		if(szSymbol.CompareNoCase(L"rbx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rbx, TRUE);
		if(szSymbol.CompareNoCase(L"ebx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rbx, TRUE);
		if(szSymbol.CompareNoCase(L"bx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rbx, TRUE);
		if(szSymbol.CompareNoCase(L"bh") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rbx), TRUE);
		if(szSymbol.CompareNoCase(L"bl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rbx, TRUE);

		//Rcx
		if(szSymbol.CompareNoCase(L"rcx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rcx, TRUE);
		if(szSymbol.CompareNoCase(L"ecx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rcx, TRUE);
		if(szSymbol.CompareNoCase(L"cx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rcx, TRUE);
		if(szSymbol.CompareNoCase(L"ch") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rcx), TRUE);
		if(szSymbol.CompareNoCase(L"cl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rcx, TRUE);

		//Rdx
		if(szSymbol.CompareNoCase(L"rdx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rdx, TRUE);
		if(szSymbol.CompareNoCase(L"edx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rdx, TRUE);
		if(szSymbol.CompareNoCase(L"dx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rdx, TRUE);
		if(szSymbol.CompareNoCase(L"dh") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rdx), TRUE);
		if(szSymbol.CompareNoCase(L"dl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rdx, TRUE);
	
		//Rsi
		if(szSymbol.CompareNoCase(L"rsi") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rsi, TRUE);
		if(szSymbol.CompareNoCase(L"esi") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rsi, TRUE);
		if(szSymbol.CompareNoCase(L"si") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rsi, TRUE);
		if(szSymbol.CompareNoCase(L"sil") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rsi, TRUE);

		//Rdi
		if(szSymbol.CompareNoCase(L"rdi") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rdi, TRUE);
		if(szSymbol.CompareNoCase(L"edi") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rdi, TRUE);
		if(szSymbol.CompareNoCase(L"di") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rdi, TRUE);
		if(szSymbol.CompareNoCase(L"dil") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rdi, TRUE);

		//Rbp
		if(szSymbol.CompareNoCase(L"rbp") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rbp, TRUE);
		if(szSymbol.CompareNoCase(L"ebp") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rbp, TRUE);
		if(szSymbol.CompareNoCase(L"bp") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rbp, TRUE);
		if(szSymbol.CompareNoCase(L"bpl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rbp, TRUE);

		//Rsp
		if(szSymbol.CompareNoCase(L"rsp") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rsp, TRUE);
		if(szSymbol.CompareNoCase(L"esp") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rsp, TRUE);
		if(szSymbol.CompareNoCase(L"sp") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rsp, TRUE);
		if(szSymbol.CompareNoCase(L"spl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rsp, TRUE);

		//R8
		if(szSymbol.CompareNoCase(L"r8") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R8, TRUE);
		if(szSymbol.CompareNoCase(L"r8d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R8, TRUE);
		if(szSymbol.CompareNoCase(L"r8w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R8, TRUE);
		if(szSymbol.CompareNoCase(L"r8b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R8, TRUE);

		//R9
		if(szSymbol.CompareNoCase(L"r9") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R9, TRUE);
		if(szSymbol.CompareNoCase(L"r9d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R9, TRUE);
		if(szSymbol.CompareNoCase(L"r9w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R9, TRUE);
		if(szSymbol.CompareNoCase(L"r9b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R9, TRUE);

		//R10
		if(szSymbol.CompareNoCase(L"r10") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R10, TRUE);
		if(szSymbol.CompareNoCase(L"r10d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R10, TRUE);
		if(szSymbol.CompareNoCase(L"r10w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R10, TRUE);
		if(szSymbol.CompareNoCase(L"r10b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R10, TRUE);

		//R11
		if(szSymbol.CompareNoCase(L"r11") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R11, TRUE);
		if(szSymbol.CompareNoCase(L"r11d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R11, TRUE);
		if(szSymbol.CompareNoCase(L"r11w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R11, TRUE);
		if(szSymbol.CompareNoCase(L"r11b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R11, TRUE);

		//R12
		if(szSymbol.CompareNoCase(L"r12") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R12, TRUE);
		if(szSymbol.CompareNoCase(L"r12d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R12, TRUE);
		if(szSymbol.CompareNoCase(L"r12w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R12, TRUE);
		if(szSymbol.CompareNoCase(L"r12b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R12, TRUE);
	
		//R13
		if(szSymbol.CompareNoCase(L"r13") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R13, TRUE);
		if(szSymbol.CompareNoCase(L"r13d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R13, TRUE);
		if(szSymbol.CompareNoCase(L"r13w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R13, TRUE);
		if(szSymbol.CompareNoCase(L"r13b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R13, TRUE);

		//R14
		if(szSymbol.CompareNoCase(L"r14") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R14, TRUE);
		if(szSymbol.CompareNoCase(L"r14d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R14, TRUE);
		if(szSymbol.CompareNoCase(L"r14w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R14, TRUE);
		if(szSymbol.CompareNoCase(L"r14b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R14, TRUE);

		//R15
		if(szSymbol.CompareNoCase(L"r15") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R15, TRUE);
		if(szSymbol.CompareNoCase(L"r15d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R15, TRUE);
		if(szSymbol.CompareNoCase(L"r15w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R15, TRUE);
		if(szSymbol.CompareNoCase(L"r15b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R15, TRUE);

		//Rip
		if(szSymbol.CompareNoCase(L"rip") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rip, TRUE);
		if(szSymbol.CompareNoCase(L"eip") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rip, TRUE);

		//Segment registers
		if(szSymbol.CompareNoCase(L"cs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegCs, TRUE);
		if(szSymbol.CompareNoCase(L"ss") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegSs, TRUE);
		if(szSymbol.CompareNoCase(L"ds") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegDs, TRUE);
		if(szSymbol.CompareNoCase(L"es") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegEs, TRUE);
		if(szSymbol.CompareNoCase(L"fs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegFs, TRUE);
		if(szSymbol.CompareNoCase(L"gs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegGs, TRUE);

		EFlags eflags;
		eflags.dwValue = ctx.EFlags;

		//EFlags
		if(szSymbol.CompareNoCase(L"eflags") == 0)
			return ParseInfo(ptr, TYPE_DWORD, eflags.dwValue, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.cf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.CF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.pf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.PF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.af") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.AF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.zf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.ZF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.sf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.SF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.tf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.TF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.if") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.IF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.df") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.DF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.of") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.OF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.iopl0") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.IOPL0, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.iopl1") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.IOPL1, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.nt") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.NT, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.rf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.RF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.vm") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.VM, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.ac") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.AC, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.vif") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.VIF, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.vip") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.VIP, TRUE);
		if(szSymbol.CompareNoCase(L"eflags.id") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.ID, TRUE);

		//Debug registers
		if(szSymbol.CompareNoCase(L"dr0") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr0, TRUE);
		if(szSymbol.CompareNoCase(L"dr1") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr1, TRUE);
		if(szSymbol.CompareNoCase(L"dr2") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr2, TRUE);
		if(szSymbol.CompareNoCase(L"dr3") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr3, TRUE);
		if(szSymbol.CompareNoCase(L"dr6") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr6, TRUE);
		if(szSymbol.CompareNoCase(L"dr7") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr7, TRUE);

		//Handle to cheat engine
		UINT_PTR Address;

		char szBuffer[0x100];
		WideCharToMultiByte(CP_ACP, 0, szSymbol, -1, szBuffer, sizeof(szBuffer), NULL, NULL );

		if(CheatEngine.sym_nameToAddress(szBuffer, &Address))
			return ParseInfo(ptr, TYPE_UNKNOWN, Address, TRUE);

		return ParseInfo(0, TYPE_QWORD, 0, FALSE);
	}

	ParseInfo Bracket(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		int i = ParseSymbol(ptr);
		CString szSymbol(ptr, i);
		
		ParseType nType = TYPE_UNKNOWN;

		if(szSymbol.CompareNoCase(L"qword") == 0) {
			nType = TYPE_QWORD;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase(L"dword") == 0) {
			nType = TYPE_DWORD;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase(L"word") == 0) {
			nType = TYPE_WORD;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase(L"byte") == 0) {
			nType = TYPE_BYTE;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase(L"bool") == 0) {
			nType = TYPE_BOOL;
			ptr += i;
		}

		if(*ptr == L'[') {
			ParseInfo info = Expression(ptr + 1);
			if(info.ptr && *info.ptr == L']') {
				info.ptr++;
				//Bool type pointer not exists
				if(nType == TYPE_BOOL)
					return ParseInfo();

				return info.ReadPtr(*CheatEngine.OpenedProcessHandle, nType);
			}
			return ParseInfo();
		}
		if(*ptr == L'(') {
			ParseInfo info = Expression(ptr + 1);
			if(info.ptr && *info.ptr == L')') {
				info.ptr++;
				if(nType != TYPE_UNKNOWN)
					return ParseInfo(info.ptr, nType, info.value, info.bValidPtr);

				return info;
			}
			return ParseInfo();
		}
		
		return Token(ptr);
	}

	ParseInfo Unary(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo result;

		switch(*ptr) {
		case L'*': result = *Unary(ptr + 1); break;
		case L'+': result = +Unary(ptr + 1); break;
		case L'-': result = -Unary(ptr + 1); break;
		case L'~': result = ~Unary(ptr + 1); break;
		case L'!': result = !Unary(ptr + 1); break;
		}

		if(result.ptr)
			return result;

		return Bracket(ptr);
	}

	ParseInfo Term(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Unary(ptr), result;
		if(!info.ptr) return ParseInfo();

		switch(*info.ptr) {
			case L'*': result = info * Term(info.ptr + 1); break;
			case L'/': result = info / Term(info.ptr + 1); break;
			case L'%': result = info % Term(info.ptr + 1); break;
		}
		
		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Add(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Term(ptr), result;
		if(!info.ptr) return ParseInfo();

		switch(*info.ptr) {
			case L'+': result = info + Add(info.ptr + 1); break;
			case L'-': result = info - Add(info.ptr + 1); break;
		}

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Shift(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Add(ptr), result;
		if(!info.ptr) return ParseInfo();

		CString szStr(info.ptr);

		if(szStr.Left(2) == L"<<")		result = info << Shift(info.ptr + 2);
		else if(szStr.Left(2) == L">>")	result = info >> Shift(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Compare(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Shift(ptr), result;
		if(!info.ptr) return ParseInfo();

		CString szStr(info.ptr);

		if(szStr.Left(2) == L"<=")		result = info <= Compare(info.ptr + 2);
		else if(szStr.Left(2) == L">=")	result = info >= Compare(info.ptr + 2);
		else if(szStr.Left(1) == L"<")	result = info < Compare(info.ptr + 1);
		else if(szStr.Left(1) == L">")	result = info > Compare(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Equal(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Compare(ptr), result;
		if(!info.ptr) return ParseInfo();

		CString szStr(info.ptr);

		if(szStr.Left(2) == L"==")		result = info == Equal(info.ptr + 2);
		else if(szStr.Left(2) == L"!=")	result = info != Equal(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo BitAnd(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Equal(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(*info.ptr == L'&')
			result = info & BitAnd(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo BitXor(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = BitAnd(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(*info.ptr == L'^')
			result = info ^ BitXor(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo BitOr(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = BitXor(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(*info.ptr == L'|')
			result =  info | BitOr(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo LogicAnd(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = BitOr(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(CString(info.ptr).Left(2) == L"&&")
			result = info && LogicAnd(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo LogicOr(const WCHAR* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = LogicAnd(ptr), result;
			if(!info.ptr) return ParseInfo();

		if(CString(info.ptr).Left(2) == L"||")
			result = info || LogicOr(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Expression(const WCHAR* ptr) {
		return LogicOr(ptr);
	}

	BOOL Parse(const WCHAR* szString, ParseInfo& info) {
		WCHAR* szCopy = new WCHAR[wcslen(szString) + 1];
		wcscpy(szCopy, szString);
		RemoveSpace(szCopy);
		info = Expression(szCopy);

		BOOL bResult = (info.ptr && *info.ptr == 0);

		delete[] szCopy;
		info.ptr = 0;

		return bResult;
	}
};

