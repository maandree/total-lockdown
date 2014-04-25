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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <linux/kd.h>
#include <inttypes.h>

# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-pedantic"
#include "layout.c" /* When building, the user must do `loadkeys -C THE_USED_TTY -m THE_PREFERED_LAYOUT > src/layout.c`.
		     * It may be possible to look for the first /dev/tty* owned by $USER and get the keyboard from KEYMAP
		     * in rc.conf or vconsole.conf. */
# pragma GCC diagnostic pop


void fdputucs(int fd, int32_t c);
void fdprint(int fd, const char* str);
void verifier(int fd);


const char* KVAL_MAP[15][256] = {
  [KT_LATIN]  = {NULL},
  [KT_LETTER] = {NULL},
  [KT_FN]     = {NULL /* There are 256 keys here, see <linux/keyboard.h> if you need any of them. */},
  [KT_SPEC]   = {[KVAL(K_HOLE)] = NULL /* No assignment */,
		 [KVAL(K_ENTER)] = "\n",
		 [KVAL(K_SH_REGS)] = NULL /* TODO: What is this? */,
		 [KVAL(K_SH_MEM)] = NULL /* TODO: What is this? */,
		 [KVAL(K_SH_STAT)] = NULL /* TODO: What is this? */,
		 [KVAL(K_BREAK)] = NULL /* Break (pause), no output */,
		 [KVAL(K_CONS)] = NULL /* TODO: What is this? */,
		 [KVAL(K_CAPS)] = NULL /* TODO: What is this? */,
		 [KVAL(K_NUM)] = NULL /* TODO: What is this? */,
		 [KVAL(K_HOLD)] = NULL /* TODO: What is this? */,
		 [KVAL(K_SCROLLFORW)] = NULL /* TODO: What is this? */,
		 [KVAL(K_SCROLLBACK)] = NULL /* TODO: What is this? */,
		 [KVAL(K_BOOT)] = NULL /* TODO: What is this? */,
		 [KVAL(K_CAPSON)] = NULL /* TODO: What is this? */,
		 [KVAL(K_COMPOSE)] = NULL /* Compose key (TODO) */,
		 [KVAL(K_SAK)] = NULL /* TODO: What is this? */,
		 [KVAL(K_DECRCONSOLE)] = NULL /* Switch to previous VT. Fat chance! */,
		 [KVAL(K_INCRCONSOLE)] = NULL /* Switch to next VT. Fat chance! */,
		 [KVAL(K_SPAWNCONSOLE)] = NULL /* Spawn and switch to a new VT. Fat chance! */,
		 [KVAL(K_BARENUMLOCK)] = NULL /* TODO: What is this? */,
		 [KVAL(K_ALLOCATED)] = NULL /* do nothing (see <linux/keyboard.h> for more information) */,
		 [KVAL(K_NOSUCHMAP)] = NULL /* do nothing (see <linux/keyboard.h> for more information) */},
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
		 [KVAL(K_PPLUS)] = "+",
		 [KVAL(K_PMINUS)] = "-",
		 [KVAL(K_PSTAR)] = "*",
		 [KVAL(K_PSLASH)] = "/",
		 [KVAL(K_PENTER)] = "\n",
		 [KVAL(K_PCOMMA)] = ",",
		 [KVAL(K_PDOT)] = ".",
		 [KVAL(K_PPLUSMINUS)] = "±",
		 [KVAL(K_PPARENL)] = "(",
		 [KVAL(K_PPARENR)] = ")"},
  [KT_DEAD]   = {[KVAL(K_DGRAVE)] = NULL,
		 [KVAL(K_DACUTE)] = NULL,
		 [KVAL(K_DCIRCM)] = NULL,
		 [KVAL(K_DTILDE)] = NULL,
		 [KVAL(K_DDIERE)] = NULL,
		 [KVAL(K_DCEDIL)] = NULL},
  [KT_CONS]   = {NULL},
  [KT_CUR]    = {[KVAL(K_DOWN)] = "\033[B",
		 [KVAL(K_LEFT)] = "\033[D",
		 [KVAL(K_RIGHT)] = "\033[C",
		 [KVAL(K_UP)] = "\033[A"},
  [KT_SHIFT]  = {[KVAL(K_SHIFT)] = NULL,
		 [KVAL(K_CTRL)] = NULL,
		 [KVAL(K_ALT)] = NULL,
		 [KVAL(K_ALTGR)] = NULL,
		 [KVAL(K_SHIFTL)] = NULL,
		 [KVAL(K_SHIFTR)] = NULL,
		 [KVAL(K_CTRLL)] = NULL,
		 [KVAL(K_CTRLR)] = NULL,
		 [KVAL(K_CAPSSHIFT)] = NULL},
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
		 [KVAL(K_HEXf)] = NULL},
  [KT_LOCK]   = {[KVAL(K_SHIFTLOCK)] = NULL,
		 [KVAL(K_CTRLLOCK)] = NULL,
		 [KVAL(K_ALTLOCK)] = NULL,
		 [KVAL(K_ALTGRLOCK)] = NULL,
		 [KVAL(K_SHIFTLLOCK)] = NULL,
		 [KVAL(K_SHIFTRLOCK)] = NULL,
		 [KVAL(K_CTRLLLOCK)] = NULL,
		 [KVAL(K_CTRLRLOCK)] = NULL,
		 [KVAL(K_CAPSSHIFTLOCK)] = NULL},
  [KT_SLOCK]  = {[KVAL(K_SHIFT_SLOCK)] = NULL,
		 [KVAL(K_CTRL_SLOCK)] = NULL,
		 [KVAL(K_ALT_SLOCK)] = NULL,
		 [KVAL(K_ALTGR_SLOCK)] = NULL,
		 [KVAL(K_SHIFTL_SLOCK)] = NULL,
		 [KVAL(K_SHIFTR_SLOCK)] = NULL,
		 [KVAL(K_CTRLL_SLOCK)] = NULL,
		 [KVAL(K_CTRLR_SLOCK)] = NULL,
		 [KVAL(K_CAPSSHIFT_SLOCK)] = NULL},
  [KT_DEAD2]  = {NULL},
  [KT_BRL]    = {[KVAL(K_BRL_BLANK)] = NULL,
		 [KVAL(K_BRL_DOT1)] = NULL,
		 [KVAL(K_BRL_DOT2)] = NULL,
		 [KVAL(K_BRL_DOT3)] = NULL,
		 [KVAL(K_BRL_DOT4)] = NULL,
		 [KVAL(K_BRL_DOT5)] = NULL,
		 [KVAL(K_BRL_DOT6)] = NULL,
		 [KVAL(K_BRL_DOT7)] = NULL,
		 [KVAL(K_BRL_DOT8)] = NULL,
		 [KVAL(K_BRL_DOT9)] = NULL,
		 [KVAL(K_BRL_DOT10)] = NULL}
};


