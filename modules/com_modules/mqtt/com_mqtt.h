/*
 * com_mqtt.h
 *
 *  Created on: 21 Apr 2017
 *      Author: Raluca Diaconu
 */

#ifndef COM_MQTT_H_
#define COM_MQTT_H_

#include <mosquitto.h>


struct _mqtt_channel__;

void connect_callback(struct mosquitto *mosq, void *obj, int result);
void disconnect_callback(struct mosquitto *mosq, void *obj, int result);
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

void mqtt_run_listen_thread(struct _mqtt_channel__ *channel);


#endif /* COM_MQTT_H_ */
