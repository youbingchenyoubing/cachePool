#ifndef TRIE_H
#define TRIE_H
//字典树的实现
# include <hash_map>
# include <iostream>
using namespace std;
namespace Trie
{
class Tries
{
public:
    Tries()
    {
        end =  false;
    }
    bool addCheck( char chs[], int index, int length )
    {
        if ( end )
        {
            return true;
        }
        if ( i == length )
        {
            end = true;
            return !children.isEmpty();
        }
        if ( !children.find( chs[i] ) )
        {
            children.insert( chs[i], Tries() );
        }

        return ( children.find( chs[i] ).second ).addCheck( chs, i + 1 );
    }
private:
    hash_map<char, Tries> children;
    bool end;


};
}
#endif