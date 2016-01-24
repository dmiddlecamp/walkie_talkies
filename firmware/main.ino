

#include "SparkIntervalTimer.h"
#include "SimpleRingBuffer.h"

//WiFi.selectAntenna(ANT_EXTERNAL);

#define MICROPHONE_PIN DAC1
#define SPEAKER_PIN DAC2
#define BUTTON_PIN A0
#define BROADCAST_PORT 3443
#define UDP_BROADCAST_PORT 3444
#define AUDIO_BUFFER_MAX 8192

//#define SERIAL_DEBUG_ON true

#define AUDIO_TIMING_VAL 125 /* 8,000 hz */
//#define AUDIO_TIMING_VAL 62 /* 16,000 hz */
//#define AUDIO_TIMING_VAL 50  /* 20,000 hz */

UDP Udp;
IPAddress broadcastAddress(255,255,255,255);

int audioStartIdx = 0, audioEndIdx = 0;
int rxBufferLen = 0, rxBufferIdx = 0;

//uint16_t audioBuffer[AUDIO_BUFFER_MAX];
uint8_t txBuffer[AUDIO_BUFFER_MAX];
//uint8_t rxBuffer[AUDIO_BUFFER_MAX];


SimpleRingBuffer audio_buffer;
SimpleRingBuffer recv_buffer;


// IntervalTimer readMicTimer;
// IntervalTimer sendAudioTimer;

// version without timers
unsigned long lastRead = micros();
unsigned long lastSend = millis();
char myIpAddress[24];

TCPClient audioClient;
TCPClient checkClient;
TCPServer audioServer = TCPServer(BROADCAST_PORT);

IntervalTimer readMicTimer;
//int led_state = 0;
float _volumeRatio = 0.25;
int _sendBufferLength = 0;
unsigned int lastPublished = 0;


void setup() {
    #if SERIAL_DEBUG_ON
    Serial.begin(115200);
    #endif

    pinMode(MICROPHONE_PIN, INPUT);
    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(D7, OUTPUT);

    Particle.function("setVolume", onSetVolume);

    Particle.variable("ipAddress", myIpAddress, STRING);
    IPAddress myIp = WiFi.localIP();
    sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);

    recv_buffer.init(AUDIO_BUFFER_MAX);
    audio_buffer.init(AUDIO_BUFFER_MAX);

    Udp.setBuffer(1024);
    Udp.begin(UDP_BROADCAST_PORT);


//    Udp.beginPacket(broadcastAddress, UDP_BROADCAST_PORT);
//    Udp.write(rxBuffer, 10);
//    Udp.endPacket();


    // 1/16,000th of a second is ~62 mcsec
    //readMicTimer.begin(readMic, 62, uSec);


    // // send a chunk of audio every 1/2 second
    // sendAudioTimer.begin(sendAudio, 1000, hmSec);

    audioServer.begin();

    lastRead = micros();
}

bool _isRecording = false;
void startRecording() {

    if (!_isRecording) {
        // 1/8000th of a second is 125 microseconds
        readMicTimer.begin(readMic, AUDIO_TIMING_VAL, uSec);
    }

    _isRecording = true;
}

void stopRecording() {
    if (_isRecording) {
        readMicTimer.end();
    }
    _isRecording = false;
}

void loop() {
//    checkClient = audioServer.available();
//    if (checkClient.connected()) {
//        audioClient = checkClient;
//    }

    //listen for 100ms, taking a sample every 125us,
    //and then send that chunk over the network.
    //listenAndSend(100);

    if (digitalRead(BUTTON_PIN) == HIGH) {
        digitalWrite(D7, HIGH);
        startRecording();
        sendEvery(100);
    }
    else {
        digitalWrite(D7, LOW);
        stopRecording();
    }

    readAndPlay();
    //led_state = !led_state;
}


int onSetVolume(String cmd) {
    _volumeRatio = cmd.toFloat() / 100;
}

void readAndPlay() {


    while (Udp.parsePacket() > 0) {
       while (Udp.available() > 0) {
            recv_buffer.put(Udp.read());
       }
       if (recv_buffer.getSize() == 0) {
           analogWrite(SPEAKER_PIN, 0);
       }
    }





//    #if SERIAL_DEBUG_ON
//        Serial.println("received " + String(count));
//    #endif

//    //read as much as we can off the buffer.
//    rxBufferLen = Udp.read(rxBuffer, AUDIO_BUFFER_MAX);
//    rxBufferIdx = 0;

    playRxAudio();
}

