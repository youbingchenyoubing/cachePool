# include "entrance.h"
# include<iostream>

using namespace std;
//extern MemoryManager * memory_Instance = NULL;
int main( int argc, char* const* argv )
{
    Entrance entranceFunction;

    entranceFunction.parseParameters( argc, argv );

    entranceFunction.initProgram();


    return 0;
}