#include <iostream>
#include <algorithm>
#include <iostream>
#include <fstream> // 包含文件操作的头文件
#include <locale>
#include <codecvt>

#include <wz/Node.hpp>
#include <wz/File.hpp>
#include <wz/Directory.hpp>
#include <wz/Property.hpp>



#define U8 static_cast<u8>
#define IV4(A, B, C, D)            \
    {                              \
        U8(A), U8(B), U8(C), U8(D) \
    }

int main()
{
    const auto iv = IV4(0xb9, 0x7d, 0x63, 0xe9);
    wz::File file(iv, "E:/Temp2/CMS072_WZ/Map.wz");
    if (!file.parse())
    {
        std::cout << "Error for parsing file!" << std::endl;
        return -1;
    }

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    wz::Node *pNode = file.get_root()->find_from_path(u"Map/Map1/101000000.img");
    std::cout << "children size: " << pNode->children.size() << std::endl;
    for (const auto &[name, _] : pNode->children)
    {
        std::cout << "child name: " << converter.to_bytes(name) << std::endl;
    }
    return 0;
}