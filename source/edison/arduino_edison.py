import serial
import time
import os
import re
import json
import sys
import subprocess

SERIALPORT = "/dev/ttyMFD1"
BAUDRATE = 9600

ser = serial.Serial(SERIALPORT, BAUDRATE)

ser.bytesize = serial.EIGHTBITS #number of bits per bytes

ser.parity = serial.PARITY_NONE #set parity check: no parity

ser.stopbits = serial.STOPBITS_ONE #number of stop bits

ser.timeout = None            #timeout block read

ser.xonxoff = False     #disable software flow control

ser.rtscts = False     #disable hardware (RTS/CTS) flow control

ser.dsrdtr = False       #disable hardware (DSR/DTR) flow control

ser.writeTimeout = 0     #timeout for write

# Send Mock Data around, don't use rest api call of DEBUG is on
if len(sys.argv) > 1:
    print "DEBUG MODE---->";
    DEBUG = True
else:
    DEBUG = False

def processMessage(message):
    # tear it apart
    # a2e:highscore?;
    # a2e:name=____,score-_____;

    matchObj = re.match(r'^a2e:(.*);', message)
    if matchObj is None:
        raise Exception("processMessage Exception: message not expected, do nothing");

    mess = matchObj.group(1)
    test = 'highscore?'
    if DEBUG:
        print mess
        test = 'uname'
    if mess == test:
        return ''
    else:
        timestamp = time.time()
        # test string
        if DEBUG:
            mess = "name=WonderBread,score=00000"
        matchObj = re.match(r'name=(.*),score=(.*)', mess);
        if matchObj is None:
            raise Exception("processMessage Exception: message format not expected, do nothing")
        name = matchObj.group(1)
        score = matchObj.group(2)

        # construct the json string send
        # email is primary key and needs to be unqiue
        # we'll just make one up, so its like old video games, unique entry
        # for each score
        jsonString = '[{"name":"' + name + '","lander_score":"'+ score +'","email":"' + name + str(timestamp) + '@rocketLander.com"}]'
        print jsonString
        return jsonString

HANDSHAKE = "e2a:ack;\r\n"
print 'Starting Up Serial Monitor'

try:
    ser.open()
except Exception, e:
    print "error open serial port: " + str(e)

if ser.isOpen():

    try:
        ser.flushInput() #flush input buffer, discarding all its contents
        ser.flushOutput()#flush output buffer, aborting current output

        ser.write(HANDSHAKE)
        time.sleep(0.5)

        while True:
            sys.stdout.flush()
            response = ser.readline()

            print("\n\n\n\n>>>>>>>> read data: " + response )
            try:
                jsonString = processMessage(response)
            except Exception, e:
                print str(e)
                continue;
            try:
                if jsonString:
                    # submit high score
                    print "Submit High Score"

                    data = json.loads(jsonString)
                    print json.dumps(data, indent=4, sort_keys=True)
                    if not DEBUG:
                        os.spawnlp(os.P_NOWAIT, "/usr/bin/python", "python", "rocketLanderRest.py", jsonString)

                    ser.write(HANDSHAKE)
                else:
                    # get the high score
                    print "Get high score"
                    
                    highscore = ""
                    if not DEBUG:
                        output = subprocess.check_output(["/usr/bin/python /home/root/rocketLander.py"], shell=True);
                        parsedOutput = json.loads(output)
                        name = parsedOutput[0]['name']
                        score = parsedOutput[0]['lander_score']

                        # e2a:name=_____,score=______;
                        highscore = 'e2a:name=' + name + ',score=' + score + ";"
                    else:
                        highscore = 'e2a:name=MojoSuperDoggie,score=10'
                    
                    print "HS: " + highscore
                    ser.write(highscore)
                        
            except Exception, e:
                print "unable to spawn rest api call...: " + str(e)

        ser.close()

    except Exception, e:
        print "error communicating...: " + str(e)

else:
    print "cannot open serial port "




    
