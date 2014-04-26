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
#define _XOPEN_SOURCE
#define _GNU_SOURCE

#include "security.h"

#ifndef NO_SHADOW
# ifndef HAVE_SHADOW
#  define HAVE_SHADOW
# endif
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <crypt.h>
#include <sys/types.h>
#ifdef HAVE_SHADOW
# include <shadow.h>
#else
# warning: compiling without shadow file support
#endif



/**
 * Get the real user's password entry in /etc/shadow or /etc/passwd,
 * also do some privilege checks
 * 
 * @return  The real user's password encrypted
 */
char* getcrypt(void)
{
#ifdef HAVE_SHADOW
  struct spwd* spwd;
#endif
  struct passwd* pwd;
  struct group* grp;
  char* name;
  char* crypted;
  
  /* get information about the user */
  pwd = getpwuid(getuid());
  if ((pwd == NULL) || ((name = pwd->pw_name) == NULL))
    {
      fprintf(stderr, "You do not exist, go away!\n");
      return NULL;
    }
  
  /* drop privileges, we have not use for them anymore */
  seteuid(getuid());
  setegid(getgid());
  
  /* if the herd 'lockdown' exists, check that the user is a member of it */
  grp = getgrnam("lockdown");
  if (grp != NULL)
    {
      gid_t lockdown_gid = grp->gr_gid;
      char** lockdown_members = grp->gr_mem;
      int authed = lockdown_gid == getgid(); /* test primary herd (does not really belong here, but anyway) */
      authed |= lockdown_gid == getegid(); /* do not care if setgid it used the herd is set to lockdown */
      
      /* check members of the herd, the user might have been give access to it while logged in */
      if (authed == 0)
	while (*lockdown_members != NULL)
	  {
	    if ((authed = !strcmp(*lockdown_members++, name)))
	      break;
	  }
      
      /* check the user's supplemental herd list, we assume that it has be been removed, but
       * that the user's herd privileges can have been escalated. */
      if (authed == 0)
	{
	  gid_t* groups;
	  int groups_n;
	  long int groups_max;
	  int i;
	  
	  groups_max = sysconf(_SC_NGROUPS_MAX) + 1;
	  groups = malloc((size_t)groups_max * sizeof(gid_t));
	  if (groups == NULL)
	    {
	      perror("total-lockdown");
	      return NULL;
	    }
	  groups_n = getgroups((int)groups_max, groups);
	  if (groups_n < 0)
	    {
	      perror("total-lockdown");
	      return NULL;
	    }
	  
	  for (i = 0; i < groups_n; i++)
	    if ((authed = (*(groups + i) == lockdown_gid)))
	      break;
	}
      
      if (authed == 0)
	{
	  fprintf(stderr, "You are not authorised!\n");
	  return NULL;
	}
    }
  
#ifdef HAVE_SHADOW
  spwd = getspnam(pwd->pw_name);
  endspent();
  if (spwd != NULL)
    {
      crypted = spwd->sp_pwdp;
      if (crypted == NULL)
	crypted = pwd->pw_passwd;
    }
  else
    {
#endif
      crypted = pwd->pw_passwd;
#ifdef HAVE_SHADOW
      if (crypted && *crypted && (strchr(crypted, '$') == NULL))
	if ((errno == EACCES) && (geteuid() != 0))
	  crypted = NULL;
    }
#endif
  
  if (crypted == NULL)
    {
      perror("total-lockdown");
      return NULL;
    }
  
  if (*crypted == '\0')
    {
      fprintf(stderr, "You do not even have a passphrase!\n");
      return NULL;
    }
  
  if (strchr(crypted, '$') == NULL)
    {
      fprintf(stderr, "You are not a real user, go away!\n");
      return NULL;
    }
  
  if (crypt("", crypted) == NULL)
    {
      perror("total-lockdown");
      return NULL;
    }
  
  return crypted;
}


/**
 * get the real user's real name and fall back to username
 * 
 * @return  The real user's name
 */
char* getname(void)
{
  struct passwd* pwd = getpwuid(getuid());
  char* name = pwd->pw_gecos ? pwd->pw_gecos : pwd->pw_name;
  if (name)
    {
      *(strchrnul(name, ',')) = '\0';
      if (strlen(name) == 0)
	name = pwd->pw_name;
    }
  return name == NULL ? NULL : strdup(name);
}

