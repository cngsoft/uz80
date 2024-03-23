// UU  UU ZZZZZZ  8888   0000  // --------------------------------- //
// UU  UU     ZZ 88  88 00  00 // UZ80, a small Z80 macro assembler //
// UU  UU    ZZ  88  88 00  00 // written by Cesar Nicolas-Gonzalez //
// UU  UU   ZZ    8888  00  00 // since 2018-02-23 17:05 till today //
// UU  UU  ZZ    88  88 00  00 // --------------------------------- //
// UU  UU ZZ     88  88 00  00 // http://cngsoft.no-ip.org/uz80.htm //
//  UUUU  ZZZZZZ  8888   0000  // --------------------------------- //

#define DATETIME "20240224"

#define GPL_3_INFO \
	"This program comes with ABSOLUTELY NO WARRANTY; for more details" "\n" \
	"please see the GNU General Public License. This is free software" "\n" \
	"and you are welcome to redistribute it under certain conditions."

/* This notice applies to the source code of UZ80 and its binaries.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Contact information: <mailto:cngsoft@gmail.com> */

#include <stdio.h> // printf,fopen...
#include <stdlib.h> // malloc,realloc...
#include <string.h> // strchr,strcasecmp...
#ifdef _WIN32
	#define PATHCHAR '\\'
	#define strcasecmp _stricmp
#else
	#define PATHCHAR '/'
#endif
#define INLINE // `inline`: ignored by TCC and GCC4, harmful in GCC5!

#define INPUT_MAXIMUM 9
FILE *input[INPUT_MAXIMUM]; int inputs,input_source[INPUT_MAXIMUM],macro0[INPUT_MAXIMUM],local0[INPUT_MAXIMUM];

int LABEL_MAXIMUM=1<<10;
int *label,labels,*value;
int CACHE_MAXIMUM=1<<10;
int *cache,caches,*macro,macros,*macro_source;
int DOUBT_MAXIMUM=1<<12;
int *doubt,doubts,*doubt_target,*doubt_source;
int CHAIN_MAXIMUM=1<<12;
int *chain,chains;
#define CREATE_ARRAY(x,y) (x=(int*)malloc(sizeof(int[y])))
#define RESIZE_ARRAY(x,y) (x=(int*)realloc(x,sizeof(int[y])))

#define SIZEOF_OUTPUT (1<<16) // to catch accidental overflows
int ascizs,ASCIZ_MAXIMUM=1<<16; char *asciz,param0[INPUT_MAXIMUM<<9];
unsigned char output[SIZEOF_OUTPUT+512],source[1<<9],input0[1<<9],folder[1<<9],incpath[1<<9],newpath[1<<9];
int param[INPUT_MAXIMUM][10],params,locals;
int target,origin,remote,dollar,flag_dollar=0,flag_z=-1;

INLINE int isnumber(int i) { return (i>='0'&&i<='9'); }
INLINE int isletter(int i) { return (i>='A'&&i<='Z')||(i>='a'&&i<='z')||i=='_'; }
INLINE int iseither(int i) { return (i>='0'&&i<='9')||(i>='A'&&i<='Z')||(i>='a'&&i<='z')||i=='_'||i=='.'; }
char symbol_dollar[]="$",symbol_debug[]="DEBUG";

#define FATAL_ERROR(s) return printerror(s),-1
char error_byte_overflow[]="byte out of range";
char error_cannot_create_file[]="cannot create file";
char error_cannot_open_file[]="cannot open file";
char error_cannot_write_data[]="cannot write data";
char error_char_overflow[]="offset out of range";
char error_elif_without_if[]="ELIF without IF";
char error_else_without_if[]="ELSE without IF";
char error_endif_without_if[]="ENDIF without IF";
char error_endm_without_macro[]="ENDM without MACRO";
char error_forbidden_as80[]="forbidden in AS80 mode";
char error_if_without_endif[]="IF without ENDIF";
char error_improper_argument[]="improper argument";
char error_invalid_expression[]="invalid expression";
char error_invalid_string[]="invalid string";
char error_invalid_symbol[]="invalid symbol";
char error_macro_without_endm[]="MACRO without ENDM";
char error_out_of_memory[]="out of memory";
char error_stack_overflow[]="stack overflow";
char error_symbol_already_exists[]="symbol already exists";
char error_syntax_error[]="syntax error";
char error_too_many_arguments[]="too many arguments";
char error_undefined_opcode[]="undefined opcode";
char error_undefined_symbol[]="undefined symbol";
char error_word_overflow[]="word out of range";
char string_greeting[]="UZ80 " DATETIME " (C) Cesar Nicolas-Gonzalez\n\nusage: uz80 [-qvz] [-D[label[=value]]] [-Ipath] source [-o]target\n" "\n" GPL_3_INFO "\n";
void printerror(char *s)
{
	int i;
	for (i=0;i<inputs;++i)
		fprintf(stderr,"%i:",input_source[i]);
	if (i) // any error but EOF
		fprintf(stderr," %s\n",source);
	fprintf(stderr,"error: %s!\n",s);
}

// these functions return the entry index (success) or <0 (failure)
int get_asciz(char *s,int y[],int x)
{
	int i,w=0,z;
	--x;
	while (w<=x)
	{
		if (!(z=strcasecmp(s,&asciz[y[i=(w+x)/2]])))
			return i;
		if (z>0)
			w=i+1;
		else
			x=i-1;
	}
	return ~w;
}
int put_asciz(char *s,int y[],int x,int xx)
{
	if (x>=xx)
		FATAL_ERROR(error_out_of_memory);
	char *t=&asciz[y[x]=ascizs];
	while (*t++=*s++)
		{}
	if ((ascizs=t-asciz)>=(ASCIZ_MAXIMUM-sizeof(input0)))
		asciz=realloc(asciz,ASCIZ_MAXIMUM*=2);
	return x;
}
#define get_label(s) get_asciz(s,label,labels) // search label names
int add_label(char *s,int n) // create label
{
	int i,j;
	if ((i=~get_label(s))<0)
		FATAL_ERROR(error_symbol_already_exists);
	if (labels>=LABEL_MAXIMUM)
		if (!RESIZE_ARRAY(label,LABEL_MAXIMUM*=2)||!RESIZE_ARRAY(value,LABEL_MAXIMUM))
			FATAL_ERROR(error_out_of_memory);
	j=sizeof(int[labels-i]);
	memmove(&value[i+1],&value[i],j);
	memmove(&label[i+1],&label[i],j);
	if (put_asciz(s,label,i,LABEL_MAXIMUM)<0)
		return -1;
	value[i]=n;
	return ++labels,i;
}
// macros are stored line by line, but only their title tells them apart
#define add_cache(s) put_asciz(s,cache,caches++,CACHE_MAXIMUM)
#define get_macro(s) get_asciz(s,macro,macros)
int add_macro(char *s) // create new macro
{
	int i,j;
	if ((i=~get_macro(s))<0)
		FATAL_ERROR(error_symbol_already_exists);
	if (caches>=CACHE_MAXIMUM)
		if (!RESIZE_ARRAY(cache,CACHE_MAXIMUM*=2)||!RESIZE_ARRAY(macro,CACHE_MAXIMUM)||!RESIZE_ARRAY(macro_source,CACHE_MAXIMUM))
			FATAL_ERROR(error_out_of_memory);
	j=sizeof(int[macros-i]);
	memmove(&macro[i+1],&macro[i],j);
	memmove(&macro_source[i+1],&macro_source[i],j);
	if ((j=add_cache(s))<0)
		return -1;
	macro[i]=cache[j];
	macro_source[i]=j;
	return ++macros,i;
}
#define put_doubt(s) put_asciz(s,doubt,doubts++,DOUBT_MAXIMUM)
int add_doubt(char *s)
{
	int i;
	if ((i=put_doubt(s))<0)
		return -1;
	if (doubts>=DOUBT_MAXIMUM)
		if (!RESIZE_ARRAY(doubt,DOUBT_MAXIMUM*=2)||!RESIZE_ARRAY(doubt_target,DOUBT_MAXIMUM)||!RESIZE_ARRAY(doubt_source,DOUBT_MAXIMUM))
			FATAL_ERROR(error_out_of_memory);
	doubt_target[i]=dollar;
	if (inputs>1)
	{
		doubt_source[i]=~chains;
		int n;
		for (n=0;n<inputs-1;++n)
			chain[chains++]=input_source[n];
		chain[chains++]=~input_source[n]; // final item
		if (chains>=CHAIN_MAXIMUM-INPUT_MAXIMUM)
			if (!RESIZE_ARRAY(chain,CHAIN_MAXIMUM*=2))
				FATAL_ERROR(error_out_of_memory);
	}
	else
		doubt_source[i]=input_source[0];
	return i;
}

int split_param(char *s) // 0 OK, !0 ERROR
{
	if (params>=INPUT_MAXIMUM)
		FATAL_ERROR(error_stack_overflow);
	char c,*t=&param0[sizeof(param0)*params/INPUT_MAXIMUM],i=0,q=0;
	if (*s)
	{
		param[params][++i]=t-param0;
		while (*t=(c=*s++))
		{
			if (c=='"')
				q=!q;
			if (c==','&&!q)
				*t++=0,param[params][++i]=t-param0;
			else
				++t;
		}
		if (q)
			FATAL_ERROR(error_invalid_string);
		if (i>=10||(t-param0>=sizeof(param0)))
			FATAL_ERROR(error_too_many_arguments);
	}
	param[params++][0]=i;
	return *t=0;
}
#define merge_param() --params

