#include "edgelib.h"
#include <stdio.h>


void log_events (char *event) {
  /* Open log file */
  FILE *f;
  time_t t;
  time(&t);
  f = fopen("./logs/events.log", "a+");
  if (f == NULL) {
    printf("It was unable to open the log file\n");
    return;
  }
  /* Log event in file */
  fprintf(f, "%s  -  %s\n", strtok(ctime(&t), "\n"), event);
  fclose(f);
  return;
}

/* Create TCP Socket */
int createTCPIPv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }
/* Create IPv4 address socket */
struct sockaddr_in *createIPv4Address(char *ip, int port) {
  struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
  address->sin_family = AF_INET;
  address->sin_port = htons(port);
  if (strlen(ip) == 0) {
    address->sin_addr.s_addr = INADDR_ANY;
  } else {
    inet_pton(AF_INET, ip, &address->sin_addr.s_addr);
  }
  return address;
}
/* Print error and exit */
void error(const char *msg) {
  log_events((char *)msg);
  perror(msg);
  exit(0);
}
/* Allow package Byte */
uint8_t *pack_uint8(uint8_t *dest, uint8_t src) {
  *(dest++) = src;
  return dest;
}
/* Allow package integer 16-bits */
uint8_t *pack_uint16be(uint8_t *dest, uint16_t src) {
  *(dest++) = (src >> 8);
  *(dest++) = src & 0xFF;
  return dest;
}
/* Build package */
uint8_t *build_packet(uint16_t transaction_id, uint16_t protocol_id,
                      uint16_t length, uint8_t unit_id, uint8_t func_code,
                      uint16_t address, uint16_t count) {
  uint8_t *packet = malloc(12);
  uint8_t *p = packet;
  p = pack_uint16be(p, transaction_id);
  p = pack_uint16be(p, protocol_id);
  p = pack_uint16be(p, length);
  p = pack_uint8(p, unit_id);
  p = pack_uint8(p, func_code);
  p = pack_uint16be(p, address);
  p = pack_uint16be(p, count);
  // memcpy(p, body, body_length);
  return packet;
}
/* Function to join two bytes into a word */
int join(uint8_t a, uint8_t b) { return (a << 8) | b; }

/* Print Modbus exception from code number */
void print_modbus_exception(uint8_t code_number) {
  switch (code_number & 0xff) {
  case 1:
    printf("Exception code: 0%x - Illegal Function\n", code_number & 0xff);
    break;
  case 2:
    printf("Exception code: 0%x - Illegal Data Address\n", code_number & 0xff);
    break;
  case 3:
    printf("Exception code: 0%x - Illegal Data Value\n", code_number & 0xff);
    break;
  case 4:
    printf("Exception code: 0%x - Server Device Failure\n", code_number & 0xff);
    break;
  case 5:
    printf("Exception code: 0%x - Acknowledge\n", code_number & 0xff);
    break;
  case 6:
    printf("Exception code: 0%x - Server Device Busy\n", code_number & 0xff);
    break;
  case 8:
    printf("Exception code: 0%x - Memory Party Error\n", code_number & 0xff);
    break;
  }
}

float ieee754(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
  int fraction = ((byte3 & 0x7F) << 16) | (byte2 << 8) | byte1;
  int exponent = ((byte4 & 0x7F) << 1) | (byte3 >> 7);
  int sign = (byte4 >> 7);
  int p = 0;
  float fract = 0;
  while (p < 23) {
    fract = fract + (fraction & 0x01) * pow(2, p - 23);
    p++;
    fraction = fraction >> 1;
  }
  float numb = pow(-1, sign) * (1 + fract) * pow(2, exponent - 127);
  return numb;
}

