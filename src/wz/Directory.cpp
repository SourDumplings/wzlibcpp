#include "Directory.hpp"

wz::Directory::Directory(File *root_file, bool img, int new_size, int new_checksum, unsigned int new_offset)
    : image(img), size(new_size), checksum(new_checksum), offset(new_offset), Node(img ? Type::Image : Type::Directory, root_file)
{
}

u32 wz::Directory::get_offset() const
{
    return offset;
}

bool wz::Directory::is_image() const
{
    return image;
}

bool wz::Directory::parse_image(Node *node)
{
    if (is_image())
    {
        node->reader = reader;
        node->path = this->path;
        const auto current_offset = get_offset();
        reader->set_position(current_offset);
        if (reader->is_wz_image())
        {
            return parse_property_list(node, current_offset);
        }
    }
    return false;
}