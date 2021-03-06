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
#include <inttypes.h>
#include <linux/kd.h>
#include <crypt.h>
#include <string.h>
#include <passphrase.h>

#include "security.h"
#include "kbddriver.h"


#if defined(EBUG) && !defined(DEBUG)
# define DEBUG
#endif
#if defined(DEBUG) && !defined(EBUG)
# define EBUG
#endif
#ifndef DEBUG
# warning If you are compiling for testing consider compiling with -DEBUG to avoid locking your system.
#endif


int verifier(int fd, char* encrypted);


int main(int argc, char** argv)
{
  struct termios stty;
  struct termios saved_stty;
  int saved_kbd_mode;
  pid_t pid;
  char* tty;
  char* encrypted;
  char* name;
  
  /* verify that we are in a real VT, otherwise we cannot possibly lock it down */
  tty = ttyname(STDIN_FILENO);
  if (strstr(tty, "/dev/tty") != tty)
    {
      fprintf(stderr, "A Linux console is required (as stdin).\n");
      return 1;
    }
  
  /* get the real user's real name or username */
  name = getname();
  
  /* get the real user's encrypted passphrase */
  if ((encrypted = getcrypt()) == NULL)
    {
#ifndef DEBUG
      return 2;
#else
      encrypted = "$6$MWcK52I9$xKtRFG3JIRfuC80R/8fu3vDO6qPRy6IK6B8GsaA6n.HvdP8J3M9n0.nNc/ZcdkzHWApXCVsQBk4V.YGsmfkNv1";
      /* Passphrase is ‘ppp’ when testing without setuid permission, which is needed for valgrind. */
#endif
    }
  
  /* lock down */
#ifndef DEBUG
  printf("\033[H\033[2J\033[3J"); /* \e[3J should (but will probably not) erase the scrollback */
#endif
  fflush(stdout);
  tcgetattr(STDIN_FILENO, &saved_stty);
  stty = saved_stty;
  stty.c_lflag &= 0 /* (tcflag_t)~(ECHO | ICANON | ISIG) */;
  stty.c_iflag = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty);
  ioctl(STDIN_FILENO, KDGKBMODE, &saved_kbd_mode);
  ioctl(STDIN_FILENO, KDSKBMODE, K_MEDIUMRAW); /* Now we have full access to the keyboard, the
						* intruder cannot change TTY, but we need to
					        * implement RESTRICTED kernel keyboard support. */
  
  printf("\n");
 retry:
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
      int fd, fd_child;
      int fds_pipe[2];
      
      if (pipe(fds_pipe))
	abort();
      
      if ((pid = fork()) == (pid_t)-1)
	abort();
      
#ifdef DEBUG
      alarm(60); /* when testing, we are aborting after 60 seconds */
#endif
      
      fd = fds_pipe[1];
      fd_child = fds_pipe[0];
      
      if (!pid)
	return verifier(fd_child, encrypted);
      else
	{
	  int status;
	  
	  if (name == NULL)
	    printf("    Enter passphrase: ");
	  else
	    printf("    Enter passphrase for %s: ", name);
	  fflush(stdout);
	  
	  readkbd(fd);
	  waitpid(pid, &status, 0);
	  
	  close(fds_pipe[0]);
	  close(fds_pipe[1]);
	  
	  if (WIFEXITED(status) ? WEXITSTATUS(status) : 1)
	    goto retry;
	  return 0;
	}
    }
  
  /* unlock */
  ioctl(STDIN_FILENO, KDSKBMODE, saved_kbd_mode);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
#ifndef DEBUG
  printf("\033[H\033[2J");
#endif
  fflush(stdout);
  
  if (name)
    free(name);
  
  return (pid == (pid_t)-1) ? 10 : 0;
}


int verifier(int fd, char* encrypted)
{
  char* passphrase;
  char* passphrase_crypt;
  
  close(STDIN_FILENO);
  dup2(fd, STDIN_FILENO);
  
  passphrase = passphrase_read();
  passphrase_crypt = crypt(passphrase, encrypted);
  memset(passphrase, 0, strlen(passphrase)); /* wipe it! */
  free(passphrase);
  
  if (passphrase_crypt == NULL)
    {
      /* This should not happen */
      perror("total-lockdown");
      sleep(5);
      return 2;
    }
  
  if (!strcmp(passphrase_crypt, encrypted))
    return 0;
  
  sleep(3);
  return 1;
}

