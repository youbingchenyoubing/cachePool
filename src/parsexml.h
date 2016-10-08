#ifndef PARSEXML_H
#define PARSEXML_H
# include "similarlinkedhashmap.h"
//# include <iostream>
#ifndef __LOG_DEBUG
# include "./Log/Log.h"
#endif
# include <memory>

using namespace  std;
namespace cachemanager
{
class parsexml
{
public:
    static void parseXML( const char* name, shared_ptr <configureInfo> configurefile ); // 真正解析
    parsexml( const char* name );
private:
    static void getAbasPath( const char* name, char abspath [] );
};
}
#endif