/**
 * total-lockdown – Lock the current TTY and hinder switch to another
 * Copyright © 2013, 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <stdlib.h>


/**
 * Symbol map, `NULL` is used if the key does not produce any
 * symbol or (in the case of KT_LATIN, KT_LETTER, KT_META) if
 * the output can be calcuated from the key value.
 */
const char* KVAL_MAP[][256] = {
  [KT_LATIN]  = {NULL},
  [KT_LETTER] = {NULL},
  [KT_FN]     = {NULL /* There are symbols keys here, see <linux/keyboard.h> if you need any of them. */},
  [KT_SPEC]   = {[KVAL(K_HOLE)]         = NULL /* No assignment */,
		 [KVAL(K_ENTER)]        = "\n",
		 [KVAL(K_SH_REGS)]      = NULL /* Alternative graph + Scroll Lock, do nothing */,
		 [KVAL(K_SH_MEM)]       = NULL /* Shift + Scroll Lock, do nothing */,
		 [KVAL(K_SH_STAT)]      = NULL /* Control + Scroll Lock, do nothing */,
		 [KVAL(K_BREAK)]        = NULL /* Break (pause), do nothing */,
		 [KVAL(K_CONS)]         = NULL /* Switch to last VT, Fat chance! */,
		 [KVAL(K_CAPS)]         = NULL /* Royal Canterlot Voice, ignore it */,
		 [KVAL(K_NUM)]          = NULL,
		 [KVAL(K_HOLD)]         = NULL /* Scroll Lock, do nothing */,
		 [KVAL(K_SCROLLFORW)]   = NULL /* Shift-next, scroll down, do nothing  */,
		 [KVAL(K_SCROLLBACK)]   = NULL /* Shift-prior, scroll up, do nothing */,
		 [KVAL(K_BOOT)]         = NULL /* Three finger salute, ignore it */,
		 [KVAL(K_CAPSON)]       = NULL /* TODO: What is this? */,
		 [KVAL(K_COMPOSE)]      = NULL,
		 [KVAL(K_SAK)]          = NULL /* https://en.wikipedia.org/wiki/Secure_attention_key, ignore it */,
		 [KVAL(K_DECRCONSOLE)]  = NULL /* Switch to previous VT. Fat chance! */,
		 [KVAL(K_INCRCONSOLE)]  = NULL /* Switch to next VT. Fat chance! */,
		 [KVAL(K_SPAWNCONSOLE)] = NULL /* Spawn and switch to a new VT. Fat chance! */,
		 [KVAL(K_BARENUMLOCK)]  = NULL,
		 [KVAL(K_ALLOCATED)]    = NULL /* do nothing (see <linux/keyboard.h> for more information) */,
		 [KVAL(K_NOSUCHMAP)]    = NULL /* do nothing (see <linux/keyboard.h> for more information) */,
		 NULL /* we always put NULL at the end so all invalid onces will not do things. */},
  [KT_PAD]    = {[KVAL(K_P0)] = "0",
		 [KVAL(K_P1)] = "1",
		 [KVAL(K_P2)] = "2",
		 [KVAL(K_P3)] = "3",
		 [KVAL(K_P4)] = "4",
		 [KVAL(K_P5)] = "5",
		 [KVAL(K_P6)] = "6",
		 [KVAL(K_P7)] = "7",
		 [KVAL(K_P8)] = "8",
		 [KVAL(K_P9)] = "9",
		 [KVAL(K_PPLUS)]      = "+",
		 [KVAL(K_PMINUS)]     = "-",
		 [KVAL(K_PSTAR)]      = "*",
		 [KVAL(K_PSLASH)]     = "/",
		 [KVAL(K_PENTER)]     = "\n",
		 [KVAL(K_PCOMMA)]     = ",",
		 [KVAL(K_PDOT)]       = ".",
		 [KVAL(K_PPLUSMINUS)] = "±",
		 [KVAL(K_PPARENL)]    = "(",
		 [KVAL(K_PPARENR)]    = ")",
		 NULL},
  [KT_DEAD]   = {[KVAL(K_DGRAVE)] = "`",
		 [KVAL(K_DACUTE)] = "'",
		 [KVAL(K_DCIRCM)] = "^",
		 [KVAL(K_DTILDE)] = "~",
		 [KVAL(K_DDIERE)] = "\"",
		 [KVAL(K_DCEDIL)] = ",",
		 NULL},
  [KT_CONS]   = {NULL},
  [KT_CUR]    = {[KVAL(K_DOWN)]  = "\033[B",
		 [KVAL(K_LEFT)]  = "\033[D",
		 [KVAL(K_RIGHT)] = "\033[C",
		 [KVAL(K_UP)]    = "\033[A",
		 NULL},
  [KT_SHIFT]  = {[KVAL(K_SHIFT)]     = NULL,
		 [KVAL(K_CTRL)]      = NULL,
		 [KVAL(K_ALT)]       = NULL,
		 [KVAL(K_ALTGR)]     = NULL,
		 [KVAL(K_SHIFTL)]    = NULL,
		 [KVAL(K_SHIFTR)]    = NULL,
		 [KVAL(K_CTRLL)]     = NULL,
		 [KVAL(K_CTRLR)]     = NULL,
		 [KVAL(K_CAPSSHIFT)] = NULL,
		 NULL},
  [KT_META]   = {NULL},
  [KT_ASCII]  = {[KVAL(K_ASC0)] = NULL,
		 [KVAL(K_ASC1)] = NULL,
		 [KVAL(K_ASC2)] = NULL,
		 [KVAL(K_ASC3)] = NULL,
		 [KVAL(K_ASC4)] = NULL,
		 [KVAL(K_ASC5)] = NULL,
		 [KVAL(K_ASC6)] = NULL,
		 [KVAL(K_ASC7)] = NULL,
		 [KVAL(K_ASC8)] = NULL,
		 [KVAL(K_ASC9)] = NULL,
		 [KVAL(K_HEX0)] = NULL,
		 [KVAL(K_HEX1)] = NULL,
		 [KVAL(K_HEX2)] = NULL,
		 [KVAL(K_HEX3)] = NULL,
		 [KVAL(K_HEX4)] = NULL,
		 [KVAL(K_HEX5)] = NULL,
		 [KVAL(K_HEX6)] = NULL,
		 [KVAL(K_HEX7)] = NULL,
		 [KVAL(K_HEX8)] = NULL,
		 [KVAL(K_HEX9)] = NULL,
		 [KVAL(K_HEXa)] = NULL,
		 [KVAL(K_HEXb)] = NULL,
		 [KVAL(K_HEXc)] = NULL,
		 [KVAL(K_HEXd)] = NULL,
		 [KVAL(K_HEXe)] = NULL,
		 [KVAL(K_HEXf)] = NULL,
		 NULL},
  [KT_LOCK]   = {[KVAL(K_SHIFTLOCK)]     = NULL,
		 [KVAL(K_CTRLLOCK)]      = NULL,
		 [KVAL(K_ALTLOCK)]       = NULL,
		 [KVAL(K_ALTGRLOCK)]     = NULL,
		 [KVAL(K_SHIFTLLOCK)]    = NULL,
		 [KVAL(K_SHIFTRLOCK)]    = NULL,
		 [KVAL(K_CTRLLLOCK)]     = NULL,
		 [KVAL(K_CTRLRLOCK)]     = NULL,
		 [KVAL(K_CAPSSHIFTLOCK)] = NULL,
		 NULL},
  [KT_SLOCK]  = {[KVAL(K_SHIFT_SLOCK)]     = NULL,
		 [KVAL(K_CTRL_SLOCK)]      = NULL,
		 [KVAL(K_ALT_SLOCK)]       = NULL,
		 [KVAL(K_ALTGR_SLOCK)]     = NULL,
		 [KVAL(K_SHIFTL_SLOCK)]    = NULL,
		 [KVAL(K_SHIFTR_SLOCK)]    = NULL,
		 [KVAL(K_CTRLL_SLOCK)]     = NULL,
		 [KVAL(K_CTRLR_SLOCK)]     = NULL,
		 [KVAL(K_CAPSSHIFT_SLOCK)] = NULL,
		 NULL},
  [KT_DEAD2]  = {NULL},
  [KT_BRL]    = {[KVAL(K_BRL_BLANK)] = NULL,
		 [KVAL(K_BRL_DOT1)]  = NULL,
		 [KVAL(K_BRL_DOT2)]  = NULL,
		 [KVAL(K_BRL_DOT3)]  = NULL,
		 [KVAL(K_BRL_DOT4)]  = NULL,
		 [KVAL(K_BRL_DOT5)]  = NULL,
		 [KVAL(K_BRL_DOT6)]  = NULL,
		 [KVAL(K_BRL_DOT7)]  = NULL,
		 [KVAL(K_BRL_DOT8)]  = NULL,
		 [KVAL(K_BRL_DOT9)]  = NULL,
		 [KVAL(K_BRL_DOT10)] = NULL,
		 NULL}
};


