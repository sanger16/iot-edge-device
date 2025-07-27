#include "edgelib/edgelib.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
TODO: Manage multiple commands associated with the variables list
TODO: Create config file for MQTT connection
TODO: Configure SSL connection with MQTT Broker
**
**
**
**
 */

/* MQTT parameter connection */
const char *MQTT_BROKER_URI =
    "";
const char *MQTT_CLIENT_ID = "";
const char *USERNAME = "";
const char *PASSWORD = "";
const int MQTT_PORT = 8883;

/* Modbus Header values */
uint trans_id = 0;
const int PROT_ID = 0x0000;
const int LEN = 0x0006;
/* Modbus protocol header params */
const int RETRY = 3;
const int MS_TIMEOUT = 1000;
/* Head/Tail of command list */
cmd_list *HEAD_CMD;
cmd_list *TAIL_CMD;
/* Head/Tail of variable list */
variable_list *HEAD_VAR;
variable_list *TAIL_VAR;

int main()
{
  /* Modbus Commands list definition */
  cmd_list *commands = malloc(sizeof(cmd_list));
  commands->next = NULL;
  commands->prev = NULL;
  HEAD_CMD = commands;
  TAIL_CMD = commands;
  if (commands == NULL)
  {
    /* Log event */
    log_events("Could not allocate memory for Command list... Exiting");
    return 1;
  }
  bool cmd_exists = false;
  /* Modbus Variables list definition */
  variable_list *mb_variable_list = malloc(sizeof(variable_list));
  mb_variable_list->next = NULL;
  mb_variable_list->prev = NULL;
  HEAD_VAR = mb_variable_list;
  TAIL_VAR = mb_variable_list;
  if (mb_variable_list == NULL)
  {
    /* Log event */
    log_events("Could not allocate memory for Variable list... Exiting");
    return 1;
  }
  bool var_exists = false;
  /* Read config from config.conf file */
  read_config(commands, mb_variable_list, &cmd_exists, &var_exists);

  /*
   *Check if commands exist
   */
  if (cmd_exists)
  {
    /* CONNECTION PORT NUMBER */
    int portno = commands->cmd.port_numb;
    /* IP ADDRESS OF THE SERVER TO CONNECT TO */
    char *server_ip;
    server_ip = commands->cmd.ip_addr;
    /*
    TODO: Ceate socket for all IP address defined in cmd list
     */
    /* CREATE SOCKET */
    int socketfd = createTCPIPv4Socket();
    /* IN CASE OF NEGATIVE VALUE, ERROR */
    if (socketfd < 0)
    {
      error("Error opening socket");
    }
    printf("Modbus -> Socket open succesfully\n");
    /* Log event */
    log_events("Modbus -> Socket successfully open");
    /* CREATE IP ADDRESS FOR CONNECTION */
    struct sockaddr_in *address = createIPv4Address(server_ip, portno);
    /* ESTABLIESH TCP CONNECTION */
    int connection = connect(socketfd, (struct sockaddr *)address,
                             sizeof(struct sockaddr_in));
    if (connection != 0)
    {
      /* Log event */
      log_events("Modbus -> Could not connect to target");
      printf("Modbus -> Could not connect to target\n");
      dequeue_cmd();
      dequeue_var();
      free(address);
      return 1;
    }
    /* IN CASE OF 0, SUCCESFULL */
    /* Log event */
    log_events("Modbus -> Socket connection successfull");
    printf("Modbus -> Socket connection successfull\n");

    /* Initialize the counter for retries */
    int count_retry = 0;

    /* Buffer array to store reponse data */
    char buffer[256];
    int i = 0;
    uint8_t *query;
    signal(SIGINT, INThandler);
    /* Pointer to list of variables */
    variable_list *ptr2;
    float mqtt_value;
    char mqtt_value_str[10];
    /* SQLITE db pointer */
    sqlite3 *db;
    /* Issue Modbus queries and receive data */
    while (true)
    {
      i++;
      query = build_packet(i, PROT_ID, LEN, commands->cmd.unit_id,
                           commands->cmd.func_code, commands->cmd.offset_addr,
                           commands->cmd.count);
      /* ENVIA LA CONSULTA MODBUS */
      sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);
      ssize_t amountWasSent = send(socketfd, query, 12, MSG_NOSIGNAL);
      /* RECIBE LA RESPUESTA MODBUS */
      int received = recv(socketfd, buffer, sizeof(buffer) - 1, 0);
      /*
      When client does not get data from server then try so many times retry is
      configure then
       */
      if (received == 0)
      {
        /* Log event */
        log_events("Any data received");
        printf("Modbus -> Any data received\n");
        usleep(MS_TIMEOUT * 1000);
        count_retry++;
      }
      if (count_retry == RETRY || (signal(SIGPIPE, SIG_IGN) == SIG_ERR))
      {
        /* Log event */
        log_events("Modbus -> Retries reached, leaving connection...");
        printf("Retries reached, leaving connection...\n");
        free(query);
        break;
      }
      /*
      When a valid response is received
       */
      if (received > 0)
      {
        buffer[received] = '\n';
        if (buffer[7] == commands->cmd.func_code)
        {
          int j = 9;
          int k = 0;
          ptr2 = mb_variable_list;
          while (ptr2 != NULL)
          {
            /*
            TODO: Complete conditional for 32-bits Double integer, 16-bits
            integer
             */
            if (strcmp(ptr2->modbus_var.datatype, "float") == 0)
            {
              if (!ptr2->modbus_var.swap)
              {
                mqtt_value =
                    ieee754(buffer[9 + ptr2->modbus_var.address + 1] & 0xFF,
                            buffer[9 + ptr2->modbus_var.address + 0] & 0xFF,
                            buffer[9 + ptr2->modbus_var.address + 3] & 0xFF,
                            buffer[9 + ptr2->modbus_var.address + 2] & 0xFF);
              }
              printf("Percentage trigger %s: %f - last value: %lf\n", ptr2->modbus_var.tag_name, mqtt_value * (1 + ptr2->modbus_var.perc_trig / 100), ptr2->modbus_var.last_value);
              if (limit(mqtt_value, ptr2->modbus_var.perc_trig, ptr2->modbus_var.last_value))
              {
                /*
            ✅TODO: Check how to send data only when change a percentage
            Publish MQTT data
            */
                snprintf(mqtt_value_str, sizeof(mqtt_value_str), "%.2f",
                         mqtt_value);
                printf("message to publish: %s\n", mqtt_value_str);
                /*
                 *close_conn = 0 para publicar
                 */
                paho_mqtt_publish((char *)MQTT_BROKER_URI, (char *)USERNAME,
                                  (char *)PASSWORD, mqtt_value_str,
                                  ptr2->modbus_var.topic);
                ptr2->modbus_var.last_value = (double)mqtt_value;

                /* Insert value in local DB */
                insert_value_db(db, ptr2->modbus_var.tag_name, mqtt_value_str);
              }
            }
            ptr2 = ptr2->next;
          }
        }
        else if ((buffer[7] & 0xF0) == 0x80)
        {
          /*
           * Exception Codes
           * -
           */
          print_modbus_exception(buffer[8]);
        }
        /* Poll Interval */
        usleep(commands->cmd.ms_poll * 10000);
      }
      free(query);
    }
  }
  /* Unload lists */
  dequeue_cmd();
  dequeue_var();
  printf("Bye\n");
  return 0;
}
