#pragma once

#include "stdafx.h"

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

	const char* ptr;
	ParseType type;
	__int64 value;
	BOOL bValidPtr;

	ParseInfo() : ptr(0), type(TYPE_QWORD), value(0), bValidPtr(FALSE) {}

	ParseInfo(const char* ptr, ParseType type, QWORD value, BOOL bValidPtr) :
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
	static void RemoveSpace(char *s) {
		char* i = s;
		char* j = s;
		do { 
			while(isspace(*i)) {
				i++;
			} 
		} while(*j++ = *i++);
	}

	static int ParseSymbol(const char* ptr) {
		if(!ptr)
			return 0;
		int i = 0;
		for(i = 0 ; isalnum(ptr[i]) || ptr[i] == '_' || ptr[i] == '.' ; i++);
		return i;
	}

	Parser(const CONTEXT& ctx) : ctx(ctx) {}

	ParseInfo Token(const char* ptr) {
		int i = ParseSymbol(ptr);
		CStringA szSymbol(ptr, i);
		ptr += i;

		//Rax
		if(szSymbol.CompareNoCase("rax") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rax, TRUE);
		if(szSymbol.CompareNoCase("eax") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rax, TRUE);
		if(szSymbol.CompareNoCase("ax") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rax, TRUE);
		if(szSymbol.CompareNoCase("ah") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rax), TRUE);
		if(szSymbol.CompareNoCase("al") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rax, TRUE);

		//Rbx
		if(szSymbol.CompareNoCase("rbx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rbx, TRUE);
		if(szSymbol.CompareNoCase("ebx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rbx, TRUE);
		if(szSymbol.CompareNoCase("bx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rbx, TRUE);
		if(szSymbol.CompareNoCase("bh") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rbx), TRUE);
		if(szSymbol.CompareNoCase("bl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rbx, TRUE);

		//Rcx
		if(szSymbol.CompareNoCase("rcx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rcx, TRUE);
		if(szSymbol.CompareNoCase("ecx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rcx, TRUE);
		if(szSymbol.CompareNoCase("cx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rcx, TRUE);
		if(szSymbol.CompareNoCase("ch") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rcx), TRUE);
		if(szSymbol.CompareNoCase("cl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rcx, TRUE);

		//Rdx
		if(szSymbol.CompareNoCase("rdx") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rdx, TRUE);
		if(szSymbol.CompareNoCase("edx") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rdx, TRUE);
		if(szSymbol.CompareNoCase("dx") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rdx, TRUE);
		if(szSymbol.CompareNoCase("dh") == 0)
			return ParseInfo(ptr, TYPE_BYTE, HIBYTE(ctx.Rdx), TRUE);
		if(szSymbol.CompareNoCase("dl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rdx, TRUE);
	
		//Rsi
		if(szSymbol.CompareNoCase("rsi") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rsi, TRUE);
		if(szSymbol.CompareNoCase("esi") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rsi, TRUE);
		if(szSymbol.CompareNoCase("si") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rsi, TRUE);
		if(szSymbol.CompareNoCase("sil") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rsi, TRUE);

		//Rdi
		if(szSymbol.CompareNoCase("rdi") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rdi, TRUE);
		if(szSymbol.CompareNoCase("edi") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rdi, TRUE);
		if(szSymbol.CompareNoCase("di") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rdi, TRUE);
		if(szSymbol.CompareNoCase("dil") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rdi, TRUE);

		//Rbp
		if(szSymbol.CompareNoCase("rbp") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rbp, TRUE);
		if(szSymbol.CompareNoCase("ebp") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rbp, TRUE);
		if(szSymbol.CompareNoCase("bp") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rbp, TRUE);
		if(szSymbol.CompareNoCase("bpl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rbp, TRUE);

		//Rsp
		if(szSymbol.CompareNoCase("rsp") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rsp, TRUE);
		if(szSymbol.CompareNoCase("esp") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rsp, TRUE);
		if(szSymbol.CompareNoCase("sp") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.Rsp, TRUE);
		if(szSymbol.CompareNoCase("spl") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.Rsp, TRUE);

		//R8
		if(szSymbol.CompareNoCase("r8") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R8, TRUE);
		if(szSymbol.CompareNoCase("r8d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R8, TRUE);
		if(szSymbol.CompareNoCase("r8w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R8, TRUE);
		if(szSymbol.CompareNoCase("r8b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R8, TRUE);

		//R9
		if(szSymbol.CompareNoCase("r9") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R9, TRUE);
		if(szSymbol.CompareNoCase("r9d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R9, TRUE);
		if(szSymbol.CompareNoCase("r9w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R9, TRUE);
		if(szSymbol.CompareNoCase("r9b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R9, TRUE);

		//R10
		if(szSymbol.CompareNoCase("r10") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R10, TRUE);
		if(szSymbol.CompareNoCase("r10d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R10, TRUE);
		if(szSymbol.CompareNoCase("r10w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R10, TRUE);
		if(szSymbol.CompareNoCase("r10b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R10, TRUE);

		//R11
		if(szSymbol.CompareNoCase("r11") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R11, TRUE);
		if(szSymbol.CompareNoCase("r11d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R11, TRUE);
		if(szSymbol.CompareNoCase("r11w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R11, TRUE);
		if(szSymbol.CompareNoCase("r11b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R11, TRUE);

		//R12
		if(szSymbol.CompareNoCase("r12") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R12, TRUE);
		if(szSymbol.CompareNoCase("r12d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R12, TRUE);
		if(szSymbol.CompareNoCase("r12w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R12, TRUE);
		if(szSymbol.CompareNoCase("r12b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R12, TRUE);
	
		//R13
		if(szSymbol.CompareNoCase("r13") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R13, TRUE);
		if(szSymbol.CompareNoCase("r13d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R13, TRUE);
		if(szSymbol.CompareNoCase("r13w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R13, TRUE);
		if(szSymbol.CompareNoCase("r13b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R13, TRUE);

		//R14
		if(szSymbol.CompareNoCase("r14") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R14, TRUE);
		if(szSymbol.CompareNoCase("r14d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R14, TRUE);
		if(szSymbol.CompareNoCase("r14w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R14, TRUE);
		if(szSymbol.CompareNoCase("r14b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R14, TRUE);

		//R15
		if(szSymbol.CompareNoCase("r15") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.R15, TRUE);
		if(szSymbol.CompareNoCase("r15d") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.R15, TRUE);
		if(szSymbol.CompareNoCase("r15w") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.R15, TRUE);
		if(szSymbol.CompareNoCase("r15b") == 0)
			return ParseInfo(ptr, TYPE_BYTE, ctx.R15, TRUE);

		//Rip
		if(szSymbol.CompareNoCase("rip") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Rip, TRUE);
		if(szSymbol.CompareNoCase("eip") == 0)
			return ParseInfo(ptr, TYPE_DWORD, ctx.Rip, TRUE);

		//Segment registers
		if(szSymbol.CompareNoCase("cs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegCs, TRUE);
		if(szSymbol.CompareNoCase("ss") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegSs, TRUE);
		if(szSymbol.CompareNoCase("ds") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegDs, TRUE);
		if(szSymbol.CompareNoCase("es") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegEs, TRUE);
		if(szSymbol.CompareNoCase("fs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegFs, TRUE);
		if(szSymbol.CompareNoCase("gs") == 0)
			return ParseInfo(ptr, TYPE_WORD, ctx.SegGs, TRUE);

		EFlags eflags;
		eflags.dwValue = ctx.EFlags;

		//EFlags
		if(szSymbol.CompareNoCase("eflags") == 0)
			return ParseInfo(ptr, TYPE_DWORD, eflags.dwValue, TRUE);
		if(szSymbol.CompareNoCase("eflags.cf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.CF, TRUE);
		if(szSymbol.CompareNoCase("eflags.pf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.PF, TRUE);
		if(szSymbol.CompareNoCase("eflags.af") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.AF, TRUE);
		if(szSymbol.CompareNoCase("eflags.zf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.ZF, TRUE);
		if(szSymbol.CompareNoCase("eflags.sf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.SF, TRUE);
		if(szSymbol.CompareNoCase("eflags.tf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.TF, TRUE);
		if(szSymbol.CompareNoCase("eflags.if") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.IF, TRUE);
		if(szSymbol.CompareNoCase("eflags.df") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.DF, TRUE);
		if(szSymbol.CompareNoCase("eflags.of") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.OF, TRUE);
		if(szSymbol.CompareNoCase("eflags.iopl0") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.IOPL0, TRUE);
		if(szSymbol.CompareNoCase("eflags.iopl1") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.IOPL1, TRUE);
		if(szSymbol.CompareNoCase("eflags.nt") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.NT, TRUE);
		if(szSymbol.CompareNoCase("eflags.rf") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.RF, TRUE);
		if(szSymbol.CompareNoCase("eflags.vm") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.VM, TRUE);
		if(szSymbol.CompareNoCase("eflags.ac") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.AC, TRUE);
		if(szSymbol.CompareNoCase("eflags.vif") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.VIF, TRUE);
		if(szSymbol.CompareNoCase("eflags.vip") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.VIP, TRUE);
		if(szSymbol.CompareNoCase("eflags.id") == 0)
			return ParseInfo(ptr, TYPE_BOOL, eflags.ID, TRUE);

		//Debug registers
		if(szSymbol.CompareNoCase("dr0") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr0, TRUE);
		if(szSymbol.CompareNoCase("dr1") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr1, TRUE);
		if(szSymbol.CompareNoCase("dr2") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr2, TRUE);
		if(szSymbol.CompareNoCase("dr3") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr3, TRUE);
		if(szSymbol.CompareNoCase("dr6") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr6, TRUE);
		if(szSymbol.CompareNoCase("dr7") == 0)
			return ParseInfo(ptr, TYPE_QWORD, ctx.Dr7, TRUE);

		//Handle to cheat engine
		UINT_PTR Address;
		if(CheatEngine.sym_nameToAddress(szSymbol, &Address))
			return ParseInfo(ptr, TYPE_UNKNOWN, Address, TRUE);

		return ParseInfo(0, TYPE_QWORD, 0, FALSE);
	}

	ParseInfo Bracket(const char* ptr) {
		if(!ptr) return ParseInfo();

		int i = ParseSymbol(ptr);
		CStringA szSymbol(ptr, i);
		
		ParseType nType = TYPE_UNKNOWN;

		if(szSymbol.CompareNoCase("qword") == 0) {
			nType = TYPE_QWORD;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase("dword") == 0) {
			nType = TYPE_DWORD;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase("word") == 0) {
			nType = TYPE_WORD;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase("byte") == 0) {
			nType = TYPE_BYTE;
			ptr += i;
		}
		else if(szSymbol.CompareNoCase("bool") == 0) {
			nType = TYPE_BOOL;
			ptr += i;
		}

		if(*ptr == '[') {
			ParseInfo info = Expression(ptr + 1);
			if(info.ptr && *info.ptr == ']') {
				info.ptr++;
				//Bool type pointer not exists
				if(nType == TYPE_BOOL)
					return ParseInfo();

				return info.ReadPtr(*CheatEngine.OpenedProcessHandle, nType);
			}
			return ParseInfo();
		}
		if(*ptr == '(') {
			ParseInfo info = Expression(ptr + 1);
			if(info.ptr && *info.ptr == ')') {
				info.ptr++;
				if(nType != TYPE_UNKNOWN)
					return ParseInfo(info.ptr, nType, info.value, info.bValidPtr);

				return info;
			}
			return ParseInfo();
		}
		
		return Token(ptr);
	}

	ParseInfo Unary(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo result;

		switch(*ptr) {
		case '*': result = *Unary(ptr + 1); break;
		case '+': result = +Unary(ptr + 1); break;
		case '-': result = -Unary(ptr + 1); break;
		case '~': result = ~Unary(ptr + 1); break;
		case '!': result = !Unary(ptr + 1); break;
		}

		if(result.ptr)
			return result;

		return Bracket(ptr);
	}

	ParseInfo Term(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Unary(ptr), result;
		if(!info.ptr) return ParseInfo();

		switch(*info.ptr) {
			case '*': result = info * Term(info.ptr + 1); break;
			case '/': result = info / Term(info.ptr + 1); break;
			case '%': result = info % Term(info.ptr + 1); break;
		}
		
		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Add(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Term(ptr), result;
		if(!info.ptr) return ParseInfo();

		switch(*info.ptr) {
			case '+': result = info + Add(info.ptr + 1); break;
			case '-': result = info - Add(info.ptr + 1); break;
		}

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Shift(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Add(ptr), result;
		if(!info.ptr) return ParseInfo();

		CStringA szStr(info.ptr);

		if(szStr.Left(2) == "<<")		result = info << Shift(info.ptr + 2);
		else if(szStr.Left(2) == ">>")	result = info >> Shift(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Compare(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Shift(ptr), result;
		if(!info.ptr) return ParseInfo();

		CStringA szStr(info.ptr);

		if(szStr.Left(2) == "<=")		result = info <= Compare(info.ptr + 2);
		else if(szStr.Left(2) == ">=")	result = info >= Compare(info.ptr + 2);
		else if(szStr.Left(1) == "<")	result = info < Compare(info.ptr + 1);
		else if(szStr.Left(1) == ">")	result = info > Compare(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Equal(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Compare(ptr), result;
		if(!info.ptr) return ParseInfo();

		CStringA szStr(info.ptr);

		if(szStr.Left(2) == "==")		result = info == Equal(info.ptr + 2);
		else if(szStr.Left(2) == "!=")	result = info != Equal(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo BitAnd(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = Equal(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(*info.ptr == '&')
			result = info & BitAnd(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo BitXor(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = BitAnd(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(*info.ptr == '^')
			result = info ^ BitXor(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo BitOr(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = BitXor(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(*info.ptr == '|')
			result =  info | BitOr(info.ptr + 1);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo LogicAnd(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = BitOr(ptr), result;
		if(!info.ptr) return ParseInfo();

		if(CStringA(info.ptr).Left(2) == "&&")
			result = info && LogicAnd(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo LogicOr(const char* ptr) {
		if(!ptr) return ParseInfo();

		ParseInfo info = LogicAnd(ptr), result;
			if(!info.ptr) return ParseInfo();

		if(CStringA(info.ptr).Left(2) == "||")
			result = info || LogicOr(info.ptr + 2);

		if(result.ptr)
			return result;

		return info;
	}

	ParseInfo Expression(const char* ptr) {
		return LogicOr(ptr);
	}

	BOOL Parse(const char* szString, ParseInfo& info) {
		char* szCopy = new char[strlen(szString) + 1];
		strcpy(szCopy, szString);
		RemoveSpace(szCopy);
		info = Expression(szCopy);

		BOOL bResult = (info.ptr && *info.ptr == 0);

		delete[] szCopy;
		info.ptr = 0;

		return bResult;
	}
};

