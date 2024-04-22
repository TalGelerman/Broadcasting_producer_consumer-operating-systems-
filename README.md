The overall scenario that is simulated is that of news broadcasting. Different types of stories are produced and the system sorts them and displays them to the public.
The ‘new stories’ are simulated by simple strings which are displayed to the screen in the order they arrive.

![image](https://github.com/TalGelerman/Operating_systems/assets/106587324/ae719e9a-8ad9-4c9f-bbef-2c18c4209bae)

There are 4 types of active actors:
- Producer 
    Each producer creates a number of strings in the following format:
    “producer <i>  <type>  <j>”

Each of the producers passes its information to the Dispatcher (introduced below) via its own private queue. Each of the Producers private queue is shared between the Producer and the Dispatcher. Each of the string products is inserted by the Producer to its ‘producers queue’. After inserting all the products, the Producer sends a ‘DONE’ string through its Producers queue.

- Dispatcher
    The Dispatcher continuously accepts messages from the Producers queues. It scans the Producers queue using a Round Robin algorithm. The Dispatcher does not block when the queues are empty. Each message is "sorted" by the Dispatcher and inserted to a one of the Dispatcher queues which includes strings of a single type. When the Dispatcher receives a "DONE" message from all Producers, it sends a "DONE" message through each of its queues.

- Co-Editors
    For each type of possible messages there is a Co-Editor that receives the message through the Dispatchers queue, "edits" it, and passes it to the screen manager via a single shared queue. The editing process will be simulated by the Co-Editors by blocking for one tenth (0.1) of a second. When a Co-Editor receives a "DONE" message, it passes it without waiting through the shared queue.

- Screen-manager
  The Screen-manager displays the strings it receives via the Co-Editors  queue to the screen (std-output). After printing all messages to the screen and receiving three "DONE" messages, the Screen manager displays a ‘DONE’ statement.

      
