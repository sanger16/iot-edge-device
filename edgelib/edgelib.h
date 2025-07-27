#ifndef _EDGELIB_H_
#define _EDGELIB_H_

#include <arpa/inet.h>
#include <math.h>
#include <mosquitto.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "MQTTClient.h"
#include "../sqlite3/sqlite3.h"


/* Modbus Command structure */
typedef struct _commands {
  uint8_t unit_id;
  uint8_t func_code;
  uint offset_addr;
  int count;
  int ms_poll;
  char ip_addr[16];
  int port_numb;
} commands;

/* Modbus Variables */
typedef struct _mb_variable {
  char tag_name[6];
  int address;
  int map;
  char datatype[6];
  int scalar;
  bool swap;
  char topic[16];
  float perc_trig;
  double last_value;
} mb_variable;

typedef struct _cmd_list {
  commands cmd;
  struct _cmd_list *prev;
  struct _cmd_list *next;
} cmd_list;

extern cmd_list *HEAD_CMD;
extern cmd_list *TAIL_CMD;

typedef struct _variable_list {
  mb_variable modbus_var;
  struct _variable_list *prev;
  struct _variable_list *next;
} variable_list;

extern variable_list *HEAD_VAR;
extern variable_list *TAIL_VAR;

/* PROTOTYPES */

void log_events (char *event);

int createTCPIPv4Socket();
struct sockaddr_in *createIPv4Address(char *ip, int port);
void error(const char *msg);
uint8_t *pack_uint8(uint8_t *dest, uint8_t src);
uint8_t *build_packet(uint16_t transaction_id, uint16_t protocol_id,
                      uint16_t length, uint8_t unit_id, uint8_t func_code,
                      uint16_t address, uint16_t count);
int join(uint8_t a, uint8_t b);
void print_modbus_exception(uint8_t code_number);
float ieee754(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4);
void read_config(cmd_list *cmd, variable_list *var, bool *cmd_exists,
                 bool *var_exists);
void dequeue_cmd();
void dequeue_var();
void INThandler(int sig);

/**
*This function create the MQTT Client element, set the callbacks functions and establish the connection with broker
* @param broker_uri The URI of broker in the form ssl://name:port
* @param username
* @param password
* @param conn_status A connection status pointer to store the status code when try to establish connection with the broker
*/
MQTTClient paho_mqtt_create(char *broker_uri, char *username, char *password,
                            int *conn_status);
void paho_delivery_complete(void *context, MQTTClient_deliveryToken dt);
void paho_conn_lost(void *context, char *cause);
void paho_mqtt_publish(char *broker_uri, char *username, char *password,
                       char *message, char *topic);
/** 
*Allow to trigger events
* @param value is the current value of variable to compare
* @param percentage is the percentage of change to trigger event
* @param last_value is the las value of the variable to compare and trigger
 */
bool limit (float value, float percentage, double last_value);
/** 
* This funtion allows to insert values in database, it will be necessary to create a table for each variable
* @param db_name: is a pointer to the database
* @param table: pointer to table where the valus is going to be inserted
* @param value: float value to insert in table
 */
void insert_value_db (sqlite3 *db_name, char *tag_name, char *value);

#endif //_EDGELIB_H