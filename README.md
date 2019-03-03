WatchExpression CE Plugin

Youtube tutorial
https://youtu.be/uLP4Ckhylwg

This plugin hooks CE's debug event. it captures registers which are needed by your custom
expression and log them to list control. My original focus is hooking function's return
address but I extended my think so the plugin can hook any expression. if you're debugging
x86 process and want to hook function's return address, use this expression "[ebp+4]"

Beginners guide

Execute CE.
Go to "Settings" -> "Plugins"
Click "Add new" and add WatchExpression plugin.
Enable plugin by clicking check box.
Open process to debug.
Go to "Memory View" and go to address to hook
Click mouse right buttona and select "Watch expression"(or press shortcut "CTRL+W")
Customise your expression and choose data type.
Press start button.
Enjoy debugging!
Features

Works with UCE(tested at some UCEs)
All debugger methods are supported(VEH, Windows, Kernelmode)
All breakpoint mothods are supported(Hardware BP, Software BP, Page exceptions BP)
Multiple watcher windows at single breakpoint
Using CE symbol is allowed.(Userdefined symbol, DLL name, DLL exported functions, just address and etc..)
Dynamic data length using expression(if data type is string or AOB)
Limitations

The plugin works on only CE 6.5+(Debugevent callback is not implemented under 6.5 version.)
The plugin is for only x64 "system".(Using on x86 processes is possible.)
You can use only "hexadecimal number".
Registers supported on expression
rax, eax, ax, ah, al
rbx, ebx, bx, bh, bl
rcx, ecx, cx, ch, cl
rdx, edx, dx, dh, dl
rsi, esi, si, sil
rdi, edi, di, dil
rbp, ebp, bp, bpl
rsp, esp, sp, spl
r8, r8d, r8w, r8b
r9, r9d, r9w, r9b
..
..
..
r15, r15d, r15w, r15b
rip, eip
cs, ss, ds, es, fs, gs
eflags
eflags.cf, eflags.pf, eflags.af, eflags.zf, eflags.sf, eflags.tf, eflags.if, eflags.df eflags.of
eflags.iopl0, eflags.iopl1, eflags.nt, eflags.rf, eflags.vm, eflags.ac, eflags.vif, eflags.vip, eflags.id
dr0, dr1, dr2, dr3, dr6, dr7

Operators supported on expression(Almost of them are based on C language)
High priority
qword[Exp], dword[Exp], word[Exp], byte[Exp], [Exp]	: Pointer operators
qword(Exp), dword(Exp), word(Exp), byte(Exp), bool(Exp), (Exp)	: Casting operators
*, +, -, ~, !	: Unary operators
*, /, %	: Multiplicative operators
+, -	: Additive operators
<<, >>	: Shift operators
<, <=, >, >=	: Compare operators
==, !=	: Equality operators
& : Bit and
^ : Bit xor
| : Bit or
&& : Logic and
|| : Logic or
Low priority

Select data type you want to hook

Integer
Opcode	*Use this when hooking function return addresses or vtable values.
*If you double click a list entry, memoryview will be showed up at proper address.
Float, Double
String
Array of bytes
