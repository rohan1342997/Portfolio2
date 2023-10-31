/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Rohan Ali
	UIN: 732009815
	Date: 9/24/2023
*/

#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int m1 = MAX_MESSAGE;
	bool nc = false;
	vector<FIFORequestChannel*> channels;

	string filename = "";
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
				m1 = atoi (optarg);
				break;
			case 'c':
				nc = true;
				break;
		}
	}

	pid_t pid = fork();

	if(pid < 0){
		perror("failed fork");
		exit(1);
	}
	else if(pid == 0){
		string m2 = to_string(m1);
		char* m3 = &m2[0];
		char f[] = "./server";
		char mcomm[] = "-m";
		char* server_arguments[] = {f, mcomm, m3, NULL};
		execvp("./server", server_arguments);
	}

	FIFORequestChannel* nchan2;
    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);
	
	if(nc){
		//writes message for new channel and reads new channel name from server and puts null terminated const char* as argument for new channel constructor
		MESSAGE_TYPE nchan = NEWCHANNEL_MSG;
		string chanName = "";
		char curChar = 'a';

		cont_chan.cwrite(&nchan, sizeof(MESSAGE_TYPE));

		//reading new server name
		while(curChar != '\0'){
			cont_chan.cread(&curChar, sizeof(char));
			chanName += curChar;
			if(curChar == '\0'){
				break;
			}
		}
		
		

		const char* nchan3 = chanName.c_str();
		
		//construct new channel
		nchan2 = new FIFORequestChannel(nchan3, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(nchan2);
	}


	FIFORequestChannel chan = *(channels.back());


	//single point
	// example data point request
    if(p != -1 && t != -1.0 && e != -1){
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	//first 1000 points
	else if(p != -1){
		t = 0.0;
		fstream fRec;
		ostringstream conv;
		fRec.open("received/x1.csv", ios::out);
		string out = "";

		for(int i = 0; i < 1000; i++){
			out = "";
			char buf[MAX_MESSAGE]; // 256
			datamsg x(p, t, 1);
			
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer
			
			char buf2[MAX_MESSAGE]; // 256
			datamsg x2(p, t, 2);

			memcpy(buf2, &x2, sizeof(datamsg));
			chan.cwrite(buf2, sizeof(datamsg)); // question
			double reply2;
			chan.cread(&reply2, sizeof(double)); //answer
			
			//precise conversions
			conv << t;
			out += conv.str();
			conv.str("");
			conv.clear();
			out += ",";

			conv << reply;
			out += conv.str();
			conv.str("");
			conv.clear();
			out += ",";

			conv << reply2;
			out += conv.str();
			conv.str("");
			conv.clear();

			if(i != 999)
				out += "\n";
			
			//writing to file
			fRec << out;

			//time change
			t = t + 0.004;
		}
		
		//closing file
		fRec.close();
	}



    // file transfer
	filemsg fm(0, 0);
	string fname = filename;
	ofstream fCopy;
	fCopy.open("received/" + fname);

	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);  // I want the file length;

	int64_t filesize = 0;
	chan.cread(&filesize, sizeof(int64_t));

	char* buf3 = new char[m1];
	filemsg* f_req = (filemsg*)buf2;

	if(filename != ""){
		for(int i = 0; i < (filesize/m1) + 1; i++){
			f_req->offset = i * m1;
			

			if(i == filesize/m1){
				f_req->length = filesize % m1;
			}
			else{
				f_req->length = m1;
			}
			//writing and reading according to offset and length with attention to last segment of bytes
			chan.cwrite(buf2, len);
			chan.cread(buf3, f_req->length);

			//write to file as bytes
			fCopy.write(buf3, f_req->length);
		}
	}

	fCopy.close();
	
	
	delete[] buf3;
	delete[] buf2;

	//closes whichever channel calculations were done in
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	
	//deletes new channel pointer and ends control channel if a new channel was created 
	if(nc){
		delete nchan2;
		nchan2 = nullptr;

		MESSAGE_TYPE endchan = QUIT_MSG;
    	cont_chan.cwrite(&endchan, sizeof(MESSAGE_TYPE));
	}
}
