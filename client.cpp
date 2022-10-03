#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#define MAXLINE 500

using namespace std;

bool isConnected;

/**
*  This is a user defined struct of Msg which contain type_id and msg 
* type id tells about type of msg and msg has message in char array 
*/
struct Msg {
	int type_id;
	char msg[MAXLINE];
};

typedef struct sockaddr SA;
static int	read_count;
static char	*read_ptr;
static char	read_buf[MAXLINE];

/**
 This function message to char return char array which is gernrated after taking message
 the struct Msg structure  
 **/
char* msgToChar(struct Msg message){
	//we create a space using mallac of length MAXLINE*char 
	char *charMsgOutput = (char*) malloc((1+MAXLINE)*sizeof(char));
	charMsgOutput[0] = (char) ((int) '0' + message.type_id);
	int i;
	strcpy(charMsgOutput+1, message.msg);
	return charMsgOutput;
}


/** 
 * This function char to message return a pointer to " Struct Msg" which is gerrated by taking type id
 * and message from input(char, array) and we will assign correct values to the struct 
 **/
struct Msg* charToMessage(char* input){
	struct Msg* structMsgOutput = (struct Msg*) malloc(sizeof(struct Msg));
	strcpy(structMsgOutput->msg, input + 1);
	structMsgOutput->type_id = (int) (input[0] - '0');
	return structMsgOutput;
}

/**
 * The function characterToBits convert a character array into bits array 
 * - it convert each bytes in there binary representation and then insert into and integer array
 * - For base64 we nees pair of 6-6 so it adds 0 as paading bits if last some bits is not in 
 *   multiple of 6
 * - bitLength = length of integer array which contains all bits
 * - length = its length of encoded msg 
 **/
int *characterToBits(char *msg, int *bitLength, int *length){
	int x = strlen(msg);
	int size = x*8;
	if(size%6)
		size += (6 -size%6);
	*length = size/6;
	if(x%3==1)
		(*length)+=2;
	if(x%3==2)
		(*length)+=1;

	*bitLength = size;
	int *bits = (int *)malloc(size*sizeof(int));
	int idx = 0,i,j;
	for(i = 0;i < x;i++){
		int p  = (int)msg[i];
		for(j=0;j<8;j++){
			bits[idx + 7-j] = (p%2);
			p/=2;
		}
		idx += 8;
	}
	for(i=idx;i<size;i++)
		bits[i] = 0;
	return bits;
}


// This fucntion base64Mapping returns corresponding char for each int in base64 encoding.
char base64Mapping(int i){
	if(i < 26)
        return 'A' + i;
    else if(i < 52)
        return 'a' + i -26;
    else if(i < 62)
        return '0' + i - 52;
    else
        return (i == 62)? '+':'/';
}


/**
 * This function binToChar contain bits array and integer range start to end
 * bits is array 0's and 1's only 
 * returns ascii char value of the binary value from bits[start] to bits[end].
*/
char binToChar(int *bits, int start,int end) {
	int p = 0,i;
	for(i=end;i>=start ;i--)
		p = p + bits[i]*(1 << end-i);
	return base64Mapping(p);
}


/**
 * @brief 
 * This encode function take charter array in argument and encode messgae into
	base 64 and then  transmit it to server 
*/
void encode(char*data){
	int f,i,j;
	int n = strlen(data);
	char text_msg[MAXLINE];
	//copy data into text_msg so that we can manipulate it later 
	strcpy(text_msg, data);
	//find and set the data by finding end of the message 
	int length = strlen(data);
	for(i=0;i<length;i++) {
		if(data[i] == '\r' || data[i] == '\n' || data[i] == '\0') {
			f = i;
			break;
		}
	}
	text_msg[f] = '\0';
	
	//Now we will conver char into bits using characterToBits function 
	int encoded_msg_length, bits_length ;
	int *bits = characterToBits(text_msg, &bits_length , &encoded_msg_length);
	char *encoded_message = (char*) malloc((encoded_msg_length+n-f+1)*sizeof(char));
	

	for(i=0;i<bits_length/6;i++)
		encoded_message[i] = binToChar(bits, i*6, i*6 + 5);
	for(i=bits_length /6;i<encoded_msg_length;i++)
		encoded_message[i] = '=';
	encoded_message[encoded_msg_length] = '\0';


	int p = encoded_msg_length;
    for(i=p;i<p+(n-f+1);i++)
		encoded_message[i] = data[f + i-p];
	strcpy(data, encoded_message);
    free(encoded_message);
	free(bits);
}


/**
 *
 * This function dec_To_Bin_Digit takes an array and two integer and 
 * it converts the decimal number into binary number representation  
 *  
 * */
void dec_To_Bin_Digit(char arr[][6], long n, int I ) {
    int rem; 
    int j = 5;
    while(n != 0) {
        rem = n%2;
        n = n/2;
		if(rem == 0) arr[I][j] = '0';
		else		arr[I][j] = '1';
		j--;
	}

    int k;
    for(k=0;k<=j;k++)
	arr[I][k] = '0';

}


