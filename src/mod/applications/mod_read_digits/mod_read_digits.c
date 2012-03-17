/**
 * @file
 *
 * @author Richard Screene
 *
 * The file implements a module used for the record side of the NetDev challenge.  The specification for the
 * script is as follows:
 *
 * "Dial 1234 from a SIP soft phone - FS sends a tone, and then listens for four DTMF digits.
 * Caller enters four DTMF digits (nnnn). FS collects the digits, and then
 * records the caller speech to <somewhere>/nnnn.wav until the caller hangs up."
 *
 * This file implements the read_digits function which can be used to collect DTMF
 * tone and filter out the non-digits (*,#).  The number of digits to retreive is specified
 * and these non-digits do not count towards that total.  An inter-digit timeout is provided.
 *
 * The existing modules always return * and #.  It might cause issues if these were used to
 * create the message path.
 */

#include <switch.h>

/* Prototypes */
SWITCH_MODULE_SHUTDOWN_FUNCTION( mod_read_digits_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION( mod_read_digits_runtime);
SWITCH_MODULE_LOAD_FUNCTION( mod_read_digits_load);

/* SWITCH_MODULE_DEFINITION(name, load, shutdown, runtime) 
 * Defines a switch_loadable_module_function_table_t and a static const char[] modname
 */
SWITCH_MODULE_DEFINITION( mod_read_digits, mod_read_digits_load,
		mod_read_digits_shutdown, NULL);

/**
 * Remove any non-digits ![0-9] from the given string
 *
 * @param pInBuffer the buffer to process.  On return will
 *   contain the filtered string.  Assumed that it is
 *   null terminated
 *
 * @return the number of digits in the filtered string
 */
uint32_t filter_non_digits(char * const pInBuffer) {
	char *pFrom = pInBuffer;
	char *pTo = pInBuffer;
	switch_assert(pInBuffer != NULL);

	while (*pFrom != '\0') {
		if (isdigit(*pFrom)) {
			*pTo++ = *pFrom;
		}
		pFrom++;
	}
	*pTo = '\0'; // terminate

	//	calculate the size of the destination string
	return (uint32_t)(pTo - pInBuffer);
}

/**
 * This function reads the specified number of DTMF signals
 * from the channel.  This differs from the usual read command
 * in that non-digit DTMF signals (eg. * and #) are ignored.
 * (ie. this function will wait for the specified number
 * of [0-9] digits).
 *
 * If the inter-digit timeout expires before all required
 * digits have been entered then the number of signal received
 * so far are returned and the timeout status is set.
 *
 * The argument string takes the following arguments:
 *   <num_digits> [<var_name> [<timeout>]]
 * Where:
 *   num_digits is the number of digits to return
 *   var_name is the variable name to return the digits.  If
 *     this is not specified then the digits are not returned.
 *   timeout is the inter-digit timeout.  Defaults to 10000ms.
 *
 * @return returns the digits read into the var_name global variable
 *   (this may be less than the required number on timeout). Will
 *   also set the status varable.
 */
SWITCH_STANDARD_APP(read_digits_function)
{
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_input_args_t args = {0};
	char *mydata;
	char *argv[3] = {0};
	int argc = 0;
	uint32_t len = 0, prev_len = 0;
	uint32_t num_digits = 0;
	uint32_t timeout = 10000;
	char digit_buffer[128] = {0};
	const char *var_name = NULL;

	switch_assert(session != NULL);

	channel = switch_core_session_get_channel(session);
	switch_assert(channel != NULL);

	if (switch_channel_pre_answer(channel) != SWITCH_STATUS_SUCCESS) {
		status = SWITCH_STATUS_FALSE;
	}

	//	parse the input string
	if (SWITCH_STATUS_SUCCESS == status) {
		if (!zstr(data) && (mydata = switch_core_session_strdup(session, data))) {
			argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
		} else {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "No arguments specified.\n");
			status = SWITCH_STATUS_FALSE;
		}
	}

	//	read and validate the input parameters
	if (SWITCH_STATUS_SUCCESS == status) {
		//	assume that argument is not -ve
		num_digits = (uint32_t) atoi(argv[0]);

		if (argc > 1) {
			var_name = argv[1];
		}

		if (argc > 2) {
			//	assume that argument is not -ve
			timeout = (uint32_t) atoi(argv[2]);
		}

		if (num_digits <= 1) {
			num_digits = 1;
		}

		if (timeout <= 1000) {
			timeout = 1000;
		}

		if (sizeof(digit_buffer) < num_digits) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Buffer too small!\n");
			status = SWITCH_STATUS_FALSE;
		}
	}

	if (SWITCH_STATUS_SUCCESS == status) {

		//	loop until falure or we've got enough digits
		while ((len < num_digits) && (SWITCH_STATUS_SUCCESS == status)) {
			args.buf = digit_buffer + len;
			args.buflen = (uint32_t) (sizeof(digit_buffer) - len);

			status = switch_ivr_collect_digits_count(session, digit_buffer, sizeof(digit_buffer) - len, num_digits - len, NULL, NULL, 0, timeout, 0);

			// strip out non-digits - only consider the recently entered digits
			len += filter_non_digits(&digit_buffer[prev_len]);
			prev_len = len;
		}

		if (var_name) {
			(void) switch_channel_set_variable(channel, var_name, digit_buffer);
		}

		if (len < num_digits) {
			status = SWITCH_STATUS_TOO_SMALL;
		}
	}

	switch (status) {
		case SWITCH_STATUS_SUCCESS:
		(void) switch_channel_set_variable(channel, SWITCH_READ_RESULT_VARIABLE, "success");
		break;
		case SWITCH_STATUS_TIMEOUT:
		(void) switch_channel_set_variable(channel, SWITCH_READ_RESULT_VARIABLE, "timeout");
		break;
		default:
		(void) switch_channel_set_variable(channel, SWITCH_READ_RESULT_VARIABLE, "failure");
		break;
	}
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "status=%d digits=%s\n",
			status, digit_buffer);
}

