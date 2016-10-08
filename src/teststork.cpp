#include<iostream>
#include<cstring>
using namespace std;
int main()
{
    char sentence[] = "This/is/a/sentence/with/7/tokens";
    cout << "The string to be tokenized is:\n" << sentence << "\n\nThe tokens are:\n\n";
    char* tokenPtr = strpbrk( sentence, "/" );
    cout << tokenPtr << '\n';
    /*
    while(tokenPtr!=NULL){
         cout<<tokenPtr<<'\n';
         tokenPtr=strtok(NULL,"");
    } */
    cout << "After strtok,sentence=" << tokenPtr << endl;
    return 0;
}
