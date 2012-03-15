#include <switch.h>

/* Prototypes */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_net_dev_record_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_net_dev_record_runtime);
SWITCH_MODULE_LOAD_FUNCTION(mod_net_dev_record_load);

/* SWITCH_MODULE_DEFINITION(name, load, shutdown, runtime) 
 * Defines a switch_loadable_module_function_table_t and a static const char[] modname
 */
SWITCH_MODULE_DEFINITION(mod_net_dev_record, mod_net_dev_record_load, mod_net_dev_record_shutdown, NULL);

typedef enum {
	CODEC_NEGOTIATION_GREEDY = 1,
	CODEC_NEGOTIATION_GENEROUS = 2,
	CODEC_NEGOTIATION_EVIL = 3
} codec_negotiation_t;

static struct {
	char *codec_negotiation_str;
	codec_negotiation_t codec_negotiation;
	switch_bool_t sip_trace;
	int integer;
} globals;

static switch_status_t config_callback_siptrace(switch_xml_config_item_t *data, switch_config_callback_type_t callback_type, switch_bool_t changed)
{
	switch_bool_t value = *(switch_bool_t *) data->ptr;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "In siptrace callback: value %s changed %s\n",
					  value ? "true" : "false", changed ? "true" : "false");


	/*
	   if ((callback_type == CONFIG_LOG || callback_type == CONFIG_RELOAD) && changed) {
	   nua_set_params(((sofia_profile_t*)data->functiondata)->nua, TPTAG_LOG(value), TAG_END());
	   } 
	 */

	return SWITCH_STATUS_SUCCESS;
}

static switch_xml_config_string_options_t config_opt_codec_negotiation = { NULL, 0, "greedy|generous|evil" };

/* enforce_min, min, enforce_max, max */
static switch_xml_config_int_options_t config_opt_integer = { SWITCH_TRUE, 0, SWITCH_TRUE, 10 };
static switch_xml_config_enum_item_t config_opt_codec_negotiation_enum[] = {
	{"greedy", CODEC_NEGOTIATION_GREEDY},
	{"generous", CODEC_NEGOTIATION_GENEROUS},
	{"evil", CODEC_NEGOTIATION_EVIL},
	{NULL, 0}
};

static switch_xml_config_item_t instructions[] = {
	/* parameter name        type                 reloadable   pointer                         default value     options structure */
	SWITCH_CONFIG_ITEM("codec-negotiation-str", SWITCH_CONFIG_STRING, CONFIG_RELOADABLE, &globals.codec_negotiation_str, "greedy",
					   &config_opt_codec_negotiation,
					   "greedy|generous|evil", "Specifies the codec negotiation scheme to be used."),
	SWITCH_CONFIG_ITEM("codec-negotiation", SWITCH_CONFIG_ENUM, CONFIG_RELOADABLE, &globals.codec_negotiation, (void *) CODEC_NEGOTIATION_GREEDY,
					   &config_opt_codec_negotiation_enum,
					   "greedy|generous|evil", "Specifies the codec negotiation scheme to be used."),
	SWITCH_CONFIG_ITEM_CALLBACK("sip-trace", SWITCH_CONFIG_BOOL, CONFIG_RELOADABLE, &globals.sip_trace, (void *) SWITCH_FALSE,
								(switch_xml_config_callback_t) config_callback_siptrace, NULL,
								"yes|no", "If enabled, print out sip messages on the console."),
	SWITCH_CONFIG_ITEM("integer", SWITCH_CONFIG_INT, CONFIG_RELOADABLE, &globals.integer, (void *) 100, &config_opt_integer,
					   NULL, NULL),
	SWITCH_CONFIG_ITEM_END()
};

static switch_status_t do_config(switch_bool_t reload)
{
	memset(&globals, 0, sizeof(globals));

/*	if (switch_xml_config_parse_module_settings("net_dev_record.conf", reload, instructions) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Could not open net_dev_record.conf\n");
		return SWITCH_STATUS_FALSE;
	}
*/
	return SWITCH_STATUS_SUCCESS;
}

