
#include "rc_ixfile.h"

#include <direct.h>
#include <stdio.h>
#include <string>
#include <algorithm>


std::string dirname(std::string const &file)
{
    std::string s(file);
    std::replace(s.begin(), s.end(), '\\', '/');
    s = s.substr(0, s.find_last_of('/'));
    if (s.size() == 0)
    {
        s = ".";
    }
    return s;
}

std::string filename(std::string const &file)
{
    std::string s(file);
    std::replace(s.begin(), s.end(), '\\', '/');
    s = s.substr(s.find_last_of('/') + 1);
    return s;
}

std::string fileext(std::string const &file)
{
    size_t pos = file.find_last_of('.');
    if (pos == std::string::npos || pos == file.size() - 1)
    {
        throw std::runtime_error(std::string("File name does not have an extension: ") + file);
    }
    return file.substr(pos + 1);
}

std::string filenoext(std::string const &file)
{
    size_t pos = file.find_last_of('.');
    if (pos == std::string::npos)
    {
        throw std::runtime_error(std::string("File name does not have an extension: ") + file);
    }
    return file.substr(0, pos);
}

void copy_file(std::string const &src, std::string const &dst)
{
    IxRead *ir = IxRead::readFromFile(src.c_str());
    try
    {
        IxWrite *iw = IxWrite::writeToFile(dst.c_str());
        try
        {
            iw->write(ir->dataSegment(0, ir->size()), ir->size());
        }
        catch (...)
        {
            delete iw;
            throw;
        }
        delete iw;
    }
    catch (...)
    {
        delete ir;
        throw;
    }
    delete ir;
} 



IxRead::~IxRead()
{
    ::operator delete(data_);
}

IxRead *IxRead::readFromFile(char const *name)
{
    IxRead *ir = 0;
    FILE *f = 0;
    std::string path(name);
#if defined(_WINDOWS)
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
    char cwd[256];
    _getcwd(cwd, 256);
    cwd[255] = 0;
    errno_t r = fopen_s(&f, path.c_str(), "rb");
    if (!f)
    {
        char serr[256];
        strerror_s(serr, 256, r);
        throw std::runtime_error(std::string("File not found: ") + path +
            "\nCurrent directory: " + cwd +
            "\n" + serr);
    }
    try
    {
        fseek(f, 0, 2);
        long l = ftell(f);
        if (l > MAX_FILE_SIZE)
        {
            throw std::runtime_error(std::string("File ") + path + " is too large.");
        }
        fseek(f, 0, 0);
        ir = new IxRead();
        ir->data_ = ::operator new(l);
        ir->size_ = l;
        fread(ir->data_, 1, ir->size_, f);
    }
    catch (...)
    {
        fclose(f);
        delete ir;
        throw;
    }
    fclose(f);
    char const *ext = strrchr(name, '.');
    ext = ext ? ext + 1 : "";
    ir->type_ = ext;
    return ir;
}

void IxRead::read(void *data, size_t size)
{
    if (size > size_ || size_ - size < pos_)
    {
        throw std::runtime_error("Read past end of file");
    }
    memcpy(data, (char *)data_ + pos_, size);
    pos_ += size;
}

void IxRead::read_at(void *data, uint32_t size, uint32_t pos)
{
    if (pos > size_ || size > size_ || size_ - size < pos)
    {
        throw std::runtime_error("Read_at past end of file");
    }
    memcpy(data, (char *)data_ + pos, size);
}

uint32_t IxRead::size() const
{
    return size_;
}

uint32_t IxRead::pos() const
{
    return pos_;
}

void IxRead::seek(uint32_t pos)
{
    if (pos > size_)
    {
        throw std::runtime_error("Seek past end of file");
    }
    pos_ = pos;
}

std::string const &IxRead::type() const
{
    return type_;
}

void const *IxRead::dataSegment(uint32_t pos, uint32_t size) const
{
    if (pos > size_ || size > size_ || size_ - size < pos)
    {
        throw std::runtime_error("Index out of range in dataSegment()");
    }
    return (char *)data_ + pos;
}

IxRead::IxRead() :
    data_(0),
    size_(0),
    pos_(0)
{
}


IxWrite::~IxWrite()
{
    if (handle_)
    {
        fclose((FILE *)handle_);
    }
}

IxWrite *IxWrite::writeToFile(char const *name)
{
    FILE *f = 0;
    std::string pathName(name);
#if defined(_WINDOWS)
    std::replace(pathName.begin(), pathName.end(), '/', '\\');
#endif
    fopen_s(&f, pathName.c_str(), "wb");
    if (!f)
    {
        throw std::runtime_error(std::string("Cannot write to file: ") + pathName);
    }
    IxWrite *ret = new IxWrite();
    ret->handle_ = f;
    return ret;
}

void IxWrite::write(void const *data, uint32_t size)
{
    fwrite(data, 1, size, (FILE *)handle_);
    pos_ = pos_ + size;
    if (size_ < pos_)
    {
        size_ = pos_;
    }
}

void IxWrite::write_at(void const *data, uint32_t size, uint32_t pos)
{
    if (pos > size_)
    {
        throw std::runtime_error("Write_at past end of file");
    }
    fseek((FILE *)handle_, pos, 0);
    fwrite(data, 1, size, (FILE *)handle_);
    if (pos + size > size_)
    {
        size_ = pos + size;
    }
    fseek((FILE *)handle_, pos_, 0);
}

uint32_t IxWrite::size() const
{
    return size_;
}

uint32_t IxWrite::pos() const
{
    return pos_;
}

void IxWrite::seek(uint32_t pos)
{
    if (pos_ > size_)
    {
        throw std::runtime_error("Attempt to seek past end of file");
    }
    pos_ = pos;
}

IxWrite::IxWrite() :
    handle_(0),
    pos_(0),
    size_(0)
{
}