/* Macro expands to: switch_status_t mod_read_digits_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool) */
SWITCH_MODULE_LOAD_FUNCTION(mod_read_digits_load)
{
	switch_application_interface_t *app_interface;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "mod_read_digits Loaded!\n");

	SWITCH_ADD_APP(app_interface, "read_digits", "read_digits APP", "read_digits APP", read_digits_function, "<num_digits> [<var_name> [<timeout>]]", SAF_NONE);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/*
 Called when the system shuts down
 Macro expands to: switch_status_t mod_read_digits_shutdown() */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_read_digits_shutdown)
{
	return SWITCH_STATUS_SUCCESS;
}

#ifdef TEST
int main()
{
	char buf[128];

	(void) strcpy(buf, "1234");
	assert(filter_non_digits(buf) == 4);
	assert(!strcmp(buf, "1234"));

	(void) strcpy(buf, "*1234");
	assert(filter_non_digits(buf) == 4);
	assert(!strcmp(buf, "1234"));

	(void) strcpy(buf, "12*34");
	assert(filter_non_digits(buf) == 4);
	assert(!strcmp(buf, "1234"));

	(void) strcpy(buf, "1234*");
	assert(filter_non_digits(buf) == 4);
	assert(!strcmp(buf, "1234"));

	(void) strcpy(buf, "*1*2*3*4*");
	assert(filter_non_digits(buf) == 4);
	assert(!strcmp(buf, "1234"));

	(void) strcpy(buf, "*****");
	assert(filter_non_digits(buf) == 0);
	assert(!strcmp(buf, ""));

	(void) strcpy(buf, "*");
	assert(filter_non_digits(buf) == 0);
	assert(!strcmp(buf, ""));

	(void) strcpy(buf, "");
	assert(filter_non_digits(buf) == 0);
	assert(!strcmp(buf, ""));

	// will fail
	//assert(filter_non_digits(NULL) == 0);
}
#endif /*TEST*/

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