void read_config(cmd_list *cmd, variable_list *var, bool *cmd_exists,
                 bool *var_exists) {
  /* Open file */
  FILE *config = fopen("config.conf", "r");
  /* Check is could open file or if valid */
  if (config == NULL) {
    printf("Could not open the configuration file\n");
    return;
  }
  /* Read the file */
  int size = 250;
  char str[size];
  int ctl = 2;
  /* Iterate the file buffer line by line */
  while (fgets(str, size, config)) {
    /* Delete all comment lines */
    str[strcspn(str, "#\r\n")] = 0;
    /* Skip empty lines */
    if (strcmp(str, "\n") == 0)
      continue;
    int cmd_params[6], k = 0;
    /* When the line content the keyword COMMANDS the command definition starts
     */
    if (strcmp(str, "COMMANDS") == 0) {
      ctl = 0;
      continue;
    } else if (strcmp(str, "VARIABLES") == 0) {
      /* The Variable definition starts */
      ctl = 1;
      continue;
    }
    char *save = str;
    char *tk = strtok_r(str, " ", &save);
    if (ctl == 0) {
      /* Maps all the command definition into de structure array */
      /* strtk split the line string by space in order to get all the parameters
       */
      // char *tk = strtok(str, " ");
      while (tk != NULL) {
        if (atoi(&str[0]) == 0) {
          while (tk != NULL) {
            switch (k) {
            case 1:
              cmd->cmd.unit_id = atoi(tk);
              break;
            case 2:
              cmd->cmd.func_code = atoi(tk);
              break;
            case 3:
              cmd->cmd.offset_addr = atoi(tk);
              break;
            case 4:
              cmd->cmd.count = atoi(tk);
              break;
            case 5:
              cmd->cmd.ms_poll = atoi(tk);
              break;
            case 6:
              strcpy(cmd->cmd.ip_addr, tk);
              break;
            case 7:
              cmd->cmd.port_numb = atoi(tk);
              // HEAD_CMD = cmd;
              // TAIL_CMD = cmd;
              TAIL_CMD->next = NULL;
              *cmd_exists = true;
              break;
            }
            k++;
            tk = strtok_r(NULL, " ", &save);
          }
        } else {
          cmd_list *new_item = malloc(sizeof(cmd_list));
          if (new_item == NULL) {
            return;
          }
          while (tk != NULL) {
            switch (k) {
            case 1:
              new_item->cmd.unit_id = atoi(tk);
              break;
            case 2:
              new_item->cmd.func_code = atoi(tk);
              break;
            case 3:
              new_item->cmd.offset_addr = atoi(tk);
              break;
            case 4:
              new_item->cmd.count = atoi(tk);
              break;
            case 5:
              new_item->cmd.ms_poll = atoi(tk);
              break;
            case 6:
              strcpy(new_item->cmd.ip_addr, tk);
              break;
            case 7:
              new_item->cmd.port_numb = atoi(tk);
              new_item->prev = TAIL_CMD;
              new_item->next = NULL;
              TAIL_CMD->next = new_item;
              TAIL_CMD = new_item;
              break;
            }
            k++;
            tk = strtok_r(NULL, " ", &save);
          }
        }
      }
    } else if (ctl == 1) {
      /* Process Variable Configuration */
      if (atoi(&str[0]) == 0) {
        while (tk != NULL) {
          switch (k) {
          case 1:
            strcpy(var->modbus_var.tag_name, tk);
            break;
          case 2:
            var->modbus_var.address = atoi(tk);
            break;
          case 3:
            var->modbus_var.map = atoi(tk);
            break;
          case 4:
            strcpy(var->modbus_var.datatype, tk);
            break;
          case 5:
            var->modbus_var.scalar = atoi(tk);
            break;
          case 6:
            var->modbus_var.swap = atoi(tk);
            break;
          case 7:
            strcpy(var->modbus_var.topic, tk);
            break;
          case 8:
            var->modbus_var.perc_trig = atoi(tk);
            /* Init the threshold */
            var->modbus_var.last_value = 0;
            // HEAD_VAR = var;
            // TAIL_VAR = var;
            TAIL_VAR->next = NULL;
            *var_exists = true;
            break;
          }
          k++;
          tk = strtok_r(NULL, " ", &save);
        }
      } else {
        variable_list *new_var = malloc(sizeof(variable_list));
        if (new_var == NULL) {
          return;
        }
        while (tk != NULL) {
          switch (k) {
          case 1:
            strcpy(new_var->modbus_var.tag_name, tk);
            break;
          case 2:
            new_var->modbus_var.address = atoi(tk);
            break;
          case 3:
            new_var->modbus_var.map = atoi(tk);
            break;
          case 4:
            strcpy(new_var->modbus_var.datatype, tk);
            break;
          case 5:
            new_var->modbus_var.scalar = atoi(tk);
            break;
          case 6:
            new_var->modbus_var.swap = atoi(tk);
            break;
          case 7:
            strcpy(new_var->modbus_var.topic, tk);
            break;
          case 8:
            new_var->modbus_var.perc_trig = atoi(tk);
            /* Init the threshold */
            new_var->modbus_var.last_value = 0;
            new_var->prev = TAIL_VAR;
            new_var->next = NULL;
            TAIL_VAR->next = new_var;
            TAIL_VAR = new_var;
            break;
          }
          k++;
          tk = strtok_r(NULL, " ", &save);
        }
      }
    }
  }
  fclose(config);
}