void net_dev_record_event_handler(switch_event_t *event)
{
	switch_assert(event); // Just a sanity check
 	
 	switch(event->event_id) {
 		case SWITCH_EVENT_CHANNEL_CREATE:
 			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "A new channel is born, its called \"%s\"\n", 
 				switch_event_get_header_nil(event, "channel-name")); // This function isnt case-sensitive 
 			break;
 		case SWITCH_EVENT_CHANNEL_DESTROY:
 			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "A channel named \"%s\" has been destroyed\n",
 				switch_event_get_header_nil(event, "channel-name"));
 			break;
 		case SWITCH_EVENT_DTMF:
 			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Got event %d - DTMF\n", event->event_id);
			// if (switch_channel_has_dtmf(channel)) {
//                 switch_channel_dequeue_dtmf_string(channel, dtmf, sizeof(dtmf));
			break;
        default :
 			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Got event %d\n", event->event_id);
 	}

}

SWITCH_STANDARD_APP(net_dev_record_function)
{
	switch_status_t status = SWITCH_STATUS_GENERR;
	switch_channel_t *channel = switch_core_session_get_channel(session);
//    switch_dtmf_t dtmf;



    switch_channel_answer(channel);

	do_config(SWITCH_TRUE);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "mod_net_dev_record Actually doing it!!!!\n");

if (switch_event_bind("mod_net_dev_record", SWITCH_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, net_dev_record_event_handler, NULL) != SWITCH_STATUS_SUCCESS) {
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot bind to event handler!\n");
 }



//if (!switch_is_file_path(file)) {
//       switch_channel_flush_dtmf(switch_core_session_get_channel(session));

switch_ivr_sleep(session, 2000, SWITCH_TRUE, NULL);

status = switch_ivr_play_file(session, NULL, "ivr/8000/ivr-welcome.wav", NULL);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "status=%d good=%d\n", status, SWITCH_STATUS_SUCCESS);

switch_core_session_send_dtmf_string(session, "12345678");


/*        if ((status = switch_core_session_recv_dtmf(session, dtmf) != SWITCH_STATUS_SUCCESS)) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Error recv dtmf - status=%d good=%d\n", status, SWITCH_STATUS_SUCCESS);
        }
        else
        {
//        if (is_dtmf(new_dtmf.digit)) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Gt DTMF=%d", dtmf->digit);
        }
  */
switch_ivr_sleep(session, 3000, SWITCH_TRUE, NULL);

//switch_ivr_inband_dtmf_session(session);
//switch_ivr_record_file(session, &fh, file_path, &args, profile->max_record_len);
//does this turn them off???
switch_ivr_gentones(session, "%(1000, 0, 640)", 0, NULL);

{
        switch_file_handle_t fh = { 0 };
        switch_input_args_t args = { 0 };
		char input[10] = "";

               memset(&fh, 0, sizeof(fh));
                fh.thresh = 1000;
                fh.silence_hits = 2000;
                fh.samplerate = 500;

                memset(input, 0, sizeof(input));
                args.input_callback = cancel_on_dtmf;
                args.buf = input;
                args.buflen = sizeof(input);

                switch_ivr_record_file(session, &fh, "/tmp/xx", &args, 1000);
}

         while (switch_channel_ready(channel)) {
	        switch_ivr_sleep(session, 1000, SWITCH_FALSE, NULL);
		}

}

/* Macro expands to: switch_status_t mod_net_dev_record_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool) */
SWITCH_MODULE_LOAD_FUNCTION(mod_net_dev_record_load)
{
	switch_application_interface_t *app_interface;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "mod_net_dev_record Hello World!\n");

	do_config(SWITCH_FALSE);

	SWITCH_ADD_APP(app_interface, "net_dev_record", "net_dev_record APP", "net_dev_record APP", net_dev_record_function, "", SAF_NONE);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/*
  Called when the system shuts down
  Macro expands to: switch_status_t mod_net_dev_record_shutdown() */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_net_dev_record_shutdown)
{
	/* Cleanup dynamically allocated config settings */
	switch_xml_config_cleanup(instructions);
	return SWITCH_STATUS_SUCCESS;
}

/*
  If it exists, this is called in it's own thread when the module-load completes
  If it returns anything but SWITCH_STATUS_TERM it will be called again automatically
  Macro expands to: switch_status_t mod_net_dev_record_runtime()
SWITCH_MODULE_RUNTIME_FUNCTION(mod_net_dev_record_runtime)
{
	while(looping)
	{
		switch_cond_next();
	}
	return SWITCH_STATUS_TERM;
}
*/

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4
 */
