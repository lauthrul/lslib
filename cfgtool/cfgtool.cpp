#pragma warning(disable:4251)
#pragma warning(disable:4805)
#pragma warning(disable:4800)
#pragma warning(disable:4996)

#define USE_LOG4CXX
#define USE_LIBCURL
#define USE_LIBICONV

#include <libls/libls.h>
using namespace lslib;

//////////////////////////////////////////////////////////////////////////
void print_table_row(_lpcstr c1, _lpcstr c2, _lpcstr c3, _lpcstr c4)
{
    printf("%-30s %-30s %-30s %-30s \n", c1, c2, c3, c4);
}

void print_table_header()
{
    string str(125, '_');
    printf("%s\n", str.c_str());
    print_table_row("module", "key", "type", "value");
    printf("%s\n", str.c_str());
}

void print_table_tail()
{
    string str(125, '_');
    printf("%s\n", str.c_str());
}

int view_config(_lpcstr file)
{
    if (strtool::is_empty(file))
        return -1;

    SCfgData_list lst;
    CCfgHandler::LoadCfg(lst, file);

    print_table_header();
    string value;
    for (SCfgData_list::iterator it = lst.begin(); it != lst.end(); it++)
    {
        const SCfgData& data = *it;
        switch (data.eValueType)
        {
            case CVT_INT:
                value = strtool::format("%d", data.v_int);
                break;
            case CVT_FLOAT:
                value = strtool::format("%f", data.v_float);
                break;
            case CVT_STRING:
                value = data.v_str;
                break;
            case CVT_BLOB:
                value = "[hex] " + strtool::byte_array_to_hex_str((_lpbyte)data.v_blob.c_str(), data.v_bloblen);
                break;
        }
        print_table_row(data.strModule.c_str(), data.strKey.c_str(), enum_str(ECfgValueType, data.eValueType).c_str(), value.c_str());
    }
    print_table_tail();

    return 0;
}

int set_cfg(_lpcstr module, _lpcstr key, _lpcstr type, _lpcstr value, int value_len, _lpcstr file)
{
    if (strtool::is_empty(module) || strtool::is_empty(key) || strtool::is_empty(type) || strtool::is_empty(value))
        return -1;

    int ret = 0;
    switch (enum_from_str(ECfgValueType, type))
    {
        case CVT_INT:
            ret = CCfgHandler::SetCfg(module, key, strtool::to_int(value), file);
            break;
        case CVT_FLOAT:
            ret = CCfgHandler::SetCfg(module, key, (float)strtool::to_float(value), file);
            break;
        case CVT_STRING:
            ret = CCfgHandler::SetCfg(module, key, value, file);
            break;
        case CVT_BLOB:
            ret = CCfgHandler::SetCfg(module, key, (_lpvoid)value, value_len, file);
            break;
    }

    if (ret == 0)   printf("set config success\n");
    else            printf("set config fail. code: %d\n", ret);
    view_config(file);

    return 0;
}

int get_cfg(_lpcstr module, _lpcstr key, _lpcstr type, _lpcstr file)
{
    if (strtool::is_empty(module) || strtool::is_empty(key) || strtool::is_empty(type))
        return -1;

    string value;
    switch (enum_from_str(ECfgValueType, type))
    {
        case CVT_INT:
            value = strtool::format("%d", CCfgHandler::GetCfgInt(module, key, 0, file));
            break;
        case CVT_FLOAT:
            value = strtool::format("%f", CCfgHandler::GetCfgFloat(module, key, 0, file));
            break;
        case CVT_STRING:
            value = CCfgHandler::GetCfgText(module, key, "", file);
            break;
        case CVT_BLOB:
            _lpbyte pbuf = NULL;
            int len = CCfgHandler::GetCfgBlob((_lpvoid&)pbuf, module, key, file);
            value = "[hex] " + strtool::byte_array_to_hex_str(pbuf, len);
            lsfree(pbuf);
            break;
    }

    print_table_header();
    print_table_row(module, key, type, value.c_str());
    print_table_tail();
    return 0;
}

int main(int argc, _lpcstr argv[])
{
    _lpcstr usage = "usage: \n"
                    "   view config:        cfgtool <file> view\n"
                    "   get config:         cfgtool <file> get <type> <module> <key>\n"
                    "   set config:         cfgtool <file> set <type> <module> <key> <value> [value len]\n"
                    "param: \n"
                    "      <file>           config file path\n"
                    "      <type>           value type. [int|string|float|blob]\n"
                    "      <module>         module name\n"
                    "      <key>            key name\n"
                    "      <value>          value to be set\n"
                    "      [value len]      value length. for <type> = blob\n";

    int ret = 0;
    if (argc < 3) ret = -1;
    else
    {
        _lpcstr file = argv[1];
        _lpcstr action = argv[2];

        if (strcmp(action, "view") == 0)
        {
            ret = view_config(file);
        }
        else if (strcmp(action, "get") == 0)
        {
            if (argc < 6) ret = -1;
            else
            {
                _lpcstr type = argv[3];
                _lpcstr module = argv[4];
                _lpcstr key = argv[5];
                ret = get_cfg(module, key, type, file);
            }
        }
        else if (strcmp(action, "set") == 0)
        {
            if (argc < 7) ret = -1;
            else
            {
                _lpcstr type = argv[3];
                _lpcstr module = argv[4];
                _lpcstr key = argv[5];
                _lpcstr value = argv[6];
                int value_len = (argc >= 8) ? atoi(argv[7]) : 0;

                if (strcmp(type, "blob") == 0 && argc < 8)
                    ret = -1;
                else
                    ret = set_cfg(module, key, type, value, value_len, file);
            }
        }
    }

    if (ret == -1)
    {
        printf(usage);
        return -1;
    }
    return 0;
}
