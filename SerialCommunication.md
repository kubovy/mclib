# Serial Communication

Serial communicator enabling to communicated with a peripheral device over a
[Channel](src/main/java/com/poterion/communication/serial/Channel.kt) using binary messages. The first 2 bytes of every
message have a fixed semantic:

     | CRC|KIND|...
     |0xXX|0xYY|...

First byte is a [checksum](src/main/java/com/poterion/communication/serial/Utils.kt) and the second byte is a
[MessageKind](src/main/java/com/poterion/communication/serial/MessageKind.kt). The rest of the message depends on the
implementation of a concrete [MessageKind](src/main/java/com/poterion/communication/serial/MessageKind.kt). The maximum
length of a message is determined by the
[Channel.maxPacketSize](src/main/java/com/poterion/communication/serial/Channel.kt). If necessary a message needs to be
split to different packets.

A communicator manages three threads: connection thread, inbound thread, and outbound thread.

The connection thread in [State.CONNECTING](src/main/java/com/poterion/communication/serial/Communicator.kt) interrupts
both, inbound and outboud, communication threads, if such are alive. Clean up the connection and tries to start new set
of inbound and outbound communication threads. In all other
[states](src/main/java/com/poterion/communication/serial/Communicator.kt) it just sleeps.

The inbound thread waits for [nextMessage](src/main/java/com/poterion/communication/serial/Communicator.kt), which is
expected to be implemented as a blocking function. Only non empty messages are considered.

First byte of that message is extracted as a message-checksum calculated by the sender. Then the rest of the message is
used to [calculate a checksum](src/main/java/com/poterion/communication/serial/Utils.kt) on the receiver side.
If both, the received and the calculated, checksums match, the message is considered as valid for further processing.
Otherwise, the message is discarded.

A [MessageKind](src/main/java/com/poterion/communication/serial/MessageKind.kt) is then determined based on the second
byte. All non-[CRC](src/main/java/com/poterion/communication/serial/MessageKind.kt) messages a CRC checksum is send
back to the sender containing the [calculated checksum](src/main/java/com/poterion/communication/serial/Utils.kt):

      | CRC|KIND|CONTENT|
      |0xXX|0x00|  0xXX |

A [MessageKind.CRC](src/main/java/com/poterion/communication/serial/MessageKind.kt) message will store the received CRC
to compare with the last sent message. This way the communicator determines when a sent message was successfully
received by the other side.

A [MessageKind.IDD](src/main/java/com/poterion/communication/serial/MessageKind.kt) message implements a protocol to
initialize a connection and determine its stability.

Any other [MessageKind](src/main/java/com/poterion/communication/serial/MessageKind.kt) will be simply forwarded using 
the [CommunicatorListener.onMessageReceived](src/main/java/com/poterion/communication/serial/CommunicatorListener.kt)
and has to be implemented further.

Last, the outbound thread monitors 2 queues: the _checksum queue_ and the _message queue_. The _checksum queue_
has priority over the _message queue_. The _checksum queue_ contains a list of
[checksum bytes](src/main/java/com/poterion/communication/serial/Utils.kt) of recently received messages to be
confirmed to the sender. The _message queue_ contains a list of binary messages to be send to the receiver without the
checksum byte (the 1st byte). The checksum will be calculated freshly before any send attempt.

The following flows and situactions are considered during communication between the sender and the receiver. Both
side share this implementation:

In good case transmitting a message works first time and is being confirmed right-away:

![Success](https://github.com/kubovy/serial-communication/raw/master/src/img/communication-success.png)

One typical error case is when a message does not reach its target. In this case the message is resend again after
a certain timeout (by default 500ms -
[MESSAGE_CONFIRMATION_TIMEOUT](src/main/java/com/poterion/communication/serial/Communicator.kt), but can be overwritten
for different [MessageKind](src/main/java/com/poterion/communication/serial/MessageKind.kt) -
[MessageKind.delay](src/main/java/com/poterion/communication/serial/MessageKind.kt)).

![Sent timeout](https://github.com/kubovy/serial-communication/raw/master/src/img/communication-sent-timeout.png)

In the same way it may happen, that the message got received corretly, but the CRC was not. Each
[MessageKind](src/main/java/com/poterion/communication/serial/MessageKind.kt) needs to be implemented in a idempotent
way so receiving multiple same messages after each other should not result in errors.

![CRC timeout](https://github.com/kubovy/serial-communication/raw/master/src/img/communication-confirmation-timeout.png)

To prevent resent starvation, one message is retried up to 20 times by default
([MAX_SEND_ATTEMPTS](src/main/java/com/poterion/communication/serial/Communicator.kt)) and then it is dropped. The
developer is responsible for resolution of such situations.

The connection can be in one of the following states:

 - [State.DISCONNECTED](src/main/java/com/poterion/communication/serial/Communicator.kt)
 - [State.CONNECTING](src/main/java/com/poterion/communication/serial/Communicator.kt)
 - [State.CONNECTED](src/main/java/com/poterion/communication/serial/Communicator.kt)
 - [State.DISCONNECTING](src/main/java/com/poterion/communication/serial/Communicator.kt)


## Message Types and their payload


### [0x00] Cyclic redundancy check message (CRC)

    |==============|
    | (A) CRC      |
    |--------------|
    |  0 |  1 |  2 |
    | CRC|KIND| CRC|
    |==============|

- **CRC** : Checksum of the packet
- **KIND**: Message kind


### [0x01] ID of device message (IDD)

ID of device message

    |=============================|
    | (A) Ping                    |
    |-----------------------------|
    |  0 |  1 |  2 |              |
    | CRC|KIND| RND|              |
    |=============================|
    | (B) State                   |
    |-----------------------------|
    |  0 |  1 |  2 |  3 |         |
    | CRC|KIND| RND|STAT|         |
    |=============================|
    | (B) Response                |
    |-----------------------------|
    |  0 |  1 |  2 |  3 |  4... 5 |
    | CRC|KIND|0x00|STAT| PAYLOAD |
    |=============================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`RND `**: Random number to force different CRC
- **`STAT`**: Requested state
    - `0x00`: Request capabilities
    - `0x01`: Request device name
- **`PAYLOAD`**: Payload A, depending on `STAT`:
    - `0x00`: Capabilities. Bits:
        - `0`: Bluetooth connectivity
        - `1`: USB connectivity
        - `2`: Temperature, Humidity sensor
        - `3`: LCD capability
        - `4`: Registry available
        - `5`: Motion sensor
        - `6`: _N/A_
        - `7`: _N/A_
        - `8`: RGB strip
        - `9`: RGB LEDs (e.g. WS281x)
        - `10`: RGB LED Light (e.g. ES281x)
        - `11`: _N/A_
        - `12`: _N/A_
        - `13`: _N/A_
        - `14`: _N/A_
        - `15`: _N/A_
    - `0x01`: Device name

### [0x02] Consistency check (CONSISTENCY_CHECK)

    |========================|
    | (A) Request w/ part    |
    |------------------------|
    |  0 |  1 |  2 |    |    |
    | CRC|KIND|PART|    |    |
    |========================|
    | (B) Response w/ part   |
    |------------------------|
    |  0 |  1 |  2 |  3 |    |
    | CRC|KIND|PART|CKSM|    |
    |========================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`PART`**: Part to check
- **`CKSM`**: State machine checksum


### [0x03] Data transfer (DATA)

    |============================================|
    | (A) Pull                                   |
    |--------------------------------------------|
    |  0 |  1 |  2 |    |    |    |    |    |    |
    | CRC|KIND|PART|    |    |    |    |    |    |
    |============================================|
    | (A) Pull part using starting address       |
    |--------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |    |    |
    | CRC|KIND|PART|ADRH|ADRL|LENH|LENL|    |    |
    |============================================|
    | (B) Push                                   |
    |--------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7...X  |
    | CRC|KIND|PART|ADRH|ADRL|LENH|LENL|   DATA  |
    |============================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`NUM `**: Data number/ID
- **`ADRH`**: Starting address in this packet high byte
- **`ADRL`**: Starting address in this packet low byte
- **`LENH`**: Length high byte
- **`LENL`**: Length low byte
- **`DATA`**: Data till end of the packet


### [0x0F] Plain message (PLAIN)

Plain message

    |===================|
    | (A) Plain message |
    |-------------------|
    |  0 |  1 |  2...LEN|
    | CRC|KIND|   DATA  |
    |===================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`DATA`**: Message


### [0x10] Input/Output message (IO)

    |===================|
    | (A) Request       |
    |-------------------|
    |  0 |  1 |  2 |    |
    | CRC|KIND|PORT|    |
    |===================|
    | (B) Set/Response  |
    |-------------------|
    |  0 |  1 |  2 |  3 |
    | CRC|KIND|PORT| BIT|
    |===================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`PORT`**: IO port number
- **`BIT `**: Bit value, `BIT & 0x80` means setting, otherwise it is a response.


### [0x11] Temperature & Humidity sensor message (TEMP)

    |=============================|
    | (A) Request sensor count    |
    |-----------------------------|
    |  0 |  1 |    |    |    |    |
    | CRC|KIND|    |    |    |    |
    |=============================|
    | (B) Response sensor count   |
    |-----------------------------|
    |  0 |  1 |  2 |  3 |    |    |
    | CRC|KIND|0xFF| CNT|    |    |
    |=============================|
    | (C) Request                 |
    |-----------------------------|
    |  0 |  1 |  2 |    |    |    |
    | CRC|KIND| NUM|    |    |    |
    |=============================|
    | (D) Response                |
    |-----------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |
    | CRC|KIND| NUM| TEMPx10 | RH |
    |=============================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`NUM `**: DHT11 number
- **`CNT `**: Count
- **`TEMPx10`**: Temperature in &deg;C multiplied by 10 (`TEMP = TEMPx10 / 10`)
- **`RH  `**: Relative humidity in %


### [0x12] LCD display message (LCD)

LCD display message

    |=======================================|
    | (A) Request LCD count                 |
    |---------------------------------------|
    |  0 |  1 |    |    |    |    |    |    |
    | CRC|KIND|    |    |    |    |    |    |
    |=======================================|
    | (B) Response LCD count                |
    |---------------------------------------|
    |  0 |  1 |  2 |    |    |    |    |    |
    | CRC|KIND| CNT|    |    |    |    |    |
    |=======================================|
    | (C) Request a line                    |
    |---------------------------------------|
    |  0 |  1 |  2 |  3 |    |    |    |    |
    | CRC|KIND| NUM|LINE|    |    |    |    |
    |=======================================|
    | (D) Send/receive command              |
    |---------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |    |    |    |
    | CRC|KIND| NUM|0xFF| CMD|    |    |    |
    |=======================================|
    | (E) Set/Response                      |
    |---------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |  6...X  |
    | CRC|KIND| NUM| CMD|LINE| LEN|   VAL   |
    |---------------------------------------|
    | X = 6 + LEN                           |
    |=======================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`NUM `**: LCD device num (to support multiple displays
- **`CMD `**:
    - `0x7B` = Clear display (only in _B_)
    - `0x7C` = Reset display (only in _B_)
    - `0x7D` = Backlight
    - `0x7E` = No backlight
- **`LINE`**: Line - `LINE = 0x80` will request all lines. This limits the number of lines to `128`.
              If `LINE >= 0x80` means to request a item on index `LINE - 0x7F` and continue with `LINE + 1`.
- **`LEN `**: Value length
- **`VAL `**: Value


### [0x13] Registry (REGISTRY)

    |=======================================|
    | (A) Request all registries            |
    |---------------------------------------|
    |  0 |  1 |  2 |    |    |    |    |    |
    | CRC|KIND|ADDR|    |    |    |    |    |
    |=======================================|
    | (B) Request a registry                |
    |---------------------------------------|
    |  0 |  1 |  2 |  3 |    |    |    |    |
    | CRC|KIND|ADDR| REG|    |    |    |    |
    |=======================================|
    | (C) Response/Write a registry         |
    |---------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |    |    |    |
    | CRC|KIND|ADDR| REG| VAL|    |    |    |
    |=======================================|
    | (D) Response/Write multiple registries|
    |---------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5  ... CNT  |
    | CRC|KIND|ADDR| REG| CNT| VAL1...VALx  |
    |=======================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`ADDR`**: SPI/I2C address
- **`REG `**: (Starting) Registry.
- **`CNT `**: Registry count
- **`VAL `**: Value


### [0x15] RGB LED Strip (RGB)

    |=====================================================================|
    | (A) Request strip count                                             |
    |---------------------------------------------------------------------|
    |  0 |  1 |    |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND|    |    |    |    |    |    |    |    |    |    |    |    |
    |=====================================================================|
    | (B) Response strip count                                            |
    |---------------------------------------------------------------------|
    |  0 |  1 |  2 |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND|SCNT|    |    |    |    |    |    |    |    |    |    |    |
    |=====================================================================|
    | (C) Request a configuration item of a strip                         |
    |---------------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND| NUM| IDX|    |    |    |    |    |    |    |    |    |    |
    |=====================================================================|
    | (D) Set configuration of a strip                                    |
    |---------------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7...8  |  9 |  10| 11 |    |    |
    | CRC|KIND| NUM|PATN|  R |  G |  B |  DELAY  | MIN| MAX|TOUT|    |    |
    |=====================================================================|
    | (E) Response                                                        |
    |---------------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9...10 | 11 | 12 | 13 |
    | CRC|KIND| NUM| CNT| IDX|PATN|  R |  G |  B |  DELAY  | MIN| MAX|TOUT|
    |=====================================================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`SCNT`**: Strip count
- **`NUM `**: Strip number
- **`CNT `**: Configuration count.
- **`IDX `**: Index of item to request. `IDX & 0x80` will request all items. This limits the number of items
              in a list to `128`. `IDX > 0x80` means to request a item on index `IDX - 0x7F` and continue with
              `IDX + 1`.
- **`PATN`**: Animation pattern. `PATN & 0x80` will replace the list with this one item, otherwise a new
              item will be added to the end of the list.
    - `0x00`: Off
    - `0x01`: Light
    - `0x02`: Blink
    - `0x03`: Fade in
    - `0x04`: Fade out
    - `0x05`: Fade in/out
- **`R`,`G`,`B`**: Red, green and blue component of the requested color
- **`DELAY`**: Animation delay (depends on pattern implementation)
- **`MIN`,`MAX`**: Minimum and maximum color values (depends on pattern implementation)
- **`TOUT`**: Number of times to repeat pattern animation before switching to next item in the list


### [0x16] RGB LED Indicators (INDICATORS)

    |================================================================|
    | (A) Request strip count                                        |
    |----------------------------------------------------------------|
    |  0 |  1 |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND|    |    |    |    |    |    |    |    |    |    |    |
    |================================================================|
    | (B) Response strip count                                       |
    |----------------------------------------------------------------|
    |  0 |  1 |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND|SCNT|    |    |    |    |    |    |    |    |    |    |
    |================================================================|
    | (C) Request an LED                                             |
    |----------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |    |    |    |    |    |    |    |    |    |
    | CRC|KIND| NUM| LED|    |    |    |    |    |    |    |    |    |
    |================================================================|
    | (D) Set all LEDs                                               |
    |----------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |    |    |    |    |    |    |    |
    | CRC|KIND| NUM|  R |  G |  B |    |    |    |    |    |    |    |
    |================================================================|
    | (E) Configuration for concrete strip and LED                   |
    |----------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |   4|  5 |  6 |  7 |  8 |  9 | 10 | 11 |    |
    | CRC|KIND| NUM| LED|PATN|  R |  G |  B |  DELAY  | MIN| MAX|    |
    |================================================================|
    | (F) Response                                                   |
    |----------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 |
    | CRC|KIND| NUM| CNT| LED|PATN|  R |  G |  B |  DELAY  | MIN| MAX|
    |================================================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`SCNT`**: Strip count
- **`NUM `**: Strip number
- **`CNT `**: LED count in the strip
- **`LED `**: LED index. `LED & 0x80` will request all LEDs. This limits the number of LEDs  to `128`.
              `LED > 0x80` means to request a LED on index `LED - 0x7F` and continue with `LED + 1`.
- **`PATN`**: Animation pattern.
    - `0x00`: Off
    - `0x01`: Light
    - `0x02`: Blink
    - `0x03`: Fade in
    - `0x04`: Fade out
    - `0x05`: Fade in/out
- **`R`,`G`,`B`**: LED color
- **`DELAY`**: Animation delay (depends on pattern implementation)
- **`MIN`,`MAX`**: Minimum and maximum color values (depends on pattern implementation)


### [0x17] RGB Light (LIGHT)

    |===============================================================================|
    | (A) Request light count                                                       |
    |-------------------------------------------------------------------------------|
    |  0 |  1 |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND|    |    |    |    |    |    |    |    |    |    |    |    |    |    |
    |===============================================================================|
    | (B) Response light count                                                      |
    |-------------------------------------------------------------------------------|
    |  0 |  1 |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND|SCNT|    |    |    |    |    |    |    |    |    |    |    |    |    |
    |===============================================================================|
    | (C) Request an configuration item of a light                                  |
    |-------------------------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |    |    |    |    |    |    |    |    |    |    |    |    |
    | CRC|KIND| NUM| IDX|    |    |    |    |    |    |    |    |    |    |    |    |
    |===============================================================================|
    | (D) Add/set configuration for concrete strip NUM                              |
    |-------------------------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |    4 ...  24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |    |    |
    | CRC|KIND| NUM|PATN|  RGB0...RGB6 |  DELAY  |  W | FAD| MIN| MAX|TOUT|    |    |
    |===============================================================================|
    | (E) Response                                                                  |
    |-------------------------------------------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 |  5 |    6 ...  26 | 27 | 28 | 29 | 30 | 31 | 32 | 33 |
    | CRC|KIND| NUM| CNT| IDX|PATN|  RGB0...RGB6 |  DELAY  |  W | FAD| MIN| MAX|TOUT|
    |===============================================================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`SCNT`**: Strip count
- **`NUM `**: Strip number
- **`CNT `**: Item count
- **`IDX `**: Index of item to request. In (A) `IDX & 0x80` will request all items. This limits the number of
              items in a list to `128`. `IDX > 0x80` means to request a item on index `IDX - 0x7F` and continue
              with `IDX + 1`.
- **`PATN`**: Animation pattern. `PATN & 0x80` will replace the list with this one item, otherwise a new
              item will be added to the end of the list.
    - `0x00`: Off
    - `0x01`: Light
    - `0x02`: Blink
    - `0x03`: Fade in
    - `0x04`: Fade out
    - `0x05`: Fade in/out
    - `0x06`: Fade toggle
    - `0x07`: Rotation
    - `0x08`: Wipe
    - `0x09`: Lighthouse
    - `0x0A`: Chaise
    - `0x0B`: Theater
- **`RGBx`**: 7 colors (`x = 0..6`), each consists of 3 bytes in the `R`, `G`, `B` order (`7 * 3 = 21 bytes`)
- **`DELAY`**: Animation delay (depends on pattern implementation)
- **`W   `**: Animation width (depends on pattern implementation)
- **`FAD `**: Animation fading  (depends on pattern implementation)
- **`MIN`,`MAX`**: Minimum and maximum color values (depends on pattern implementation)
- **`TOUT`**: Number of times to repeat pattern animation before switching to next item in the list


### [0x20] Bluetooth settings (BLUETOOTH)

    |==================================|
    | (A) Request                      |
    |----------------------------------|
    |  0 |  1 |    |    |    |    |    |
    | CRC|KIND|    |    |    |    |    |
    |==================================|
    | (B) Set/Response                 |
    |----------------------------------|
    |  0 |  1 |  2 |  3...8  |  9...24 |
    | CRC|KIND|PMOD|   PIN   |   NAME  |
    |==================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`PMOD`**: Pairing mode
    - `0x00`: PIN
    - `0x01`: Just Work
    - `0x02`: Passkey
    - `0x03`: User confirmation
- **`PIN `**: PIN code
- **`NAME`**: Device name

### [0x80] State machine state (SM_STATE_ACTION)

    |===========================================================|
    | (A) Set/Action                                            |
    |-----------------------------------------------------------|
    |  0 |  1 |  2 |  3 |  4 | 5...LEN1| .. |    |    |  ...LENx|
    | CRC|KIND| CNT|DEV1|LEN1|  DATA1  | .. |DEVx|LENx|  DATAx  |
    |===========================================================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`CNT `**: State count
- **`DEVx`**: Device of state `x`
- **`LENx`**: Length of state `x`
- **`DATAx`**: Data of state `x`


### [0x81] State machine input (SM_INPUT)

    |=============================|
    | (A) Input                   |
    |-----------------------------|
    |  0 |  1 |  2 |  3 |  4...LEN|
    | CRC|KIND| NUM| LEN|  VALUE  |
    |=============================|

- **`CRC `**: Checksum of the packet
- **`KIND`**: Message kind
- **`NUM `**: Num
- **`LEN `**: Length
- **`VALUE`**: Value


### [0xFE] Debug message (DEBUG)

Not applicable (N/A)


### [0xFF] Unknown message (UNKNOWN)

Not applicable (N/A)