// these functions return 0 on failure, as fopen(), fgets() and fclose() do
#define INIT_INPUT if (inputs>=INPUT_MAXIMUM) return printerror(error_stack_overflow),0
#define EXIT_INPUT(f) input[inputs]=f; input_source[inputs++]=0; return 1
int open_input(char *s)
{
	INIT_INPUT;
	FILE *f;
	if (!(f=(strcmp(s,"-")?fopen(s,"r"):stdin)))
		return 0;
	EXIT_INPUT(f);
}
int open_macro(int i)
{
	INIT_INPUT;
	local0[inputs]=locals++;
	macro0[inputs]=macro_source[i];
	EXIT_INPUT(0);
}
int read_input(void)
{
	if (!inputs)
		return 0;
	FILE *f;
	int c; unsigned char *s,*t=input0;
	if (f=input[inputs-1])
	{
		if (!fgets(s=source,sizeof(source),f))
			return 0;
		if (239==*s&&187==s[1]&&191==s[2]) s+=3; // skip unsigned UTF8 BOM
		while ((c=*s++)>32&&c!=';'&&c!=':'&&c!='=')
			*t++=c; // label
		if (c&&c!=';')
		{
			*t++=32;
			if (c!='=')
			{
				while ((c=*s)&&c<=32)
					++s; // blanks
				while (((c=*s++)>32)&&c!=';'&&c!='=')
					*t++=c; // opcode
			}
			if (c&&c!=';')
			{
				if (c=='=')
					*t++=c;
				*t++=32;
				int q=0;
				while (c=*s++)
				{
					if (c==';'&&!q) // comment
						break;
					if (c>32||q) // parameters
						*t++=c;
					if (c=='"') // string
						q=!q;
				}
			}
		}
		do
			*t=0; // clean spaces up
		while (t>input0&&((c=*--t)&&c<=32));
		++input_source[inputs-1];
	}
	else
	{
		if (!strcasecmp(symbol_dollar,s=&asciz[cache[++input_source[inputs-1]+macro0[inputs-1]]]))
			return 0;
		while (c=*s++)
		{
			if (c=='\\') // macro parameter?
			{
				if ((c=*s++)=='?')
					t+=sprintf(t,"%04X",local0[inputs-1]);
				else if (isnumber(c))
				{
					if (c-='0')
					{
						if (c<=param[params-1][0])
						{
							char *r=&param0[param[params-1][c]];
							while (*t++=*r++)
								{}
							--t;
						}
					}
					else
						*t++='0'+param[params-1][0];
				}
				else
					*t++='\\',*t++=c;
			}
			else
				*t++=c;
		}
		*t=0;
	}
	strcpy(source,input0);
	return 1;
}
int close_input(void)
{
	if (!inputs)
		return 0;
	FILE *f;
	if (f=input[--inputs])
	{
		if (f!=stdin)
			fclose(f);
	}
	else
		merge_param();
	input_source[inputs]=0;
	return 1;
}

#define split_symbol input0
unsigned char *split_opcode,*split_parmtr;
void split_input(void)
{
	unsigned char *s=input0;
	while (*s>32)
		++s;
	if (!*s)
		s[1]=0;
	*s++=0;
	split_opcode=s;
	while (*s>32)
		++s;
	if (!*s)
		s[1]=0;
	*s++=0;
	split_parmtr=s;
}
void merge_input(void)
{
	if (*split_parmtr)
		split_parmtr[-1]=32;
	if (*split_opcode)
		split_opcode[-1]=32;
}

// shunting-yard algorithm for expression evaluation
int eval_status; // 0 OK, <0 ERROR, >0 DOUBT
int eval_hex2i(char *s,char *t)
{
	if (s>=t)
		return eval_status=-1;
	int i=0; char c;
	while (s<t)
	{
		if (i<0)
			return eval_status=-1; // long!
		if ((c=*s)>='0'&&c<='9')
			i=i*16+c-'0';
		else if ((c>='A'&&c<='F')||(c>='a'&&c<='f'))
			i=i*16+9+(c&7);
		else
			return eval_status=-1; // bad!
		++s;
	}
	return i;
}
int eval_dec2i(char *s,char *t,int z)
{
	if (s>=t)
		return eval_status=-1;
	int i=0; char c,k='0'+z;
	while (s<t)
	{
		if (i<0)
			return eval_status=-1; // long!
		if ((c=*s)>='0'&&c<k)
			i=i*z+c-'0';
		else
			return eval_status=-1; // bad!
		++s;
	}
	return i;
}
enum opertr_ {
	EVAL_NULL	, // 0!
	EVAL_OP1_PLS	, // "+"
	EVAL_OP1_MNS	, // "-"
	EVAL_OP1_NOT	, // "!"
	EVAL_OP1_CPL	, // "~"
	EVAL_UNARIES	, // CATEGORY
	EVAL_OP2_MUL	, // "*"
	EVAL_OP2_DIV	, // "/"
	EVAL_OP2_MOD	, // "%"
	EVAL_OP2_SHL	, // "<<"
	EVAL_OP2_SHR	, // ">>"
	EVAL_BINARY_H	, // CATEGORY
	EVAL_OP2_AND	, // "&"
	EVAL_OP2_XOR	, // "^"
	EVAL_BINARY_M	, // CATEGORY
	EVAL_OP2_ADD	, // "+"
	EVAL_OP2_SUB	, // "-"
	EVAL_OP2_ORR	, // "|"
	EVAL_BINARY_L	, // CATEGORY
	EVAL_OP2_L_E	, // "<="
	EVAL_OP2_LSS	, // "<"
	EVAL_OP2_G_E	, // ">="
	EVAL_OP2_GRT	, // ">"
	EVAL_OP2_EQU	, // "=" / "=="
	EVAL_OP2_NEQ	, // "<>" / "!="
	EVAL_BINARIES	, // CATEGORY
	EVAL_P_INIT	, // "("
	EVAL_P_EXIT	, // ")"
};
#define EVAL_MAXIMUM (3<<4)
int eval_int[EVAL_MAXIMUM],eval_ints,eval_ops;
char eval_op[EVAL_MAXIMUM],eval_pr[EVAL_MAXIMUM];
char eval_priorities[]={5,5,5,5,5,4,4,4,4,4,4,3,3,3,2,2,2,2,1,1,1,1,1,1,1,0,0,0};
//INLINE int eval_priority(int i) { return (i<EVAL_UNARIES)+(i<EVAL_BINARY_H)+(i<EVAL_BINARY_M)+(i<EVAL_BINARY_L)+(i<EVAL_BINARIES); }
void eval_push_int(int i)
{
	if (eval_ints>=EVAL_MAXIMUM)
		eval_status=-1; // FULL!
	else
		eval_int[eval_ints++]=i;
}
int eval_pop_int(void)
{
	return eval_ints?eval_int[--eval_ints]:(eval_status=-1); // EMPTY!
}
void eval_push_op(int i)
{
	if (eval_ops>=EVAL_MAXIMUM)
		eval_status=-1; // FULL!
	else
		eval_pr[eval_ops]=eval_priorities[eval_op[eval_ops]=i],++eval_ops;
}
int eval_pop_op(void)
{
	if (!eval_ops)
		return eval_status=-1; // EMPTY!
	return eval_op[--eval_ops];
}
int eval_op1(char o,int x)
{
	switch (o)
	{
		case EVAL_OP1_PLS:
			return +x;
		case EVAL_OP1_MNS:
			return -x;
		case EVAL_OP1_NOT:
			return !x;
		case EVAL_OP1_CPL:
			return ~x;
	}
	return eval_status=-1;
}
int eval_op2(char o,int x,int y)
{
	switch (o)
	{
		case EVAL_OP2_ADD:
			return x+y;
		case EVAL_OP2_SUB:
			return x-y;
		case EVAL_OP2_MUL:
			return x*y;
		case EVAL_OP2_DIV:
			return y?x/y:(eval_status=eval_status?eval_status:-1); // DIVISION BY ZERO!
		case EVAL_OP2_MOD:
			return y?x%y:(eval_status=eval_status?eval_status:-1); // DIVISION BY ZERO!
		case EVAL_OP2_AND:
			return x&y;
		case EVAL_OP2_XOR:
			return x^y;
		case EVAL_OP2_ORR:
			return x|y;
		case EVAL_OP2_SHL:
			return y>0?x<<y:(eval_status=eval_status?eval_status:-1); // NEGATIVE SHIFT!
		case EVAL_OP2_L_E:
			return x<=y;
		case EVAL_OP2_LSS:
			return x<y;
		case EVAL_OP2_SHR:
			return y>0?x>>y:(eval_status=eval_status?eval_status:-1); // NEGATIVE SHIFT!
		case EVAL_OP2_G_E:
			return x>=y;
		case EVAL_OP2_GRT:
			return x>y;
		case EVAL_OP2_EQU:
			return x==y;
		case EVAL_OP2_NEQ:
			return x!=y;
	}
	return eval_status=-1;
}
void eval_operate(void)
{
	char o=eval_pop_op();
	int i=eval_pop_int();
	if (o<EVAL_UNARIES)
		eval_push_int(eval_op1(o,i));
	else
		eval_push_int(eval_op2(o,eval_pop_int(),i));
}
char *eval_cursor;
int eval_get_prefix(void)
{
	switch (*eval_cursor++)
	{
		case '(':
			return EVAL_P_INIT;
		case '+':
			return EVAL_OP1_PLS;
		case '-':
			return EVAL_OP1_MNS;
		case '~':
			return EVAL_OP1_CPL;
		case '!':
			return EVAL_OP1_NOT;
	}
	return --eval_cursor,EVAL_NULL;
}
int eval_get_suffix(void)
{
	char c;
	switch (*eval_cursor++)
	{
		case ')':
			return EVAL_P_EXIT;
		case '+':
			return EVAL_OP2_ADD;
		case '-':
			return EVAL_OP2_SUB;
		case '*':
			return EVAL_OP2_MUL;
		case '/':
			return EVAL_OP2_DIV;
		case '%':
			return EVAL_OP2_MOD;
		case '&':
			return EVAL_OP2_AND;
		case '|':
			return EVAL_OP2_ORR;
		case '^':
			return EVAL_OP2_XOR;
		case '<':
			if ((c=*eval_cursor++)=='<')
				return EVAL_OP2_SHL;
			if (c=='=')
				return EVAL_OP2_L_E;
			if (c=='>')
				return EVAL_OP2_NEQ;
			return --eval_cursor,EVAL_OP2_LSS;
		case '>':
			if ((c=*eval_cursor++)=='>')
				return EVAL_OP2_SHR;
			if (c=='=')
				return EVAL_OP2_G_E;
			return --eval_cursor,EVAL_OP2_GRT;
		case '=':
			if (*eval_cursor=='=')
				++eval_cursor;
			return EVAL_OP2_EQU;
		case '!':
			if (*eval_cursor=='=')
				return ++eval_cursor,EVAL_OP2_NEQ;
		case 0: case ',': // end of expr.
			break;
		default: // unknown character!
			eval_status=-1;
	}
	return --eval_cursor,EVAL_NULL;
}
void eval_shunt(char c)
{
	if (c==EVAL_P_EXIT)
	{
		while (eval_ops&&(eval_op[eval_ops-1]!=EVAL_P_INIT))
			eval_operate();
		if (eval_pop_op()!=EVAL_P_INIT)
			eval_status=-1;
	}
	else
	{
		int i=eval_priorities[c];
		//if (c<EVAL_UNARIES)
			//while (eval_ops&&(i<eval_pr[eval_ops-1]))
				//eval_operate();
		//else
		if (c!=EVAL_P_INIT)
			while (eval_ops&&(i<=eval_pr[eval_ops-1]))
				eval_operate();
		eval_push_op(c);
	}
}
int eval_escape(int i)
{
	if (i!='\\')
		return i;
	switch(*++eval_cursor)
	{
		case 'a':
			return 7;
		case 'b':
			return 8;
		case 'e':
			return 27;
		case 'f':
			return 12;
		case 'n':
			return 10;
		case 'r':
			return 13;
		case 't':
			return 9;
		case '\\':
			return i;
	}
	return eval_status=-1;
}
int eval(char *s)
{
	int i=0; // perhaps this should be "long long"...
	char c,q=eval_status=eval_ints=eval_ops=0;
	eval_cursor=s;
	while (eval_status>=0)
	{
		c=*eval_cursor;
		if (!q&&c=='"') // string
		{
			i=0;
			while(eval_status>=0&&(c=*++eval_cursor)&&c!='"')
				i=(i<<8)+(eval_escape(c)&255);
			if (!c)
				eval_status=-1;
			else
			{
				++eval_cursor;
				eval_push_int(i);
				q=1;
			}
		}
		else if (!q&&(iseither(c)||(c>='#'&&c<='%'))) // integer or symbol
		{
			s=eval_cursor;
			while ((c=*++eval_cursor)&&iseither(c))
				{}
			if ((c=*s)=='$'&&!iseither(s[1]))
				i=flag_dollar?dollar:target; // special case: target address
			else if (c=='#'||c=='$')
				i=eval_hex2i(&s[1],eval_cursor); // prefixed hexadecimal
			else if (c=='0'&&(s[1]|32)=='x')
				i=eval_hex2i(&s[2],eval_cursor); // prefixed hexadecimal C-style
			else if (c=='%')
				i=eval_dec2i(&s[1],eval_cursor,2); // prefixed binary
			else if (isnumber(c))
				switch (eval_cursor[-1]|32)
				{
					case 'h': i=eval_hex2i(s,&eval_cursor[-1]); // suffixed hexadecimal
						break;
					case 'd': i=eval_dec2i(s,&eval_cursor[-1],10); // suffixed decimal
						break;
					case 'o': i=eval_dec2i(s,&eval_cursor[-1],8); // suffixed octal
						break;
					case 'b': i=eval_dec2i(s,&eval_cursor[-1],2); // suffixed binary
						break;
					default: i=eval_dec2i(s,eval_cursor,10); // pure decimal
				}
			else
			{
				c=*eval_cursor;
				*eval_cursor=0;
				i=get_label(s);
				*eval_cursor=c;
				if (i<0)
					eval_status|=i=1;
				else
					i=value[i];
			}
			eval_push_int(i);
			q=1;
		}
		else if (!q&&(c=eval_get_prefix())) // prefix
			eval_shunt(c);
		else if (q&&(c=eval_get_suffix())) // suffix
			eval_shunt(c),q=c==EVAL_P_EXIT;
		else if (q&&(!c||c==',')) // end of expression
			break;
		else
			eval_status=-1; // invalid character!
	}
	if (!eval_status)
	{
		while (eval_ops)
			eval_operate();
		i=eval_pop_int();
		if (eval_ints)
			eval_status=-1;
	}
	return i;
}

