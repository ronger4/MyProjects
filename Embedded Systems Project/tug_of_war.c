#include <msp430.h>
/*
Tug Of War
Ron Alter - 200836393
Tamara Baybachov - 308208936
Matan Cohen - 204332910
Ron Gershburg - 313164766
*/

//States:

#define INIT 0
#define ERROR 1
#define START 2
#define END_GAME 3

#define DIFF 20
char error_text[] = "Please enter a valid number! \r\n";
char startingtext[] = "  is selected! Game is about to begin, prepare..\r\n";
char here_text[] = "Are you here??\r\n";
char indicator[] = "[--------------------|--------------------]\r\n\r\n";
char no_one[] = "It's a tie\r\n";
char black[] = "The winner is black!! \r\n";
char white[] = "The winner is white!! \r\n";
volatile int timeCount, blinktimer;
volatile int count_diff, playeralive;
volatile int userInput, endgame = 0, stopgame = 0;
volatile int time[3] = { 10, 15, 20 };
volatile int winnerindex = 21, lastcount_diff;

int j = 0;
int flagStart = 0;
int state = INIT;

// black is for plus - P3 for signal and P1 for GND
// white is for minus - P4 for signal and P12 for GND

void game();
void init();
int main(void){
    WDTCTL = WDTPW + WDTHOLD; // stop watchdog timer

    BCSCTL1 = CALBC1_1MHZ; // make the SMCLK to work exactly in 1MHZ
    DCOCTL = CALDCO_1MHZ; // make the SMCLK to work exactly in 1MHZ
    //Clock Settings
    BCSCTL3 |= LFXT1S_2;     //set clock to work in very-low-power
    TACCTL0 = CCIE;  // TACCR0 interrupt enabled
    TACCR0 = 12000;   // ~ 1 sec
    //UART Settings
    P3SEL |= 0x30; // Pin P3.5 and P3.4 and P3.3 used by USART module
    //UCA0BR1 = 0x00;
    UCA0CTL1 |= UCSSEL_2 + UCSWRST; // use SMCLK, USCI software reset
    UCA0BR0 = 104; // Divider for getting 9600 baud rate
    UCA0MCTL = 0x02; // UCBRSx=1
    UCA0CTL1 &= ~UCSWRST; //initialize the USCI
    //Led Settings
    P1DIR |= 0x03;  // configure P1.0 and P1.1 as output
    P1OUT = 0;
    //Button Settings
    P2SEL &= ~(BIT0 | BIT1);         // configure ports p2.0/1 as IO
    P2DIR &= ~(BIT0 | BIT1); // configure input ports: p2.0=3pin p2.1=4pin on the board
    __bis_SR_register(GIE); //  enable GI
    game();
    UC0IE = 0;
    __bis_SR_register(GIE+LPM4_bits); // enable General interrupt
}

void game(void){
    char text[] = "Lets Play!\r\nplease enter a number:\r\nfor 10 sec game press 1\r\nfor 15 sec game press 2\r\nfor 20 sec game press 3\r\n";
    char play[] = "Do you want to play again?\r\nfor a new game press 1\r\nto exit press 0\r\n";
    char goodbye[] = "Thank you , goodbye :)\r\n";
    int i, dump;
    init();
    while (stopgame == 0){
        for (i = 0; i < 112; i++){
            while (!(IFG2 & UCA0TXIFG));//wait if the buffer isnt clean
            UCA0TXBUF = text[i];
        }
        UC0IE |= UCA0RXIE;
        __bis_SR_register(GIE + LPM3_bits); //  enable General interrupt + CPU
        while (endgame == 0);
        while((UCA0STAT & UCOE) == UCOE){ //make sure user is not using the keyboard during the game
            dump = UCA0RXBUF - '0';
        }
        for (i = 0; i < 70; i++){ //play
            while (!(IFG2 & UCA0TXIFG)); //wait if the buffer isnt clean
            UCA0TXBUF = play[i];
        }
        UC0IE |= UCA0RXIE;
        __bis_SR_register(GIE + LPM3_bits);
        while (endgame == 1);
    }
    for (i = 0; i < 25; i++){//goodbye
        while (!(IFG2 & UCA0TXIFG)); //wait if the buffer isnt clean
        UCA0TXBUF = goodbye[i];
    }
    init();
    return (0);
}

void init(void){
    indicator[winnerindex + lastcount_diff] = 45; //make the array ready for a new game
    indicator[winnerindex] = 124; //make the array ready for a new game
    P1OUT = 0;
    flagStart = 0;
    timeCount = 0;
    count_diff = 0;
    lastcount_diff = 0;
    playeralive = 0;
    TACTL &= !MC_1; // shutdown the clock
    P2IE &= ~(BIT0 | BIT1); //make sure P2 will not get interupts
}
//Buttons
#pragma vector=PORT2_VECTOR
__interrupt void Port2(void){
    if (P2IFG & 0x01)
        count_diff++;
    if (P2IFG & 0x02)
        count_diff--;
    playeralive = 1;
    P2IFG &= 0xFC;
}

