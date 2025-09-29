/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Sam Huhn
	UIN: 433008144
	Date: 9/26/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <chrono>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m=MAX_MESSAGE;
	bool newChannel=false;
	
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
				m=atoi(optarg);
				break;
			case 'c':
				newChannel=true;
				break;
		}
	}
	//fork
    pid_t pid=fork();

    if(pid<0){
	EXITONERROR("ERROR: Fork Failed");
    }
    if(pid==0){
	char *args[4];
	args[0]=(char*)"./server";
	args[1]=(char*)"-m";
	string cap = to_string(m);
	char* cap_cstr = new char[cap.size() +1];
	strcpy(cap_cstr,cap.c_str());
	args[2]=cap_cstr;
	args[3]=NULL;
	execvp(args[0],args);
	EXITONERROR("ERROR: execvp failed");
    }

    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);
    FIFORequestChannel* data_chan = &control_chan;

    if(newChannel) {
	MESSAGE_TYPE nc = NEWCHANNEL_MSG;
	control_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
	char new_chan_name[MAX_MESSAGE];
	control_chan.cread(new_chan_name, sizeof(new_chan_name));
	data_chan = new FIFORequestChannel(new_chan_name, FIFORequestChannel::CLIENT_SIDE);
   }
	
     // data point requests
     if(p != -1 && t != -1 && e != -1){
	datamsg x(p, t, e);
	data_chan->cwrite(&x, sizeof(datamsg)); // question
	double reply;
	data_chan->cread(&reply, sizeof(double)); //answer
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else if(p != -1 && t == -1 && e == -1){
	ofstream out("received/x1.csv");
	for(int i=0;i<1000;i++){
		double time = i*0.004;
		datamsg d1(p,time,1);
		data_chan->cwrite(&d1, sizeof(datamsg));
		double value1;
		data_chan->cread(&value1, sizeof(double));
		datamsg d2(p,time,2);
		data_chan->cwrite(&d2, sizeof(datamsg));
		double value2;
		data_chan->cread(&value2, sizeof(double));
		out<<time<<","<<value1<<","<<value2<<endl;
	}
	out.close();
	cout<<"Wrote first 1000 data points for patient "<<p<<" to received/x1.csv"<<endl;
    	}
    // file request
    if(filename!=""){
	auto start = chrono::high_resolution_clock::now();
	filemsg fm(0, 0);
	int len = sizeof(filemsg)+filename.size()+1;
	char* buf = new char[len];
	memcpy(buf, &fm,sizeof(filemsg));
	strcpy(buf + sizeof(filemsg), filename.c_str());
	data_chan->cwrite(buf,len);
	__int64_t filesize;
	data_chan->cread(&filesize, sizeof(__int64_t));
	ofstream ofs("received/"+filename,ios::binary);
	char* filebuf = new char[m];
	__int64_t offset=0;
	while(offset < filesize){
		int chunk = min((__int64_t)m, filesize-offset);
		((filemsg*)buf)->offset=offset;
		((filemsg*)buf)->length=chunk;
		data_chan->cwrite(buf,len);
		data_chan->cread(filebuf,chunk);
		ofs.write(filebuf,chunk);
		offset+=chunk;
	}
    ofs.close();
    delete[] buf;
    delete[] filebuf;
    auto end = chrono::high_resolution_clock::now();
    double sec = chrono::duration<double>(end-start).count();
    cout<<"Transfer time: " <<sec<<" secounds"<<endl;
    cout<< "Received file "<<filename<<"("<<filesize<<" bytes) into received/" << filename<<endl;
    }
	
    MESSAGE_TYPE qm = QUIT_MSG;
    if(newChannel){
	data_chan->cwrite(&qm,sizeof(MESSAGE_TYPE));
	delete data_chan;
    }
    control_chan.cwrite(&qm,sizeof(MESSAGE_TYPE));

    wait(nullptr);
}