// tables and functions of opcodes and parameters

#define GET_SEARCH(x) { int i,j=0,k=sizeof(x)/sizeof(x[0])-1,l; do { if (!(l=strcasecmp(s,x[i=((j+k)/2)].name))) return i; if (l>0) j=i+1; else k=i-1; } while (j<=k); return -1; } // >=0 OK, <0 ERROR
typedef struct { char name[8]; long int code; long int type; } t_opcode;
enum opcode_ {
	PSEUDO_NULL	, // 0!
	PSEUDO_ALIGN	,
	PSEUDO_DEFB	,
	PSEUDO_DEFC	,
	PSEUDO_DEFD	,
	PSEUDO_DEFS	,
	PSEUDO_DEFW	,
	PSEUDO_DEFZ	,
	PSEUDO_ELIF	,
	PSEUDO_ELSE	,
	PSEUDO_END	,
	PSEUDO_ENDIF	,
	PSEUDO_ENDM	,
	PSEUDO_EQU	,
	PSEUDO_IF	,
	PSEUDO_INCBIN	,
	PSEUDO_INCLUDE	,
	PSEUDO_MACRO	,
	PSEUDO_ORG	,
	OPCODE_COPY1	,
	OPCODE_COPY2	,
	//OPCODE_COPY3	,
	//OPCODE_COPY4	,
	OPCODE_ADC	,
	OPCODE_ADD	,
	OPCODE_BIT	,
	OPCODE_CALL	,
	OPCODE_DJNZ	,
	OPCODE_EX	,
	OPCODE_IM	,
	OPCODE_IN	,
	OPCODE_INC	,
	OPCODE_JP	,
	OPCODE_JR	,
	OPCODE_LD	,
	OPCODE_MULUB	,
	OPCODE_MULUW	,
	OPCODE_NOP	,
	OPCODE_OUT	,
	OPCODE_POP	,
	OPCODE_RET	,
	OPCODE_RLC	,
	OPCODE_RST	,
	OPCODE_SUB	,
};
t_opcode opcode[]={ // MUST BE IN ALPHABETICAL ORDER!
	{	"="	,	0	,	PSEUDO_EQU	},
	{	"adc"	,	+0	,	OPCODE_ADC	},
	{	"add"	,	0	,	OPCODE_ADD	},
	{	"align"	,	0	,	PSEUDO_ALIGN	},
	{	"and"	,	+4	,	OPCODE_SUB	},
	{	"bit"	,	+0x40	,	OPCODE_BIT	},
	{	"brk"	,	0xEDFF	,	OPCODE_COPY2	}, // RASM breakpoint
	{	"call"	,	0	,	OPCODE_CALL	},
	{	"ccf"	,	0x3F	,	OPCODE_COPY1	},
	{	"cp"	,	+7	,	OPCODE_SUB	},
	{	"cpd"	,	0xEDA9	,	OPCODE_COPY2	},
	{	"cpdr"	,	0xEDB9	,	OPCODE_COPY2	},
	{	"cpi"	,	0xEDA1	,	OPCODE_COPY2	},
	{	"cpir"	,	0xEDB1	,	OPCODE_COPY2	},
	{	"cpl"	,	0x2F	,	OPCODE_COPY1	},
	{	"daa"	,	0x27	,	OPCODE_COPY1	},
	{	"db"	,	0	,	PSEUDO_DEFB	},
	{	"dc"	,	0	,	PSEUDO_DEFC	},
	{	"dd"	,	0	,	PSEUDO_DEFD	},
	{	"dec"	,	+1	,	OPCODE_INC	},
	{	"defb"	,	0	,	PSEUDO_DEFB	},
	{	"defc"	,	0	,	PSEUDO_DEFC	},
	{	"defd"	,	0	,	PSEUDO_DEFD	},
	{	"defm"	,	0	,	PSEUDO_DEFB	},
	{	"defs"	,	0	,	PSEUDO_DEFS	},
	{	"defw"	,	0	,	PSEUDO_DEFW	},
	{	"defz"	,	0	,	PSEUDO_DEFZ	},
	{	"di"	,	0xF3	,	OPCODE_COPY1	},
	{	"djnz"	,	0	,	OPCODE_DJNZ	},
	{	"ds"	,	0	,	PSEUDO_DEFS	},
	{	"dw"	,	0	,	PSEUDO_DEFW	},
	{	"dz"	,	0	,	PSEUDO_DEFZ	},
	{	"ei"	,	0xFB	,	OPCODE_COPY1	},
	{	"elif"	,	0	,	PSEUDO_ELIF	},
	{	"else"	,	0	,	PSEUDO_ELSE	},
	{	"end"	,	0	,	PSEUDO_END	},
	{	"endif"	,	0	,	PSEUDO_ENDIF	},
	{	"endm"	,	0	,	PSEUDO_ENDM	},
	{	"equ"	,	0	,	PSEUDO_EQU	},
	{	"ex"	,	0	,	OPCODE_EX	},
	{	"exa"	,	0x08	,	OPCODE_COPY1	}, // RASM synonym
	{	"exx"	,	0xD9	,	OPCODE_COPY1	},
	{	"halt"	,	0x76	,	OPCODE_COPY1	},
	{	"if"	,	0	,	PSEUDO_IF	},
	{	"im"	,	0	,	OPCODE_IM	},
	{	"in"	,	0	,	OPCODE_IN	},
	{	"inc"	,	+0	,	OPCODE_INC	},
	{	"incbin",	0	,	PSEUDO_INCBIN	},
	{	"include",	0	,	PSEUDO_INCLUDE	},
	{	"ind"	,	0xEDAA	,	OPCODE_COPY2	},
	{	"indr"	,	0xEDBA	,	OPCODE_COPY2	},
	{	"ini"	,	0xEDA2	,	OPCODE_COPY2	},
	{	"inir"	,	0xEDB2	,	OPCODE_COPY2	},
	{	"jp"	,	0	,	OPCODE_JP	},
	{	"jr"	,	0	,	OPCODE_JR	},
	{	"ld"	,	0	,	OPCODE_LD	},
	{	"ldd"	,	0xEDA8	,	OPCODE_COPY2	},
	{	"lddr"	,	0xEDB8	,	OPCODE_COPY2	},
	{	"ldi"	,	0xEDA0	,	OPCODE_COPY2	},
	{	"ldir"	,	0xEDB0	,	OPCODE_COPY2	},
	{	"list"	,	0	,	PSEUDO_NULL	},
	{	"macro"	,	0	,	PSEUDO_MACRO	},
	{	"mulub"	,	0	,	OPCODE_MULUB	},
	{	"muluw"	,	0	,	OPCODE_MULUW	},
	{	"neg"	,	0xED44	,	OPCODE_COPY2	},
	{	"nolist",	0	,	PSEUDO_NULL	},
	{	"nop"	,	0x00	,	OPCODE_NOP	},
	{	"or"	,	+6	,	OPCODE_SUB	},
	{	"org"	,	0	,	PSEUDO_ORG	},
	{	"otdr"	,	0xEDBB	,	OPCODE_COPY2	},
	{	"otir"	,	0xEDB3	,	OPCODE_COPY2	},
	{	"out"	,	0	,	OPCODE_OUT	},
	{	"outd"	,	0xEDAB	,	OPCODE_COPY2	},
	{	"outi"	,	0xEDA3	,	OPCODE_COPY2	},
	{	"pop"	,	+0xC1	,	OPCODE_POP	},
	{	"push"	,	+0xC5	,	OPCODE_POP	},
	{	"res"	,	+0x80	,	OPCODE_BIT	},
	{	"ret"	,	0	,	OPCODE_RET	},
	{	"reti"	,	0xED4D	,	OPCODE_COPY2	},
	{	"retn"	,	0xED45	,	OPCODE_COPY2	},
	{	"rl"	,	+0x10	,	OPCODE_RLC	},
	{	"rla"	,	0x17	,	OPCODE_COPY1	},
	{	"rlc"	,	+0x00	,	OPCODE_RLC	},
	{	"rlca"	,	0x07	,	OPCODE_COPY1	},
	{	"rld"	,	0xED6F	,	OPCODE_COPY2	},
	{	"rr"	,	+0x18	,	OPCODE_RLC	},
	{	"rra"	,	0x1F	,	OPCODE_COPY1	},
	{	"rrc"	,	+0x08	,	OPCODE_RLC	},
	{	"rrca"	,	0x0F	,	OPCODE_COPY1	},
	{	"rrd"	,	0xED67	,	OPCODE_COPY2	},
	{	"rst"	,	0	,	OPCODE_RST	},
	{	"sbc"	,	+1	,	OPCODE_ADC	},
	{	"scf"	,	0x37	,	OPCODE_COPY1	},
	{	"set"	,	+0xC0	,	OPCODE_BIT	},
	{	"shl"	,	+0x20	,	OPCODE_RLC	}, // synonym
	{	"shr"	,	+0x38	,	OPCODE_RLC	}, // synonym
	{	"sl1"	,	+0x30	,	OPCODE_RLC	}, // RASM synonym
	{	"sla"	,	+0x20	,	OPCODE_RLC	},
	{	"sll"	,	+0x30	,	OPCODE_RLC	},
	{	"sra"	,	+0x28	,	OPCODE_RLC	},
	{	"srl"	,	+0x38	,	OPCODE_RLC	},
	{	"sub"	,	+2	,	OPCODE_SUB	},
	{	"xor"	,	+5	,	OPCODE_SUB	},
};
int get_opcode(char *s) GET_SEARCH(opcode)
typedef struct { char name[6]; short int type; } t_parmtr;
enum parmtr_ {
	PARMTR_NULL	, // 0!
	PARMTR_INTEGER	,
	PARMTR_POINTER	,
	PARMTR_P_BC	,
	PARMTR_P_C	,
	PARMTR_P_DE	,
	PARMTR_P_HL	,
	PARMTR_P_IX	,
	PARMTR_P_IY	,
	PARMTR_P_SP	,
	PARMTR_A	,
	PARMTR_AF	,
	PARMTR_AF2	,
	PARMTR_B	,
	PARMTR_BC	,
	PARMTR_C	,
	PARMTR_D	,
	PARMTR_DE	,
	PARMTR_E	,
	PARMTR_H	,
	PARMTR_HL	,
	PARMTR_I	,
	PARMTR_IX	,
	PARMTR_IY	,
	PARMTR_L	,
	PARMTR_NC	,
	PARMTR_NS	,
	PARMTR_NV	,
	PARMTR_NZ	,
	PARMTR_R	,
	PARMTR_S	,
	PARMTR_SP	,
	PARMTR_V	,
	PARMTR_XH	,
	PARMTR_XL	,
	PARMTR_YH	,
	PARMTR_YL	,
	PARMTR_Z	,
};
t_parmtr parmtr[]={ // MUST BE IN ALPHABETICAL ORDER!
	{	"(bc)"	, PARMTR_P_BC	},
	{	"(c)"	, PARMTR_P_C	},
	{	"(de)"	, PARMTR_P_DE	},
	{	"(hl)"	, PARMTR_P_HL	},
	{	"(ix)"	, PARMTR_P_IX	},
	{	"(iy)"	, PARMTR_P_IY	},
	{	"(sp)"	, PARMTR_P_SP	},
	{	"[bc]"	, PARMTR_P_BC	},
	{	"[c]"	, PARMTR_P_C	},
	{	"[de]"	, PARMTR_P_DE	},
	{	"[hl]"	, PARMTR_P_HL	},
	{	"[ix]"	, PARMTR_P_IX	},
	{	"[iy]"	, PARMTR_P_IY	},
	{	"[sp]"	, PARMTR_P_SP	},
	{	"a"	, PARMTR_A	},
	{	"af"	, PARMTR_AF	},
	{	"af'"	, PARMTR_AF2	},
	{	"b"	, PARMTR_B	},
	{	"bc"	, PARMTR_BC	},
	{	"c"	, PARMTR_C	},
	{	"d"	, PARMTR_D	},
	{	"de"	, PARMTR_DE	},
	{	"e"	, PARMTR_E	},
	{	"h"	, PARMTR_H	},
	{	"hix"	, PARMTR_XH	},
	{	"hiy"	, PARMTR_YH	},
	{	"hl"	, PARMTR_HL	},
	{	"hx"	, PARMTR_XH	},
	{	"hy"	, PARMTR_YH	},
	{	"i"	, PARMTR_I	},
	{	"ix"	, PARMTR_IX	},
	{	"ixh"	, PARMTR_XH	},
	{	"ixl"	, PARMTR_XL	},
	{	"iy"	, PARMTR_IY	},
	{	"iyh"	, PARMTR_YH	},
	{	"iyl"	, PARMTR_YL	},
	{	"l"	, PARMTR_L	},
	{	"lix"	, PARMTR_XL	},
	{	"liy"	, PARMTR_YL	},
	{	"lx"	, PARMTR_XL	},
	{	"ly"	, PARMTR_YL	},
	{	"m"	, PARMTR_S	},
	{	"nc"	, PARMTR_NC	},
	{	"ns"	, PARMTR_NS	},
	{	"nv"	, PARMTR_NV	},
	{	"nz"	, PARMTR_NZ	},
	{	"p"	, PARMTR_NS	},
	{	"pe"	, PARMTR_V	},
	{	"po"	, PARMTR_NV	},
	{	"r"	, PARMTR_R	},
	{	"s"	, PARMTR_S	},
	{	"sp"	, PARMTR_SP	},
	{	"v"	, PARMTR_V	},
	{	"xh"	, PARMTR_XH	},
	{	"xl"	, PARMTR_XL	},
	{	"yh"	, PARMTR_YH	},
	{	"yl"	, PARMTR_YL	},
	{	"z"	, PARMTR_Z	},
};
int get_parmtr(char *s) GET_SEARCH(parmtr)

