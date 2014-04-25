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
void fdprint(int fd, char* str);
void verifier(int fd);


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
		* manual reboot via physical button. */
  
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
	  
	  /* /usr/including/linux/keyboard.h is interresting here... */
	  if (c == K_COMPOSE)
	    printf("    (compose)\n"); /* how do we use compose key? */
	  else if (KTYP(c) == KT_DEAD)
	    printf("    (dead)...\n"); /* how do we use dead keys? */
	  else if (c == K_ENTER)      fdputucs(fd, 10);
	  else if (c == K_DOWN)	      fdprint(fd, "\033[B");
	  else if (c == K_LEFT)	      fdprint(fd, "\033[D");
	  else if (c == K_RIGHT)      fdprint(fd, "\033[C");
	  else if (c == K_UP)	      fdprint(fd, "\033[A");
	  else if (KTYP(c) == KT_FN)  fdprint(fd, func_table[KVAL(c)]);
	  else if ((KTYP(c) == KT_LETTER) || (KTYP(c) == KT_LATIN))
	    if (KVAL(c) == 'q')
	      break;
	    else
	      fdputucs(fd, KVAL(c) & 255);
	  else if (c != K_HOLE)
	    printf("    (%i %i)\n", KTYP(c), KVAL(c));
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
    putchar(c);
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
      printf("%s", ucs_buffer + off);
      fflush(stdout);
    }
}


void fdprint(int fd, char* str)
{
}


void verifier(int fd)
{
  close(STDIN_FILENO);
  dup2(fd, STDIN_FILENO);
}