// converts a binary array representing an int to its decimal form and return it as int.
int binaryToDecimal_d(char *t ){
    int decimal = 0, i = 0;
    while (i<8) {
        if(t[7-i] == '1')
        	decimal += (int) (1 << i);
        ++i;
    }
    return decimal;
 
}
/**
 * - This function decode takes input an char array and then traverse it to remove '\r', '/n' and '\0'
 * - Then its makes a copy of text msg with name EncodedTextMessage 
 * - Its check no of '=' append in last of this program 
 * - then it translate encodded binary data into its decimal representation 
 * - In last step it add all extra character like '\0' , '\r'  after encode the message content  
 **/
void decode(char *text_msg){
	//char array to store encoded message 
	char EncodedTextMessage[100000];
	//variables used for iteration 
	int f,x;

	//find end of the message in orginal message 
	for(x=0;x<strlen(text_msg);x++){
		if(text_msg[x] == '\r' || text_msg[x] == '\n' || text_msg[x] == '\0'){
			f = x;
			break;
		}
	}

	//copy whole message into encodedTextMessage 
	for(x=0;x<f;x++)
		EncodedTextMessage[x] = text_msg[x];
	//set last bit as '\0'	
	EncodedTextMessage[f] = '\0';

	//Here we remove all '=' message and find actual length of message 
	int length = strlen(EncodedTextMessage);
	int sub = 0;
	int alen = length;
	if(EncodedTextMessage[length-1] == '='){
		if(EncodedTextMessage[length-2] == '='){
			alen = length-2;
			sub = 4;
		}
		else{
			alen = length-1;
			sub = 2;
		}
	}
	alen = (alen*6 - sub)/8;
	//This is char array of base64 encoding which we will use to access index directly 
    char mapping[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	int i,j;
	char temp[100000][6];
	char tempf[600000];
	//conver decimal to binary 
	for(i=0;i<strlen(EncodedTextMessage);i++) {
		for(j=0;j<=63;j++) {
			if(EncodedTextMessage[i]==mapping[j]) {
				dec_To_Bin_Digit(temp,j,i);
				break;
			}	
		}
	}
	//store the value of remainder 
	int remai = strlen(EncodedTextMessage) * 6  - strlen(temp[0]);
	
	//remove value according to remainder 
	if(remai == 6) {
		int i;
		for(i=0;i<strlen(temp[0])-2;i++) // removed two 0
			tempf[i] = temp[0][i];
	}
	else if(remai == 12) {
		int i;
		for(i=0;i<strlen(temp[0])-4;i++)  // removed four 0
			tempf[i] = temp[0][i];
	}
	else {
		int i;
		for(i=0;i<strlen(temp[0]);i++)
			tempf[i] = temp[0][i];
	}

	//char array to store decoded Message 
	char DecodedMessage[100000];
	int k = 0;

	int flag = strlen(tempf) / 8;
	int a = flag;
	//again convert binary to decimal 
	while(flag){
		char x[8];
		int j;
		for(j=0;j<8;j++)
		    x[j] = tempf[(a-flag)*8 + j];
		DecodedMessage[k++] = (char) binaryToDecimal_d(x);
		flag--;
	}

	DecodedMessage[alen] = '\0';
	
	int p = strlen(DecodedMessage);
	int n = strlen(text_msg);
	for(x=p;x<p+(n-f+1);x++)
		DecodedMessage[x] = text_msg[f + x-p];

	//copy decodeMessage into textmsg
	//Print decoded  message 	
	strcpy(text_msg, DecodedMessage);
	for(i=0;i< 100000;i++)
		DecodedMessage[i] = '\0';
	for(i=0;i< 600000;i++)
		tempf[i] = '\0';

}

static ssize_t my_read(int fd, char *ptr){
	if(read_count <= 0){
	again:
		if ( (read_count = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)		goto again;
			return(-1);
		} 
		else if (read_count == 0)		return(0);
		read_ptr = read_buf;
	}
	read_count--;
	*ptr = *read_ptr++;
	return(1);
}

int cfd;


/** 
 * - This function reads stream of the till it doesnot get '\n' 
 * - or it gets eof (End of file )
 * - or it gets error in between it return -1
 **/
ssize_t readline(int fd, void *vptr, size_t maxlen){
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = (char *)vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}


ssize_t readlinebuf(void **vptrptr){
	if (read_count)
		*vptrptr = read_ptr;
	return(read_count);
}


// handles readline errors.
ssize_t Readline(int fd, void *ptr, size_t maxlen) {
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0){
		cout<<"readline error\n";	
		exit(1);
	}
	return(n);
}

// writes n bytes to a descriptor
ssize_t writeBytesCheker(int fd, const void *vptr, size_t n) {
    size_t  nleft;
    ssize_t  nwritten;
    const char  *ptr;

    ptr = (char *)vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR)
                nwritten = 0;           /* and call write() again */
            else
                return(-1);             /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}


/*
This Writen function handles error if writeBytesCheker is not able to write 
correct no of bytes i.e if it is less or greater then a its shows writeBytesCheker error
*/
void Writen(int fd, void *ptr, size_t nbytes){
    if (writeBytesCheker(fd, ptr, nbytes) != nbytes)
        perror("writen error");
}

