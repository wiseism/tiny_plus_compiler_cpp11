# TINY+ 词法与语法规则

TINY+
	We define here a programming language called TINY+, which is a superset of TINY in that it includes declarations, if statement, do-while statement, string type and so on.
	The following consists of:
Lexical conventions of the language, including a description of the tokens of the language
EBNF description of each language construct
An description of the main semantics
Sample programs in TINY+

注意：蓝色部分属于TINY语言，红色部分是TINY+新增加的，下同。

## Part 1 Lexical Conventions of TINY+
The keywords of the language are the following:
true		false	or		and 	not		int		bool	string	while	do	if		then	else	end		repeat	until	read	write
All keywords are reserved and must be written in lowcase

实验一原来的方案是所有的关键字Kind都为KEY，如：(KEY, xxx) 这对实验二会造成麻烦。故为每个关键字都分配一个Kind值，具体如下：
(TK_TRUE, true), (TK_FALSE, false), (TK_OR, or)
(TK_AND, and), (TK_NOT, not), (TK_INT, int),
(TK_BOOL, bool), (TK_STRING, ’…’), (TK_WHILE, while),
(TK_DO, do), (TK_IF, if), (TK_THEN, then),
(TK_ELSE, else), (TK_END, end), (TK_REPEAT, repeat),
(TK_UNTIL, until), (TK_READ, read), (TK_WRITE, write),

Special symbols are the following:
	>		<=		>=		,	'
{	}	;	:=	+	-	*	/	(	)	<	=

为每个操作符分配一个Kind值，原因如上。
(TK_GTR, >),  (TK_LEQ, <=),  (TK_GEQ, >=)
(TK_COMMA, ,),  (TK_SEMICOLON, ;),  (TK_ASSIGN, :=),
(TK_ADD, +),  (TK_SUB, -),  (TK_MUL, *),
(TK_DIV, /),  (TK_LP, (),  (TK_RP, )),
(TK_LSS, <),  (TK_EQU, =)

Other tokens are ID, NUM and STRING which are defined by the following regular expressions:
ID=letter (letter | digit)*
	Identifier is letter followed by letters and digits
NUM=digit digit*
字符串定义
STRING=' any character except ' '
字符串是定义在单引号’…’内的,除了单引号’之外的其它字符都可以出现在字符串中,但字符串的定义不能跨行
letter=a|…|z|A|…|Z
digit=0|…|9
Lower and uppercase letters are distinct
White space consists of blanks, newlines and tabs. White space is ignored except that it must separate IDs, NUMs, and keywords
Comments are enclosed in curly brackets {…} and cannot be nested. Comments can include more than one line.

（TINY+的EBNF文法，实验二用到）
## Part 2 Syntax of TINY+
An EBNF grammar for TINY+ is as follows:
蓝色的符号为非终结符，黑色+粗体+斜体为终结符
program -> declarations stmt-sequence
declarations -> decl ; declarations |ε
decl -> type-specifier varlist
type-specifier -> int | bool | string
varlist -> identifier [ , varlist ]
stmt-sequence	-> statement [ ; stmt-sequence ]
statement	-> if-stmt | repeat-stmt | assign-stmt | read-stmt | write-stmt | while-stmt
while-stmt -> while logical-or-exp do stmt-sequence end
if-stmt -> if  logical-or-exp then stmt-sequence [else stmt-sequence] end
repeat-stmt	-> repeat stmt-sequence until logical-or-exp
assign-stmt	-> identifier := logical-or-exp
read-stmt	-> read identifier
write-stmt	-> write logical-or-exp
logical-or-exp	-> logical-and-exp [ or logical-or-exp ]
logical-and-exp -> comparison-exp [ and logical-and-exp]
comparison-exp -> add-exp [ comparison-op comparison-exp ]
comparison-op  -> < | = | > | >= | <=
add-exp	-> mul-exp [ addop add-exp ]
addop  -> + | -
mul-exp	-> factor [ mulop mul-exp ]
mulop  -> * | /
factor  -> number | string | identifier | true | false| ( logical-or-exp )
实验二选做内容的文法：
首先修改产生式1为program -> declarations function-def-list
然后添加以下产生式：
function-def-list -> function-def  function-def-list |ε
function-def -> type-specifier identifier ( parameters-list ) [declarations]  stmt-sequence end
parameters-list -> parameter [ , parameters-list ]
parameter -> type-specifier identifier |ε

## Part 3 Main semantics description of TINY+
A program consists of variable declarations and a sequence of statements. Variable declarations may be empty but there must be at least one statement.
All variables must be declared before they are used, and each variable name can be declared only once
The type of variables and expressions may be int, bool or string, type checking must be done on them

## Part 4 Sample programs in TINY+
string str;
int x, fact;
str:= 'sample program in TINY+ language- computes factorial' ;
read x;
if x>0 and x<100 then {don’t compute if x<=0}
	fact:=1;
	while x>0 do
		fact:=fact*x;
		x:=x-1
	end;
	write fact
end

