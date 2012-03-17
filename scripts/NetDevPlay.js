/*
 * @file
 *
 * @author Richard Screene
 * 
 * The file implements a script for the playback side of the NetDev challenge.  The specification for the
 * script is as follows:
 * 
 * "Dial 5678 from a SIP soft phone - FS sends a different tone, and then
 * listens for four DTMF digits. Caller enters four DTMF digits (nnnn). FS collects the 
 * digits, and then plays back <somewhere>/nnnn.wav to the caller, looping until the caller
 * hangs up."
 * 
 * NB. Nothing has been dne to make the prompts better - eg. insert delay before starting.
 */

var config; // global object holding the configuration options

/*
 * Reads the configuration.
 * 
 * Configuration parameters are:
 * 
 * welcome-prompt - the filename of the prompt to play on answer (mandatory)
 * 
 * num-digits - the number of digits in the message filename (mandatory)
 * 
 * timeout - time timeout when wating for the digit (mandatory)
 * 
 * error-prompt - filename of the prompt to be played when an internal error
 * occurs (optional)
 * 
 * invalid-prompt - filename of the prompt to be played when the digits are
 * entered incorrectly (optional)
 * 
 * message-directory - the directory in which the message should be saved
 * (mandatory)
 * 
 * message-extension - the extenstion for the message (mandatory)
 * 
 * 
 * The configuration is invalid if a mandatory option is not defined, or a
 * numeric value exceeds limits.
 * 
 * @return a config object if configuration was valid, null otherwise
 */
function readConfig()
{
	var status = true;
	config = new Object();

	console_log("notice", "Reading config\n");

	config.welcome_tone = getGlobalVariable("welcome-tone");
	config.num_digits = getGlobalVariable("num-digits");
	config.timeout = getGlobalVariable("timeout");
	config.error_prompt = getGlobalVariable("error-prompt");
	config.invalid_prompt = getGlobalVariable("invalid-prompt");
	config.message_directory = getGlobalVariable("message-directory");
	config.message_extension = getGlobalVariable("message-extension");

	if ("" == config.welcome_tone)
	{
		console_log("error", "Error in welcome-prompt configuration\n");
		status = false;
	}
	
	if ((config.num_digits <= 0) || (config.num_digits > 10))
	{
		console_log("error", "Error in num-digits configuration\n");
		status = false;
	}

	if ((config.timeout <= 0) || (config.timeout > 999999))
	{
		console_log("error", "Error in timeout configuration\n");
		status = false;
	}

	if ("" == config.mesage_directory)
	{
		console_log("error", "Error in message-directory configuration\n");
		status = false;

	}
	else if (!fileExists(config.message_directory))
	{
		console_log("error", "message-directory (" + config.message_directory + ") does not exist\n");
		status = false;
	}

	if ("" == config.mesage_extension)
	{
		console_log("error", "Error in message-extension configuration\n");
		status = false;
	}

	if (status)
	{
		console_log("notice", "welcome_tone=" + config.welcome_tone + " num_digits=" + config.num_digits +
				" timeout=" + config.timeout + " error_prompt=" + config.error_prompt + 
				" invalid_prompt=" + config.invalid_prompt + " message_directory=" +
				config.message_directory + " message_extension=" + config.message_extension + "\n");
		return config;
	}
	else
	{
		return null;
	}
}

/**
 * Called whenever a DTMF signal is received. If the signal is a digit then it
 * is added to the store, otherwise it is ignored.
 * 
 * @return false if the store has enough digits, or true otherwise
 */
function dtmfCallback(session, type, data, arg) {
	if ("dtmf" == type) {
		// ignore non-numeric DTMF - we don't want '*' in the filename
		if ((data.digit >= '0') && (data.digit <= '9'))
		{
			arg.digits += data.digit;
		}
		if (arg.digits.length >= config.num_digits) {
			return false;
		}
	}
	return true;
}

/**
 * Initialises the receipt of the DTMF signals.
 * 
 * @return the DTMF object if enough digits have been entered before timeout, or
 *         a null value otherwise.
 */
function collectDtmf()
{
	console_log("notice", "Collecting " + config.num_digits + " DTMF within " + config.timeout + "msecs\n");

	session.flushDigits();

	var dtmf = new Object();

	dtmf.digits = "";

	if (session.ready()) {
		session.collectInput(dtmfCallback, dtmf, config.timeout, 0);
	}

	if (dtmf.digits.length < config.num_digits) {
		console_log("notice", "Didn't enter enough digits\n");
		return null;	
	}
	else
	{
		console_log("notice", "Entered " + dtmf.digits + "\n");
		return dtmf;
	}
}

/**
 * Play the DTMF digit entered to the caller
 * 
 * @param dtmf
 *            object containing the DTMF digits
 */
function playDtmf(dtmf)
{
	for (var i = 0; i < dtmf.digits.length; i ++) {
		session.streamFile("digits/8000/" + dtmf.digits[i] + ".wav");
	}
}

/**
 * Play the given prompt.
 * 
 * Prompts are not interruptible. On error an attempt will be made to play the
 * error prompt (if it has been specified) but this could also fail.
 * 
 * @param path
 *            the prompt to be played
 */
function playFile(path)
{
	try {
		session.streamFile(path);
	} catch (err) {
		console.log("error", "Error playing file: " + path + " message: " + err.message + "\n")
		// try to play an error - this might fail too
		if ("" != config.error_prompt) {
			session.streamFile(config.error_prompt);
		}
		session.hangup("SERVICE_UNAVAILABLE");
		exit();
	}
}

/**
 * Determines if the specified file exists.
 * 
 * @param path
 *            the file to check
 * 
 * @return true if it exists, false otherwise
 */
function fileExists(path)
{
	var fd = new File(path);
	return fd.exists;
}


// -------------- START OF MAIN SCRIPT --------------


config = readConfig();
if (null == config)
{
	console_log("error", "Error in configuration\n");
	session.hangup("SERVICE_UNAVAILABLE");
	exit();
}

session.answer();

// play the welcome tone
session.execute("gentones", config.welcome_tone);

var dtmf = collectDtmf();
if (null == dtmf) {
	console_log("warning", "Not enough digits\n");
	// play the invalid prompt if specified
	if ("" != config.invalid_prompt) {
		playFile(config.invalid_prompt);
	}
	exit();
}

console_log("notice", "Collected digits=" + dtmf.digits + "\n");

//playDtmf(dtmf);

var path = config.message_directory + "/" + dtmf.digits + config.message_extension;
console_log("notice", "Reading from file " + path + "\n");
if (!fileExists(path))
{
	console_log("warning", "File does not exist\n");
	// play an invalid prompt if specified
	if ("" != config.invalid_prompt) {
		playFile(config.invalid_prompt);
	}
	exit();
}

while (session.ready()) {
	playFile(path);
}

exit();// end of script
