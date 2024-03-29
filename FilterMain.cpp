#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);

    for (int i=0; i < size; i++)
    {
      for (int j=0; j < size; j++)
      {
        int value;
        input >> value;
        filter -> set(i,j,value);
        filter -> set(i,j,value);
        filter -> set(i,j,value);

      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{
  /*
  EXTRA OPTIMIZATION: changed -O0 to -O3, so that compiler would make optimizaitions on its own (did not make a difference in scores). Boats score = 89, Blocks score = 85.
  Changed -O3 to -Ofast. -Ofast ignores cimpliancy rules, such as IEEE floating point standards, but also uses -O3 optimizations. Boats score = 98, Blocks score = 105
  */

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width; //Reduce memory references
  output -> height = input -> height; //Reduce memory references
  int divisor = filter -> getDivisor(); //Reduce memory references, and function calls
  
  
  int arrayOfGetCalls[9] = {filter -> get(0, 0), filter -> get(1, 0), filter -> get(2, 0),
                            filter -> get(0, 1), filter -> get(1, 1), filter -> get(2, 1),
                            filter -> get(0, 2), filter -> get(1, 2), filter -> get(2, 2)}; //Store function calls in cache


   /*
   Used opemMP to paralellize loop. Boats score = 89, Blocks score = 85
   */ 
   #pragma omp parallel for collapse(2)
    
    
  /*
  Flipped is loop(col), with second loop(row). Array is stored as color[row][col][plane]. Now stride-1. Got this through trial and error, with switching around for loop statements. Boats score = 70, Blocks score = 71
  */
  //for(int col = 1; col < (input -> width) - 1; col = col + 1)
  //for(int row = 1; row < (input -> height) - 1 ; row = row + 1)
  for(int plane = 0; plane < 3; plane++)
  {
    //for(int row = 1; row < (input -> height) - 1 ; row = row + 1)
    //for(int col = 1; col < (input -> width) - 1; col = col + 1)
    for(int row = 1; row < (input -> height) - 1 ; row = row + 1)
    {
      /*
      Tried to unroll this loop(plane), but score ended up being the same as before loop was unrolled. UPDATE: switching plane, and row loops made no difference in performance.
      */
      //for(int plane = 0; plane < 3; plane++)
      for(int col = 1; col < (input -> width) - 1; col = col + 1)
      {
          int acc0;
          int acc1;
          int acc2;
          int acc3;
          int acc4;
          int acc5;
          int acc6;
          int acc7;
          int acc8;
/*
Get rid of unecessary memory references (variable is reassigned at last line of for loop). Boats score = 72, Blocks score = 73
*/
        int outputColor = 0;
        output -> color[plane][row][col] = 0;

/*      for (int j = 0; j < filter -> getSize(); j++) {
           for (int i = 0; i < filter -> getSize(); i++) {	
             output -> color[plane][row][col]
            = output -> color[plane][row][col]
               + (input -> color[plane][row + i - 1][col + j - 1] 
              * filter -> get(i, j) );
          }
        }
*/
          
/*
getSize() always returns 3, because all of the filters are 3X3 matricies. Since the two innermost for loops use getSize() for the loop limit, the innermost operation can be unrolled into 9 computations. Boats score = 61, Blocks score = 59
*/
        /*
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row - 1][col - 1] * filter -> get(0, 0));
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row + 0][col - 1] * filter -> get(1, 0));
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row + 1][col - 1] * filter -> get(2, 0));
          
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row - 1][col + 0] * filter -> get(0, 1));
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row + 0][col + 0] * filter -> get(1, 1));
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row + 1][col + 0] * filter -> get(2, 1));
          
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row - 1][col + 1] * filter -> get(0, 2));
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row + 0][col + 1] * filter -> get(1, 2));
        output -> color[plane][row][col] = output -> color[plane][row][col] + (input -> color[plane][row + 1][col + 1] * filter -> get(2, 2));
        */
          
/*
Remove dependencies, so that each sum can be completed in parallel (remove dependencies on 'output -> color[plane][row][col]'). Boats score = 65, Blocks score = 61
*/          
        /*
        acc0 = output -> color[plane][row][col] + (input -> color[plane][row - 1][col - 1] * filter -> get(0, 0));
        acc1 = output -> color[plane][row][col] + (input -> color[plane][row + 0][col - 1] * filter -> get(1, 0));
        acc2 = output -> color[plane][row][col] + (input -> color[plane][row + 1][col - 1] * filter -> get(2, 0));
          
        acc3 = output -> color[plane][row][col] + (input -> color[plane][row - 1][col + 0] * filter -> get(0, 1));
        acc4 = output -> color[plane][row][col] + (input -> color[plane][row + 0][col + 0] * filter -> get(1, 1));
        acc5 = output -> color[plane][row][col] + (input -> color[plane][row + 1][col + 0] * filter -> get(2, 1));
          
        acc6 = output -> color[plane][row][col] + (input -> color[plane][row - 1][col + 1] * filter -> get(0, 2));
        acc7 = output -> color[plane][row][col] + (input -> color[plane][row + 0][col + 1] * filter -> get(1, 2));
        acc8 = output -> color[plane][row][col] + (input -> color[plane][row + 1][col + 1] * filter -> get(2, 2));
          
        output -> color[plane][row][col] = acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7 + acc8;
        */
        
        /*
        acc0 = outputColor + (input -> color[plane][row - 1][col - 1] * filter -> get(0, 0));
        acc1 = outputColor + (input -> color[plane][row + 0][col - 1] * filter -> get(1, 0));
        acc2 = outputColor + (input -> color[plane][row + 1][col - 1] * filter -> get(2, 0));
          
        acc3 = outputColor + (input -> color[plane][row - 1][col + 0] * filter -> get(0, 1));
        acc4 = outputColor + (input -> color[plane][row + 0][col + 0] * filter -> get(1, 1));
        acc5 = outputColor + (input -> color[plane][row + 1][col + 0] * filter -> get(2, 1));
          
        acc6 = outputColor + (input -> color[plane][row - 1][col + 1] * filter -> get(0, 2));
        acc7 = outputColor + (input -> color[plane][row + 0][col + 1] * filter -> get(1, 2));
        acc8 = outputColor + (input -> color[plane][row + 1][col + 1] * filter -> get(2, 2));
          
        outputColor = acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7 + acc8;
        */
          
        
/*
By storing the get() function calls in an array, the values can be stored in the cache. Part of this idea came from Prashanth giving you the hint that the function calls could be moved outside of the for loop. Boats score = 78, Blocks score = 80
*/
        acc0 = outputColor + (input -> color[plane][row - 1][col - 1] * arrayOfGetCalls[0]);
        acc1 = outputColor + (input -> color[plane][row + 0][col - 1] * arrayOfGetCalls[1]);
        acc2 = outputColor + (input -> color[plane][row + 1][col - 1] * arrayOfGetCalls[2]);
          
        acc3 = outputColor + (input -> color[plane][row - 1][col + 0] * arrayOfGetCalls[3]);
        acc4 = outputColor + (input -> color[plane][row + 0][col + 0] * arrayOfGetCalls[4]);
        acc5 = outputColor + (input -> color[plane][row + 1][col + 0] * arrayOfGetCalls[5]);
          
        acc6 = outputColor + (input -> color[plane][row - 1][col + 1] * arrayOfGetCalls[6]);
        acc7 = outputColor + (input -> color[plane][row + 0][col + 1] * arrayOfGetCalls[7]);
        acc8 = outputColor + (input -> color[plane][row + 1][col + 1] * arrayOfGetCalls[8]);
          
        outputColor = acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7 + acc8;
        
          
          
//reduce function calls. Boats score = 63, Blocks score = 60
        //output -> color[plane][row][col] = output -> color[plane][row][col] / filter -> getDivisor();
        outputColor = outputColor / divisor;

         /*
        if (output -> color[plane][row][col]  < 0)
        {
          output -> color[plane][row][col] = 0;
        }

        if (output -> color[plane][row][col]  > 255)
        { 
          output -> color[plane][row][col] = 255;
        }
        */
/*
Remove branching. Branch misprediction is costly, because it adds more instructions(the instructions from incorrect address that the program jumps to). For ternary operator, evaluation, and aggisnment is done with one instruction, so program will not need to jump around. Boats score = 69, Blocks score = 70
*/
          /*
          output -> color[plane][row][col] = (output -> color[plane][row][col]  < 0) ? 0 : (output -> color[plane][row][col]);
          output -> color[plane][row][col] = (output -> color[plane][row][col]  > 255) ? 255 : (output -> color[plane][row][col]);
          */
          outputColor = (outputColor  < 0) ? 0 : (outputColor);
          outputColor = (outputColor  > 255) ? 255 : (outputColor);
          
          
          output -> color[plane][row][col] = outputColor;
      }
    }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
