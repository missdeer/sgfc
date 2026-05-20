/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 protos.h
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdint.h>
#include <stdarg.h>
#include <iconv.h>

/**** options.c ****/

void PrintHelp(enum option_help format);
void PrintStatusLine(const struct SGFInfo *sgfc);
void PrintGameSignatures(const struct SGFInfo *sgfc);
bool ParseArgs(struct SGFInfo *sgfc, int argc, const char *argv[]);
struct SGFCOptions *SGFCDefaultOptions(void);
struct SGFCConfig *SGFCDefaultConfig(void);

struct SGFInfo *SetupSGFInfo(struct SGFCOptions *options, struct SGFCConfig *config);
void FreeSGFInfo(struct SGFInfo *sgfc);


/**** load.c ****/

bool LoadSGF(struct SGFInfo *sgfc, const char *name);
bool LoadSGFFromFileBuffer(struct SGFInfo *sgfc);
bool LoadSGFFromStdin(struct SGFInfo *sgfc);


/**** encoding.c ****/

char *DetectEncoding(struct SGFInfo *sgfc, enum encoding_source *source);
char *DecodeSGFBuffer(struct SGFInfo *sgfc, const char **encbuffer_end, char **encoding_name);
char *DecodeBuffer(struct SGFInfo *sgfc, iconv_t cd, char *buffer, size_t size,
				   size_t err_offset, const char **buffer_end);
iconv_t OpenIconV(struct SGFInfo *sgfc, const char *encoding, const char **encoding_name);

/**** save.c ****/

int SaveBufferIO_open(struct SaveFileHandler *sfh, const char *path, const char *mode);
int SaveBufferIO_close(struct SaveFileHandler *sfh, uint32_t error);

struct SaveFileHandler *SetupSaveFileIO(void);
struct SaveFileHandler *SetupSaveBufferIO(
			int (*open)(struct SaveFileHandler *sfh, const char *path, const char *mode),
			int (*close)(struct SaveFileHandler *sfh, uint32_t error));

bool SaveSGF(struct SGFInfo *sgfc, struct SaveFileHandler *(*setup_sfh)(void), const char *base_name);


/**** properties.c ****/

extern const struct SGFToken sgf_token[];


/**** parse.c ****/

parse_result_t Parse_Number(char *value, size_t *len, ...);
parse_result_t Parse_Move(char *value, size_t *len, ...);
parse_result_t Parse_Float(char *value, size_t *len, ...);
parse_result_t Parse_Color(char *value, size_t *len, ...);
parse_result_t Parse_Triple(char *value, size_t *len, ...);
parse_result_t Parse_Charset(char *value, size_t *len, ...);

parse_result_t Parse_Float_Offset(char *value, size_t *len, size_t offset);
int Parse_Text(struct SGFInfo *sgfc, struct PropValue *v, int prop_num, uint16_t flags);

bool Check_Value(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v,
					uint16_t flags, parse_result_t (*Parse_Value)(char *value, size_t *len, ...));
bool Check_Text(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);
bool Check_Label(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);
bool Check_Pos(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);
bool Check_Stone(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);
bool Check_AR_LN(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);
bool Check_Figure(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);

void Check_Properties(struct SGFInfo *sgfc, struct Node *n, struct BoardStatus *st);


/**** parse2.c ****/

bool ExpandPointList(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v, bool print_error);
void CompressPointList(struct SGFInfo *sgfc, struct Property *p);

void SplitNode(struct SGFInfo *sgfc, struct Node *n, uint16_t flags, token id, bool move);
bool InitAllTreeInfo(struct SGFInfo *sgfc);
bool ParseSGF(struct SGFInfo *sgfc);


/**** execute.c ****/

bool Do_Move(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_AddStones(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_Letter(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_Mark(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_Markup(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_Annotate(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_Root(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_GInfo(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);
bool Do_View(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st);


/**** gameinfo.c ****/

bool Check_GameInfo(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v);


/**** error.c ****/

struct ErrorC_internal *SetupErrorC_internal(void);

extern bool (*print_error_handler)(uint32_t type, struct SGFInfo *sgfc, va_list arglist);
extern void (*print_error_output_hook)(struct SGFCError *error);
extern void (*panic_hook)(uint32_t error, const char *detail);

int PrintError(uint32_t type, struct SGFInfo *sgfc, ...);
ATTRIBUTE_NORETURN void panic(uint32_t error, const char *detail);
ATTRIBUTE_NORETURN void ExitWithFatalError(uint32_t error, const char *detail);
bool PrintErrorHandler(uint32_t type, struct SGFInfo *sgfc, va_list arglist);
void PrintErrorOutputHook(struct SGFCError *error);
void CommonPrintErrorOutputHook(struct SGFCError *error, FILE *stream);


/**** util.c ****/

int  DecodePosChar(char c);
char EncodePosChar(int c);

void f_AddTail(struct ListHead *h, struct ListNode *n);
void f_Enqueue(struct ListHead *h, struct ListNode *n);
void f_Delete(struct ListHead *h, struct ListNode *n);

char *SafeDupString(const char *src, const char *err);
char *SafeDupText(const char *src, size_t len, const char *err);
char *SafeDupText2(const char *src, size_t len, size_t min_capacity, const char *err);
void *SafeMalloc(size_t size, const char *err);
void *SafeCalloc(size_t size, const char *err);
size_t SafeAddSize(size_t a, size_t b, const char *err);

bool strnccmp(const char *a, const char *b, size_t len);
bool stridcmp(const char *a, const char *b);
void strnpcpy(char *dst, const char *src, size_t len);
size_t KillChars(char *value, size_t *len, uint16_t kill, const char *cset);
size_t TestChars(const char *value, uint16_t test, const char *cset);

struct Property *FindProperty(struct Node *n, token id);
struct Property *AddProperty(struct Node *n, token id, uint32_t row, uint32_t col, const char *id_str);
struct Property *DelProperty(struct Node *n, struct Property *p);
struct PropValue *AddPropValue(struct SGFInfo *sgfc, struct Property *p, uint32_t row, uint32_t col,
							   const char *value, size_t size, const char *value2, size_t size2);
struct Property *NewPropValue(struct SGFInfo *sgfc, struct Node *n, token id,
							  const char *value, const char *value2, bool unique);
struct PropValue *DelPropValue(struct Property *p, struct PropValue *v);
struct Node *NewNode(struct SGFInfo *sgfc, struct Node *parent, uint32_t row, uint32_t col, bool new_child);
void DelNode(struct SGFInfo *sgfc, struct Node *n, uint32_t error);
struct TreeInfo *FreeTreeInfo(struct TreeInfo *ti);

bool CalcGameSig(struct TreeInfo *ti, char *buffer);


/**** strict.c ****/

void StrictChecking(struct SGFInfo *sgfc);
