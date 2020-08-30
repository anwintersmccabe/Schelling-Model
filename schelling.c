/* A simulation of the schelling segregation model
 *
 * Authors: Sydney Finkelstein, Annabel Winters-McCabe
 * 
*/

#include <stdio.h>
#include<stdlib.h>
#include<math.h>
#include<unistd.h>

int check_placement(char* board, int row, int col, int num_rows, int num_cols,
  float threshold, char type);
int unhappy_boolean(int*unhappy_spots, int num_chars);
int* check_agents(char*board,int* unhappy_spots,int rows, int cols,
  float threshold);
char* move_placement(char*board, int row, int col, int rows, int cols);
void print_board(char* board, int rows, int cols);



int main(int argc, char** argv){
  //check for right number of command line arguments
  if(argc != 3) {
    printf("Expected 2 command arguments:\n");
    printf("usage: ./products a\n");
    exit(1);
  }

  //bring in file and read
  FILE *infile = fopen(argv[1], "r");
  if (infile == NULL){
    printf("Couldn't open file: %s\n" , argv[1]);
    fclose(infile);
    exit(1);
  }

  //get verbosity level
  int verbosity = atoi(argv[2]);

  //check if verbosity is valid
  if (verbosity<0||verbosity>2){
    printf("Verbosity level invalid. Should be 0, 1, or 2\n");
    fclose(infile);
    exit(1);
  }

  int rows, cols;
  int num_iterations;
  float threshold;
  int num_d, num_p;

  //bring in file info, while checking to make sure the file is correctly
  //formatted
  if (fscanf(infile, "%d %d", &rows, &cols)!=2||
  fscanf(infile, "%d", &num_iterations)!=1||fscanf(infile, "%f\n", &threshold)!=1
  ||fscanf(infile, "%d", &num_d)!=1){
    printf("File is not correctly formatted\n");
    fclose(infile);
    exit(2);
  }

  char* board = (char*) malloc(rows*cols * sizeof(char)); //allocating space for
  //length by width board where "board" pointing to beginning of this
  //chunk of memory

  //if the space was not allocated, return error code 3
  if(board == NULL) {
    printf("malloc of size %d failed!\n", rows*cols);
    fclose(infile);
    free(board);
    board=NULL;
    exit(3);   // return an error to caller
  }

  int row;
  int col;
  //initialize board with empty spots (spaces)
  for (int i=0; i<rows; i++){
    for (int j=0; j<cols; j++){
              board[i*cols + j]=' ';
    }
  }

  //initialize board w/ "$" in correct locations
  for(int i=0; i<num_d; i++){
    if (fscanf(infile, "%d %d", &row, &col)!=2){
      printf("File is not correctly formatted\n");
      free(board);
      board=NULL;
      fclose(infile);
      exit(2);
    }
    board[row*cols+col]='$';
  }

  if (fscanf(infile,"%d",&num_p)!=1){
    printf("File is not correctly formatted\n");
    free(board);
    board=NULL;
    fclose(infile);
    exit(2);
  }

  //initialize board w/ "." in correct locations
  for (int i=0; i<num_p; i++){
    if (fscanf(infile, "%d %d", &row, &col)!=2){
      printf("File is not correctly formatted\n");
      free(board);
      board=NULL;
      fclose(infile);
      exit(2);
    }
    board[row*cols+col]='.';
  }

  if (verbosity==2){
    system("clear");
    print_board(board, rows, cols);
  }

  //allocate space for an array containing which spots in the board are
  //unhappy
  int* unhappy_spots = (int*) malloc((num_p+num_d)*sizeof(int));
  if(unhappy_spots == NULL) {
    printf("malloc of size %d failed!\n", num_p+num_d);
    fclose(infile);
    free(board);
    board=NULL;
    exit(3);   // return an error to caller
  }

  int current_row;
  int current_col;

  //fill board with -1's before checking which are happy and unhappy
  for (int j=0; j<num_p+num_d; j++){
    unhappy_spots[j]=-1;
  }


  unhappy_spots=check_agents(board,unhappy_spots,rows,cols, threshold);

  //run simulation while there are still iterations & unhappy agents
  while(unhappy_boolean(unhappy_spots, num_d+num_p)==1&&num_iterations>0){


    for(int i=0; i<num_p+num_d;i++){
      if(unhappy_spots[i]!=-1){
        current_col = unhappy_spots[i]%cols;
        current_row = (unhappy_spots[i]/cols);
        board = move_placement(board, current_row, current_col, rows, cols);

        //decrement iterations after each move
        num_iterations--;

        //if there are no more iterations left, free allocated memory and return
        if(num_iterations==0){
          if (verbosity==1){
            print_board(board, rows, cols);
          }
          //free allocated memory
          free(unhappy_spots);
          free(board);
          board = NULL;
          unhappy_spots = NULL;
          fclose(infile);
          return 0;
        }

        if (verbosity==2){
          if(num_iterations!=0){
            system("clear");
          }
          print_board(board, rows, cols);
          usleep(10000);
        }
      }
    }

    //reset unhappy spots to all placeholders
    for (int j=0; j<num_p+num_d; j++){
      unhappy_spots[j]=-1;
    }
    //get new array of unhappy agents
    unhappy_spots=check_agents(board,unhappy_spots,rows,cols, threshold);
  }

  if (verbosity==1){
    print_board(board, rows, cols);
  }
  free(unhappy_spots);
  free(board);
  unhappy_spots=NULL;
  board = NULL;
  fclose(infile);
  return 0;
}