// returns true if M starts with "exit"
bool isExit(char *M){
	if(strlen(M) < 4 || M[0] != 'e' || M[1] != 'x'|| M[2] != 'i'||M[3] != 't')
		return false;
	return true;
}


/**
 * 
 * - Start client function takes input from the user 
 * - then it checks whether its a close message or nomral message and find its type id 
 * - Creates "Struct msg " varible and copy all related information into it 
 * - then using using encode funtion it enocode the message 
 * - encode function use various other fucntion to encode it into base64 encoding 
 * - then it convert strcut msg varoble into and character array 
 * - send this function to server
 * - then server reply ack message 
 * */

void startClient(FILE *fp, int sockfd){
	//two char array to store send and recived msg 
	char sendMessage[MAXLINE], recieveMessage[MAXLINE];
	//run while loop until it want to close connection 
	while(fgets(sendMessage, MAXLINE, fp) != NULL){
		struct Msg message;
		//decide the message type  whether its close or genral msg
		message.type_id = isExit(sendMessage)? 3:1;
		// message.type_id = isClose(sendMessage)? 3:1;
		//copy whole message into message struct 
		strcpy(message.msg,sendMessage);
		//encode the message using encode funtion 
		encode(message.msg);
		//convert struct msg into char message
		char *message_com = msgToChar(message);
		Writen(sockfd, message_com, strlen(message_com));
		//Check recived message send by the server if its length is 0 then close the connection 
		while(readline(sockfd, recieveMessage, MAXLINE) <= 0){
			 cout<<"server terminated prematurely\n";
			 cout<<"closing sockets.....\n";
			 close(sockfd);	
			 cout<<"exiting....\n";
			 exit(0);
		}
		//delete message_com
		free(message_com);
		//conver message into strcut message 
		struct Msg* cmessage = charToMessage(recieveMessage);
		//check and print message id and type id and print it 
		cout<<"Msg received from server:\n";
		cout<<"Type_id: "<<cmessage->type_id<<" || message: "<<cmessage->msg<<endl;
		//decode this message
		decode(cmessage->msg);
		///check is it close message or some other
		if(isExit(cmessage->msg)){
			cout<<"server terminated prematurely\n";
			cout<<"closing sockets.....\n";
			close(sockfd);	
			cout<<"exiting....\n";
			exit(0);	
		}
		fputs(cmessage->msg, stdout);
		free(cmessage);

		//if send message is close message then close socket 
		if(isExit(sendMessage)){
			cout<<"closing socket....\n";
			close(sockfd);
			cout<<"exiting client.....\n";
			exit(1);
		}
	}
}


// returns true if s is an integer
bool isInteger(char *s){
	int i;
	for(i=0;i<strlen(s);i++)
		if(s[i] < '0' || s[i] > '9')
			return false;
	return true;
}


// This funtion is calles when we press ctrl-c to send close signal to server 
void signalHandler(int signalValue){
	//if isConnected is false
	//That means its not connected to server 
	if(!isConnected){
		cout<<"Error connecting to server\n";
		exit(0);
	}
	cout<<"sending close to server..\n";
	//char array to send message to server
	char sendMessage[MAXLINE];
	//It set sendMessage as close 
	snprintf(sendMessage, sizeof(sendMessage), "close");
	//encode the message 
	encode(sendMessage);
	struct Msg message;
	//set the type id as 3 as this is exit message 
	message.type_id = 3;
	strcpy(message.msg, sendMessage);
	char* message_com = msgToChar(message);
	Writen(cfd, message_com, strlen(message_com));
	free(message_com);
	cout<<"closing client sockets..\n";
	close(cfd);
	cout<<"exiting..\n";
	exit(0);
}


int main(int argc, char **argv){
	
	// set signal handler for ctrl-c interupt
	signal(SIGINT, signalHandler); 
	
	bool isArg = true;
	//check whether writes arguments passed or not
	if(argc != 3){
		cout<<"\n Please enter IP address and port no as argument \n ";
		cout<<"usage: <executable> <IP address> <port>\n";	
		exit(1);
	}

	//This function check whether port no is in unsigned integer or not 
	if(!isInteger(argv[2])){
		cout<<"port must be an unsigned integer\n";
		exit(1);
	}	
	
	// Here we check that whether user give correct port no or not (i.e in range )
	int serv_port = atoi(argv[2]);
	if(serv_port < 0 || serv_port > 65535){
		cout<<"port number must be between 0 and 65535.\n";
	}

	cout<<"\n If you want to close the connection: type exit \nWaiting for connection...\n";

	isConnected = false;

	// creating socket.
	int sockfd;
	struct sockaddr_in serv_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	cfd = sockfd;

	// fill socket address structure
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serv_port);
	inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

	// connect to server
	connect(sockfd, (SA *)&serv_addr, sizeof(serv_addr));

	cout<<"Connected!\n";

	//Set connection values as ture 
	isConnected = true;

	//start the client Now we can send the message to server 
	startClient(stdin, sockfd);

	return 0;
}