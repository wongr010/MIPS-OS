/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


// draft for is lab

/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
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
#include <curthread.h>
#include <machine/spl.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2


/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */


struct semaphore* B1sem; //2 bowls
struct semaphore* B2sem;

void catEatBowl(struct semaphore* bowlSem, unsigned long catnumber,
        int bowlNum, struct semaphore* anotherSem) {


    Pcat(bowlSem, anotherSem); //check which bowl is empty and acquire it, I think...

    sem_eat("cat", catnumber, bowlNum, curthread->iteration); //eat from tis bowl

    Vcat(bowlSem, anotherSem); //leave the bowl

    curthread->iteration++; //this cat has eaten n times 
}

void mouseEatBowl(struct semaphore* bowlSem, unsigned long mousenumber,
        int bowlNum, struct semaphore* anotherSem) {



    Pmouse(bowlSem, anotherSem);

    sem_eat("mouse", mousenumber, bowlNum, curthread->iteration);

    Vmouse(bowlSem, anotherSem);

    curthread->iteration++;
}

static void
sem_eat(const char *who, int num, int bowl, int iteration) {

    kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num,
            bowl, iteration);
    clocksleep(1);
    kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num,
            bowl, iteration);
}

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static void catsem(void * unusedpointer,
        unsigned long catnumber) {
    /*
     * Avoid unused variable warnings.
     */

    (void) unusedpointer;
    (void) catnumber;



    while (curthread->iteration < 3) { //Eat 3 times and leave



        catEatBowl(B2sem, catnumber, 2, B1sem); //Choose an empty bowl to eat from, eat 3 times
        catEatBowl(B1sem, catnumber, 1, B2sem); 



    }



    //    while (curthread->iteration < 4) {
    //
    //
    //        if (B2sem->count == 0) {
    //            catEatBowl(B1sem, catnumber, 1, B2sem);
    //        } else {
    //            catEatBowl(B2sem, catnumber, 2, B1sem);
    //        }
    //
    //    }





    kprintf("\n ********* cat %d is done eating!\n", catnumber);

}

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer,
        unsigned long mousenumber) {
    /*
     * Avoid unused variable warnings.
     */

    (void) unusedpointer;
    (void) mousenumber;




    while (curthread->iteration < 3) {

        mouseEatBowl(B1sem, mousenumber, 1, B2sem);
        mouseEatBowl(B2sem, mousenumber, 2, B1sem);


    }



    //    while (curthread->iteration < 4) {
    //
    //
    //        if (B1sem->count == 0) {
    //            mouseEatBowl(B2sem, mousenumber, 2, B1sem);
    //        } else {
    //            mouseEatBowl(B1sem, mousenumber, 1, B2sem);
    //        }
    //
    //    }

    kprintf("\n ********* mouse %d is done eating!\n", mousenumber);

}

/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
        char ** args) {
    int index, error;

    /*
     * Avoid unused variable warnings.
     */

    (void) nargs;
    (void) args;

    // initialize semaphores
    const char* name1 = "B1";
    const char* name2 = "B2";

    B1sem = sem_create(name1, 1);
    B2sem = sem_create(name2, 1);

    /*
     * Start NCATS catsem() threads.
     */

    for (index = 0; index < NCATS; index++) {

        error = thread_fork("catsem Thread",
                NULL,
                index,
                catsem,
                NULL
                );

        /*
         * panic() on error.
         */

        if (error) {

            panic("catsem: thread_fork failed: %s\n",
                    strerror(error)
                    );
        }
    }

    /*
     * Start NMICE mousesem() threads.
     */

    for (index = 0; index < NMICE; index++) {

        error = thread_fork("mousesem Thread",
                NULL,
                index,
                mousesem,
                NULL
                );

        /*
         * panic() on error.
         */

        if (error) {

            panic("mousesem: thread_fork failed: %s\n",
                    strerror(error)
                    );
        }
    }

    return 0;
}


/*
 * End of catsem.c
 */