int eval_parmtr(char *s,int *e)
{
	*e=eval_status=0;
	if (!*s)
		return PARMTR_NULL;
	int i;
	if ((i=get_parmtr(s))>=0)
		return parmtr[i].type; // parmtr
	char q=0,*t=s,z;
	while (*t)
		++t;
	if ((z=*--t)==')'&&*s=='(') // catch special case (EXPRESSION) OP (EXPRESSION)
	{
		char *r=s; q=1;
		while (q&&++r<t)
			switch (*r)
			{
				case '(': ++q; break;
				case ')': --q; break;
			}
		q=(q==1); // only q==1 ensures that this is a pointer
	}
	if (q|=(z==']'&&*s=='[')) // POINTER, PARMTR_P_IX or PARMTR_P_IY?
	{
		char x,o;
		if ((s[1]|32)=='i'&&((x=s[2]|32)=='x'||x=='y')&&((o=s[3])=='+'||o=='-'))
		{
			char y=*t;
			*t=0;
			*e=flag_dollar?(eval(&s[4])*(o=='-'?-1:1)):eval(&s[3]);
			*t=y;
			return eval_status<0?-1:((x&1)?PARMTR_P_IY:PARMTR_P_IX);
		}
		*t=0; ++s;
	}
	*e=eval(s);
	*t=z;
	return eval_status<0?-1:(q?PARMTR_POINTER:PARMTR_INTEGER);
}

