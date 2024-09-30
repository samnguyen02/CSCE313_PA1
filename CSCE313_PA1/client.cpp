
/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name:
	UIN:
	Date:
*/
/*
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	string filename = "";

	//Add other arguments here
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	//Task 1:
	//Run the server process as a child of the client process

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	//Task 4:
	//Request a new channel
	
	//Task 2:
	//Request data points
    char buf[MAX_MESSAGE];
    datamsg x(1, 0.0, 1);
	
	memcpy(buf, &x, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg));
	double reply;
	chan.cread(&reply, sizeof(double));
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
	//Task 3:
	//Request files
	filemsg fm(0, 0);
	string fname = "1.csv";
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);

	delete[] buf2;
	__int64_t file_length;
	chan.cread(&file_length, sizeof(__int64_t));
	cout << "The length of " << fname << " is " << file_length << endl;
	
	//Task 5:
	// Closing all the channels
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
*/

#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	
	int m = MAX_MESSAGE; // buffer capacity at max capacity
	bool newChannel = false;
	vector<FIFORequestChannel*> availableChannels;
	
	string filename;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
		case 'p': 
			p = atoi (optarg);
			break;
		case 't':
			t = atof (optarg);
			break;
		case 'e':
			e = atoi (optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'm':
			m = atoi (optarg);
			break;
		case 'c':
			newChannel = true;
			break;
		}
	}

	int child_pid=fork(); //fork runs server when running client

	if (child_pid==0){
		//runs through child process
		char *args[] = {(char*)"./server", (char*)"-m", (char*)(to_string(m).c_str()), NULL};
		int ret = execvp(args[0], args);
		if(ret == -1){
			perror("execvp");
			exit(EXIT_FAILURE);
		}
	}else{
		FIFORequestChannel controlChan("control", FIFORequestChannel::CLIENT_SIDE);
		availableChannels.push_back(&controlChan);

		char chanName[30]; //keep new channel name

		if (newChannel){
			//make new channel
			MESSAGE_TYPE newC = NEWCHANNEL_MSG;
			controlChan.cwrite(&newC, sizeof(MESSAGE_TYPE));

			//use chanName to read
			//cread response from server
			controlChan.cread(&chanName,30);

			//create new channel using chanName read from server
			FIFORequestChannel* newChan = new FIFORequestChannel(chanName, FIFORequestChannel::CLIENT_SIDE);
			//use new to mark channel as new for if statement

			//add new channel to available channels list
			availableChannels.push_back(newChan);
			
		}

		FIFORequestChannel chan = *(availableChannels.back()); //grab end of available channels list
		char buf[MAX_MESSAGE]; //buffer

		if ((p != -1) && (t != -1) && (e != -1)){
			//if asking for single data point
			datamsg x(p,t,e); //use vals to get datamsg

			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

		}else if ((p != -1) && (t == -1) && (e == -1) && (filename.empty())){
			//write to x1.csv
			ofstream oFile;
			oFile.open("received/x1.csv");
			double time = 0.0;

			double r1,r2;

			//loop first 1000 lines for specified person -> 2000 requests for cg1 & cg2
			for (int i = 0; i < 1000; i++){
				datamsg e1(p,time,1);
				memcpy(buf, &e1, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				chan.cread(&r1, sizeof(double)); //answer

				datamsg e2(p,time,2);
				memcpy(buf, &e2, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				chan.cread(&r2, sizeof(double)); //answer

				oFile << time << ',' << r1 << ',' << r2 << '\n';

				time += 0.004;
			}

			oFile.close(); //remember to close file
		}else if (!(filename.empty())){
			//requesting full file
			ofstream timingInfo;
			timingInfo.open("timingInfo" + filename + ".txt");

			filemsg fm(0, 0);
			string fname = filename;
			ofstream oFile;
			//get destination to write to
			FILE* dest = fopen((char *)(((string)"received/").append(fname)).c_str(),"wb");

			int len = sizeof(filemsg) + (fname.size() + 1);
			char* buf2 = new char[len]; //new buf
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan.cwrite(buf2, len); //want file length

			int64_t filesize = 0;
			chan.cread(&filesize, sizeof(int64_t));

			char* buf3 = new char[m];

			int64_t currOffset = 0;
			int64_t remaining = filesize; //use new file size & buf capacity to loop through every line
			for (int i = 0; i < (filesize / m) + 1; i++){
				//make file message instance
				filemsg* fileRequest = (filemsg*)buf2; //casts char to filemsg
				fileRequest->offset = currOffset; // set offset in the file

				if (remaining >= m){
					//set length
					fileRequest->length = m;
				}else{
					fileRequest->length = remaining;
				}

				chan.cwrite(buf2, len); //read response
				//read server response and place in buf3 -- sets length of file request
				chan.cread(buf3,fileRequest->length);
				//write buf3 into dest
				fwrite(buf3,1,fileRequest->length,dest);
				currOffset += m;//update currOffset and remaining for loop
				remaining-=m;
			}

			//make sure to delete and close files for memory leaks
			delete[] buf2;
			delete[] buf3;
			fclose(dest);
		}

		MESSAGE_TYPE mess = QUIT_MSG;

		if (newChannel){//write to available channels 
			availableChannels.at(0)->cwrite(&mess,sizeof(MESSAGE_TYPE));
			availableChannels.at(1)->cwrite(&mess, sizeof(MESSAGE_TYPE));
			delete availableChannels.at(1);
		}else{
			chan.cwrite(&mess, sizeof(MESSAGE_TYPE)); //else quit message
		}
	}
}
