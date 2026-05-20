/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 helpers.h
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <ctype.h>

/* list handling macros */
#define AddTail(h,n) f_AddTail((struct ListHead *)(h), (struct ListNode *)(n))
#define Enqueue(h,n) f_Enqueue((struct ListHead *)(h), (struct ListNode *)(n))
#define Delete(h,n) f_Delete((struct ListHead *)(h), (struct ListNode *)(n))

/* panicking */
#define TO_STR_HELPER(x) #x
#define TO_STR(x) TO_STR_HELPER(x)

#define panic_out_of_memory(x) panic(FE_OUT_OF_MEMORY, (x))
#define panic_impossible() panic(FE_INTERNAL_ERROR, __FILE__ ":" TO_STR(__LINE__))
#define safe_add(a, b) SafeAddSize((a), (b), "overflow " __FILE__ ":" TO_STR(__LINE__))
#define safe_add3(a, b, c) safe_add((a), safe_add((b), (c)))

/* convinience functions to avoid manual casting */
static inline int ch_isalpha(char c) { return isalpha((unsigned char)c); }
static inline int ch_isdigit(char c) { return isdigit((unsigned char)c); }
static inline int ch_islower(char c) { return islower((unsigned char)c); }
static inline int ch_isupper(char c) { return isupper((unsigned char)c); }
static inline int ch_isspace(char c) { return isspace((unsigned char)c); }
static inline int ch_iscntrl(char c) { return iscntrl((unsigned char)c); }