int get_parmtr_addhl(int i)
{
	switch(i)
	{
		case PARMTR_BC:
			return 0x00;
		case PARMTR_DE:
			return 0x10;
		case PARMTR_HL:
			return 0x20;
		case PARMTR_SP:
			return 0x30;
	}
	return -1;
}
int get_parmtr_bit(int i)
{
	switch(i)
	{
		case PARMTR_B:
			return 0;
		case PARMTR_C:
			return 1;
		case PARMTR_D:
			return 2;
		case PARMTR_E:
			return 3;
		case PARMTR_H:
			return 4;
		case PARMTR_L:
			return 5;
		case PARMTR_P_HL:
			return 6;
		case PARMTR_A:
			return 7;
		case PARMTR_P_IX:
			return 16+0+6;
		case PARMTR_P_IY:
			return 16+8+6;
	}
	return -1;
}
int get_parmtr_call(int i)
{
	switch(i)
	{
		case PARMTR_NZ:
			return 0xC4;
		case PARMTR_Z:
			return 0xCC;
		case PARMTR_NC:
			return 0xD4;
		case PARMTR_C:
			return 0xDC;
		case PARMTR_NV:
			return 0xE4;
		case PARMTR_V:
			return 0xEC;
		case PARMTR_NS:
			return 0xF4;
		case PARMTR_S:
			return 0xFC;
		case PARMTR_INTEGER:
			return 0xCD;
	}
	return -1;
}
int get_parmtr_in(int i)
{
	switch(i)
	{
		case PARMTR_B:
			return 0;
		case PARMTR_C:
			return 1;
		case PARMTR_D:
			return 2;
		case PARMTR_E:
			return 3;
		case PARMTR_H:
			return 4;
		case PARMTR_L:
			return 5;
		case PARMTR_NULL:
			return 6;
		case PARMTR_A:
			return 7;
	}
	return -1;
}
int get_parmtr_inc(int i)
{
	switch(i)
	{
		case PARMTR_B:
			return 0;
		case PARMTR_C:
			return 1;
		case PARMTR_D:
			return 2;
		case PARMTR_E:
			return 3;
		case PARMTR_H:
			return 4;
		case PARMTR_L:
			return 5;
		case PARMTR_P_HL:
			return 6;
		case PARMTR_A:
			return 7;
		case PARMTR_XH:
			return 16+0+4;
		case PARMTR_XL:
			return 16+0+5;
		case PARMTR_P_IX:
			return 16+0+6;
		case PARMTR_YH:
			return 16+8+4;
		case PARMTR_YL:
			return 16+8+5;
		case PARMTR_P_IY:
			return 16+8+6;
		case PARMTR_BC:
			return 32+00+0+0;
		case PARMTR_DE:
			return 32+00+0+1;
		case PARMTR_HL:
			return 32+00+0+2;
		case PARMTR_SP:
			return 32+00+0+3;
		case PARMTR_IX:
			return 32+16+0+2;
		case PARMTR_IY:
			return 32+16+8+2;
	}
	return -1;
}
int get_parmtr_jp(int i)
{
	switch(i)
	{
		case PARMTR_NZ:
			return 0xC2;
		case PARMTR_Z:
			return 0xCA;
		case PARMTR_NC:
			return 0xD2;
		case PARMTR_C:
			return 0xDA;
		case PARMTR_NV:
			return 0xE2;
		case PARMTR_V:
			return 0xEA;
		case PARMTR_NS:
			return 0xF2;
		case PARMTR_S:
			return 0xFA;
		case PARMTR_HL:
		case PARMTR_P_HL:
			return 0xE9;
		case PARMTR_IX:
		case PARMTR_P_IX:
			return 0xDDE9;
		case PARMTR_IY:
		case PARMTR_P_IY:
			return 0xFDE9;
		case PARMTR_INTEGER:
			return 0xC3;
	}
	return -1;
}
int get_parmtr_jr(int i)
{
	switch(i)
	{
		case PARMTR_NZ:
			return 0x20;
		case PARMTR_Z:
			return 0x28;
		case PARMTR_NC:
			return 0x30;
		case PARMTR_C:
			return 0x38;
		case PARMTR_INTEGER:
			return 0x18;
	}
	return -1;
}
int get_parmtr_ld(int i)
{
	switch(i)
	{
		case PARMTR_INTEGER:
			return 128;
		case PARMTR_POINTER:
			return 64;
		case PARMTR_B:
			return 0;
		case PARMTR_C:
			return 1;
		case PARMTR_D:
			return 2;
		case PARMTR_E:
			return 3;
		case PARMTR_H:
			return 4;
		case PARMTR_L:
			return 5;
		case PARMTR_P_HL:
			return 6;
		case PARMTR_A:
			return 7;
		case PARMTR_XH:
			return 16+0+4;
		case PARMTR_XL:
			return 16+0+5;
		case PARMTR_P_IX:
			return 16+0+6;
		case PARMTR_YH:
			return 16+8+4;
		case PARMTR_YL:
			return 16+8+5;
		case PARMTR_P_IY:
			return 16+8+6;
		case PARMTR_BC:
			return 32+00+0+0;
		case PARMTR_DE:
			return 32+00+0+1;
		case PARMTR_HL:
			return 32+00+0+2;
		case PARMTR_SP:
			return 32+00+0+3;
		case PARMTR_IX:
			return 32+16+0+2;
		case PARMTR_IY:
			return 32+16+8+2;
		case PARMTR_P_BC:
			return 256+0;
		case PARMTR_P_DE:
			return 256+1;
		case PARMTR_I:
			return 256+2;
		case PARMTR_R:
			return 256+3;
	}
	return -1;
}
int get_parmtr_pop(int i)
{
	switch(i)
	{
		case PARMTR_BC:
			return 0x00;
		case PARMTR_DE:
			return 0x10;
		case PARMTR_HL:
			return 0x20;
		case PARMTR_AF:
			return 0x30;
		case PARMTR_IX:
			return 0xDD20;
		case PARMTR_IY:
			return 0xFD20;
	}
	return -1;
}
int get_parmtr_ret(int i)
{
	switch(i)
	{
		case PARMTR_NZ:
			return 0xC0;
		case PARMTR_Z:
			return 0xC8;
		case PARMTR_NC:
			return 0xD0;
		case PARMTR_C:
			return 0xD8;
		case PARMTR_NV:
			return 0xE0;
		case PARMTR_V:
			return 0xE8;
		case PARMTR_NS:
			return 0xF0;
		case PARMTR_S:
			return 0xF8;
		case PARMTR_NULL:
			return 0xC9;
	}
	return -1;
}
int get_parmtr_rlc(int i)
{
	switch(i)
	{
		case PARMTR_B:
			return 0;
		case PARMTR_C:
			return 1;
		case PARMTR_D:
			return 2;
		case PARMTR_E:
			return 3;
		case PARMTR_H:
			return 4;
		case PARMTR_L:
			return 5;
		case PARMTR_P_HL:
			return 6;
		case PARMTR_A:
			return 7;
		case PARMTR_P_IX:
			return 16+0+6;
		case PARMTR_P_IY:
			return 16+8+6;
		 // SJASM shortcuts
		case PARMTR_BC:
			return 8;
		case PARMTR_DE:
			return 10;
		case PARMTR_HL:
			return 12;
	}
	return -1;
}
int get_parmtr_sub(int i)
{
	switch(i)
	{
		case PARMTR_B:
			return 0;
		case PARMTR_C:
			return 1;
		case PARMTR_D:
			return 2;
		case PARMTR_E:
			return 3;
		case PARMTR_H:
			return 4;
		case PARMTR_L:
			return 5;
		case PARMTR_P_HL:
			return 6;
		case PARMTR_A:
			return 7;
		case PARMTR_XH:
			return 16+0+4;
		case PARMTR_XL:
			return 16+0+5;
		case PARMTR_P_IX:
			return 16+0+6;
		case PARMTR_YH:
			return 16+8+4;
		case PARMTR_YL:
			return 16+8+5;
		case PARMTR_P_IY:
			return 16+8+6;
		case PARMTR_INTEGER:
			return 128;
	}
	return -1;
}

#define NEXTBYTE output[target++]
#define CHECK_OVERFLOW(x,a,z,e) do{ if (!eval_status&&(x<a||x>z)) FATAL_ERROR(e); }while(0)
#define CHECK_BAD_CHAR(x) CHECK_OVERFLOW(x,-128,+127,error_char_overflow)
#define CHECK_BAD_BYTE(x) CHECK_OVERFLOW(x,-128,+255,error_byte_overflow)
#define CHECK_BAD_WORD(x) CHECK_OVERFLOW(x,-32768,+65535,error_word_overflow)

int assemble_filler(int i,int j) // 0 OK, !0 ERROR
{
	if (i<0)
		FATAL_ERROR(error_improper_argument);
	if ((target+i)>SIZEOF_OUTPUT)
		FATAL_ERROR(error_out_of_memory);
	while (i--)
		NEXTBYTE=j;
	return 0;
}

#define FETCH_PARMTR(a,aa) do{ t=s; q=0; while ((c=*t)&&(c!=','||q)) { ++t; if (c=='"') q=!q; }; *t=0; if ((a=eval_parmtr(s,&aa))<0) FATAL_ERROR(error_syntax_error); if (*t=c) ++t; s=t; }while(0)
#define FATAL_PARMTR FATAL_ERROR(error_improper_argument)
#define GET_PARMTR_F(a,f) if ((a=f(a))<0) FATAL_PARMTR
#define INDEX_PREFIX(a) if ((a)&16) NEXTBYTE=((a)&8)*4+0xDD

