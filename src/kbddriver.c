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
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <stdio.h>

#include "kbddriver.h"


# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-pedantic"
#include "layout.c" /* When building, the user must do `loadkeys -C THE_USED_TTY -m THE_PREFERED_LAYOUT > src/layout.c`.
		     * It may be possible to look for the first /dev/tty* owned by $USER and get the keyboard from KEYMAP
		     * in rc.conf or vconsole.conf. */
# pragma GCC diagnostic pop



/* from keyboard.c */

/**
 * Symbol map, `NULL` is used if the key does not produce any
 * symbol or (in the case of KT_LATIN, KT_LETTER, KT_META) if
 * the output can be calcuated from the key value.
 */
extern const char* KVAL_MAP[][256];

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
extern struct kbdiacr fallback_accent_table[];



/**
 * Print a text to a file by its descriptor
 * 
 * @param  fd   The file descriptor
 * @param  str  The text to write
 */
static void fdprint(int fd, const char* str)
{
  size_t n = strlen(str);
  ssize_t wrote;
  while (n)
    {
      wrote = write(fd, str, n);
      if (wrote < 0)
	break;
      n -= (size_t)wrote;
      str += (size_t)wrote;
    }
}


/**
 * Print a single character in UTF-8 to a file by its descriptor
 * 
 * @param  fd  The file descriptor
 * @param  c   The character
 */
static void fdputucs(int fd, int32_t c)
{
  static char ucs_buffer[8] = {[7] = 0};
  if (c < 0)
    ; /* cannot, if it does, ignore it */
  else if (c < 0x80)
    {
      ucs_buffer[6] = (char)c;
      fdprint(fd, ucs_buffer + 6);
    }
  else
    {
      long off = 7;
      *ucs_buffer = (int8_t)0x80;
      while (c)
	{
	  *(ucs_buffer + --off) = (char)((c & 0x3F) | 0x80);
	  *ucs_buffer |= (*ucs_buffer) >> 1;
	  c >>= 6;
	}
      if ((*ucs_buffer) & (*(ucs_buffer + off) & 0x3F))
	*(ucs_buffer + --off) = (char)((*ucs_buffer) << 1);
      else
	*(ucs_buffer + off) |= (char)((*ucs_buffer) << 1);
      fdprint(fd, ucs_buffer + off);
    }
}


void readkbd(int fd)
{
  static int next_is_dead2 = 0;
  static int have_dead_key = 0;
  static int modifiers = 0;
  
  for (;;) /* Please fix or report any inconsistency with the Linux VT keyboard. */
    {
      int c = getchar();
      int released = !!(c & 0x80);
      
      if ((KTYP(key_maps[0][c & 0x7F]) & 0x0F) == KT_SHIFT)
	{
	  c = key_maps[0][c & 0x7F];
	  if (released)
	    modifiers &= ~(1 << KVAL(c));
	  else
	    modifiers |= 1 << KVAL(c);
	  continue;
	}
      
      if (key_maps[modifiers] == NULL)
	continue;
      c = key_maps[modifiers][c] & 0x0FFF;
      
      switch (KTYP(c))
	{
	case KT_LETTER: /* Symbols that are affected by the Royal Canterlot Voice key */
	case KT_LATIN:  /* Symbols that are not affected by the Royal Canterlot Voice key */
	  if (KVAL(c) == 'q')
	    return;
	  if (next_is_dead2)
	    {
	      next_is_dead2 = 0;
	      have_dead_key = KVAL(c) & 255;
	    }
	  else if (have_dead_key) /* TODO: how does multiple dead keys work? */
	    {
	      unsigned int i;
	      c = KVAL(c) & 255;
	      for (i = 0; i < accent_table_size; i++)
		if (accent_table[i].diacr == have_dead_key)
		  if (accent_table[i].base == c)
		    {
		      c = accent_table[i].result;
		      break;
		    }
	      if (i == accent_table_size)
		{
		  for (i = 0; fallback_accent_table[i].result; i++)
		    if (fallback_accent_table[i].diacr == have_dead_key)
		      if (fallback_accent_table[i].base == c)
			{
			  c = fallback_accent_table[i].result;
			  break;
			}
		  if (fallback_accent_table[i].result == 0)
		    {
		      if (c == ' ')
			c = have_dead_key;
		      else if (c != have_dead_key)
			fdputucs(fd, have_dead_key);
		    }
		}
	      fdputucs(fd, c);
	      have_dead_key = 0;
	    }
	  else
	    fdputucs(fd, KVAL(c) & 255);
	  break;
	  
	case KT_META:   /* Just like KT_LATIN, except with meta modifier */
	  fdputucs(fd, '\033'); /* We will assume this mode rather than set 8:th bit-mode */
	  fdputucs(fd, KVAL(c) & 255);
	  /* TODO how should `next_is_dead2` and `have_dead_key` behave here? */
	  break;
	  
	case KT_FN:     /* Customisable keys, usally for escape sequnces. Includes F-keys and some misc. keys */
	  if (func_table[KVAL(c)] != NULL)
	    fdprint(fd, func_table[KVAL(c)]);
	  break;
	  
	case KT_DEAD:   /* Dead key */
	  next_is_dead2 = 0;
	  have_dead_key = *(KVAL_MAP[KTYP(c)][KVAL(c)]) & 255;
	  break;
	  
	case KT_DEAD2:  /* Table-assisted customisable dead keys */
	  next_is_dead2 = 0;
	  have_dead_key = KVAL(c);
	  break;
	  
	case KT_SPEC:   /* Special keys*/
	case KT_PAD:    /* Keypad */
	case KT_CUR:    /* Arrows keys */
	case KT_ASCII:  /* This is what happens when somepony holds down Alternative whil using the keypad */
	  if (KVAL_MAP[KTYP(c)][KVAL(c)] != NULL)
	    fdprint(fd, KVAL_MAP[KTYP(c)][KVAL(c)]);
	  else if (KTYP(c) == KT_SPEC)
	    switch (c)
	      {
	      case K_COMPOSE:     /* Compose key */
		next_is_dead2 = 1;
		break;
		
	      case K_NUM:         /* TODO: Num Lock */
	      case K_BARENUMLOCK: /* No difference as far as this program is consired.
				   * (See linux-howtos/Keyboard-and-Console-HOWTO for more information.)  */
		break;
		
	      default: /* Other keys do nothing */
		break;
	      }
	  break;
	  
	case KT_SHIFT:  /* A modifier is used, we took care about this before the switch, so this should not happen */
	case KT_CONS:   /* Somepony is trying to switch VT. Fat chance! */
	case KT_LOCK:   /* TODO: Is this sticky keys that toggle? */
	case KT_SLOCK:  /* TODO: Is this sticky keys that resemble dead keys? */
	case KT_BRL:    /* TODO: Braille, how does this work? */
	default:        /* What?! This should not happen! */
	  break;
	}
    }
}

