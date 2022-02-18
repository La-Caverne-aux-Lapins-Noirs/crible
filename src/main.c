/*
** Jason Brillante "Damdoshi"
** La Caverne aux Lapins Noirs 2014-2022
**
** Crible
*/

#include		"crible.h"

t_bunny_response	message(int			fd,
				const void		*buf,
				size_t			siz,
				void			*data)
{
  char			buffer[siz];
  t_crible		*crible = data;

  if (crible->drop_next)
    {
      if (crible->client->fd)
	fprintf(stderr, "The proxy target sent a message and it was dropped.\n");
      else
	fprintf(stderr, "The proxy client sent a message and it was dropped.\n");
      crible->drop_next = false;
      return (GO_ON);
    }
  if (crible->alt_next)
    {
      if (crible->client->fd)
	fprintf(stderr, "The proxy target sent a message and it was alterated.\n");
      else
	fprintf(stderr, "The proxy client sent a message and it was alterated.\n");
      int		damages = rand() % (siz - 1) + 1;
      int		i;

      crible->alt_next = false;
      memcpy(buffer, buf, siz);
      for (i = 0; i < damages; ++i)
	buffer[rand() % siz] = rand();
      buf = &buffer[0];
    }
  if (fd == crible->client->fd)
    { // La cible du proxy a parlé
      if (bunny_server_write(crible->server, buf, siz, crible->single_client) == false)
	{
	  fprintf(stderr, "Cannot transfert message from proxy target to proxy client.\n");
	  return (EXIT_ON_ERROR);
	}
      fprintf(stderr, "The proxy target message was sent to the proxy client.\n");
    }
  else
    { // Un client du proxy a parlé
      if (bunny_client_write(crible->client, buf, siz) == false)
	{
	  fprintf(stderr, "Cannot transfert message from proxy client to proxy target.\n");
	  return (EXIT_ON_ERROR);
	}
      fprintf(stderr, "The proxy client message was sent to the proxy message.\n");
    }
  return (GO_ON);
}

t_bunny_response	nconnect(int			fd,
				 t_bunny_event_state	state,
				 void			*data)
{
  t_crible		*crible = data;

  if (state == CONNECTED)
    {
      if (crible->single_client == 0)
	{
	  fprintf(stdout, "Proxy client have connected.\n");
	  crible->single_client = fd;
	}
    }
  else
    {
      if (crible->client->fd == fd)
	{
	  fprintf(stderr, "Proxy target have disconnected.\n");
	  return (EXIT_ON_SUCCESS);
	}
      if (crible->single_client == fd)
	{
	  fprintf(stdout, "Proxy client have disconnected.\n");
	  crible->single_client = 0;
	}
    }
  return (GO_ON);
}

t_bunny_response	loop(void			*data)
{
  t_crible		*crible = data;

  if (crible->close_freq >= 0)
    if ((crible->close_delay += bunny_get_current_time()) > crible->close_freq)
      {
	crible->close_delay = 0;
	if (crible->single_client)
	  {
	    fprintf(stderr, "Closing client as perturbation.\n");
	    bunny_server_doom_client(crible->server, crible->single_client);
	  }
      }
  if (crible->alt_delay >= 0)
    if ((crible->alt_delay += bunny_get_current_time()) > crible->alterate_message)
      {
	crible->alt_delay = 0;
	if (crible->alt_next == false)
	  fprintf(stderr, "Alterating communication as perturbation loaded.\n");
	crible->alt_next = true;
      }
  if (crible->drop_delay >= 0)
    if ((crible->drop_delay += bunny_get_current_time()) > crible->drop_message)
      {
	crible->drop_delay = 0;
	if (crible->drop_next == false)
	  fprintf(stderr, "Dropping message as perturbation loaded.\n");
	crible->drop_next = true;
      }
  const t_bunny_communication *com = bunny_client_poll(crible->client, 1000 * bunny_get_delay() / 2.0);

  if (com->comtype == BCT_MESSAGE)
    if (message(com->message.fd, com->message.buffer, com->message.size, crible) != GO_ON)
      return (EXIT_ON_ERROR);
  if (com->comtype == BCT_NETDISCONNECTED)
    if (nconnect(com->connected.fd, DISCONNECTED, crible) != GO_ON)
      return (EXIT_ON_ERROR);

  com = bunny_server_poll(crible->server, 1000 * bunny_get_delay() / 2.0);
  if (com->comtype == BCT_MESSAGE)
    if (message(com->message.fd, com->message.buffer, com->message.size, crible) != GO_ON)
      return (EXIT_ON_ERROR);
  if (com->comtype == BCT_NETCONNECTED)
    if (nconnect(com->connected.fd, CONNECTED, crible) != GO_ON)
      return (EXIT_ON_ERROR);
  if (com->comtype == BCT_NETDISCONNECTED)
    if (nconnect(com->connected.fd, DISCONNECTED, crible) != GO_ON)
      return (EXIT_ON_ERROR);
  return (GO_ON);
}

int			main(int			argc,
			     char			**argv)
{
  static t_crible	crible;
  double		*freq;
  int			i;

  if (argc < 4)
    {
      puts("Usage is:\n"
	   "\t./crible target_ip target_port listen_port [options]*\n\n"
	   "\tOptions are:\n"
	   "\t\t-c frequency\tClose client at given frequency\n"
	   "\t\t-a frequency\tAlterate client message at given frequency\n"
	   "\t\t-d frequency\tDrop client message at given frequency\n"
	   );
      return (EXIT_FAILURE);
    }
  if ((crible.client = bunny_new_client(argv[1], atoi(argv[2]))) == NULL)
    {
      fprintf(stderr, "Cannot link with proxy target on %s %s.\n", argv[1], argv[2]);
      return (EXIT_FAILURE);
    }
  if ((crible.server = bunny_new_server(atoi(argv[3]))) == NULL)
    {
      fprintf(stderr, "Cannot open proxy server on %s.\n", argv[3]);
      return (EXIT_FAILURE);
    }
  crible.close_freq = -1;
  crible.alterate_message = -1;
  crible.drop_message = -1;
  for (i = 4; i < argc; ++i)
    {
      if (strcmp(argv[i], "-c") == 0)
	freq = &crible.close_freq;
      else if (strcmp(argv[i], "-a") == 0)
	freq = &crible.alterate_message;
      else if (strcmp(argv[i], "-d") == 0)
	freq = &crible.drop_message;
      else
	continue ;
      if (fabs(atof(argv[++i])) > 0.0001)
	*freq = 1.0 / (atof(argv[i]));
    }

  bunny_set_loop_main_function(loop);
  bunny_loop(NULL, 100, &crible);

  bunny_delete_server(crible.server);
  bunny_delete_client(crible.client);
  return (EXIT_SUCCESS);
}
