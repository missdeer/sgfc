/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 protos.h
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdarg.h>
#include <iconv.h>

/**** options.c ****/

void PrintHelp(enum option_help);
void PrintStatusLine(const struct SGFInfo *);
void PrintGameSignatures(const struct SGFInfo *);
bool ParseArgs(struct SGFInfo *, int, const char *[]);
struct SGFCOptions *SGFCDefaultOptions(void);

struct SGFInfo *SetupSGFInfo(struct SGFCOptions *, struct SaveFileHandler *);
void FreeSGFInfo(struct SGFInfo *);


/**** load.c ****/

bool LoadSGF(struct SGFInfo *, const char *);
bool LoadSGFFromFileBuffer(struct SGFInfo *);


/**** encoding.c ****/

char *DetectEncoding(struct SGFInfo *);
char *ConvertEncoding(struct SGFInfo *, const char *, char **);
bool DecodeWholeSGFBuffer(struct SGFInfo *);
struct StreamReader *InitStreamReader(const char *, char *, char *);
int StreamDecode(struct StreamReader *);
int NextDecodedChar(struct StreamReader *);


char *DecodeBuffer(struct SGFInfo *sgfc, iconv_t cd,
				   char *in_buffer, size_t in_size,
				   U_LONG row, U_LONG col,
				   char **buffer_end);
iconv_t DetectTreeEncoding(struct SGFInfo *sgfc, struct Node *n);

/**** save.c ****/

int SaveBufferIO_close(struct SaveFileHandler *, U_LONG);

struct SaveFileHandler *SetupSaveFileIO(void);
struct SaveFileHandler *SetupSaveBufferIO(int (*)(struct SaveFileHandler *, U_LONG));
struct SaveC_internal *SetupSaveC_internal(void);

bool SaveSGF(struct SGFInfo *, const char *);


/**** properties.c ****/

extern const struct SGFToken sgf_token[];


/**** parse.c ****/

int Parse_Number(char *, size_t *, ...);
int Parse_Move(char *, size_t *, ...);
int Parse_Float(char *, size_t *, ...);
int Parse_Color(char *, size_t *, ...);
int Parse_Triple(char *, size_t *, ...);

int Parse_Float_Offset(char *, size_t *, size_t);
int Parse_Text(struct SGFInfo *, struct PropValue *, int prop_num, U_SHORT flags);

bool Check_Value(struct SGFInfo *, struct Property *, struct PropValue *,
				U_SHORT, int (*)(char *, size_t *, ...));
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
void InitAllTreeInfo(struct SGFInfo *);
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


/**** error.c ****/

struct ErrorC_internal *SetupErrorC_internal(void);

extern bool (*print_error_handler)(U_LONG, struct SGFInfo *, va_list);
extern void (*print_error_output_hook)(struct SGFCError *);
extern void (*oom_panic_hook)(const char *);

int PrintError(U_LONG, struct SGFInfo *, ...);
void ExitWithOOMError(const char *);
bool PrintErrorHandler(U_LONG, struct SGFInfo *, va_list);
void PrintErrorOutputHook(struct SGFCError *);
void CommonPrintErrorOutputHook(struct SGFCError *, FILE *);


/**** util.c ****/

int  DecodePosChar(char);
char EncodePosChar(int);

void f_AddTail(struct ListHead *, struct ListNode *);
void f_Enqueue(struct ListHead *, struct ListNode *);
void f_Delete(struct ListHead *, struct ListNode *);

bool strnccmp(const char *, const char *, size_t);
bool stridcmp(const char *, const char *);
void strnpcpy(char *, const char *, size_t);
char *SaveDupString(const char *, size_t, const char *);
U_LONG KillChars(char *, size_t *, U_SHORT, const char *);
U_LONG TestChars(const char *, U_SHORT, const char *);

struct Property *FindProperty(struct Node *, token);
struct Property *AddProperty(struct Node *, token, U_LONG, U_LONG, const char *);
struct Property *DelProperty(struct Node *, struct Property *);
struct PropValue *AddPropValue(struct SGFInfo *, struct Property *, U_LONG, U_LONG,
							   const char *, size_t, const char *, size_t);
struct Property *NewPropValue(struct SGFInfo *, struct Node *, token, const char *, const char *, bool);
struct PropValue *DelPropValue(struct Property *, struct PropValue *);
struct Node *NewNode(struct SGFInfo *, struct Node *, bool);
void DelNode(struct SGFInfo *, struct Node *, U_LONG);

bool CalcGameSig(struct TreeInfo *, char *);


/**** strict.c ****/

void StrictChecking(struct SGFInfo *);


/**** protos.h ****/

#define AddTail(h,n) f_AddTail((struct ListHead *)(h), (struct ListNode *)(n))
#define Enqueue(h,n) f_Enqueue((struct ListHead *)(h), (struct ListNode *)(n))
#define Delete(h,n) f_Delete((struct ListHead *)(h), (struct ListNode *)(n))

#define SaveMalloc(type, v, sz, err)  {	v = (type)malloc((size_t)(sz)); \
										if(!(v)) { \
											(*oom_panic_hook)(err); /* function will not return */ \
                                            /* will never be reached; safe-guard and hint for linting */ \
											exit(20); \
										} \
									  }
