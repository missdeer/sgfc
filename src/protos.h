/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 protos.h
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdarg.h>

/**** options.c ****/

struct SGFCOptions *ParseArgs(int , char *[]);


/**** load.c ****/

void CopyValue(struct SGFInfo *, char *, const char *, size_t , int );
struct PropValue *Add_PropValue(struct SGFInfo *, struct Property *, char *,
								const char *, size_t, const char *, size_t);
struct Property *Add_Property(struct Node *, token, char *, char *);
struct Node *NewNode(struct SGFInfo *, struct Node * , int);

char *SkipText(struct SGFInfo *, char *, const char *, char , unsigned int );
void LoadSGF(struct SGFInfo *, char *);
void LoadSGFFromFileBuffer(struct SGFInfo *);


/**** save.c ****/

extern struct SaveFileHandler save_file_io;
int SaveFile_BufferIO_Close(struct SaveFileHandler *, U_LONG );
struct SaveFileHandler *init_save_buffer_io(int (* )(struct SaveFileHandler *, U_LONG));

void SaveSGF(struct SGFInfo * , char *);


/**** properties.c ****/

extern struct SGFToken sgf_token[];


/**** parse.c ****/

int Parse_Number(char *, ...);
int Parse_Move(char *, ...);
int Parse_Float(char *, ...);
int Parse_Color(char *, ...);
int Parse_Triple(char *, ...);
int Parse_Text(char *, ...);

int Check_Value(struct SGFInfo *, struct Property *, struct PropValue *,
				U_SHORT , int (*)(char *, ...));
int Check_Text(struct SGFInfo *, struct Property *, struct PropValue *);
int Check_Label(struct SGFInfo *, struct Property *, struct PropValue *);
int Check_Pos(struct SGFInfo *, struct Property *, struct PropValue *);
int Check_AR_LN(struct SGFInfo *, struct Property *, struct PropValue *);
int Check_Figure(struct SGFInfo *, struct Property *, struct PropValue *);

void Check_Properties(struct SGFInfo *, struct Node *, struct BoardStatus *);


/**** parse2.c ****/

int ExpandPointList(struct SGFInfo *, struct Property *, struct PropValue *, int );
void CompressPointList(struct SGFInfo *, struct Property * );

void Split_Node(struct SGFInfo *, struct Node *, U_SHORT, token, int);
void ParseSGF(struct SGFInfo * );


/**** execute.c ****/

int Do_Move(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_Addstones(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_Letter(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_Mark(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_Markup(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_Annotate(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_Root(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_GInfo(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
int Do_View(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);


/**** gameinfo.c ****/

int Check_GameInfo(struct SGFInfo *, struct Property *, struct PropValue *);


/**** util.c ****/

extern int error_count;
extern int critical_count;
extern int warning_count;
extern int ignored_count;
extern char error_enabled[MAX_ERROR_NUM];

extern int (*print_error_handler)(U_LONG, struct SGFInfo *, va_list);
extern void (*print_error_output_hook)(struct SGFCError *);

void SearchPos(const char * , struct SGFInfo * , int * , int * );
int PrintError(U_LONG, struct SGFInfo *, ... );
int  __attribute__((noreturn)) PrintFatalError(U_LONG, struct SGFInfo *, ... );
int PrintErrorHandler(U_LONG, struct SGFInfo *, va_list);
void PrintErrorOutputHook(struct SGFCError *);

int  DecodePosChar(char );
char EncodePosChar(int );

void FreeSGFInfo(struct SGFInfo *);

void f_AddTail(struct ListHead * , struct ListNode * );
void f_Enqueue(struct ListHead * , struct ListNode * );
void f_Delete(struct ListHead * , struct ListNode * );

int strnccmp(char * , char * , size_t);
U_LONG Kill_Chars(char * , U_SHORT , char * );
U_LONG Test_Chars(char * , U_SHORT , char * );

struct Property *Find_Property(struct Node *, token );

struct PropValue *Del_PropValue(struct Property *, struct PropValue *);
struct Property *Del_Property(struct Node *, struct Property *);
struct Node *Del_Node(struct SGFInfo *, struct Node *, U_LONG);

struct Property *New_PropValue(struct SGFInfo *, struct Node *, token, const char *, const char *, int);


/**** strict.c ****/

void Strict_Checking(struct SGFInfo *sgf);


/**** protos.h ****/

#define AddTail(h,n) f_AddTail((struct ListHead *)(h), (struct ListNode *)(n))
#define Enqueue(h,n) f_Enqueue((struct ListHead *)(h), (struct ListNode *)(n))
#define Delete(h,n) f_Delete((struct ListHead *)(h), (struct ListNode *)(n))

#define SaveMalloc(type, v, sz, err)	{ v = (type)malloc((size_t)(sz)); if(!(v)) PrintFatalError(FE_OUT_OF_MEMORY, NULL, err); }
