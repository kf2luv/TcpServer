#include <iostream>
#include <typeinfo>

class Any
{
public:
    Any() : _content(nullptr) {};
    ~Any()
    {
        if (_content)
        {
            delete _content;
        }
    };

    // 根据传入参数的类型构造Any
    template <class T>
    Any(const T &val) : _content(new PlaceHolder<T>(val)) {}

    Any(const Any &other)
        : _content(nullptr)
    {
        if (other._content)
        {
            _content = other._content->clone();
        }
    }

    template <class T>
    Any &operator=(const T &val)
    {
        if (_content)
        {
            delete _content; // 释放旧的content
        }
        _content = new PlaceHolder<T>(val); // 创建新的PlaceHolder，这将改变Any存储类型
        return *this;
    }

    Any &operator=(const Any &other)
    {
        if (other._content)
        {
            if (_content)
            {
                delete _content;
            }
            _content = other._content->clone();
        }
        return *this;
    }

    // 获取Any中的实际数据指针
    template <class T>
    T *get()
    {
        return &((PlaceHolder<T> *)_content)->_val;
    }

private:
    class Holder
    {
    public:
        virtual ~Holder() {};
        virtual const std::type_info& type() = 0;
        virtual Holder *clone() = 0;
    };

    template <class T>
    class PlaceHolder : public Holder
    {
    public:
        PlaceHolder(const T &val) : _val(val) {}

        const std::type_info& type() // 返回类型
        {
            return typeid(T);
        }
        Holder *clone() // 克隆（深拷贝）一个新的指针
        {
            return new PlaceHolder(_val);
        }

    public:
        T _val;
    };

private:
    Holder *_content;
};