/* Determines whether the character at the given spot in the board is happy or
 * unhappy.
 *
 * Parameters:
 *  board: the allocated array representing the
 *      board full of chars and empty spaces
 *  row: integer representing the row of the given character
 *  col: integer representing the column of the given character
 *  rows: integer representing the total number of rows in the board
 *  cols: integer representing the total number of columns in the board
 *  threshold: floating point number representing the threshold to determine
 *      whether the given spot can be considered unhappy
 *  type: character that is the character at the given spot
 *
 * Returns:
 *  returns 1 if the character in the given spot in the board is happy,
 *    and 0 if it is unhappy
 */
int check_placement(char* board, int row, int col, int rows,
  int cols,float threshold, char type){
  //if the character at that spot is empty, the spot is considered happy
  if (type==' '){return 1;}
  float count_same=0.0;
  float count_different=0.0;

  //initialize variables that represent the given spot's neighbors
  int right = row*cols+col+1;
  int left = row*cols+col-1;
  int bottom = (row+1)*cols+col;
  int top = (row-1)*cols+col;
  int top_right = (row-1)*cols+col+1;
  int top_left = (row-1)*cols+col-1;
  int bottom_right = (row+1)*cols+col+1;
  int bottom_left = (row+1)*cols+col-1;

  //if not the top row
  if(row!=0){
    //check above
    if(board[top]!=' '){
      if(board[top]==type){count_same++;}
      else{count_different++;}
    }
    //if not left column
    if(col!=0){
      //check top left
      if(board[top_left]!=' '){
        if(board[top_left]==type){count_same++;}
        else{count_different++;}
      }
    }
    //if not right column
    if(col!=cols-1){
      //check top right
      if(board[top_right]!=' '){
        if(board[top_right]==type){count_same++;}
        else{count_different++;}
      }
    }
  }
  //if not bottom row
  if(row!=rows-1){
    //check below
    if(board[bottom]!=' '){
      if(board[bottom]==type){count_same++;}
      else{count_different++;}
    }
    //if not left column
    if(col!=0){
      //check bottom left
      if(board[bottom_left]!=' '){
        if(board[bottom_left]==type){count_same++;}
        else{count_different++;}
      }
    }
    //if not right column
    if(col!=cols-1){
      //check bottom right
      if(board[bottom_right]!=' '){
        if(board[bottom_right]==type){count_same++;}
        else{count_different++;}
      }
    }
  }
  //if not left column
  if(col!=0){
    //check left
    if(board[left]!=' '){
      if (board[left]==type){count_same++;}
      else{count_different++;}
    }
  }
  //if not right column
  if(col!=cols-1){
    //check right
    if (board[right]!=' '){
      if (board[right]==type){count_same++;}
      else{count_different++;}
    }
  }

  //divide the number of the spot's neighbors that are the same as the given
  //spot by the total number of neighbors that are characters and compare to
  //threshold
  float amt_same;
  amt_same = count_same/(count_same+count_different);
  if (amt_same<threshold){return 0;}
  else{return 1;}
}

