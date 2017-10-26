/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <queue.h>
#include <thread.h>
#include <curthread.h>



/*
 *
 * Constants
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

struct lock *NW, *NE, *SW, *SE, *car1, *car2, *N, *E, *S, *W;
//struct cv=north, east, south, west;





static void initialize(){
	

	const char *name1="NW";
	const char *name2="NE";
	const char *name3="SW";
	const char *name4="SE";
	
	const char *bmw="car1";
	const char *peugot="car2";
	NW=lock_create(name1);
	NE=lock_create(name2);
	SW=lock_create(name3);
	SE=lock_create(name4);
	
	N=lock_create(name1);
	E=lock_create(name2);
	W=lock_create(name3);
	S=lock_create(name4);
	car1=lock_create(bmw);
	car2=lock_create(peugot);
	

	
}


static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
		
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */


static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        

		
		if (cardirection==2){//south, going north
				message(0, carnumber, cardirection, 0);
				
				lock_acquire(car1); //trying to enter the intersection, restricted to 2 cars
				lock_acquire(S); //make sure that the first car in the S entrance goes first
				lock_acquire(SE);
				message(1, carnumber, cardirection, 0);
				lock_release(S);
				
				lock_acquire(NE);
				message(2, carnumber, cardirection, 0);
				lock_release(SE);				
					
				
								
	

				
				message(4, carnumber, cardirection, 0);
				lock_release(NE);
				
				lock_release(car1);

			
		}

		else if (cardirection==3){//W going E
				message(0, carnumber, cardirection, 1);
				
				lock_acquire(car2);
				lock_acquire(W);
				lock_acquire(SW);
				message(1, carnumber, cardirection, 1);
				lock_release(W);
				//while(SE->count!=0 && carnumber!=q_top(se)){}		
				
							
				
				//while(NE->count!=0 && carnumber!=q_top(ne)){}
				lock_acquire(SE);
				message(2, carnumber, cardirection, 1);
				lock_release(SW);				
					
				
								
			

				
				message(4, carnumber, cardirection, 1);
				lock_release(SE);
				lock_release(car2);

			
		}

		else if (cardirection==0){//N going S
				message(0, carnumber, cardirection, 2);
			
				
						
				lock_acquire(car1);
				
				lock_acquire(N);
				lock_acquire(NW);
					message(1, carnumber, cardirection, 2);
				lock_release(N);
				//while(SE->count!=0 && carnumber!=q_top(se)){}		
			
							
				
				//while(NE->count!=0 && carnumber!=q_top(ne)){}
				lock_acquire(SW);
				message(2, carnumber, cardirection, 2);
				lock_release(NW);				
					

				
				message(4, carnumber, cardirection, 2);
				lock_release(SW);
				lock_release(car1);

			
		}

		else if (cardirection==1){//E going W
				message(0, carnumber, cardirection, 3);
			
				lock_acquire(car2);
				lock_acquire(E);
				lock_acquire(NE);
				message(1, carnumber, cardirection, 3);
				lock_release(E);
				//while(SE->count!=0 && carnumber!=q_top(se)){}		
				
							
			
				//while(NE->count!=0 && carnumber!=q_top(ne)){}
				lock_acquire(NW);
				message(2, carnumber, cardirection, 3);
				lock_release(NE);				
				
				
				message(4, carnumber, cardirection, 3);
				lock_release(NW);
				lock_release(car2);

			
		}


		
		
	
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
		
        
		if (cardirection==0){ //north going E
			

				
				message(0, carnumber, cardirection, 1);
				lock_acquire(car1); //trying to enter the intersection, restricted to 2 cars
			
				lock_acquire(N);
				lock_acquire(NW);
				message(1, carnumber, cardirection, 1);
				lock_release(N);
				//while(SE->count!=0 && carnumber!=q_top(se)){}		
				
							
	
				lock_acquire(SW);
				message(2, carnumber, cardirection, 1);
				lock_release(NW);				
				lock_acquire(SE);
				message(3, carnumber, cardirection, 1);	
				lock_release(SW);
				message(4, carnumber, cardirection, 1);
				lock_release(SE);				
				//lock_acquire(SEleft)

				
		
				
				lock_release(car1);	
	
			
		}
		else if (cardirection==1){ //E going S
		

				message(0, carnumber, cardirection, 2);
			
				lock_acquire(car2); //trying to enter the intersection, restricted to 2 cars
				lock_acquire(E);
				lock_acquire(NE);
				message(1, carnumber, cardirection, 2);
				lock_release(E);
				
				lock_acquire(NW);
				message(2, carnumber, cardirection, 2);
				lock_release(NE);				
				lock_acquire(SW);
				message(3, carnumber, cardirection, 2);	
				lock_release(NW);
				message(4, carnumber, cardirection, 2);
				lock_release(SW);				
				//p=q_remhead(NW);

				
				
				
				lock_release(car2);

			
			
		}

		else if (cardirection==2){ //S going W
			
				message(0, carnumber, cardirection, 3);
				
				lock_acquire(car1); 
				lock_acquire(S);
				lock_acquire(SE);
				message(1, carnumber, cardirection, 3);	
				lock_release(S);
				
				lock_acquire(NE);
				message(2, carnumber, cardirection, 3);
				lock_release(SE);				
				lock_acquire(NW);
				message(3, carnumber, cardirection, 3);	
				lock_release(NE);
					message(4, carnumber, cardirection, 3);
				lock_release(NW);						
			
				lock_release(car1);

		
		}

		else if (cardirection==3){ //W going N
			

			message(0, carnumber, cardirection, 0);
				
				lock_acquire(car2); 
				lock_acquire(W);
				lock_acquire(SW);
				message(1, carnumber, cardirection, 0);	
				lock_release(W);

				lock_acquire(SE);
				message(2, carnumber, cardirection, 0);
				lock_release(SW);				
				lock_acquire(NE);
				message(3, carnumber, cardirection, 0);	
				lock_release(SE);
			message(4, carnumber, cardirection, 0);
				lock_release(NE);						
		
				lock_release(car2);

			
		}
				
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
       if (cardirection==2){//south, going e
				message(0, carnumber, cardirection, 1);
				
				lock_acquire(car1); //trying to enter the intersection, restricted to 2 cars
				lock_acquire(S); //make sure that the first car in the S entrance goes first
				lock_acquire(SE);
				message(1, carnumber, cardirection, 1);
				lock_release(S);
				message(4, carnumber, cardirection, 1);
				lock_release(SE);				

				

				
				lock_release(car1);

			
		}

		else if (cardirection==3){//W going s
				message(0, carnumber, cardirection, 2);
				
				lock_acquire(car2);
				lock_acquire(W);
				lock_acquire(SW);
				message(1, carnumber, cardirection, 2);
				lock_release(W);
				message(4, carnumber, cardirection, 2);
				lock_release(SW);				
					
				
				
			
				lock_release(car2);

			
		}

		else if (cardirection==0){//N going W
				message(0, carnumber, cardirection, 3);
			
				
						
				lock_acquire(car1);
				
				lock_acquire(N);
				lock_acquire(NW);
				message(1, carnumber, cardirection, 3);
				lock_release(N);
				message(4, carnumber, cardirection, 3);
				lock_release(NW);				
					

			
				lock_release(car1);

			
		}

		else if (cardirection==1){//E going N
				message(0, carnumber, cardirection, 0);
			
				lock_acquire(car2);
				lock_acquire(E);
				lock_acquire(NE);
				message(1, carnumber, cardirection, 0);
				lock_release(E);
				message(4, carnumber, cardirection, 0);
				lock_release(NE);				
				
				
			

				lock_release(car2);

			
		}
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
		
        /*
         * Avoid unused variable and function warnings.
         */

      


        /*
         * cardirection is set randomly.
         */

        cardirection = random()%4;
		int turn=random()%3;
		
		
			
		
				
		//add threads to direction queue and go to sleep

		if (turn==0){
			gostraight(cardirection, carnumber);
		}

		else if (turn==1){
			turnleft(cardirection, carnumber);
		}

		else{
			turnright(cardirection, carnumber);
		}
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */


int
createcars(int nargs,
           char ** args)
{
	initialize();        
	int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }
		//loop through NCARS and wake up cars in queues in every direction
		
			

        return 0;
}