int assemble_opcode(int o,int oo) // 0 OK, !0 ERROR
{
	int x,xx,y,yy,z;
	char *s=split_parmtr,*t,c,q;
	FETCH_PARMTR(x,xx);
	z=eval_status; // 1st doubt
	switch(o)
	{
		case OPCODE_ADC:
			if (x==PARMTR_HL)
			{
				FETCH_PARMTR(y,yy);
				GET_PARMTR_F(y,get_parmtr_addhl);
				NEXTBYTE=0xED;
				NEXTBYTE=y-oo*8+0x4A;
			}
			else
			{
				if (x==PARMTR_A&&*s)
					FETCH_PARMTR(x,xx);
				GET_PARMTR_F(x,get_parmtr_sub);
				if (x<32)
				{
					INDEX_PREFIX(x);
					NEXTBYTE=(x&7)+oo*16+0x88;
					if ((x&23)==22)
					{
						NEXTBYTE=xx;
						CHECK_BAD_CHAR(xx);
					}
				}
				else if (x==128)
				{
					NEXTBYTE=oo*16+0xCE;
					NEXTBYTE=xx;
					CHECK_BAD_BYTE(xx);
				}
			}
			break;
		case OPCODE_ADD:
			if (x==PARMTR_HL||x==PARMTR_IX||x==PARMTR_IY)
			{
				FETCH_PARMTR(y,yy);
				if (x!=PARMTR_HL)
				{
					NEXTBYTE=x==PARMTR_IX?0xDD:0xFD;
					y=y==PARMTR_HL?-1:(y==x?PARMTR_HL:y); // *"ADD IX/IY,HL", "ADD IX/IY,IY/IX"...
				}
				GET_PARMTR_F(y,get_parmtr_addhl);
				NEXTBYTE=y+0x09;
			}
			else
			{
				if (x==PARMTR_A&&*s)
					FETCH_PARMTR(x,xx);
				GET_PARMTR_F(x,get_parmtr_sub);
				if (x<32)
				{
					INDEX_PREFIX(x);
					NEXTBYTE=(x&7)+0x80;
					if ((x&23)==22)
					{
						NEXTBYTE=xx;
						CHECK_BAD_CHAR(xx);
					}
				}
				else if (x==128)
				{
					NEXTBYTE=0xC6;
					NEXTBYTE=xx;
					CHECK_BAD_BYTE(xx);
				}
			}
			break;
		case OPCODE_BIT:
			if (x!=PARMTR_INTEGER||xx<0||xx>7)
				FATAL_PARMTR;
			FETCH_PARMTR(y,yy);
			GET_PARMTR_F(y,get_parmtr_bit);
			if (y<16)
			{
				NEXTBYTE=0xCB;
				NEXTBYTE=(y&7)+oo+xx*8;
			}
			else
			{
				NEXTBYTE=(y&8)*4+0xDD;
				NEXTBYTE=0xCB;
				NEXTBYTE=yy;
				NEXTBYTE=(y&7)+oo+xx*8;
				CHECK_BAD_CHAR(yy);
			}
			break;
		case OPCODE_CALL:
			GET_PARMTR_F(x,get_parmtr_call);
			if (x!=0xCD)
			{
				FETCH_PARMTR(y,xx);
				if (y!=PARMTR_INTEGER)
					FATAL_PARMTR;
			}
			NEXTBYTE=x;
			NEXTBYTE=xx;
			NEXTBYTE=xx>>8;
			CHECK_BAD_WORD(xx);
			break;
		case OPCODE_DJNZ:
			if (x!=PARMTR_INTEGER)
				FATAL_PARMTR;
			NEXTBYTE=0x10;
			xx=xx-target-1;
			NEXTBYTE=xx;
			CHECK_BAD_CHAR(xx);
			break;
		case OPCODE_EX:
			FETCH_PARMTR(y,yy);
			if (x==PARMTR_P_SP)
				x=y,y=PARMTR_P_SP;
			if (x==PARMTR_AF&&y==PARMTR_AF2)
				NEXTBYTE=0x08;
			else if ((x==PARMTR_HL&&y==PARMTR_DE)||(y==PARMTR_HL&&x==PARMTR_DE))
				NEXTBYTE=0xEB;
			else if (y==PARMTR_P_SP)
			{
				if (x==PARMTR_IX)
					NEXTBYTE=0xDD;
				else if (x==PARMTR_IY)
					NEXTBYTE=0xFD;
				else if (x!=PARMTR_HL)
					FATAL_PARMTR;
				NEXTBYTE=0xE3;
			}
			else
				FATAL_PARMTR;
			break;
		case OPCODE_IM:
			if (x!=PARMTR_INTEGER||xx<0||xx>2)
				FATAL_PARMTR;
			if (xx)
				++xx;
			NEXTBYTE=0xED;
			NEXTBYTE=xx*8+0x46;
			break;
		case OPCODE_IN:
			FETCH_PARMTR(y,yy);
			if (x==PARMTR_P_C&&!y)
				y=x,x=0;
			if (x==PARMTR_A&&y==PARMTR_POINTER)
			{
				NEXTBYTE=0xDB;
				NEXTBYTE=yy;
				CHECK_BAD_BYTE(yy);
			}
			else if (y==PARMTR_P_C) // PARMTR_P_BC?
			{
				GET_PARMTR_F(x,get_parmtr_in);
				NEXTBYTE=0xED;
				NEXTBYTE=x*8+0x40;
			}
			else
				FATAL_PARMTR;
			break;
		case OPCODE_INC:
			GET_PARMTR_F(x,get_parmtr_inc);
			INDEX_PREFIX(x);
			NEXTBYTE=(x&32)?(x&3)*16+oo*8+0x03:(x&7)*8+oo+0x04;
			if ((x&23)==22)
			{
				NEXTBYTE=xx;
				CHECK_BAD_CHAR(xx);
			}
			break;
		case OPCODE_JP:
			GET_PARMTR_F(x,get_parmtr_jp);
			if ((x&0xFF)==0xE9)
			{
				if (x&0xFF00)
					NEXTBYTE=x>>8;
				NEXTBYTE=x;
				if (xx) // JP (IX+!0)
					FATAL_PARMTR;
			}
			else
			{
				if (x!=0xC3)
				{
					FETCH_PARMTR(y,xx);
					if (y!=PARMTR_INTEGER)
						FATAL_PARMTR;
				}
				NEXTBYTE=x;
				NEXTBYTE=xx;
				NEXTBYTE=xx>>8;
				CHECK_BAD_WORD(xx);
			}
			break;
		case OPCODE_JR:
			GET_PARMTR_F(x,get_parmtr_jr);
			if (x!=0x18)
			{
				FETCH_PARMTR(y,xx);
				if (y!=PARMTR_INTEGER)
					FATAL_PARMTR;
			}
			NEXTBYTE=x;
			xx=xx-target-1;
			NEXTBYTE=xx;
			CHECK_BAD_CHAR(xx);
			break;
		case OPCODE_LD:
			GET_PARMTR_F(x,get_parmtr_ld);
			if (x==128)
				FATAL_PARMTR;
			FETCH_PARMTR(y,yy);
			GET_PARMTR_F(y,get_parmtr_ld);
			if (x&256) // LD (BC)/(DE)/I/R,A
			{
				if (y!=0x07)
					FATAL_PARMTR;
				if (x&2)
				{
					NEXTBYTE=0xED;
					NEXTBYTE=(x&1)*8+0x47;
				}
				else
					NEXTBYTE=0x02+(x&1)*16;
			}
			else if (y&256) // LD A,(BC)/(DE)/I/R
			{
				if (x!=0x07)
					FATAL_PARMTR;
				if (y&2)
				{
					NEXTBYTE=0xED;
					NEXTBYTE=(y&1)*8+0x57;
				}
				else
					NEXTBYTE=0x0A+(y&1)*16;
			}
			else if (x&32) // LD REG16,...
			{
				INDEX_PREFIX(x|y);
				if (x==35&&(y&35)==34) // LD SP,HL/IX/IY
					NEXTBYTE=0xF9;
				else if (y==128) // ...NN
				{
					NEXTBYTE=(x&3)*16+0x01;
					NEXTBYTE=yy;
					NEXTBYTE=yy>>8;
					CHECK_BAD_WORD(yy);
				}
				else if (y==64) // ...(NN)
				{
					if ((x&3)==2)
						NEXTBYTE=0x2A;
					else
					{
						NEXTBYTE=0xED;
						NEXTBYTE=(x&3)*16+0x4B;
					}
					NEXTBYTE=yy;
					NEXTBYTE=yy>>8;
					CHECK_BAD_WORD(yy);
				}
				else if (x<35&&(y&23)==22) // SJASM shortcut: LD BC,(IX+0) = LD C,(IX+0) + LD B,(IX+1)
				{
					NEXTBYTE=(x&3)*16+0X4E;
					NEXTBYTE=yy;
					INDEX_PREFIX(y);
					NEXTBYTE=(x&3)*16+0X46;
					NEXTBYTE=++yy;
					CHECK_BAD_CHAR(yy);
				}
				else if (y&32&&((x<35||x&16)&&(y<35||y&16))) // shortcut: LD REG16,REG16 = LD REG8H,REG8H + LD REG8L,REG8L
				{
					if (((x&y&3)==2)&&((x^y)&(16|8))) // *"LD HL,IX", "LD IX,IY"...
						FATAL_PARMTR;
					NEXTBYTE=c=(0x40+(x&3)*16+(y&3)*2);
					INDEX_PREFIX(x|y);
					NEXTBYTE=c+9;
				}
				else
					FATAL_PARMTR;
			}
			else if (y&32) // LD ...,REG16
			{
				INDEX_PREFIX(x|y);
				if (x==64) // (NN)...
				{
					if ((y&3)==2)
						NEXTBYTE=0x22;
					else
					{
						NEXTBYTE=0xED;
						NEXTBYTE=(y&3)*16+0x43;
					}
					NEXTBYTE=xx;
					NEXTBYTE=xx>>8;
					CHECK_BAD_WORD(xx);
				}
				else if (y<35&&(x&23)==22) // SJASM shortcut: LD (IX+0),BC = LD (IX+0),C + LD (IX+1),B
				{
					NEXTBYTE=(y&3)*2+0X71;
					NEXTBYTE=xx;
					INDEX_PREFIX(x);
					NEXTBYTE=(y&3)*2+0X70;
					NEXTBYTE=++xx;
					CHECK_BAD_CHAR(xx);
				}
				else
					FATAL_PARMTR;
			}
			else
			{
				if (((x&7)==6)&&((y&7)==6))
					FATAL_PARMTR; // *"LD (HL),(HL)"...
				if ((x|y)&16)
				{
					NEXTBYTE=((x|y)&8)*4+0xDD;
					if (((x&y)&16)&&((x^y)&8))
						FATAL_PARMTR; // *"LD XL,YL"...
				}
				if (x<32) // LD REG8,...
				{
					if (y<32) // ...REG8
					{
						if (((x^y)&16)&&((x&6)==4)&&((y&6)==4))
							FATAL_PARMTR; // *"LD XL,L"...
						NEXTBYTE=(x&7)*8+(y&7)+0x40;
						if ((x&7)==6)
						{
							if (y&16)
								FATAL_PARMTR; // *"LD (IX),XL", *"LD (HL),XL"...
							if (x&16)
							{
								NEXTBYTE=xx;
								CHECK_BAD_CHAR(xx);
							}
						}
						else if ((y&7)==6)
						{
							if (x&16)
								FATAL_PARMTR; // *"LD XL,(IX)", *"LD XL,(HL)"...
							if (y&16)
							{
								NEXTBYTE=yy;
								CHECK_BAD_CHAR(yy);
							}
						}
					}
					else if (y==128) // ...N
					{
						NEXTBYTE=(x&7)*8+0x06;
						if ((x&23)==22)
						{
							NEXTBYTE=xx;
							CHECK_BAD_CHAR(xx);
						}
						NEXTBYTE=yy;
						CHECK_BAD_BYTE(yy);
					}
					else if (x==7&&y==64) // LD A,(NN)
					{
						NEXTBYTE=0x3A;
						NEXTBYTE=yy;
						NEXTBYTE=yy>>8;
						CHECK_BAD_WORD(yy);
					}
					else
						FATAL_PARMTR;
				}
				else if (x==64&&y==7) // LD (NN),A
				{
					NEXTBYTE=0x32;
					NEXTBYTE=xx;
					NEXTBYTE=xx>>8;
					CHECK_BAD_WORD(xx);
				}
				else
					FATAL_PARMTR;
			}
			break;
		case OPCODE_MULUB: // R800 only!
			if (flag_dollar)
				FATAL_ERROR(error_forbidden_as80);
			if (x==PARMTR_A&&*s)
				FETCH_PARMTR(x,xx); // accept both "MULUB r8" and "MULUB A,r8"
			GET_PARMTR_F(x,get_parmtr_in);
			NEXTBYTE=0XED;
			NEXTBYTE=x*8+0xC1;
			break;
		case OPCODE_MULUW: // R800 only!
			if (flag_dollar)
				FATAL_ERROR(error_forbidden_as80);
			if (x!=PARMTR_HL||!*s)
				FATAL_PARMTR; // reject "MULUW r16" instead of "MULUW HL,r16"!
			FETCH_PARMTR(x,xx);
			GET_PARMTR_F(x,get_parmtr_addhl);
			NEXTBYTE=0XED;
			NEXTBYTE=x+0xC3;
			break;
		case OPCODE_NOP:
			if (!x)
				xx=1;
			else if (x!=PARMTR_INTEGER||eval_status)
				FATAL_PARMTR;
			if (assemble_filler(xx,0))
				return -1;
			break;
		case OPCODE_OUT:
			FETCH_PARMTR(y,yy);
			if (x==PARMTR_POINTER&&y==PARMTR_A)
			{
				NEXTBYTE=0xD3;
				NEXTBYTE=xx;
				CHECK_BAD_BYTE(xx);
			}
			else if (x==PARMTR_P_C) // PARMTR_P_BC?
			{
				GET_PARMTR_F(y,get_parmtr_in);
				NEXTBYTE=0xED;
				NEXTBYTE=y*8+0x41;
			}
			else
				FATAL_PARMTR;
			break;
		case OPCODE_POP:
			// shortcut: PUSH HL,DE... = PUSH HL + PUSH DE...; POP HL,DE... = ...POP DE + POP HL
			y=0;
			while (x)
			{
				asciz[ASCIZ_MAXIMUM-++y]=x;
				FETCH_PARMTR(x,xx);
			}
			if (oo&4)
				xx=1,yy=+1; // PUSH
			else
				xx=y,yy=-1; // POP!
			while (y--)
			{
				x=asciz[ASCIZ_MAXIMUM-xx];
				GET_PARMTR_F(x,get_parmtr_pop);
				if (x&0xFF00)
					NEXTBYTE=x>>8;
				NEXTBYTE=oo+x;
				xx+=yy;
			}
			break;
		case OPCODE_RET:
			GET_PARMTR_F(x,get_parmtr_ret);
			NEXTBYTE=x;
			break;
		case OPCODE_RLC:
			GET_PARMTR_F(x,get_parmtr_rlc);
			if (x<8)
			{
				NEXTBYTE=0xCB;
				NEXTBYTE=(x&7)+oo;
			}
			else if (x<16) // SJASM shortcut: SLA HL = SLA L + RL H; SRA HL = SRA H + RR L
			{
				if (oo<16) // reject RLC and RRC!
					FATAL_PARMTR;
				NEXTBYTE=0xCB;
				if (oo&8) // right: hi + lo
					NEXTBYTE=(x++&7)+oo;
				else // left: lo + hi
					NEXTBYTE=(x&7)+oo+1;
				NEXTBYTE=0xCB;
				NEXTBYTE=(x&7)+(oo&8)+16; // the second opcode is always RL or RR
			}
			else
			{
				NEXTBYTE=(x&8)*4+0xDD;
				NEXTBYTE=0xCB;
				NEXTBYTE=xx;
				NEXTBYTE=(x&7)+oo;
				CHECK_BAD_BYTE(xx);
			}
			break;
		case OPCODE_RST:
			if (x!=PARMTR_INTEGER||xx<0||xx>63||(xx%8&&xx>8))
				FATAL_PARMTR;
			if (xx<8)
				xx*=8;
			NEXTBYTE=xx+0xC7;
			break;
		case OPCODE_SUB:
			if (x==PARMTR_HL) // SJASM shortcut: SUB HL,BC = CP A + SBC HL,BC
			{
				NEXTBYTE=0xBF;
				FETCH_PARMTR(y,yy);
				GET_PARMTR_F(y,get_parmtr_addhl);
				NEXTBYTE=0xED;
				NEXTBYTE=y+0x42;
			}
			else
			{
				if (x==PARMTR_A&&*s)
					FETCH_PARMTR(x,xx);
				GET_PARMTR_F(x,get_parmtr_sub);
				if (x==128)
				{
					NEXTBYTE=oo*8+0xC6;
					NEXTBYTE=xx;
					CHECK_BAD_BYTE(xx);
				}
				else
				{
					INDEX_PREFIX(x);
					NEXTBYTE=oo*8+(x&7)+0x80;
					if ((x&23)==22)
					{
						NEXTBYTE=xx;
						CHECK_BAD_CHAR(xx);
					}
				}
			}
			break;
		default:
			FATAL_ERROR(error_syntax_error);
			break;
	}
	if (*s)
		FATAL_ERROR(error_too_many_arguments);
	eval_status|=z; // merge doubts
	return 0;
}