/* Checks whether the board still has unhappy characters, or whether all unhappy
 * characters from the round have been moved
 *
 * Parameters:
 *  unhappy_spots: dynamically allocated int array representing which spots in
 *    the board are unhappy and which are happy. Unhappy_spots contains a
 *    placeholder of -1 where the spots are happy
 *  num_chars: integer representing the total number of characters in the board
 *
 *  Return:
 *  1 if there is at least one dissatisifed agent left on the board, 0 if there
 *  are none.
 */
int unhappy_boolean(int*unhappy_spots, int num_chars){
  for(int i=0;i<num_chars;i++){
    if(unhappy_spots[i]!=-1){
      return 1;
    }
  }
  return 0;
}

/*
*Checks the board for dissatisfied agents and returns an array of their indices
*
*Parameters:
* board: dynamically allocated 2D array representing the state of the simulation
* unhappy_spots:dynamically allocated int array representing which spots in
*    the board are unhappy and which are happy
* rows: integer representing the # of rows in the board.
* cols: integer representing the # of columns in the board.
*
* Return:
* unhappy_spots
*/
int* check_agents(char*board, int* unhappy_spots,int rows, int cols,
  float threshold){
  char type;
  int index=0;
  int happiness;
  for (int row=0; row<rows; row++){
    for (int col=0; col<cols; col++){
      type = board[row*cols + col];
      happiness = check_placement(board, row, col, rows, cols, threshold, type);
      if (happiness == 0){
        unhappy_spots[index]=row*cols + col;
        index++;
      }
    }
  }
  return unhappy_spots;
}

/* Moves a dissatisfied agent to the nearest open spot, wrapping around to the
* beginning index of the board if necessary.
*
*Parameters:
* board: dynamically allocated 2D array representing the state of the simulation
* unhappy_spots:dynamically allocated int array representing which spots in
*    the board are unhappy and which are happy
* int row: current x coordinate of the agent to be moved
* int col: current y coordinate of the agent to be moved
* rows: integer representing the # of rows in the board.
* cols: integer representing the # of columns in the board.
*
* Return:
* an updated board
*/
char* move_placement(char*board, int row, int col, int rows, int cols){
  int current_position = row*cols+col;
  //while the current_position has not yet reached the end of the board
  while (current_position < rows*cols-1){
    current_position++;
    if (board[current_position] == ' '){
      board[current_position]=board[row*cols+col];
      board[row*cols+col]=' ';
      return board;
    }
  }
  //if the position has reached the end of the board without finding an open
  //spot, wrap around to the beginning of the board and look from there until
  //the original position
  current_position=0;
  while (current_position<row*cols+col){
    if (board[current_position] == ' '){
      board[current_position]=board[row*cols+col];
      board[row*cols+col]=' ';
      return board;
    }
    current_position++;
  }
  return board;
}

/* Prints the current state of the board
 *
 * Parameters:
 *  board: dynamically allocated char array representing the current state of
 *    the board
 *  rows: integer representing the total number of rows
 *  cols: integer representing the total number of columns
 *
 * Return: none
*/
void print_board(char* board, int rows, int cols){
  for (int i=0; i<rows; i++){
    for (int j=0; j<cols; j++){
      printf("%c", board[i*cols + j]);
    }
    printf("\n");
  }

  return;
}