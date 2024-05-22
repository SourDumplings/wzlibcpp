#include <cassert>
#include <ranges>
#include <memory>
#include "Node.hpp"
#include "Property.hpp"
#include "File.hpp"
#include "Directory.hpp"

wz::Node::Node()
    : parent(nullptr), file(nullptr), type(Type::NotSet)
{
}

wz::Node::Node(const Type &new_type, File *root_file)
    : parent(nullptr), file(root_file), type(new_type)
{
    reader = &file->reader;
}

wz::Node::~Node()
{
    for (auto &[_, nodes] : children)
    {
        for (auto *node : nodes)
        {
            delete node;
        }
    }
}

void wz::Node::appendChild(const wzstring &name, Node *node)
{
    assert(node);
    children[name].push_back(node);
    node->parent = this;
    node->path = this->path + u"/" + name;
    return;
}

const wz::WzMap &wz::Node::get_children() const
{
    return children;
}

wz::Node *wz::Node::get_parent() const
{
    return parent;
}

wz::WzMap::iterator wz::Node::begin()
{
    return children.begin();
}

wz::WzMap::iterator wz::Node::end()
{
    return children.end();
}

size_t wz::Node::children_count() const
{
    return children.size();
}

bool wz::Node::parse_property_list(Node *target, size_t offset)
{
    auto entryCount = reader->read_compressed_int();

    for (i32 i = 0; i < entryCount; i++)
    {
        auto name = reader->read_string_block(offset);

        auto prop_type = reader->read<u8>();
        switch (prop_type)
        {
        case 0:
        {
            auto *prop = new wz::Property<WzNull>(Type::Null, file);
            prop->path = target->path + u"/" + name;

            target->appendChild(name, prop);
        }
        break;
        case 0x0B:
            [[fallthrough]];
        case 2:
        {
            auto *prop = new wz::Property<u16>(Type::UnsignedShort, file, reader->read<u16>());
            prop->path = target->path + u"/" + name;

            target->appendChild(name, prop);
        }
        break;
        case 3:
        {
            auto *prop = new wz::Property<i32>(Type::Int, file, reader->read_compressed_int());
            prop->path = target->path + u"/" + name;

            target->appendChild(name, prop);
        }
        break;
        case 4:
        {
            auto float_type = reader->read<u8>();
            if (float_type == 0x80)
            {
                auto *prop = new wz::Property<f32>(Type::Float, file, reader->read<f32>());
                prop->path = target->path + u"/" + name;

                target->appendChild(name, prop);
            }
            else if (float_type == 0)
            {
                auto *pProp = new wz::Property<f32>(Type::Float, file, 0.f);
                pProp->path = target->path + u"/" + name;

                target->appendChild(name, pProp);
            }
        }
        break;
        case 5:
        {
            auto *prop = new wz::Property<f64>(Type::Double, file, reader->read<f64>());
            prop->path = target->path + u"/" + name;

            target->appendChild(name, prop);
        }
        break;
        case 8:
        {
            auto *prop = new wz::Property<wzstring>(Type::String, file);
            prop->path = target->path + u"/" + name;

            auto str = reader->read_string_block(offset);
            prop->set(str);
            target->appendChild(name, prop);
        }
        break;
        case 9:
        {
            auto ofs = reader->read<u32>();
            auto eob = reader->get_position() + ofs;
            parse_extended_prop(name, target, offset);
            if (reader->get_position() != eob)
                reader->set_position(eob);
        }
        break;
        default:
        {
            assert(0);
        }
        }
    }

    return true;
}

void wz::Node::parse_extended_prop(const wzstring &name, wz::Node *target, const size_t &offset)
{
    auto strPropName = reader->read_string_block(offset);

    if (strPropName == u"Property")
    {
        auto *prop = new Property<WzSubProp>(Type::SubProperty, file);
        prop->path = target->path + u"/" + name;
        reader->skip(sizeof(u16));
        parse_property_list(prop, offset);
        target->appendChild(name, prop);
    }
    else if (strPropName == u"Canvas")
    {
        auto *prop = new Property<WzCanvas>(Type::Canvas, file);
        prop->path = target->path + u"/" + name;
        reader->skip(sizeof(u8));
        if (reader->read<u8>() == 1)
        {
            reader->skip(sizeof(u16));
            parse_property_list(prop, offset);
        }

        prop->set(parse_canvas_property());

        target->appendChild(name, prop);
    }
    else if (strPropName == u"Shape2D#Vector2D")
    {
        auto *prop = new Property<WzVec2D>(Type::Vector2D, file);
        prop->path = target->path + u"/" + name;

        auto x = reader->read_compressed_int();
        auto y = reader->read_compressed_int();
        prop->set({x, y});

        target->appendChild(name, prop);
    }
    else if (strPropName == u"Shape2D#Convex2D")
    {
        auto *prop = new Property<WzConvex>(Type::Convex2D, file);
        prop->path = target->path + u"/" + name;

        int convexEntryCount = reader->read_compressed_int();
        for (int i = 0; i < convexEntryCount; i++)
        {
            parse_extended_prop(name, prop, offset);
        }

        target->appendChild(name, prop);
    }
    else if (strPropName == u"Sound_DX8")
    {
        auto *prop = new Property<WzSound>(Type::Sound, file);
        prop->path = target->path + u"/" + name;

        prop->set(parse_sound_property());

        target->appendChild(name, prop);
    }
    else if (strPropName == u"UOL")
    {
        reader->skip(sizeof(u8));
        auto *prop = new Property<WzUOL>(Type::UOL, file);
        prop->path = target->path + u"/" + name;

        prop->set({reader->read_string_block(offset)});
        target->appendChild(name, prop);
    }
    else
    {
        assert(0);
    }
}

