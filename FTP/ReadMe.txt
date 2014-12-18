Renan Santana

This is a notice:

1. I created a folder called "ServerFiles" and "ClientFiles" all under this 
	parent directory. So that I can perform the tasks easier.

2. I've managed to get the program transferring image files (small and large)
	and other formats. 

3. I incountered a problem where I'm able to do the 'put' / 'get' but
	the files had a lock icon on them. I had to use the chmod command 
	to enable me to open the file and view. I've searched how to do 
	this in C so that I don't need to type the command in the terminal 
	but it didn't work as I intended it to work.

	Here's what I'm now doing:
	sudo chmod ugo+rw /home/<user>/Desktop/<parent dir>/ClientFiles/
	
4. I didn't test how these programs will perform in a network that
	has delays and package drops.
	
5. 'get' / 'put' a empty file is a problem.