void playRxAudio() {
    unsigned long lastWrite = micros();
	unsigned long now, diff;
	int value;

	//toggleLED();

	//noInterrupts();

    //while (rxBufferIdx < rxBufferLen) {
    while (recv_buffer.getSize() > 0) {

        // ---
        //map it back from 1 byte to 2 bytes
        //map(value, fromLow, fromHigh, toLow, toHigh);
        //value = map(rxBuffer[rxBufferIdx++], 0, 255, 0, 4095);

        //play audio
        value = recv_buffer.get();
        value = map(value, 0, 255, 0, 4095);
        value = value * _volumeRatio;

        now = micros();
        diff = (now - lastWrite);
        if (diff < AUDIO_TIMING_VAL) {
            delayMicroseconds(AUDIO_TIMING_VAL - diff);
        }

        //analogWrite(SPEAKER_PIN, rxBuffer[rxBufferIdx++]);
        analogWrite(SPEAKER_PIN, value);
        lastWrite = micros();
    }

    //interrupts();

    //toggleLED();
}


void listenAndSend(int delay) {
    unsigned long startedListening = millis();

    while ((millis() - startedListening) < delay) {
        unsigned long time = micros();

        if (lastRead > time) {
            // time wrapped?
            //lets just skip a beat for now, whatever.
            lastRead = time;
        }

        //125 microseconds is 1/8000th of a second
        if ((time - lastRead) > 125) {
            lastRead = time;
            readMic();
        }
    }
    sendAudio();
}

void sendEvery(int delay) {

//    #if SERIAL_DEBUG_ON
//        Serial.println("sendEvery");
//    #endif

    // if it's been longer than 100ms since our last broadcast, then broadcast.
    if ((millis() - lastSend) >= delay) {

        sendAudio();

        lastSend = millis();
    }
}

// Callback for Timer 1
void readMic(void) {
    //read audio
    uint16_t value = analogRead(MICROPHONE_PIN);
    value = map(value, 0, 4095, 0, 255);
    audio_buffer.put(value);

//old
//    if (audioEndIdx >= AUDIO_BUFFER_MAX) {
//        audioEndIdx = 0;
//    }
//    audioBuffer[audioEndIdx++] = value;


//    //play audio
//    value = map(recv_buffer.get(), 0, 255, 0, 4095);
//    if (value >= 0) {
//        analogWrite(SPEAKER_PIN, value);
//    }


//    //play audio
//    if (rxBufferIdx < rxBufferLen) {
//
////        uint8_t lsb = rxBuffer[rxBufferIdx];
////        uint8_t msb = rxBuffer[rxBufferIdx+1];
////        rxBufferIdx +=2;
////        uint16_t value = ((msb << 8) | (lsb & 0xFF));
////        value = (value / 65536.0) * 4095.0;
////        analogWrite(SPEAKER_PIN, value);
//
//        //tcpBuffer[tcpIdx] = map(val, 0, 4095, 0, 255);
//        analogWrite(SPEAKER_PIN, rxBuffer[rxBufferIdx++]);
//    }
    //digitalWrite(D7, (led_state) ? HIGH : LOW);

//    if (rxBufferIdx < rxBufferLen) {
//        int value = map(rxBuffer[rxBufferIdx++], 0, 255, 0, 4095);
//        analogWrite(SPEAKER_PIN, value);
//    }
}

void copyAudio(uint8_t *bufferPtr) {

    int c = 0;
    while ((audio_buffer.getSize() > 0) && (c < AUDIO_BUFFER_MAX)) {
        bufferPtr[c++] = audio_buffer.get();
    }
    _sendBufferLength = c - 1;



//    //if end is after start, read from start->end
//    //if end is before start, then we wrapped, read from start->max, 0->end
//
//    int endSnapshotIdx = audioEndIdx;
//    bool wrapped = endSnapshotIdx < audioStartIdx;
//    int endIdx = (wrapped) ? AUDIO_BUFFER_MAX : endSnapshotIdx;
//
//
//    for(int i=audioStartIdx;i<endIdx;i++) {
//        // do a thing
//        bufferPtr[c++] = audioBuffer[i];
//    }
//
//    if (wrapped) {
//        //we have extra
//        for(int i=0;i<endSnapshotIdx;i++) {
//            // do more of a thing.
//            bufferPtr[c++] = audioBuffer[i];
//        }
//    }
//
//    //and we're done.
//    audioStartIdx = audioEndIdx;
//
//    if (c < AUDIO_BUFFER_MAX) {
//        bufferPtr[c] = -1;
//    }
//    _sendBufferLength = c;
}