//Timer interrupt
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A(void){
    //Code before game starts: makes leds to blink 2 times.(total of 4 sec)
    if (!flagStart){
        P1OUT ^= 0x03;   // turn red and green leds on and off (P1.1+P1.0)
        timeCount++;
        if (timeCount == 4){
            flagStart = 1;
            timeCount = 0;
            P2IFG = 0x00;
            P2IE |= (BIT0 | BIT1);       // configure p2.0/1 enable interrupt
        }
    }
    else{  //Enters while game is on.
        timeCount++;
        if (timeCount == userInput){
            TACTL &= !MC_1;
            state = END_GAME;
            UC0IE = UCA0TXIE; //enable TX interrupt
        }
        if (timeCount % blinktimer == 0){
            P1OUT ^= 0x03;
        }
        if (timeCount % 5 == 0 && blinktimer > 1){
            blinktimer--;
        }
        if (count_diff < -DIFF){ //left side.. white is winning
            indicator[winnerindex + lastcount_diff] = 45;
            indicator[1] = 124;
            lastcount_diff = -DIFF;
        }
        else if (count_diff > DIFF){ //right side.. black is winning
            indicator[winnerindex + lastcount_diff] = 45;
            indicator[41] = 124;
            lastcount_diff = DIFF;
        }
        else{
            indicator[winnerindex + lastcount_diff] = 45;
            indicator[winnerindex + count_diff] = 124;
            lastcount_diff = count_diff;
        }
        for (j = 0; j < 48; j++){
            while (!(IFG2 & UCA0TXIFG)); //wait if the buffer isnt clean
            UCA0TXBUF = indicator[j];
        }
        j = 0;
        if (winnerindex + count_diff <= 1 || winnerindex + count_diff >= 41){
            TACTL &= !MC_1;
            state = END_GAME;
            UC0IE = UCA0TXIE; //enable TX interrupt.
        }
        if (timeCount == 5 && playeralive == 0){
            for (j = 0; j < 17; j++){
                while (!(IFG2 & UCA0TXIFG))
                    ; //wait if the buffer isnt clean
                UCA0TXBUF = here_text[j];
            }
            j = 0;
        }
    }
}

//RX Uart Interrupt
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){
    userInput = UCA0RXBUF - '0'; //get input and cast from char to int
    if (endgame == 0){
        if (userInput > 3 || userInput < 1){ //check if input is right.
            state = ERROR;
        }
        else{
            state = START; //print that game starts.
            startingtext[0] = userInput + 48;
            userInput = time[userInput - 1];
            blinktimer = (userInput / 5) - 1;
        }
        UC0IE = UCA0TXIE; //enable TX interrupt
    }
    else{
        if (userInput > 1 || userInput < 0){ //check if input is right.
            state = ERROR;
            UC0IE = UCA0TXIE;
        }
        else if (userInput == 1){ //start over for a new game
            startingtext[0] = userInput + 48;
            userInput = time[userInput - 1];
            blinktimer = (userInput / 5) - 1;
            endgame = 0;
            init();
            UC0IE &= ~UCA0TXIE;
        }
        else{
            endgame=0;
            stopgame = 1;
            UC0IE &= ~UCA0TXIE;
        }
    }
    LPM3_EXIT;
}

//TX Uart Interrupt
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void){
    switch (state) {
    case ERROR: //error input state.
        UCA0TXBUF = error_text[j];
        j++;
        if (j == 32){ //108 is number of chars in the message
            j = 0;
            state = INIT; //get back to first stage in order to print new message
            UC0IE = UCA0RXIE; //enable RX
        }
        break;
    case START: //print users input.
        UCA0TXBUF = startingtext[j];
        j++;
        if (j == 51){ //51 is number of chars in the message
            j = 0;
            state = INIT; //get back to first stage in order to print new message
            UC0IE = 0; // make sure we will not get any interupts
            TACTL = TASSEL_1 + MC_1; // ACLK, upmode
            P1OUT ^= 0x03;
        }
        break;
    case END_GAME:
        if (count_diff > 0){
            UCA0TXBUF = black[j];
            j++;
            if (j == 25){
                j = 0;
                state = INIT; //get back to first stage in order to print new message
                UC0IE = 0; // make sure we will not get any interupts
                endgame = 1;
            }
            break;
        }
        else if (count_diff < 0){
            UCA0TXBUF = white[j];
            j++;
            if (j == 25){
                j = 0;
                state = INIT; //get back to first stage in order to print new message
                UC0IE = 0; // make sure we will not get any interupts
                endgame = 1;
            }
            break;
        }
        else {
            UCA0TXBUF = no_one[j];
            j++;
            if (j == 13){
                j = 0;
                state = INIT; //get back to first stage in order to print new message
                UC0IE = 0; // make sure we will not get any interupts
                endgame = 1;
            }
            break;
        }
    }
}