void dequeue_cmd() {
  cmd_list *trav = HEAD_CMD;
  if (trav->next != NULL) {
    trav = trav->next;
    free(HEAD_CMD);
    trav->prev = NULL;
    HEAD_CMD = trav;
  }
  free(HEAD_CMD);
  return;
}
void dequeue_var() {
  variable_list *trav = HEAD_VAR;
  while (trav->next != NULL) {
    trav = trav->next;
    free(HEAD_VAR);
    trav->prev = NULL;
    HEAD_VAR = trav;
  }
  free(HEAD_VAR);
  return;
}

void INThandler(int sig) {
  char c;

  signal(sig, SIG_IGN);
  printf("OUCH, did you hit Ctrl-C?\n"
         "Do you really want to quit? [y/n] ");
  c = getchar();
  if (c == 'y' || c == 'Y')
    exit(0);
  else
    signal(SIGINT, INThandler);
  getchar();
}

/*
 * Paho Callbacks
 */
MQTTClient_deliveryToken deliveredtoken;
void paho_delivery_complete(void *context, MQTTClient_deliveryToken dt) {
  /* Log event */
  log_events("MQTT Client -> Message published to broker");
  printf("MQTT Client -> Message delivered, token: %d\n", dt);
  deliveredtoken = dt;
}

void paho_conn_lost(void *context, char *cause) {
  char msg[150];
  sprintf(msg, "MQTT Client -> Connection lost, cause: %s", cause);
  /* Log event */
  log_events(msg);
  printf("MQTT Client -> Connection Lost, cause: %s\n", cause);
}
int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
  printf("Message arrived %s \n", (char*)message->payload);
  return 1;
}

/*
Implementing Paho Mqtt
 */
/**
 *This function create the MQTT Client element, set the callbacks functions and
 *establish the connection with broker
 * @param broker_uri The URI of broker in the form ssl://name:port
 * @param username
 * @param password
 * @param conn_status A connection status pointer to store the status code when
 *try to establish connection with the broker
 */
MQTTClient paho_mqtt_create(char *broker_uri, char *username, char *password,
                            int *conn_status) {
  int rc = 1; // return code
  int qos = 1;
  int retained = 0;
  MQTTClient client;
  // the serverURI has to be in the format "protocol://name:port", in this case
  // it should be "ssl://name:8883"
  MQTTClient_create(&client, broker_uri, username, MQTTCLIENT_PERSISTENCE_NONE,
                    NULL);
  if (client == NULL) {
    /* Log event */
    log_events("MQTT Client Create: Out of memory");
    error("MQTT out of Memory!");
  }
  // you can set optional callbacks for context, connectionLost, messageArrived
  // and deliveryComplete
  int i = MQTTClient_setCallbacks(client, NULL, paho_conn_lost, NULL,
                                  paho_delivery_complete);
  printf(
      "MQTT status set callbacks: %s\n",
      MQTTClient_strerror(i)); // callback 0 signalizes a successful connection

  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
  ssl_opts.enableServerCertAuth = 0;

  /*
  declare values for ssl options, here we use only the ones necessary for
  TLS, but you can optionally define a lot more look here for an example:
  https://github.com/eclipse/paho.mqtt.c/blob/master/src/samples/paho_c_sub.c
   */
  ssl_opts.verify = 1;
  ssl_opts.CApath = NULL;
  ssl_opts.keyStore = NULL;
  ssl_opts.trustStore = NULL;
  ssl_opts.privateKey = NULL;
  ssl_opts.privateKeyPassword = NULL;
  ssl_opts.enabledCipherSuites = NULL;

  /*
  use TLS for a secure connection, "ssl_opts" includes TLS
   */
  conn_opts.ssl = &ssl_opts;
  conn_opts.keepAliveInterval = 10;
  conn_opts.cleansession = 1;
  /*
  Credentials to connect to the Broker
   */
  conn_opts.username = username;
  conn_opts.password = password;
  *conn_status = MQTTClient_connect(client, &conn_opts);
  printf("MQTT trying to connect... Connection status: %s\n",
         MQTTClient_strerror(*conn_status));

  return client;
}

