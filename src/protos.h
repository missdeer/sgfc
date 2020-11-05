/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 protos.h
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdarg.h>

#ifdef _MSC_VER
#define no_return __declspec(noreturn)
#elif __GNUC__
#define no_return __attribute__((noreturn))
#else
#define no_return /* NOP */
#endif

/**** options.c ****/

void PrintHelp(enum option_help);
void PrintStatusLine(const struct SGFInfo *);
void PrintGameSignatures(struct SGFInfo *);
bool ParseArgs(struct SGFInfo *, int, char *[]);
struct SGFCOptions *SGFCDefaultOptions(void);

struct SGFInfo *SetupSGFInfo(struct SGFCOptions *, struct SaveFileHandler *);
void FreeSGFInfo(struct SGFInfo *);


/**** load.c ****/

void CopyValue(struct SGFInfo *, char *, const char *, size_t, bool);
struct PropValue *AddPropValue(struct SGFInfo *, struct Property *, char *,
							   const char *, size_t, const char *, size_t);
struct Property *AddProperty(struct Node *, token, char *, char *);
struct Node *NewNode(struct SGFInfo *, struct Node *, bool);

char *SkipText(struct SGFInfo *, char *, const char *, char, unsigned int);
bool LoadSGF(struct SGFInfo *, char *);
bool LoadSGFFromFileBuffer(struct SGFInfo *);


/**** save.c ****/

int SaveBufferIO_close(struct SaveFileHandler *, U_LONG);

struct SaveFileHandler *SetupSaveFileIO(void);
struct SaveFileHandler *SetupSaveBufferIO(int (*)(struct SaveFileHandler *, U_LONG));
struct SaveC_internal *SetupSaveC_internal(void);

bool SaveSGF(struct SGFInfo *, char *);


/**** properties.c ****/

extern const struct SGFToken sgf_token[];


/**** parse.c ****/

int Parse_Number(char *, ...);
int Parse_Move(char *, ...);
int Parse_Float(char *, ...);
int Parse_Color(char *, ...);
int Parse_Triple(char *, ...);
int Parse_Text(char *, ...);

bool Check_Value(struct SGFInfo *, struct Property *, struct PropValue *,
				U_SHORT, int (*)(char *, ...));
bool Check_Text(struct SGFInfo *, struct Property *, struct PropValue *);
bool Check_Label(struct SGFInfo *, struct Property *, struct PropValue *);
bool Check_Pos(struct SGFInfo *, struct Property *, struct PropValue *);
bool Check_AR_LN(struct SGFInfo *, struct Property *, struct PropValue *);
bool Check_Figure(struct SGFInfo *, struct Property *, struct PropValue *);

void Check_Properties(struct SGFInfo *, struct Node *, struct BoardStatus *);


/**** parse2.c ****/

bool ExpandPointList(struct SGFInfo *, struct Property *, struct PropValue *, bool);
void CompressPointList(struct SGFInfo *, struct Property *);

void SplitNode(struct SGFInfo *, struct Node *, U_SHORT, token, bool);
void ParseSGF(struct SGFInfo *);


/**** execute.c ****/

bool Do_Move(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_AddStones(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_Letter(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_Mark(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_Markup(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_Annotate(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_Root(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_GInfo(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);
bool Do_View(struct SGFInfo *, struct Node *, struct Property *, struct BoardStatus *);


/**** gameinfo.c ****/

bool Check_GameInfo(struct SGFInfo *, struct Property *, struct PropValue *);


/**** util.c ****/

struct UtilC_internal *SetupUtilC_internal(void);

extern bool (*print_error_handler)(U_LONG, struct SGFInfo *, va_list);
extern void (*print_error_output_hook)(struct SGFCError *);

void SearchPos(const char *, struct SGFInfo *, int *, int *);
int PrintError(U_LONG, struct SGFInfo *, ...);
no_return void ExitWithOOMError(char *);
bool PrintErrorHandler(U_LONG, struct SGFInfo *, va_list);
void PrintErrorOutputHook(struct SGFCError *);

int  DecodePosChar(char);
char EncodePosChar(int);

void f_AddTail(struct ListHead *, struct ListNode *);
void f_Enqueue(struct ListHead *, struct ListNode *);
void f_Delete(struct ListHead *, struct ListNode *);

bool strnccmp(char *, char *, size_t);
U_LONG KillChars(char *, U_SHORT, char *);
U_LONG TestChars(char *, U_SHORT, char *);

struct Property *FindProperty(struct Node *, token);

struct Property *NewPropValue(struct SGFInfo *, struct Node *, token, const char *, const char *, bool);
struct PropValue *DelPropValue(struct Property *, struct PropValue *);
struct Property *DelProperty(struct Node *, struct Property *);
struct Node *DelNode(struct SGFInfo *, struct Node *, U_LONG);

bool CalcGameSig(struct TreeInfo *, char *);


/**** strict.c ****/

void StrictChecking(struct SGFInfo *);


/**** protos.h ****/

#define AddTail(h,n) f_AddTail((struct ListHead *)(h), (struct ListNode *)(n))
#define Enqueue(h,n) f_Enqueue((struct ListHead *)(h), (struct ListNode *)(n))
#define Delete(h,n) f_Delete((struct ListHead *)(h), (struct ListNode *)(n))

#define SaveMalloc(type, v, sz, err)	{ v = (type)malloc((size_t)(sz)); if(!(v)) ExitWithOOMError(err); }