wz::WzCanvas wz::Node::parse_canvas_property()
{
    WzCanvas canvas;
    canvas.width = reader->read_compressed_int();
    canvas.height = reader->read_compressed_int();
    canvas.format = reader->read_compressed_int();
    canvas.format2 = reader->read<u8>();
    reader->skip(sizeof(u32));
    canvas.size = reader->read<i32>() - 1;
    reader->skip(sizeof(u8));

    canvas.offset = reader->get_position();

    auto header = reader->read<u16>();

    if (header != 0x9C78 && header != 0xDA78)
    {
        canvas.is_encrypted = true;
    }

    switch (canvas.format + canvas.format2)
    {
    case 1:
    {
        canvas.uncompressed_size = canvas.width * canvas.height * 2;
    }
    break;
    case 2:
    {
        canvas.uncompressed_size = canvas.width * canvas.height * 4;
    }
    break;
    case 513: // Format16bppRgb565
    {
        canvas.uncompressed_size = canvas.width * canvas.height * 2;
    }
    break;
    case 517:
    {
        canvas.uncompressed_size = canvas.width * canvas.height / 128;
    }
    break;
    }

    reader->set_position(canvas.offset + canvas.size);

    return canvas;
}

wz::WzSound wz::Node::parse_sound_property()
{
    WzSound sound;
    // reader->ReadUInt8();
    reader->skip(sizeof(u8));
    sound.size = reader->read_compressed_int();
    sound.length = reader->read_compressed_int();
    reader->set_position(reader->get_position() + 56);
    sound.frequency = reader->read<i32>();
    reader->set_position(reader->get_position() + 22);

    sound.offset = reader->get_position();

    reader->set_position(sound.offset + sound.size);

    return sound;
}

wz::Type wz::Node::get_type() const
{
    return type;
}

bool wz::Node::is_property() const
{
    return (bit(type) & bit(Type::Property)) == bit(Type::Property);
}

wz::MutableKey &wz::Node::get_key() const
{
    return file->key;
}

u8 *wz::Node::get_iv() const
{
    return file->iv;
}

wz::Node *wz::Node::get_child(const wz::wzstring &name)
{
    if (auto it = children.find(name); it != children.end())
    {
        return it->second[0];
    }
    return nullptr;
}

wz::Node *wz::Node::get_child(const std::string &name)
{
    return get_child(std::u16string{name.begin(), name.end()});
}

wz::Node &wz::Node::operator[](const wz::wzstring &name)
{
    return *get_child(name);
}

wz::Node *wz::Node::find_from_path(const std::u16string &path)
{
    auto next = std::views::split(std::string{path.begin(), path.end()}, '/') | std::views::common;
    wz::Node *node = this;
    for (const auto &s : next)
    {
        auto str = std::string{s.begin(), s.end()};
        if (str == "..")
        {
            node = node->parent;
            continue;
        }
        else
        {
            node = node->get_child(str);
            if (node != nullptr)
            {
                // 处理UOL
                if (node->type == wz::Type::UOL)
                {
                    node = dynamic_cast<wz::Property<wz::WzUOL> *>(node)->get_uol();
                }
                if (node->type == wz::Type::Image)
                {
                    static std::map<std::u16string, wz::Node *> img_map;
                    if (img_map.contains(node->path))
                    {
                        node = img_map[node->path];
                    }
                    else
                    {
                        auto *image = new wz::Node();
                        auto *dir = dynamic_cast<wz::Directory *>(node);
                        dir->parse_image(image);
                        node = image;
                        img_map[node->path] = node;
                    }
                    continue;
                }
            }
            else
            {
                return nullptr;
            }
        }
    }
    return node;
}

wz::Node *wz::Node::find_from_path(const std::string &path)
{
    return find_from_path(std::u16string{path.begin(), path.end()});
}