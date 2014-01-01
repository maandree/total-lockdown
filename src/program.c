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
#include <linux/kd.h>


#include "layout.c"


int main(int argc, char** argv)
{
  struct termios stty;
  struct termios saved_stty;
  int saved_kbd_mode;
  
  printf("\033[H\033[2J\033[3J");
  fflush(stdout);
  tcgetattr(STDIN_FILENO, &saved_stty);
  stty = saved_stty;
  stty.c_lflag &= ~(ECHO | ICANON | ISIG);
  stty.c_iflag = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty);
  ioctl(STDIN_FILENO, KDGKBMODE, &saved_kbd_mode);
  ioctl(STDIN_FILENO, KDSKBMODE, K_MEDIUMRAW);
  
  pid_t pid = fork();
  if (pid == (pid_t)-1)
    return 10;
  
  if (pid)
    waitpid(pid, NULL, 0);
  else
    {
      int modifiers = 0;
      
      alarm(10); /* while testing, we are aborting after ten seconds*/
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
	  
	  if ((KTYP(c) == KT_LATIN) && (KVAL(c) == 127))
	    printf("    (backspace)\n");
	  else if (c == K_COMPOSE)
	    printf("    (compose)\n"); /* how do we use compose key? */
	  else if (c == K_ENTER)
	    printf("    (enter)\n");
	  else if (c == K_DOWN)
	    printf("    (down)\n");
	  else if (c == K_LEFT)
	    printf("    (left)\n");
	  else if (c == K_RIGHT)
	    printf("    (right)\n");
	  else if (c == K_UP)
	    printf("    (up)\n");
	  else if (KTYP(c) == KT_DEAD)
	    printf("    (dead)...\n"); /* how do we use dead keys? */
	  else if (KTYP(c) == KT_FN)
	    printf("    (fn)%s\n", func_table[KVAL(c)]);
	  else if (KTYP(c) == KT_LETTER)
	    printf("    (letter)%i\n", KVAL(c)); /* just convert from utf32 to utf8? */
	  else if (KTYP(c) == KT_LATIN)
	    printf("    (latin)%i\n", KVAL(c)); /* just convert from utf32 to utf8? */
	  else
	    printf("    (%i %li)\n", KTYP(c), KVAL(c));
	}
    }
  
  ioctl(STDIN_FILENO, KDSKBMODE, saved_kbd_mode);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
  printf("\033[H\033[2J");
  fflush(stdout);
  
  return (pid == (pid_t)-1) ? 10 : 0;
}

