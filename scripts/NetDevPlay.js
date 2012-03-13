  //var languageCode = "en";
  //var soundDir = "sound/";
  var languageCode = "";
  var soundDir = "";

  function playFile(fileName, callBack, callBackArgs)
  {
    //session.streamFile(soundDir + languageCode + "/" + fileName, callBack, callBackArgs);
    session.streamFile(fileName, callBack, callBackArgs);
  }

var dtmf = new Object();
var NUM_DIGITS = 4;
var DTMF_TIMEOUT = 5000;

function dtmfCallback(session, type, data, arg) {
    if (type == "dtmf") {
        arg.digits += data.digit;
        if (arg.digits.length >= NUM_DIGITS) {
            return true;
        }
    }
    //todo- what to return here?
}

function collectDtmf(numDigits, timeout)
{
//    var dtmf = new Object();

    session.flushDigits();
    
    //todo- validate input params
    console_log("debug", "Collecting " + NUM_DIGITS + " DTMF within " + timeout + "msecs\n");

    var dtmf;
    
    dtmf = session.getDigits(numDigits, "", timeout);  
/*    dtmf.digits = "";
    if (session.ready()) {
        session.collectInput(dtmfCallback, dtmf, timeout);
    }
*/
    if (dtmf.length < numDigits) {
        console_log("debug", "Didn't enter enough digits\n");
        return null;	
    }
    console_log("debug", "Entered " + dtmf + "\n");
    return dtmf;		
}

function playDtmf(digits)
{
    console_log("debug", "Digits=" + digits + "\n");
    for (var i = 0; i < digits.length; i ++) {
        console_log("debug", "Digit[" + i + "]=" + digits[i] + "\n");
        playFile("digits/8000/" + digits[i] + ".wav");
    }
}

  session.answer();
  
  //todo-play "Enter digits"
  //we will not allow interruption of the prompt
  console_log("debug", "About to collect DTMF\n");
  //todo- these should be consts
  var theDigits = collectDtmf(NUM_DIGITS, DTMF_TIMEOUT);
  if (null == theDigits) {
      //todo- warning
      console_log("warning", "Not enough digits\n");
  } else {
      console_log("debug", "Collected " + theDigits.digits + "\n");
      // not strctly required
      //todo -play prompt
      playDtmf(theDigits);
      //todo prompt:hang-up to terminate
     
     rtn = session.recordFile("/tmp/recording" + theDigits + ".wav", dtmfCallback, "", 240, 500, 3);
      console_log("debug", "rtn=" + rtn + "\n");
 
  }
  
  playFile("misc/8000/call_monitoring_blurb.wav");



  exit();