int assemble_copypath(void) // 0 OK, !0 ERROR
{
	if (*split_parmtr!='"')
		return -1;
	int c;
	eval_cursor=split_parmtr;
	while ((c=*++eval_cursor)&&c!='"')
		if (c==(PATHCHAR^'\\'^'/'))
			*eval_cursor=PATHCHAR;
	if (c!='"')
		return -1;
	*eval_cursor++=0;
	strcpy(newpath,folder);
	strcat(newpath,++split_parmtr);
	return 0;
}

unsigned int condition0; char conditions,recording;
int assemble_input(void) // 0 OK, !0 ERROR
{
	//puts(input0);
	split_input();
	int i,j,o=PSEUDO_NULL,oo=eval_status=0;
	if (*split_opcode)
	{
		if ((i=get_opcode(split_opcode))>=0)
			o=opcode[i].type,oo=opcode[i].code;
		else
			o=-1;
	}
	if (recording)
	{
		if (o!=PSEUDO_ENDM)
		{
			// add line to macro
			merge_input();
			char c,*s=input0,*t=&asciz[ascizs];
			while (c=*s)
			{
				if (!isletter(c))
					*t++=c,++s;
				else
				{
					char *d=s;
					while (iseither(*d))
						++d;
					c=*d;
					*d=0;
					// turn macro params into "\1"..."\9"
					for (i=1;i<=param[params-1][0];++i)
						if (!strcasecmp(s,&param0[param[params-1][i]]))
							break;
					if (i<=param[params-1][0])
					{
						*t++='\\';
						*t++='0'+i;
						s=d;
					}
					else
						while (s<d)
							*t++=*s++;
					*d=c;
				}
			}
			*t=0;
			if (add_cache(&asciz[ascizs])<0)
				return -1;
		}
		else
		{
			// end macro recording
			if (*split_parmtr)
				FATAL_ERROR(error_too_many_arguments);
			if (*split_symbol)
				FATAL_ERROR(error_invalid_symbol);
			if (add_cache(symbol_dollar)<0)
				return -1;
			merge_param();
			recording=0;
		}
	}
	else
	{
		if (o==PSEUDO_ENDM)
			FATAL_ERROR(error_endm_without_macro);
		// IF, ELSE, ENDIF
		else if (o==PSEUDO_IF||o==PSEUDO_ELIF)
		{
			if (o==PSEUDO_ELIF)
			{
				if (flag_dollar)
					FATAL_ERROR(error_forbidden_as80);
				if (!conditions)
					FATAL_ERROR(error_elif_without_if);
				j=condition0&1?condition0&2:2; // SKIP status!
				--conditions;
				condition0>>=2;
			}
			else
				j=0;
			if (*split_symbol)
				FATAL_ERROR(error_invalid_symbol);
			if (!*split_parmtr)
				FATAL_ERROR(error_syntax_error);
			if (conditions>15) // (31 on 64-bit OSes)
				FATAL_ERROR(error_stack_overflow);
			if (!j) // "IF/ELIF TRUE" will SKIP all following ELIF!
			{
				j=!eval(split_parmtr);
				if (eval_status<0)
					FATAL_ERROR(error_invalid_expression);
				if (eval_status>0)
					FATAL_ERROR(error_undefined_symbol);
			}
			++conditions;
			condition0=(condition0<<2)+j; // 0 TRUE, 1 FALSE, 2 SKIP
		}
		else if (o==PSEUDO_ELSE)
		{
			if (*split_parmtr)
				FATAL_ERROR(error_too_many_arguments);
			if (*split_symbol)
				FATAL_ERROR(error_invalid_symbol);
			if (!conditions)
				FATAL_ERROR(error_else_without_if);
			condition0^=1;
		}
		else if (o==PSEUDO_ENDIF)
		{
			if (*split_parmtr)
				FATAL_ERROR(error_too_many_arguments);
			if (*split_symbol)
				FATAL_ERROR(error_invalid_symbol);
			if (!conditions)
				FATAL_ERROR(error_endif_without_if);
			--conditions;
			condition0>>=2;
		}
		else if (!condition0)
		{
			if (*split_symbol)
			{
				if (!isletter(*split_symbol)||get_opcode(split_symbol)>=0||get_parmtr(split_symbol)>=0)
					FATAL_ERROR(error_invalid_symbol);
				// <label> EQU <expression>
				else if (o==PSEUDO_EQU)
				{
					i=eval(split_parmtr);
					if (eval_status<0)
						FATAL_ERROR(error_invalid_expression);
					if (!eval_status&&add_label(split_symbol,i)<0)
						return -1;
					return 0;
				}
				// <label> MACRO [params]
				else if (o==PSEUDO_MACRO)
				{
					if (recording)
						FATAL_ERROR(error_macro_without_endm);
					if (add_macro(split_symbol)<0||(o=split_param(split_parmtr)))
						return -1;
					recording=1;
					return 0;
				}
				// <label>[:]
				else if (add_label(split_symbol,target)<0)
					return -1;
			}
			switch (o)
			{
				// INCLUDE, END, macros
				case PSEUDO_INCLUDE:
					if (assemble_copypath())
						FATAL_ERROR(error_invalid_string);
					if (*eval_cursor)
						FATAL_ERROR(error_too_many_arguments);
					if (!open_input(newpath))
						if (!*incpath||(strcpy(newpath,folder),strcat(newpath,incpath),strcat(newpath,split_parmtr),!open_input(newpath)))
							return printerror(error_cannot_open_file),-1;
					break;
				case PSEUDO_END:
					if (*split_parmtr)
						FATAL_ERROR(error_syntax_error);
					close_input();
					break;
				case -1: // macro name
					if ((oo=get_macro(split_opcode))<0)
						FATAL_ERROR(error_undefined_opcode);
					if (!open_macro(oo)||split_param(split_parmtr))
						return -1;
					break;
				// data and code output
				case PSEUDO_ALIGN:
					i=eval(split_parmtr);
					if (eval_status>0)
						FATAL_ERROR(error_undefined_symbol);
					if (eval_status<0)
						FATAL_ERROR(error_invalid_expression);
					if (*eval_cursor)
						FATAL_ERROR(error_too_many_arguments);
					if (i&&(j=target%i))
						if (assemble_filler(i-j,0))
							return -1;
					break;
				case PSEUDO_DEFB:
				case PSEUDO_DEFC:
				case PSEUDO_DEFD:
				case PSEUDO_DEFW:
				case PSEUDO_DEFZ:
					if (!*(eval_cursor=split_parmtr))
						FATAL_ERROR(error_syntax_error);
					j=0;
					do
					{
						if (*eval_cursor=='"'&&(o==PSEUDO_DEFB||o==PSEUDO_DEFC||o==PSEUDO_DEFZ)&&!(eval_cursor[eval_cursor[1]=='\\'?3:2]=='"'))
						{
							j=target; while (eval_status>=0&&(i=*++eval_cursor)&&i!='"')
								NEXTBYTE=eval_escape(i);
							if (eval_status<0||!*eval_cursor)
								FATAL_ERROR(error_invalid_string);
							if (o==PSEUDO_DEFZ)
								NEXTBYTE=0; // a DEFZ string is NULL-terminated
							else if (o==PSEUDO_DEFC)
								{ if (j!=target) output[target-1]+=128; } // the last char of a DEFC string has its top bit toggled: DEFC "DISC" = DEFM "DIS","C"+128
							j=0; ++eval_cursor;
						}
						else
						{
							i=eval(eval_cursor);
							if (eval_status<0)
								FATAL_ERROR(error_invalid_expression);
							if (eval_status>0)
								j=eval_status;
							NEXTBYTE=i;
							if (o==PSEUDO_DEFB||o==PSEUDO_DEFC||o==PSEUDO_DEFZ)
								CHECK_BAD_BYTE(i);
							else
							{
								NEXTBYTE=i>>8;
								if (o==PSEUDO_DEFW)
									CHECK_BAD_WORD(i);
								else
								{
									NEXTBYTE=i>>16;
									NEXTBYTE=i>>24;
								}
							}
						}
					}
					while (*eval_cursor++==',');
					if (*--eval_cursor)
						FATAL_ERROR(error_syntax_error);
					if (j)
						eval_status=j;
					break;
				case PSEUDO_DEFS:
					i=eval(split_parmtr);
					j=(!eval_status&&*eval_cursor==',')?eval(++eval_cursor):0;
					if (*eval_cursor)
						FATAL_ERROR(error_too_many_arguments);
					if (eval_status>0)
						FATAL_ERROR(error_undefined_symbol);
					if (eval_status<0)
						FATAL_ERROR(error_invalid_expression);
					CHECK_BAD_BYTE(j);
					if (assemble_filler(i,j))
						return -1;
					break;
				case PSEUDO_INCBIN:
					if (flag_dollar)
						FATAL_ERROR(error_forbidden_as80);
					if (assemble_copypath())
						FATAL_ERROR(error_invalid_string);
					i=(*eval_cursor==',')?eval(++eval_cursor):0;
					j=(!eval_status&&*eval_cursor==',')?eval(++eval_cursor):(1<<30);
					if (*eval_cursor)
						FATAL_ERROR(error_too_many_arguments);
					if (eval_status>0)
						FATAL_ERROR(error_undefined_symbol);
					if (eval_status<0)
						FATAL_ERROR(error_invalid_expression);
					FILE *f=fopen(newpath,"rb");
					if (!f)
						return printerror(error_cannot_open_file),-1;
					fseek(f,i,i<0?SEEK_END:SEEK_SET);
					if (j>SIZEOF_OUTPUT-target)
						j=1+SIZEOF_OUTPUT-target;
					target+=fread(&output[target],1,j,f);
					fclose(f);
					break;
				case PSEUDO_ORG:
					i=eval(split_parmtr);
					if (eval_status>0)
						FATAL_ERROR(error_undefined_symbol);
					if (eval_status<0)
						FATAL_ERROR(error_invalid_expression);
					if (*eval_cursor)
						FATAL_ERROR(error_too_many_arguments);
					if (i<target)
						FATAL_ERROR(error_improper_argument);
					if (!remote)
						origin=target=i;
					else if (assemble_filler(i-=target,flag_z))
						return -1;
					break;
				//case OPCODE_COPY4:
					//NEXTBYTE=oo>>24;
				//case OPCODE_COPY3:
					//NEXTBYTE=oo>>16;
				case OPCODE_COPY2:
					NEXTBYTE=oo>>8;
				case OPCODE_COPY1:
					NEXTBYTE=oo;
				case PSEUDO_NULL:
					if (*split_parmtr)
						FATAL_ERROR(error_too_many_arguments);
					break;
				default:
					if (assemble_opcode(o,oo))
						return -1;
			}
			if (target>SIZEOF_OUTPUT)
				FATAL_ERROR(error_out_of_memory);
			if (target>remote)
				remote=target;
		}
	}
	return 0;
}

