#pragma once

#include "Node.hpp"
#include "Reader.hpp"
#include "Wz.hpp"
#include "Keys.hpp"

namespace wz
{
    class File final
    {

    public:
#if defined(__ANDROID__)
        [[maybe_unused]] explicit File(const std::initializer_list<u8> &new_iv, std::vector<u8> buffer);
#else
        [[maybe_unused]] explicit File(const std::initializer_list<u8> &new_iv, const char *path);

        [[maybe_unused]] explicit File(u8 *new_iv, const char *path);
#endif

        ~File();

        [[maybe_unused]] bool parse(const wzstring &name = u"");

        [[maybe_unused]] [[nodiscard]] Node *get_root() const;
        Node &get_child(const wzstring &name);

        MutableKey key;

    private:
        // u8* key;
        u8 *iv;

        Node *root;

        Description desc{};

        Reader reader;

        bool parse_directories(Node *node);

        u32 get_wz_offset();

        void init_key();

        friend class Node;
    };
}