/**
 * Fallback compose map that is used then the keyboard layout does not
 * specify the common compositions. In the Linux kernel keyboard compose
 * key and dead key are similarly to each other. The only actual difference
 * is compose key turns the next key into a dead key. Each entry is a
 * 3–tuple (struct kbdiacr), where the first symbol is the diacritical,
 * the second is the base character, i.e. in the order they are typed, and
 * the third is the resulting symbol. The resulting symbol is not in ASCII
 * is it most not be specified with a character literal, rather its Unicode
 * index should be specified with a numerical literal.
 */
struct kbdiacr fallback_accent_table[] = 
  {
    /* Compositions to the higher quarter of the lower 256-character part of the character table. */
    {'`',  'A', 0x00C0},  {'`',  'a', 0x00E0},
    {'\'', 'A', 0x00C1},  {'\'', 'a', 0x00E1},
    {'^',  'A', 0x00C2},  {'^',  'a', 0x00E2},
    {'~',  'A', 0x00C3},  {'~',  'a', 0x00E3},
    {'"',  'A', 0x00C4},  {'"',  'a', 0x00E4},
    {'O',  'A', 0x00C5},  {'o',  'a', 0x00E5}, /* oa, 0a, and aa are all composions to å. */
    {'0',  'A', 0x00C5},  {'0',  'a', 0x00E5},
    {'A',  'A', 0x00C5},  {'a',  'a', 0x00E5},
    {'A',  'E', 0x00C6},  {'a',  'e', 0x00E6},
    {',',  'C', 0x00C7},  {',',  'c', 0x00E7},
    {'`',  'E', 0x00C8},  {'`',  'e', 0x00E8},
    {'\'', 'E', 0x00C9},  {'\'', 'e', 0x00E9},
    {'^',  'E', 0x00CA},  {'^',  'e', 0x00EA},
    {'"',  'E', 0x00CB},  {'"',  'e', 0x00EB},
    {'`',  'I', 0x00CC},  {'`',  'i', 0x00EC},
    {'\'', 'I', 0x00CD},  {'\'', 'i', 0x00ED},
    {'^',  'I', 0x00CE},  {'^',  'i', 0x00EE},
    {'"',  'I', 0x00CF},  {'"',  'i', 0x00EF},
    {'-',  'D', 0x00D0},  {'-',  'd', 0x00F0},
    {'~',  'N', 0x00D1},  {'~',  'n', 0x00F1},
    {'`',  'O', 0x00D2},  {'`',  'o', 0x00F2},
    {'\'', 'O', 0x00D3},  {'\'', 'o', 0x00F3},
    {'^',  'O', 0x00D4},  {'^',  'o', 0x00F4},
    {'~',  'O', 0x00D5},  {'~',  'o', 0x00F5},
    {'"',  'O', 0x00D6},  {'"',  'o', 0x00F6},
    /*             ×                    ÷       Not composed characters. */
    {'/',  'O', 0x00D8},  {'/',  'o', 0x00F8},
    {'`',  'U', 0x00D9},  {'`',  'u', 0x00F9},
    {'\'', 'U', 0x00DA},  {'\'', 'u', 0x00FA},
    {'^',  'U', 0x00DB},  {'^',  'u', 0x00FB},
    {'"',  'U', 0x00DC},  {'"',  'u', 0x00FC},
    {'\'', 'Y', 0x00DD},  {'\'', 'y', 0x00FD},
    {'T',  'H', 0x00DE},  {'t',  'h', 0x00FE},
    {'s',  's', 0x00DF},  {'"',  'y', 0x00FF},
    {'s',  'z', 0x00DF},  {'i',  'j', 0x00FF}, /* ij produces ÿ, probably not ĳ because ĳ
						* cannot be displayed by the TTY and ij
						* looks kind of like ÿ.
						* sz → ß is a synonym for ss → ß, I do not
						* know why (after all it is a long s follow
						* by a regular (final) s), by it is. */
    {0, 0, 0} /* sentinel */
  };