int assemble_doubts(int q) // Q=0: solve labels; Q!=0: solve parameters; 0 OK, !0 ERROR
{
	unsigned char *s;
	int i,j,k;
	do
	{
		j=k=0;
		for (i=0;i<doubts;++i)
			if (*(s=&asciz[doubt[i]])&&(q||(*s>32)))
			{
				++j;
				strcpy(input0,s);
				strcpy(source,input0);
				dollar=target=doubt_target[i];
				int l;
				inputs=0;
				if ((l=doubt_source[i])<0)
				{
					int n=~l;
					while ((l=chain[n++])>=0)
						input_source[inputs++]=l;
					l=~l; // final item
				}
				input_source[inputs++]=l;
				if (assemble_input())
					{}
				else if (eval_status)
				{
					if (q)
						printerror(error_undefined_symbol);
				}
				else
					*s=0,++k;
			}
	} while (k&&!q);
	return q&&(k!=j);
}

int flag_v=0;
int assemble(char *s,char *t) // assemble source `s` to target `t`; 0 OK, !0 ERROR
{
	int i,j;
	inputs=caches=macros=params=locals=doubts=chains=dollar=target=origin=remote=recording=conditions=condition0=0;
	if (!open_input(s))
		return printerror(error_cannot_open_file),-1;
	do
	{
		while (read_input())
		{
			if (assemble_input())
				return -1;
			if (eval_status>0)
				if (merge_input(),add_doubt(dollar==target?input0:split_opcode-1)<0)
					return -1;
			dollar=target; // update "$"
		}
	} while (close_input());
	//while (close_input())
		//{}
	if (recording)
		FATAL_ERROR(error_macro_without_endm);
	if (conditions)
		FATAL_ERROR(error_if_without_endif);
	if (assemble_doubts(0)||assemble_doubts(!0))
		return -1;
	FILE *f=stdout;
	if (strcmp(t,"-"))
		if (!(f=fopen(t,"wb")))
			FATAL_ERROR(error_cannot_create_file);
	i=fwrite(&output[origin],1,j=remote-origin,f);
	if (f!=stdout)
		fclose(f);
	if (i!=j)
		FATAL_ERROR(error_cannot_write_data);
	if (flag_v>=0)
		fprintf(stderr,"%s:%s (%i bytes)\n",s,t,j);
	return 0;
}

int main(int argc,char *argv[])
{
	int i,j,k;
	char *r,*s=0,*t=0;
	if (!(asciz=malloc(ASCIZ_MAXIMUM))
		||!CREATE_ARRAY(label,LABEL_MAXIMUM)
		||!CREATE_ARRAY(value,LABEL_MAXIMUM)
		||!CREATE_ARRAY(cache,CACHE_MAXIMUM)
		||!CREATE_ARRAY(macro,CACHE_MAXIMUM)
		||!CREATE_ARRAY(macro_source,CACHE_MAXIMUM)
		||!CREATE_ARRAY(doubt,DOUBT_MAXIMUM)
		||!CREATE_ARRAY(doubt_target,DOUBT_MAXIMUM)
		||!CREATE_ARRAY(doubt_source,DOUBT_MAXIMUM)
		||!CREATE_ARRAY(chain,CHAIN_MAXIMUM))
		return printerror(error_out_of_memory),1; // assuming OS and runtime release memory after exit :-I
	add_label(symbol_debug,*incpath=labels=ascizs=0);
	for (i=1;i<argc;++i)
		if (*(r=argv[i])=='-'&&r[1])
		{
			++r; while (r&&(j=*r++))
				switch (j|32)
				{
					case '$':
						flag_dollar=1;
						break;
					case 'v':
						flag_v=1;
						break;
					case 'q':
						flag_v=-1;
						break;
					case 'z':
						flag_z=0;
						break;
					case 'd':
						j=1; // -Dlabel[=1]
						if (!*r)
							r=symbol_debug; // -D[DEBUG=1]
						{
							char *u; if (u=strchr(r,'='))
							{
								*u=0; // -Dlabel=expr
								j=eval(++u);
								if (eval_status)
									i=argc; // HELP!
							}
						}
						if ((k=get_label(r))>=0)
							value[k]=j; // allow redefining
						else if (add_label(r,j)<0)
							i=argc; // HELP!
						r=0;
						break;
					case 'o':
						if (!t)
							t=r,r=0;
						else i=argc; // HELP!
						break;
					case 'i':
						if (*r)
						{
							char *u=incpath;
								while (*u++=*r++)
									;
							if (u[-1]!=PATHCHAR)
								*u=PATHCHAR,u[1]=0;
							r=0;
						}
						else i=argc; // HELP!
						break;
					default:
						i=argc; // HELP!
				}
		}
		else if (!s) s=r;
		else if (!t) t=r;
		else i=argc; // HELP!
	if (i>argc||!s||!t)
		return printf("%s",string_greeting),1; // GCC warning
	strcpy(folder,s);
	if (r=strrchr(folder,PATHCHAR)) // the source includes a path?
		r[1]=0;
	else // no path in source, use current path
		folder[0]=0;
	j=assemble(s,t);
	if (flag_v>0)
	{
		//puts("Labels:");
		for (i=0;i<labels;++i)
			printf("%-24s= %10i ; #%08X\n",&asciz[label[i]],value[i],value[i]);
		//puts("Macros:");
		for (i=0;i<macros;++i)
		{
			printf("%s MACRO\n",&asciz[cache[k=macro_source[i]]]);
			while (strcmp(symbol_dollar,r=&asciz[cache[++k]]))
				puts(r);
			puts(" ENDM");
		}
		printf("-- %i labels, %i macros, %i doubts, %i buffer\n",labels,macros,doubts,ascizs);
	}
	return j!=0; // 0 OK, !0 ERROR
}
