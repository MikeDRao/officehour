/*
	Name: Michael Rao
	ID: 1001558150

  Name: Justin Erdmann
	ID: 1001288553
*/

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4

// Global synchronization variables
pthread_mutex_t mutex;
pthread_cond_t classA;
pthread_cond_t classB;
// Indicates the class of the previous student
char prevStudent = 'A';
int consecutiveA = 0;
int consecutiveB = 0;
// Indicates the amount of students total students that haven't been processed
int totalStudents = 0;
// Indicates the amount of students from Class A that haven't been processed
int A_students_left = 0;
// Indicates the amount of students from Class B that haven't been processed
int B_students_left = 0;

/* We need to keep track of The total students, Students left from A, and
   students left from B in order to meet Requirement #4. If 5 students from
   Class A have consecutively visited the professor we need to let students
   from Class B in - unless there are no students in the queue that are class B.
*/

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;

typedef struct
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;


static int initialize(student_info *si, char *filename)
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  // initialize mutex variables
  pthread_mutex_init( &mutex, NULL);
  // initialize mutex conitionals
  pthread_cond_init( &classA, NULL);
  pthread_cond_init( &classB, NULL);


  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL)
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) &&
           i < MAX_STUDENTS )
  {
    // get a running total of class A students, total students and class b
    // students
    if(si[i].class == CLASSA){
      totalStudents++;
      A_students_left++;
    }else{
      totalStudents++;
      B_students_left++;
    }
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break
 * You do not need to add anything here.
 */
static void take_break()
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Professor thread grants students entry into the office based on the
 conditions simulated by the global variables
*/
void *professorthread(void *junk)
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */
  while (1)
  {
    // Take a break if 10 consecutive students arrive (also wait until all
    // students have left the office before taking a break)
    if(students_since_break == 10 && students_in_office == 0)
    {
      take_break();
    }
    // allow class B in office when class A finishes
    if(classb_inoffice == 0){
      pthread_cond_signal(&classA);
    }
    // allow class A in office when class B finishes
    if(classa_inoffice == 0){
      pthread_cond_signal(&classB);
    }

  }
  pthread_exit(NULL);
}


/* This function will pause the student thread until the conditions are met
  based on the global variables. It will also keep a tally of the consecutive
  students from Class A that have entered in order to preserve the fairness of
  the office per Requirement #4
*/
void classa_enter()
{
  // lock the mutex before manipulating variables
  pthread_mutex_lock(&mutex);
  // Check for break condition, 3 students in office, check 5 consecutive A
  // students and check if the office has any B students... if any of these
  // conditions are true hold the thread until all conditions are fufilled
  while(students_since_break == 10 || students_in_office == 3
   || classb_inoffice > 0 ||
   (consecutiveA == 5 && totalStudents != A_students_left))
  {
    pthread_cond_wait(&classA, &mutex);
  }
  // Once thread runs, add 1 to the consecutive A variable if the previous
  // student was an A student
  if(prevStudent == 'A')
  {
    // If the last student was from Class A -> reset the Class B consecutive
    // total..
    consecutiveA++;
    consecutiveB = 0;
  }
  else
  // if the last student was a B student then change the previous student to A
  // and add one to the consecutive A variable
  {
    consecutiveA = 1;
    prevStudent = 'A';
  }
  students_in_office += 1;
  students_since_break += 1;
  classa_inoffice += 1;
  // release the lock on the global variables
  pthread_mutex_unlock(&mutex);

}

/* This function will pause the student thread until the conditions are met
  based on the global variables. It will also keep a tally of the consecutive
  students from Class B that have entered in order to preserve the fairness of
  the office per Requirement #4
*/
void classb_enter()
{
    // lock the mutex before manipulating variables
  pthread_mutex_lock(&mutex);
  // Check for break condition, 3 students in office, check 5 consecutive A
  // students and check if the office has any A students... if any of these
  // conditions are true hold the thread until all conditions are fufilled
  while(students_since_break == 10 || students_in_office == 3
    || classa_inoffice > 0 ||
    (consecutiveB == 5 && totalStudents != B_students_left))
  {
    pthread_cond_wait(&classB, &mutex);
  }
  // Once thread runs, add 1 to the consecutive B variable if the previous
  // student was an A student
  if(prevStudent == 'B')
  {
    consecutiveB++;
    consecutiveA = 0;
  }
  // If the last student was a A student then change the previous student to B
  // and add one to the consecutive B variable
  else
  {
    consecutiveB = 1;
    prevStudent = 'B';
  }
  students_in_office += 1;
  students_since_break += 1;
  classb_inoffice += 1;
  // release the lock on the global variables
  pthread_mutex_unlock(&mutex);

}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.
 */
static void ask_questions(int t)
{
  sleep(t);
}


/* This function will modify the global variables as students leave the office
*/
static void classa_leave()
{
  pthread_mutex_lock(&mutex);
  students_in_office -= 1;
  classa_inoffice -= 1;
  // change these variables accordingly (used for 5 consecutive student
  // requirement)
  A_students_left--;
  totalStudents--;
  pthread_mutex_unlock(&mutex);
}

/* This function will modify the global variables as students leave the office
*/
static void classb_leave()
{
  pthread_mutex_lock(&mutex);
  students_in_office -= 1;
  classb_inoffice -= 1;
  // change these variables accordingly (used for 5 consecutive student
  // requirement)
  B_students_left--;
  totalStudents--;
  pthread_mutex_unlock(&mutex);
}

/* Main code for class A student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */

void* classa_student(void *si)
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );

  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si)
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args)
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2)
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0)
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result)
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++)
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);

    student_type = random() % 2;

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result)
    {
      printf("officehour: thread_fork failed for student %d: %s\n",
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++)
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");

  return 0;
}
