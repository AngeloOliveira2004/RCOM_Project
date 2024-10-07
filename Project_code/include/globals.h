//This command defines the Flag field in the frames - F field
#define FLAG 0x7E   //Synchronisation: start or end of frame
#define FLAG_STUFF 0x7D    //Escape octet
#define ESC_STUFF 0x5E //Escape octet for FLAG_STUFF
#define ESC_ESC_STUFF 0x5D   //Escape octet for ESC_STUFF

//This commands define the Address field in the frames - A field
#define A1 0x03     //Address field in frames that are commands sent by the Transmitter or replies sent by the Receiver
#define A2 0x01     //Address field in frames that are commands sent by the Receiver or replies sent by the Transmitter

//This commands define the type of frame - C field
#define SET 0x03    //SET frame: sent by the transmitter to initiate a connection
#define UA 0x07     //UA frame: confirmation to the reception of a valid supervision frame
#define RR0 0xAA    //RR0 frame: indication sent by the Receiver that it is ready to receive an information frame x\number 0
#define RR1 0xAB    //RR1 frame: indication sent by the Receiver that it is ready to receive an information frame number 1
#define REJ0 0x54   //REJ0 frame: indication sent by the Receiver that it rejects an information frame number 0 (detected an error)
#define REJ1 0x55   //REJ1 frame: indication sent by the Receiver that it rejects an information frame number 1 (detected an error)
#define DISC 0x0B   //DISC frame to indicate the termination of a connection

//BBC field will tbe the XOR of the A and C fields

/* 
    I FRAME will tbe the frame that contains the whole data
    It will resemble this structure:

    | F | A | C | BCC1 | D1 | Data | D2 | BBC2 | F |

    before stuffing and
    after destuffing
*/