/* MQTT Client Instance */
MQTTClient mqtt_client;
const int MQTT_PUB_TIMEOUT = 5000;
/**
 *This function create the MQTT Client element, set the callbacks functions and
 *establish the connection with broker
 * @param broker_uri The URI of broker in the form ssl://name:port
 * @param username
 * @param password
 * @param message message to issue to MQTT Broker
 * @param topic topic to ppublish to
 *try to establish connection with the broker
 */
void paho_mqtt_publish(char *broker_uri, char *username, char *password,
                       char *message, char *topic) {
  char event[150];
  int rc = 1; // return code
  int qos = 1;
  int retained = 0;
  MQTTClient_deliveryToken token;
  if (!MQTTClient_isConnected(mqtt_client)) {
    // the serverURI has to be in the format "protocol://name:port", in this
    // case it should be "ssl://name:8883"
    MQTTClient_create(&mqtt_client, broker_uri, username,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (mqtt_client == NULL) {
      /* Log event */
      log_events("MQTT Client -> Create: Out of memory");
      error("MQTT out of Memory!");
    }
    // you can set optional callbacks for context, connectionLost,
    // messageArrived and deliveryComplete
    int i = MQTTClient_setCallbacks(mqtt_client, NULL, paho_conn_lost, messageArrived,
                                    paho_delivery_complete);
    printf("MQTT status set callbacks: %s\n",
           MQTTClient_strerror(
               i)); // callback 0 signalizes a successful connection

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    ssl_opts.enableServerCertAuth = 0;

    /*
    declare values for ssl options, here we use only the ones necessary for
    TLS, but you can optionally define a lot more look here for an example:
    https://github.com/eclipse/paho.mqtt.c/blob/master/src/samples/paho_c_sub.c
    */
    ssl_opts.verify = 1;
    ssl_opts.CApath = NULL;
    ssl_opts.keyStore = NULL;
    ssl_opts.trustStore = NULL;
    ssl_opts.privateKey = NULL;
    ssl_opts.privateKeyPassword = NULL;
    ssl_opts.enabledCipherSuites = NULL;
    /*
    use TLS for a secure connection, "ssl_opts" includes TLS
    */
    conn_opts.ssl = &ssl_opts;
    conn_opts.keepAliveInterval = 10;
    conn_opts.cleansession = 1;
    /*
    Credentials to connect to the Broker
    */
    conn_opts.username = username;
    conn_opts.password = password;
    if ((rc = MQTTClient_connect(mqtt_client, &conn_opts)) !=
        MQTTCLIENT_SUCCESS) {
      sprintf(event, "MQTT Client -> Fail to connect to Broker, reason: %s",
             MQTTClient_strerror(rc));
      /* Log event */
      log_events(event);
      printf("Fail to connect to Broker, reason: %s\n",
             MQTTClient_strerror(rc));
      MQTTClient_destroy(mqtt_client);
    }
    printf("Client connection status: %d\n",
           MQTTClient_isConnected(mqtt_client));
  }
  /*
   * Publish part
   */
  int mqtt_msg_len = strlen(message);
  if ((rc = MQTTClient_publish(mqtt_client, topic, mqtt_msg_len, message, qos,
                               0, &token)) != MQTTCLIENT_SUCCESS) {
    sprintf(event, "MQTT Client -> Failed to publish message: %s", MQTTClient_strerror(rc));
    /* Log event */
    log_events(event);
    printf("Failed to publish message: %s\n", MQTTClient_strerror(rc));
  }
  printf("Waiting for up to %d seconds for publication of message on "
         "topic %s\n",
         (int)(MQTT_PUB_TIMEOUT / 1000), topic);
  while (deliveredtoken != token)
    ;
  rc = MQTTClient_waitForCompletion(mqtt_client, token, MQTT_PUB_TIMEOUT);
  sprintf(event, "MQTT Client -> Publish result: %s", MQTTClient_strerror(rc));
  /* Log event */
  log_events(event);
  printf("Publish result: %s\n", MQTTClient_strerror(rc));
}

/** 
*Allow to trigger events
* @param value is the current value of variable to compare
* @param percentage is the percentage of change to trigger event
* @param last_value is the las value of the variable to compare and trigger
 */
bool limit (float value, float percentage, double last_value) {
  if (value >= last_value * (1 + (percentage / 100))) {
    return true;
  }
  if (value < last_value * (1 - (percentage / 100))) {
    return true;
  }
  return false;
}

/** 
* This funtion allows to insert values in database, it will be necessary to create a table for each variable
* @param db_name: is a pointer to the database
* @param tag_name: name of the tag which is also the name of the table is going to be inserted
* @param value: value to insert in table
 */
void insert_value_db (sqlite3 *db_name, char *tag_name, char *value) {
  char event[150];
  char *err_msg = 0;
  int rc;
  /* Query to insert values into table, the table name is a paramater */
  char sqlite3_query[150];
  sprintf(sqlite3_query, "INSERT INTO %s (tag_id, val) VALUES ((SELECT id FROM tags WHERE tag_name = LOWER(?1)), ?2);", tag_name);
  sqlite3_stmt *prepared_stmt;
  /* Open the database file */
  rc = sqlite3_open("./database/edge.db", &db_name);
  if (rc) {
    sprintf(event, "SQLITE3 DB -> Could not open database: %s", sqlite3_errmsg(db_name));
    /* Log event */
    log_events(event);
    fprintf(stderr, "Could not open database: %s\n", sqlite3_errmsg(db_name));
    sqlite3_close(db_name);
    return;
  }
  /* Prepare SQL Statement in order to avoid SQL Injection, since any of the values are user/external input it could be not strictly necessary */
  rc = sqlite3_prepare_v3(db_name, sqlite3_query, -1, 0, &prepared_stmt, NULL);
  if (rc) {
    sprintf(event, "SQLITE3 DB -> Could not prepare sql statement: %s", sqlite3_errmsg(db_name));
    /* Log event */
    log_events(event);
    fprintf(stderr, "Could not prepare sql statement: %s\n", sqlite3_errmsg(db_name));
    sqlite3_finalize(prepared_stmt);
    sqlite3_close(db_name);
    return;
  }
  /* This allows to substitute parameter 1 into SQL query (statement) - Parameter 1 is the table or tag name */
  rc = sqlite3_bind_text(prepared_stmt, 1, tag_name, strlen(tag_name), NULL);
  if (rc) {
    sprintf(event, "SQLITE3 DB -> Could not bind Tag name to prepared sql statement: %s", sqlite3_errmsg(db_name));
    /* Log event */
    log_events(event);
    fprintf(stderr, "Could not bind table name to prepared sql statement: %s\n", sqlite3_errmsg(db_name));
    sqlite3_finalize(prepared_stmt);
    sqlite3_close_v2(db_name);  
  }
  /* This allows to substitute parameter 2 into SQL query (statement) - Parameter 2 is the value to be inserted */
  rc = sqlite3_bind_text(prepared_stmt, 2, value, strlen(value), NULL);
  if (rc) {
    sprintf(event, "SQLITE3 DB -> Could not bind Value to prepared sql statement: %s", sqlite3_errmsg(db_name));
    /* Log event */
    log_events(event);
    fprintf(stderr, "Could not bind Value to prepared sql statement: %s\n", sqlite3_errmsg(db_name));
    sqlite3_finalize(prepared_stmt);
    sqlite3_close_v2(db_name);
    return;
  }
  /* This allows to execute the query */
  rc = sqlite3_step(prepared_stmt);
  if (rc != SQLITE_DONE) {
    sprintf(event, "SQLITE3 DB -> Failed to Insert value %s: %s", value, sqlite3_errmsg(db_name));
    /* Log event */
    log_events(event);
    fprintf(stderr, "Failed to Insert value %s: %s\n", value, sqlite3_errmsg(db_name));
    sqlite3_reset(prepared_stmt);
    sqlite3_close_v2(db_name);
    return;  
  }
  printf("Value %s Inserted succesfully into %s!\n", value, tag_name);
  sprintf(event, "SQLITE3 DB -> Value %s Inserted succesfully into table %s", value, tag_name);
  /* Log event */
  log_events(event);
  /* After execute the query reset the statement prepared, end db transaction and close DB */
  sqlite3_reset(prepared_stmt);
  sqlite3_exec(db_name, "END TRANSACTION;", NULL, NULL, NULL);
  sqlite3_close(db_name);
  return;
}