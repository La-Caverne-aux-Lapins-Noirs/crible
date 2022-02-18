/*
** Jason Brillante "Damdoshi"
** La Caverne aux Lapins Noirs 2014-2022
**
** Crible
*/

#ifndef			__CRIBLE_H__
# define		__CRIBLE_H__
# include		<lapin.h>

typedef struct		s_crible
{
  t_bunny_client	*client;
  t_bunny_server	*server;
  int			single_client;
  double	        close_freq;
  double		close_delay;
  double		alterate_message;
  double		alt_delay;
  bool			alt_next;
  double		drop_message;
  double		drop_delay;
  bool			drop_next;
}			t_crible;

#endif	/*		__CRIBLE_H__		*/