// Callback for Timer 1
void sendAudio(void) {
//    #if SERIAL_DEBUG_ON
//        Serial.println("sendAudio");
//    #endif

    copyAudio(txBuffer);

    int i=0;
    uint16_t val = 0;


    if (audioClient.connected()) {
       write_socket(audioClient, txBuffer);
    }
    else {
        write_UDP(txBuffer);
    }
    // else {
    //     while( (val = txBuffer[i++]) < 65535 ) {
    //         Serial.print(val);
    //         Serial.print(',');
    //     }
    //     Serial.println("DONE");
    // }

}


void write_socket(TCPClient socket, uint8_t *buffer) {
    int i=0;
    uint16_t val = 0;

    int tcpIdx = 0;
    uint8_t tcpBuffer[1024];

    while( (val = buffer[i++]) < 65535 ) {
        if ((tcpIdx+1) >= 1024) {
            socket.write(tcpBuffer, tcpIdx);
            tcpIdx = 0;
        }

        tcpBuffer[tcpIdx] = val & 0xff;
        tcpBuffer[tcpIdx+1] = (val >> 8);
        tcpIdx += 2;
    }

    // any leftovers?
    if (tcpIdx > 0) {
        socket.write(tcpBuffer, tcpIdx);
    }
}

void write_UDP(uint8_t *buffer) {
    int stopIndex=_sendBufferLength;
//    uint16_t val = 0;

//    int tcpIdx = 0;
//    uint8_t tcpBuffer[1024];




//    while (( buffer[stopIndex++] < 4096 ) && (stopIndex < AUDIO_BUFFER_MAX)) {
//        ;
//    }
    #if SERIAL_DEBUG_ON
        Serial.println("SENDING " + String(stopIndex));
    #endif
    Udp.sendPacket(buffer, stopIndex, broadcastAddress, UDP_BROADCAST_PORT);

    //Udp.beginPacket(broadcastAddress, UDP_BROADCAST_PORT);

//    while( (val = buffer[i++]) < 4096 ) {
//        if ((tcpIdx+1) >= 1024) {
//
//            //works
////            Udp.beginPacket(broadcastAddress, UDP_BROADCAST_PORT);
////            Udp.write(tcpBuffer, tcpIdx);
////            Udp.endPacket();
//
//            //Doesn't work
//            Udp.sendPacket(tcpBuffer, tcpIdx, broadcastAddress, UDP_BROADCAST_PORT);
//
//
//            #if SERIAL_DEBUG_ON
//            Serial.println("SENT " + String(tcpIdx));
//            #endif
//            //delay(5);
//
//
//            tcpIdx = 0;
//            toggleLED();
//        }
//
//        //map(value, fromLow, fromHigh, toLow, toHigh);
//        tcpBuffer[tcpIdx] = val; //map(val, 0, 4095, 0, 255);
//        tcpIdx++;
//
////        tcpBuffer[tcpIdx] = val & 0xff;
////        tcpBuffer[tcpIdx+1] = (val >> 8);
////        tcpIdx += 2;
//    }
//
//    // any leftovers?
//    if (tcpIdx > 0) {
//        //works
////        Udp.beginPacket(broadcastAddress, UDP_BROADCAST_PORT);
////        Udp.write(tcpBuffer, tcpIdx);
////        Udp.endPacket();
//
//        //doesn't work
//        Udp.sendPacket(tcpBuffer, tcpIdx, broadcastAddress, UDP_BROADCAST_PORT);
//
//        #if SERIAL_DEBUG_ON
//        Serial.println("SENT " + String(tcpIdx));
//        #endif
//    }

    //toggleLED();
}

bool ledState = false;
void toggleLED() {
    ledState = !ledState;
    digitalWrite(D7, (ledState) ? HIGH : LOW);
}