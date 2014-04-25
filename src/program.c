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
void fdprintf(int fd, char* str, ...);
void verifier(int fd);


const char* KTYP_MAP[] = {
  [KT_LATIN]  = "KT_LATIN",
  [KT_FN]     = "KT_FN",
  [KT_SPEC]   = "KT_SPEC",
  [KT_PAD]    = "KT_PAD",
  [KT_DEAD]   = "KT_DEAD",
  [KT_CONS]   = "KT_CONS",
  [KT_CUR]    = "KT_CUR",
  [KT_SHIFT]  = "KT_SHIFT",
  [KT_META]   = "KT_META",
  [KT_ASCII]  = "KT_ASCII",
  [KT_LOCK]   = "KT_LOCK",
  [KT_LETTER] = "KT_LETTER",
  [KT_SLOCK]  = "KT_SLOCK",
  [KT_DEAD2]  = "KT_DEAD2",
  [KT_BRL]    = "KT_BRL"
};

const char* KVAL_MAP[15][256] = {
  [KT_LATIN]  = {NULL /* normal symbol */},
  [KT_FN]     = {"K_F1", "K_F2", "K_F3", "K_F4", "K_F5", "K_F6", "K_F7", "K_F8", "K_F9", "K_F10", "K_F11",
		 "K_F12", "K_F13", "K_F14", "K_F15", "K_F16", "K_F17", "K_F18", "K_F19", "K_F20", "K_FIND",
		 "K_INSERT", "K_REMOVE", "K_SELECT", "K_PGUP", "K_PGDN", "K_MACRO", "K_HELP", "K_DO",
		 "K_PAUSE", "K_F21", "K_F22", "K_F23", "K_F24", "K_F25", "K_F26", "K_F27", "K_F28", "K_F29",
		 "K_F30", "K_F31", "K_F32", "K_F33", "K_F34", "K_F35", "K_F36", "K_F37", "K_F38", "K_F39",
		 "K_F40", "K_F41", "K_F42", "K_F43", "K_F44", "K_F45", "K_F46", "K_F47", "K_F48", "K_F49",
		 "K_F50", "K_F51", "K_F52", "K_F53", "K_F54", "K_F55", "K_F56", "K_F57", "K_F58", "K_F59",
		 "K_F60", "K_F61", "K_F62", "K_F63", "K_F64", "K_F65", "K_F66", "K_F67", "K_F68", "K_F69",
		 "K_F70", "K_F71", "K_F72", "K_F73", "K_F74", "K_F75", "K_F76", "K_F77", "K_F78", "K_F79",
		 "K_F80", "K_F81", "K_F82", "K_F83", "K_F84", "K_F85", "K_F86", "K_F87", "K_F88", "K_F89",
		 "K_F90", "K_F91", "K_F92", "K_F93", "K_F94", "K_F95", "K_F96", "K_F97", "K_F98", "K_F99",
		 "K_F100", "K_F101", "K_F102", "K_F103", "K_F104", "K_F105", "K_F106", "K_F107", "K_F108",
		 "K_F109", "K_F110", "K_F111", "K_F112", "K_F113", "K_F114", "K_F115", "K_F116", "K_F117",
		 "K_F118", "K_F119", "K_F120", "K_F121", "K_F122", "K_F123", "K_F124", "K_F125", "K_F126",
		 "K_F127", "K_F128", "K_F129", "K_F130", "K_F131", "K_F132", "K_F133", "K_F134", "K_F135",
		 "K_F136", "K_F137", "K_F138", "K_F139", "K_F140", "K_F141", "K_F142", "K_F143", "K_F144",
		 "K_F145", "K_F146", "K_F147", "K_F148", "K_F149", "K_F150", "K_F151", "K_F152", "K_F153",
		 "K_F154", "K_F155", "K_F156", "K_F157", "K_F158", "K_F159", "K_F160", "K_F161", "K_F162",
		 "K_F163", "K_F164", "K_F165", "K_F166", "K_F167", "K_F168", "K_F169", "K_F170", "K_F171",
		 "K_F172", "K_F173", "K_F174", "K_F175", "K_F176", "K_F177", "K_F178", "K_F179", "K_F180",
		 "K_F181", "K_F182", "K_F183", "K_F184", "K_F185", "K_F186", "K_F187", "K_F188", "K_F189",
		 "K_F190", "K_F191", "K_F192", "K_F193", "K_F194", "K_F195", "K_F196", "K_F197", "K_F198",
		 "K_F199", "K_F200", "K_F201", "K_F202", "K_F203", "K_F204", "K_F205", "K_F206", "K_F207",
		 "K_F208", "K_F209", "K_F210", "K_F211", "K_F212", "K_F213", "K_F214", "K_F215", "K_F216",
		 "K_F217", "K_F218", "K_F219", "K_F220", "K_F221", "K_F222", "K_F223", "K_F224", "K_F225",
		 "K_F226", "K_F227", "K_F228", "K_F229", "K_F230", "K_F231", "K_F232", "K_F233", "K_F234",
		 "K_F235", "K_F236", "K_F237", "K_F238", "K_F239", "K_F240", "K_F241", "K_F242", "K_F243",
		 "K_F244", "K_F245", "K_UNDO"},
  [KT_SPEC]   = {"K_HOLE", "K_ENTER", "K_SH_REGS", "K_SH_MEM", "K_SH_STAT", "K_BREAK", "K_CONS",
		 "K_CAPS", "K_NUM", "K_HOLD", "K_SCROLLFORW", "K_SCROLLBACK", "K_BOOT", "K_CAPSON",
		 "K_COMPOSE", "K_SAK", "K_DECRCONSOLE", "K_INCRCONSOLE", "K_SPAWNCONSOLE",
		 "K_BARENUMLOCK", [126] = "K_ALLOCATED", [127] = "K_NOSUCHMAP"},
  [KT_PAD]    = {"K_P0", "K_P1", "K_P2", "K_P3", "K_P4", "K_P5", "K_P6", "K_P7", "K_P8", "K_P9",
		 "K_PPLUS", "K_PMINUS", "K_PSTAR", "K_PSLASH", "K_PENTER", "K_PCOMMA", "K_PDOT",
		 "K_PPLUSMINUS", "K_PPARENL", "K_PPARENR"},
  [KT_DEAD]   = {"K_DGRAVE", "K_DACUTE", "K_DCIRCM", "K_DTILDE", "K_DDIERE", "K_DCEDIL"},
  [KT_CONS]   = {NULL /* Switch T */},
  [KT_CUR]    = {"K_DOWN", "K_LEFT", "K_RIGHT", "K_UP"},
  [KT_SHIFT]  = {"K_SHIFT", "K_CTRL", "K_ALT", "K_ALTGR", "K_SHIFTL", "K_SHIFTR", "K_CTRLL",
		 "K_CTRLR", "K_CAPSSHIFT"},
  [KT_META]   = {NULL /* ESC + normal symbol */},
  [KT_ASCII]  = {"K_ASC0", "K_ASC1", "K_ASC2", "K_ASC3", "K_ASC4", "K_ASC5", "K_ASC6", "K_ASC7",
		 "K_ASC8", "K_ASC9", "K_HEX0", "K_HEX1", "K_HEX2", "K_HEX3", "K_HEX4", "K_HEX5",
		 "K_HEX6", "K_HEX7", "K_HEX8", "K_HEX9", "K_HEXa", "K_HEXb", "K_HEXc", "K_HEXd",
		 "K_HEXe", "K_HEXf"}, /* Altgr + Keypad */
  [KT_LOCK]   = {"K_SHIFTLOCK", "K_CTRLLOCK", "K_ALTLOCK", "K_ALTGRLOCK", "K_SHIFTLLOCK",
		 "K_SHIFTRLOCK", "K_CTRLLLOCK", "K_CTRLRLOCK", "K_CAPSSHIFTLOCK" /* not used? */},
  [KT_LETTER] = {NULL /* Caps lockable symbol */},
  [KT_SLOCK]  = {"K_SHIFT_SLOCK", "K_CTRL_SLOCK", "K_ALT_SLOCK", "K_ALTGR_SLOCK", "K_SHIFTL_SLOCK",
		 "K_SHIFTR_SLOCK", "K_CTRLL_SLOCK", "K_CTRLR_SLOCK", "K_CAPSSHIFT_SLOCK" /* not used? */},
  [KT_DEAD2]  = {NULL /* not used? */},
  [KT_BRL]    = {"K_BRL_BLANK", "K_BRL_DOT1", "K_BRL_DOT2", "K_BRL_DOT3", "K_BRL_DOT4", "K_BRL_DOT5",
		 "K_BRL_DOT6", "K_BRL_DOT7", "K_BRL_DOT8", "K_BRL_DOT9", "K_BRL_DOT10"}
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
      
      alarm(60); /* while testing, we are aborting after 60 seconds, you can also quit with 'q' */
      printf("\n    Enter passphrase: ");
      fflush(stdout);
      
      for (;;)
	{
	  int c = getchar();
	  int released = !!(c & 0x80);
	  printf("%i  ::  0x%x  ::  ", c & 0x7F, key_maps[0][c & 0x7F]);
	  
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
	  if ((((KTYP(c) == KT_LETTER) || (KTYP(c) == KT_LATIN))) && (KVAL(c) == 'q'))
	    break;
	  else if (KVAL_MAP[KTYP(c)][KVAL(c)] != NULL)
	    printf("<%s, %s>\n", KTYP_MAP[KTYP(c)], KVAL_MAP[KTYP(c)][KVAL(c)]);
	  else
	    {
	      printf("%s  ", KTYP_MAP[KTYP(c)]);
	  if (c == K_COMPOSE)
	    printf("    (compose)\n"); /* how do we use compose key? */
	  else if (KTYP(c) == KT_DEAD)
	    printf("    (dead)...\n"); /* how do we use dead keys? */
	  else if (c == K_ENTER)      fdputucs(fd, 10);
	  else if (c == K_DOWN)	      fdprintf(fd, "\033[B");
	  else if (c == K_LEFT)	      fdprintf(fd, "\033[D");
	  else if (c == K_RIGHT)      fdprintf(fd, "\033[C");
	  else if (c == K_UP)	      fdprintf(fd, "\033[A");
	  else if (KTYP(c) == KT_FN)  fdprintf(fd, func_table[KVAL(c)]);
	  else if ((KTYP(c) == KT_LETTER) || (KTYP(c) == KT_LATIN))
	    if (KVAL(c) == 'q')
	      break;
	    else
	      fdputucs(fd, KVAL(c) & 255);
	  else if (c != K_HOLE)
	    printf("    (%i %i)\n", KTYP(c), KVAL(c));
	    }
	}
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
    fdprintf(fd, "%c", (int)c);
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
      fdprintf(fd, "%s", ucs_buffer + off);
    }
}


void fdprintf(int fd, char* format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args); /* TODO print to fd */
  va_end(args);
}


void verifier(int fd)
{
  close(STDIN_FILENO);
  dup2(fd, STDIN_FILENO);
}

