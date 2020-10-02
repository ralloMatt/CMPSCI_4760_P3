# CMPSCI_4760_P3
Operating systems project 3.

Semaphores and Operating System Simulator

To Run Program:
	It can be run as "./oss" after executable has been made by make.
	
	If it is run with just "./oss" the defaults will be used and the 
	file "logfile.txt" will be written to.
	
	It also can be ran as described in the project with the command line options.
	
Note: 
	Using the defaults and executing with just "./oss" will produce results in file for 
	sure. It usually would end either by two simulated seconds passing by or by 100 processes 
	being made. Both of them produced results in the log file. Ending by two simulated 
	seconds seems to be more likely.
	
Files:
	Makefile
	oss.c
	user.c
	README
	*When it's first run if no file is given, it will create "logfile.txt"
	
Thoughts:
	Had some trouble figuring out how to implement semaphores. I believe it works but 
	not entirely sure. Also when incrementing the clock I could only go to about 1000 
	for each iteration because if it was too little two real seconds would pass by and 
	if it was too much two simulated seconds would pass by. Both of which would end up 
	with an empty log file.
	
Version Control:
	Used git just like previous projects. Using "git log" will show my progress.
	
Code:
	The files "oss.c" and "user.c" are commented showing what I did.
