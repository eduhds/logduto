/**
 * req_save
 * Salva requisições em arquivo .txt
 *
 * https://mongoose.ws/documentation
 */

#include <stdio.h>
#include "mongoose.h"

#define BASE_URL "http://0.0.0.0:8181"

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_http_match_uri(hm, "/save"))
    {
      char *strName = mg_json_get_str(hm->body, "$.name");
      char *strData = mg_json_get_str(hm->body, "$.data");

      char *fileName = malloc(strlen(strName) + 1 + 4);
      strcpy(fileName, strName);
      strcat(fileName, ".txt");

      FILE *file = fopen(fileName, "w");

      fprintf(file, "%s\n", strData);

      fclose(file);

      printf("• Request saved to \"%s\".\n", fileName);

      free(fileName);
      free(strName);
      free(strData);

      mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{%m:%d}\n", MG_ESC("status"), 1);
    }
    else
    {
      struct mg_http_serve_opts opts = {.root_dir = "."}; // Serve local dir
      mg_http_serve_dir(c, ev_data, &opts);
    }
  }
}

int main(int argc, char *argv[])
{
  printf("Runing at %s...\n", BASE_URL);

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);                        // Init manager
  mg_http_listen(&mgr, BASE_URL, fn, &mgr); // Setup listener

  for (;;)
  {
    mg_mgr_poll(&mgr, 1000); // Event loop
  }

  mg_mgr_free(&mgr); // Cleanup

  return 0;
}