int main(int argc, char** argv)
{
  struct termios stty;
  struct termios saved_stty;
  int saved_kbd_mode;
  pid_t pid;
  
  printf("\033[H\033[2J\033[3J"); /* \e[3J should (but will probably not) erase the scrollback */
  fflush(stdout);
  tcgetattr(STDIN_FILENO, &saved_stty);
  stty = saved_stty;
  stty.c_lflag &= (tcflag_t)~(ECHO | ICANON | ISIG);
  stty.c_iflag = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty);
  ioctl(STDIN_FILENO, KDGKBMODE, &saved_kbd_mode);
  ioctl(STDIN_FILENO, KDSKBMODE, K_MEDIUMRAW); /* Now we have full access to the keyboard, the
						* intruder cannot change TTY, but we need to
					        * implement RESTRICTED kernel keyboard support. */
  
  if ((pid = fork()) == (pid_t)-1)
    return 10; /* We do not use vfork, since we want to be absolutely
		* sure that the saved settings are not modified by a
		* memory fault. That could lock the keyboard and force
		* manual reboot via physical button. (Or an too elaborate
		* reset over SSH.) */
  
  if (pid)
    waitpid(pid, NULL, 0);
  else
    {
      int modifiers = 0;
      int fd, fd_child;
      int fds_pipe[2];
      
      if (pipe(fds_pipe))
	abort();
      
      if ((pid = fork()) == (pid_t)-1)
	abort();
      
      fd = fds_pipe[1];
      fd_child = fds_pipe[0];
      
      if (!pid)
	verifier(fd_child);
      
      alarm(10); /* while testing, we are aborting after 60 seconds, you can also quit with 'q' */
      printf("\n    Enter passphrase: ");
      fflush(stdout);
      
      for (;;)
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
	  
	  fd = STDOUT_FILENO; /* This is just testing. */
	  
	  /* /usr/include/linux/keyboard.h is interresting here... */
	  switch (KTYP(c))
	    {
	    case KT_LETTER: /* Symbols that are affected by the Royal Canterlot Voice key */
	    case KT_LATIN:  /* Symbols that are not affected by the Royal Canterlot Voice key */
	      if (KVAL(c) == 'q')
		goto stop;
	      fdputucs(fd, KVAL(c) & 255);
	      break;
	      
	    case KT_META:   /* Just like KT_LATIN, except with meta modifier */
	      fdputucs(fd, '\033'); /* We will assume this mode rather than set 8:th bit-mode */
	      fdputucs(fd, KVAL(c) & 255);
	      break;
	      
	    case KT_FN:     /* Customisable keys, usally for escape sequnces. Includes F-keys and some misc. keys */
	      fdprint(fd, func_table[KVAL(c)]);
	      break;
	      
	    case KT_SHIFT:  /* A modifier is used */
	      break;
	      
	    case KT_DEAD:   /* Dead key, can we utilise Linux for this? */
	      break;
	      
	    case KT_SPEC:   /* Special keys*/
	    case KT_PAD:    /* Keypad */
	    case KT_CUR:    /* Arrows keys */
	    case KT_ASCII:  /* This is what happens when somepony holds down Alternative whil using the keypad */
	      if (KVAL_MAP[KTYP(c)][KVAL(c)] != NULL)
		fdprint(fd, KVAL_MAP[KTYP(c)][KVAL(c)]);
	      break;
	      
	    case KT_CONS:   /* Somepony is trying to switch VT. Fat chance! */
	    case KT_LOCK:   /* TODO: What is this? */
	    case KT_SLOCK:  /* TODO: What is this? */
	    case KT_DEAD2:  /* TODO: What is this? */
	    case KT_BRL:    /* Braille, how does this work? */
	      break;
	      
	    default:        /* What?! This should not happen! */
	      break;
	    }
	}
    stop:;
    }
  
  ioctl(STDIN_FILENO, KDSKBMODE, saved_kbd_mode);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
  printf("\033[H\033[2J");
  fflush(stdout);
  
  return (pid == (pid_t)-1) ? 10 : 0;
}


void fdputucs(int fd, int32_t c)
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


void fdprint(int fd, const char* str)
{
  printf("%s", str); /* TODO print to fd */
}


void verifier(int fd)
{
  close(STDIN_FILENO);
  dup2(fd, STDIN_FILENO);
}

